#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GSTypes.h"
#include "GSCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
struct FInputActionValue;

UCLASS()
class GLOSHAUGEN_API AGSCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGSCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// --- Inventory (server) ---
	/** Gives a weapon or ammo for it. Returns true if anything was picked up. */
	bool GiveWeapon(EGSWeaponId Id);

	// --- State read by HUD ---
	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly) float Health = 100.f;
	UPROPERTY(Replicated, BlueprintReadOnly) float MaxHealth = 100.f;
	UPROPERTY(Replicated, BlueprintReadOnly) float Stamina = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_Weapons, BlueprintReadOnly) TArray<FGSWeaponSlot> Weapons;
	UPROPERTY(ReplicatedUsing = OnRep_CurrentSlot, BlueprintReadOnly) int32 CurrentSlot = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bReloading = false;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bSprinting = false;
	UPROPERTY(ReplicatedUsing = OnRep_Dead, BlueprintReadOnly) bool bDead = false;

	bool IsAiming() const { return bAiming; }
	float GetCurrentSpread() const;
	const FGSWeaponSlot* GetCurrentWeapon() const { return Weapons.IsValidIndex(CurrentSlot) ? &Weapons[CurrentSlot] : nullptr; }
	float GetLastHitMarkerTime() const { return LastHitMarkerTime; }
	float GetLastDamageTakenTime() const { return LastDamageTakenTime; }

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPWeaponMesh;   // seen only by owner
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BodyMesh;       // seen by others
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HeadMesh;       // seen by others
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TPWeaponMesh;   // seen by others

	// Input handlers
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump();
	void StopJump();
	void StartSprint();
	void StopSprint();
	void ToggleCrouch();
	void StartFire();
	void StopFire();
	void StartAim();
	void StopAim();
	void Reload();
	void SelectSlot1();
	void SelectSlot2();
	void NextWeapon();

	void FireOnce();
	void SetSprinting(bool bNew);
	void ApplyWeaponVisuals();
	void SpawnTracer(const FVector& Start, const FVector& End) const;

	UFUNCTION(Server, Reliable) void ServerFire(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, int32 Seed);
	UFUNCTION(Server, Reliable) void ServerReload();
	UFUNCTION(Server, Reliable) void ServerSetSprinting(bool bNew);
	UFUNCTION(Server, Reliable) void ServerSelectSlot(int32 Slot);
	UFUNCTION(NetMulticast, Unreliable) void MulticastShotFX(FVector_NetQuantize Start, FVector_NetQuantize End);
	UFUNCTION(Client, Unreliable) void ClientHitMarker(bool bKill);

	UFUNCTION() void OnRep_Health();
	UFUNCTION() void OnRep_Weapons();
	UFUNCTION() void OnRep_CurrentSlot();
	UFUNCTION() void OnRep_Dead();

	// Movement extras
	bool TryMantle();
	void StartSlide();
	void EndSlide();
	UFUNCTION(Server, Reliable) void ServerMantle(FVector_NetQuantize10 LaunchVelocity);
	UFUNCTION(Server, Reliable) void ServerSlide();
	UPROPERTY(Replicated) bool bSliding = false;
	FTimerHandle SlideTimer;
	float LastSlideTime = -100.f;

	void FinishReload();
	void Die(AController* Killer, bool bHeadshot);

	bool bAiming = false;
	bool bWantsFire = false;
	bool bWantsSprint = false;
	float LastFireTime = -100.f;
	float ServerLastFireTime = -100.f;
	float LastHitMarkerTime = -100.f;
	float LastDamageTakenTime = -100.f;
	float CurrentRecoil = 0.f;
	float DefaultFOV = 90.f;
	FTimerHandle AutoFireTimer;
	FTimerHandle ReloadTimer;

	static constexpr float WalkSpeed = 420.f;
	static constexpr float SprintSpeed = 680.f;
	static constexpr float AimSpeed = 260.f;
};
