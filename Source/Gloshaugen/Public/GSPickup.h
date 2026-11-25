#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSItems.h"
#include "GSPickup.generated.h"

class UStaticMeshComponent;
class USceneComponent;

/**
 * Something lying around on campus. Look at it and press E to take it.
 * The ring (and beam for Episk and better) shows the rarity.
 * Locked crates need an NTNU access card or a lock pick and always contain good loot.
 */
UCLASS()
class GLOSHAUGEN_API AGSPickup : public AActor
{
	GENERATED_BODY()

public:
	AGSPickup();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Spawns a pickup that does not respawn (dropped items, loot from dead players). */
	static AGSPickup* SpawnDropped(UWorld* World, const FVector& Location, FName ItemId, int32 Count,
		int32 Ammo = -1, int32 Reserve = -1, uint8 Attachments = 0);

	/** Server: fill this spot with something random. */
	void Roll();
	/** Server: someone took Taken items. */
	void OnTaken(int32 Taken);
	void Unlock();

	UPROPERTY(ReplicatedUsing = OnRep_State) FName ItemId;
	UPROPERTY(ReplicatedUsing = OnRep_State) int32 Count = 1;
	UPROPERTY(ReplicatedUsing = OnRep_State) bool bAvailable = true;
	UPROPERTY(ReplicatedUsing = OnRep_State) bool bLocked = false;
	// Dropped weapons keep their ammo and attachments
	int32 Ammo = -1;
	int32 Reserve = -1;
	uint8 Attachments = 0;

	bool bRespawns = true;
	bool bIsCrate = false;          // locked loot crate, relocks with new loot
	EGSRarity MinRarity = EGSRarity::Common;
	FRandomStream Rng;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Glow;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Beam;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Crate;

	UFUNCTION() void OnRep_State();
	void Respawn();

	FTimerHandle RespawnTimer;
	float BaseMeshZ = 0.f;
};
