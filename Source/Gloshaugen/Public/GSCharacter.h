#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GSTypes.h"
#include "GSItems.h"
#include "GSPickup.h"
#include "GSCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class USpotLightComponent;
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

	static constexpr int32 MaxWeapons = 4;
	static constexpr int32 BaseBackpackSlots = 6;

	// --- Inventory (server) ---
	/** Gives a weapon, or ammo for it if you already have one. Returns true if anything was taken. */
	bool GiveWeapon(FName Id, int32 Ammo = -1, int32 Reserve = -1, uint16 Attachments = 0);
	/** Puts items in the backpack. Returns how many fit. */
	int32 AddToBackpack(FName Id, int32 Count);
	/** Server: take what a pickup holds. Returns how many items were taken. */
	int32 TakeFromPickup(AGSPickup* Pickup);
	/** Server: start a boost / side effect (refreshes it if already running). */
	void AddEffect(FName EffectId);
	/** Server: flashbang went off at Location. */
	void ApplyFlash(const FVector& Location, float Radius);
	/** Server: special rounds of the shot that is hitting us right now (GSAmmoType). */
	uint8 IncomingAmmoType = 0;
	/** Owner gets a short text message on screen. */
	UFUNCTION(Client, Reliable) void ClientToast(const FString& Message);

	// --- State read by HUD ---
	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly) float Health = 100.f;
	UPROPERTY(Replicated, BlueprintReadOnly) float MaxHealth = 100.f;
	UPROPERTY(Replicated, BlueprintReadOnly) float Stamina = 100.f;
	UPROPERTY(Replicated, BlueprintReadOnly) float Hunger = 100.f;   // 100 = full, 0 = starving
	UPROPERTY(Replicated, BlueprintReadOnly) float Thirst = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_Weapons, BlueprintReadOnly) TArray<FGSWeaponSlot> Weapons;
	UPROPERTY(ReplicatedUsing = OnRep_CurrentSlot, BlueprintReadOnly) int32 CurrentSlot = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) TArray<FGSInvStack> Backpack;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 BackpackBonus = 0;
	UPROPERTY(ReplicatedUsing = OnRep_Gear, BlueprintReadOnly) FName BodyArmor;
	UPROPERTY(Replicated, BlueprintReadOnly) float ArmorPoints = 0.f;
	UPROPERTY(ReplicatedUsing = OnRep_Gear, BlueprintReadOnly) FName Helmet;
	UPROPERTY(Replicated, BlueprintReadOnly) float HelmetPoints = 0.f;
	UPROPERTY(Replicated, BlueprintReadOnly) TArray<FGSActiveEffect> Effects;
	UPROPERTY(Replicated, BlueprintReadOnly) FName UseItemId;      // item being used right now (eat, inject ...)
	UPROPERTY(Replicated, BlueprintReadOnly) float UseStartTime = 0.f;
	UPROPERTY(Replicated, BlueprintReadOnly) float UseEndTime = 0.f;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bReloading = false;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bSprinting = false;
	UPROPERTY(ReplicatedUsing = OnRep_Dead, BlueprintReadOnly) bool bDead = false;
	UPROPERTY(ReplicatedUsing = OnRep_Gear) bool bFlashlightOn = false;

	bool IsAiming() const { return bAiming; }
	float GetCurrentSpread() const;
	const FGSWeaponSlot* GetCurrentWeapon() const { return Weapons.IsValidIndex(CurrentSlot) ? &Weapons[CurrentSlot] : nullptr; }
	/** Weapon stats with attachments applied. */
	FGSWeaponStats GetStats(const FGSWeaponSlot& Slot) const;
	int32 GetMagSize(const FGSWeaponSlot& Slot) const { return GetStats(Slot).MagSize; }
	float GetLastHitMarkerTime() const { return LastHitMarkerTime; }
	float GetLastDamageTakenTime() const { return LastDamageTakenTime; }
	int32 GetBackpackSlots() const { return BaseBackpackSlots + BackpackBonus; }
	int32 CountItem(FName Id) const;
	bool HasItemKind(FName Kind) const;
	/** All running boosts combined. */
	FGSEffectMods GetMods() const;
	float GetServerTime() const;
	float EffectTimeLeft(const FGSActiveEffect& E) const { return E.EndTime - GetServerTime(); }

	// Local (owner) state for the HUD
	int32 SelectedItem = 0;
	AGSPickup* GetFocusPickup() const { return FocusPickup.Get(); }
	class AGSTerminal* GetFocusTerminal() const { return FocusTerminal.Get(); }
	/** Server: remove Count of Id from the backpack if there are enough. */
	bool ConsumeItem(FName Id, int32 Count);
	bool IsUsingBinoculars() const { return bBinoculars; }
	bool HasNightVisionOn() const { return bNightVisionGoggles; }
	FString ToastText;
	float ToastTime = -100.f;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPWeaponMesh;   // seen only by owner
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BodyMesh;       // seen by others
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HeadMesh;       // seen by others
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TPWeaponMesh;   // seen by others
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> VestMesh;       // seen by others
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HelmetMesh;     // seen by others
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpotLightComponent> Flashlight;

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
	void SelectSlot(int32 Slot);
	void SelectSlot1() { SelectSlot(0); }
	void SelectSlot2() { SelectSlot(1); }
	void SelectSlot3() { SelectSlot(2); }
	void SelectSlot4() { SelectSlot(3); }
	void NextWeapon();
	void Interact();
	void UseSelectedItem();
	void NextItem();
	void DropSelectedItem();
	void QuickMelee();

	void FireOnce();
	void SetSprinting(bool bNew);
	void ApplyWeaponVisuals();
	void ApplyGearVisuals();
	/** Uses a real animated human (the Unreal mannequin from the Third Person content pack, or Fab/Lyra characters)
	 *  for the body others see, if one is in the project. The cylinder and sphere stay as invisible hitboxes. */
	void SetupHumanBody();
	bool bHumanBody = false;
	void UpdateFocusPickup();
	void UpdateLocalVisuals(float DeltaSeconds);
	void UpdateWeaponAnimation(float DeltaSeconds);
	void AddWeaponKick(float Strength, EGSWeaponClass Class);
	void DropMagazineFX();
	void Inspect();
	void TickSurvival(float DeltaSeconds);
	void TickEffects();
	void CompleteUse();
	void CancelUse();
	bool RemoveItem(FName Id, int32 Count = 1);
	void DropEverything();
	void ThrowItem(const FGSItemDef& Def);
	void DoMeleeHit(bool bQuick);
	FString CurrentWeaponName() const;

	UFUNCTION(Server, Reliable) void ServerFire(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, int32 Seed);
	UFUNCTION(Server, Reliable) void ServerMelee(bool bQuick);
	UFUNCTION(Server, Reliable) void ServerReload();
	UFUNCTION(Server, Reliable) void ServerSetSprinting(bool bNew);
	UFUNCTION(Server, Reliable) void ServerSelectSlot(int32 Slot);
	UFUNCTION(Server, Reliable) void ServerInteract(AGSPickup* Pickup);
	UFUNCTION(Server, Reliable) void ServerUseItem(int32 Index);
	UFUNCTION(Server, Reliable) void ServerCancelUse();
	UFUNCTION(Server, Reliable) void ServerDropItem(int32 Index);
	UFUNCTION(Server, Reliable) void ServerGrapple(FVector_NetQuantize Target);
	UFUNCTION(Server, Reliable) void ServerToggleFlashlight();
	UFUNCTION(NetMulticast, Unreliable) void MulticastShotFX(FVector_NetQuantize Start, FVector_NetQuantize End, bool bSilent);
	UFUNCTION(NetMulticast, Unreliable) void MulticastPelletFX(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, int32 Seed, int32 Pellets, float ConeDeg);
	UFUNCTION(NetMulticast, Unreliable) void MulticastMeleeFX();
	UFUNCTION(Client, Unreliable) void ClientHitMarker(bool bKill);

	UFUNCTION() void OnRep_Health();
	UFUNCTION() void OnRep_Weapons();
	UFUNCTION() void OnRep_CurrentSlot();
	UFUNCTION() void OnRep_Dead();
	UFUNCTION() void OnRep_Gear();

	void DrawPellets(const FVector& Start, const FVector& Dir, int32 Seed, int32 Pellets, float ConeDeg, bool bFromMuzzle);
	void NoteShotForRadio(const FVector& Where);

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
	void Die(AController* Killer, bool bHeadshot, const FString& WeaponName);

	bool bAiming = false;
	bool bWantsFire = false;
	bool bWantsSprint = false;
	bool bBinoculars = false;
	bool bNightVisionGoggles = false;
	float LastFireTime = -100.f;
	float ServerLastFireTime = -100.f;
	float LastMeleeTime = -100.f;
	float LastHitMarkerTime = -100.f;
	float LastDamageTakenTime = -100.f;
	float CurrentRecoil = 0.f;
	float DefaultFOV = 90.f;
	float StarveAccumulator = 0.f;
	float GlideStartTime = -100.f;
	TWeakObjectPtr<AGSPickup> FocusPickup;
	TWeakObjectPtr<class AGSTerminal> FocusTerminal;
	/** The player controller if it has a PC / 3D printer open (input goes there instead). */
	class AGSPlayerController* TerminalPC() const;

	// Procedural first-person animation state (local only)
	FName AnimWeaponId;
	float EquipStart = -100.f;
	float ReloadStart = -100.f;
	float ReloadLength = 2.f;
	bool bWasReloading = false;
	bool bMagDropped = false;
	bool bWasFalling = false;
	float InspectStart = -100.f;
	float AimBlend = 0.f;
	float SprintBlend = 0.f;
	float BobTime = 0.f;
	// Camera animation (local only): strafe lean, hit flinch, landing dip, breathing
	float CamLean = 0.f;
	float CamDip = 0.f;
	float CamDipVel = 0.f;
	float CamFallSpeed = 0.f;
	bool bCamWasFalling = false;
	float FlinchSeenTime = -100.f;
	float FlinchStart = -100.f;
	float FlinchSign = 1.f;
	FVector2D LookSwayInput = FVector2D::ZeroVector;
	FRotator SwayRot = FRotator::ZeroRotator;
	FVector KickPos = FVector::ZeroVector;
	FVector KickVel = FVector::ZeroVector;
	FRotator KickRot = FRotator::ZeroRotator;
	FRotator KickRotVel = FRotator::ZeroRotator;
	TWeakObjectPtr<AGSPickup> UseTargetPickup;   // crate being lock-picked
	int32 UseIndex = INDEX_NONE;
	FTimerHandle AutoFireTimer;
	FTimerHandle ReloadTimer;
	FString LastDamageWeapon;
	TWeakObjectPtr<AController> LastDamageInstigator;

	static constexpr float WalkSpeed = 420.f;
	static constexpr float SprintSpeed = 680.f;
	static constexpr float AimSpeed = 260.f;
};
