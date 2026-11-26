#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class AGSCharacter;

/**
 * Everything that flies: rockets (Carl Gustaf), 40 mm grenades (M79), crossbow bolts,
 * and thrown things (frag, smoke, flashbang, molotov, C4, trip mine).
 * Kind decides the behaviour.
 */
UCLASS()
class GLOSHAUGEN_API AGSProjectile : public AActor
{
	GENERATED_BODY()

public:
	AGSProjectile();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;

	static AGSProjectile* Launch(UWorld* World, AGSCharacter* Owner, FName Kind, FName ItemId, const FVector& Location,
		const FVector& Velocity, float GravityScale, float Damage, float Radius, float Fuse);

	UPROPERTY(ReplicatedUsing = OnRep_Visual) FName Kind;
	UPROPERTY(ReplicatedUsing = OnRep_Visual) FName ItemId;
	UPROPERTY(ReplicatedUsing = OnRep_Exploded) bool bExploded = false;
	float Damage = 0.f;
	float Radius = 0.f;   // cm
	float Fuse = 0.f;     // seconds until it goes off (0 = on impact)

	/** Flight setup the clients need to simulate the same arc as the server between network updates. */
	UPROPERTY(ReplicatedUsing = OnRep_Flight) float NetGravityScale = 1.f;
	UPROPERTY(ReplicatedUsing = OnRep_Flight) bool bNetBounce = false;

	/** Who threw / fired it. Kept here because the pawn loses its controller when it dies. */
	TWeakObjectPtr<AController> OwnerController;
	/** Where it was thrown / fired from (server). Used for the death card and hit direction after the thrower has died. */
	FVector LaunchLocation = FVector::ZeroVector;
	/** The thrower's body right now, and whether it is still the body that threw this (false after death/respawn). */
	class AGSCharacter* GetThrowerNow(bool& bSameBody) const;

	/** Most trip mines one player can have lying out at once; placing one more removes his oldest. */
	static constexpr int32 MaxMinesPerPlayer = 5;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProjectileMovementComponent> Movement;

	UFUNCTION() void OnStop(const FHitResult& Hit);
	UFUNCTION() void OnRep_Visual();
	UFUNCTION() void OnRep_Exploded();
	UFUNCTION() void OnRep_Flight();

	void Explode();
	void DamageAround(const FVector& Center, float Amount, float R, bool bFalloff);
	bool IsKind(const TCHAR* Name) const { return Kind == FName(Name); }
	/** The thrower's controller, also after the thrower has died or respawned. */
	AController* KillerController() const;

	float SpawnTime = 0.f;
	float LastAreaTick = 0.f;
	bool bStuck = false;
};
