#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"
#include "GSFlag.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class APlayerState;
class AGSCharacter;
class AGSPlayerState;

/** Flag events sent through AGSGameState::MulticastFlagEvent. */
namespace GSFlagEvent { constexpr uint8 Taken = 1, Dropped = 2, Returned = 3, Captured = 4; }

UENUM()
enum class EGSFlagState : uint8 { Home, Carried, Dropped };

/**
 * Capture the Flag: one flag per team. The actor itself never moves (it marks the base); only the pole and
 * cloth (FlagRoot) follow the carrier or lie where he fell. The server decides everything (take, drop,
 * return, capture) by checking distances ten times a second; walking up to the flag is enough.
 */
UCLASS()
class GLOSHAUGEN_API AGSFlag : public AActor
{
	GENERATED_BODY()

public:
	AGSFlag();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(ReplicatedUsing = OnRep_Look) uint8 Team = 0;               // whose flag
	UPROPERTY(ReplicatedUsing = OnRep_Look) EGSFlagState State = EGSFlagState::Home;
	UPROPERTY(ReplicatedUsing = OnRep_Look) TObjectPtr<APlayerState> Carrier;
	UPROPERTY(ReplicatedUsing = OnRep_Look) FVector_NetQuantize DropLocation;   // floor point where it lies

	/** Where the cloth is right now (base, carrier's back or on the ground). */
	FVector GetFlagLocation() const;
	/** Server: back to the base, no event (match start / end). */
	void ResetHome();

	static constexpr float TouchRadius = 130.f;     // cm, 2D
	static constexpr float CaptureRadius = 200.f;
	static constexpr float SafetyReturnSeconds = 180.f;   // a dropped flag nobody can reach goes home

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;        // stays at the base
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BasePlate;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> FlagRoot;    // moved in world space
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Pole;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Cloth;

	UFUNCTION() void OnRep_Look();
	/** Moves the pole and cloth to GetFlagLocation(), upright or lying down; hidden for the carrier himself. */
	void UpdatePose();
	void ServerStep();
	void Take(AGSCharacter* By);
	void Drop();
	void ReturnHome(AGSPlayerState* By);   // By may be null (safety return)
	void Capture(AGSPlayerState* By);
	void Announce(uint8 Event, const FVector& Where, APlayerState* Who, const TCHAR* Verb);
	float ServerAccum = 0.f;
	float DroppedAt = 0.f;
	uint8 AppliedTeam = 255;
	/** Last place the carrier was seen on this machine (his pawn may not be relevant here). */
	mutable FVector LastCarrierLocation = FVector::ZeroVector;
	mutable float LastCarrierYaw = 0.f;
};
