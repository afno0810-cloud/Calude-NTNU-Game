#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSTypes.h"
#include "GSPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/** A weapon lying on the ground. Walk over it to take it (or get ammo if you already have it). */
UCLASS()
class GLOSHAUGEN_API AGSPickup : public AActor
{
	GENERATED_BODY()

public:
	AGSPickup();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(ReplicatedUsing = OnRep_State) EGSWeaponId WeaponId = EGSWeaponId::AG3;
	UPROPERTY(ReplicatedUsing = OnRep_State) bool bAvailable = true;

	float RespawnSeconds = 30.f;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Trigger;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Glow;

	UFUNCTION() void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);
	UFUNCTION() void OnRep_State();
	void Respawn();
	void TryGiveTo(AActor* Other);

	FTimerHandle RespawnTimer;
};
