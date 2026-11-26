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
class UBoxComponent;
struct FInputActionValue;

/** What the last hit marker was for (the HUD draws each kind differently). */
namespace GSHitKind
{
	constexpr uint8 Body = 0;
	constexpr uint8 Armor = 1;   // vest or helmet took part of the hit
	constexpr uint8 Head = 2;
	constexpr uint8 Kill = 3;
}

/** What kind of hit YOU just took (sound and screen edge differ for each). */
namespace GSHurtKind
{
	constexpr uint8 Body = 0;        // flesh
	constexpr uint8 Armor = 1;       // the vest took part of it
	constexpr uint8 Head = 2;        // headshot, no helmet
	constexpr uint8 HeadArmor = 3;   // headshot, the helmet took part of it
	constexpr uint8 ArmorBroke = 4;  // the vest or helmet broke from this hit
}

/** Which part of the body a hit landed on. */
namespace GSZone
{
	constexpr uint8 None = 0;      // no single point: blast, hunger
	constexpr uint8 Head = 1;
	constexpr uint8 Chest = 2;
	constexpr uint8 Stomach = 3;   // belly and pelvis
	constexpr uint8 Arm = 4;
	constexpr uint8 Leg = 5;
}

/** A recent hit on you, for the HUD direction wedges (local only). */
struct FGSRecentHit
{
	FVector From = FVector::ZeroVector;
	float Time = -100.f;       // local time of the latest hit from this direction
	float Strength = 0.f;      // 0.35..1, bigger wedge for harder hits
	float BurstLost = 0.f;     // health lost from this direction in the current burst (shotgun pellets add up)
};

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
	/** Death cam: while dead, the view falls, pulls back over the body and turns toward the killer. */
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
	/** Local: who the death cam turns toward (set by the player controller when you die). */
	TWeakObjectPtr<APawn> DeathCamKiller;
	/** Local: where the killer stood when you died, for when his pawn is too far away for your game to see it. */
	FVector DeathCamKillerLoc = FVector::ZeroVector;
	bool bDeathCamHasKillerLoc = false;

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
	bool bIncomingMelee = false;   // server: the hit being applied is a blade/fist
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
	/** The backpack item you wear (others see it on your back). */
	UPROPERTY(ReplicatedUsing = OnRep_Gear, BlueprintReadOnly) FName WornBackpack;
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
	/** 0 = fine, 1 = badly shot. Leg: slower, lower jumps, no sprint, limp. Arm: unsteady aim, more recoil, slow reload. */
	UPROPERTY(Replicated, BlueprintReadOnly) float LegInjury = 0.f;
	UPROPERTY(Replicated, BlueprintReadOnly) float ArmInjury = 0.f;
	/** The injury as it is felt right now (painkillers halve it). */
	float GetLegPain() const;
	float GetArmPain() const;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	bool IsAiming() const { return bAiming; }
	float GetCurrentSpread() const;
	const FGSWeaponSlot* GetCurrentWeapon() const { return Weapons.IsValidIndex(CurrentSlot) ? &Weapons[CurrentSlot] : nullptr; }
	/** Weapon stats with attachments applied. */
	FGSWeaponStats GetStats(const FGSWeaponSlot& Slot) const;
	int32 GetMagSize(const FGSWeaponSlot& Slot) const { return GetStats(Slot).MagSize; }
	float GetLastHitMarkerTime() const { return LastHitMarkerTime; }
	/** GSHitKind of the last hit marker, and whether the last kill was a headshot. */
	uint8 GetLastHitKind() const { return LastHitKind; }
	bool WasLastKillHeadshot() const { return bLastKillHeadshot; }
	/** Aiming through a magnified scope (4x / 8x or a sniper): the HUD draws the scope and the gun is hidden. */
	bool IsScopedIn() const;
	float GetAimBlend() const { return AimBlend; }
	float GetBreathHeld() const { return BreathHeld; }
	/** Where the last hit on you came from (world), and when (local time), for the HUD damage indicator. */
	FVector LastDamageFrom = FVector::ZeroVector;
	float LastDamageFromTime = -100.f;
	/** You were hit: where from (if anyone shot), how much health you lost, and what kind of hit (GSHurtKind). */
	UFUNCTION(Client, Unreliable) void ClientTookHit(FVector_NetQuantize From, bool bHasFrom, float HealthLost, uint8 Kind);
	float GetLastDamageTakenTime() const { return LastDamageTakenTime; }
	/** Local: up to 4 directions you were recently hit from (HUD wedges). */
	TArray<FGSRecentHit> RecentHits;
	/** Local: the screen-edge flash for the last hit you took (GSHurtKind), and when you were last healed. */
	float LastHurtFlashTime = -100.f;
	float LastHurtFlashStrength = 0.f;   // 0..1, how deep the edge reaches
	float LastHurtBlood = 0.f;           // 0..1, how red it is (0 when the vest/helmet took all of it)
	uint8 LastHurtKind = GSHurtKind::Body;
	float LastHealTime = -100.f;
	/** 0 above 35 % health, rising to 1 at 0 health (heartbeat, grey picture, red pulse). Always 0 when dead. */
	float GetLowHealthAlpha() const;
	/** Local time of the last heartbeat sound, so the HUD pulse beats with it. */
	float GetLastHeartbeatTime() const { return LastHeartbeatTime; }
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
	/** First-person gloved arms (SM_FPArm_R/L) holding the weapon, a magazine in the left hand while reloading,
	 *  the item being eaten/drunk/injected and the hand holding it, and your own legs when you look down. */
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPArmR;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPArmL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPMag;
	/** Attachments shown on the first-person gun: optic, muzzle device, laser, foregrip. */
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPAttSight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPAttMuzzle;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPAttLaser;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPAttGrip;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPItemMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FPItemHand;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> FPLegs;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BodyMesh;       // seen by others
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HeadMesh;       // seen by others
	/** Hit zones with the size of real body parts: head, chest, stomach, left/right arm, left/right leg. */
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> HitZones[7];
	void ApplyHitZoneLayout(bool bCrouch);
	uint8 ZoneOf(const UPrimitiveComponent* Comp) const;
	/** Keeps the body others see standing on the ground (also while crouched and for network smoothing). */
	void FixBodyMeshOffset(float HalfHeightAdjust);
	float ReloadSeconds(const FGSWeaponStats& S) const;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TPWeaponMesh;   // seen by others
	/** The same attachments on the third-person gun so other players see your scope, suppressor, laser and grip. */
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TPAttSight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TPAttMuzzle;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TPAttLaser;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TPAttGrip;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> VestMesh;       // seen by others
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HelmetMesh;     // seen by others
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BackpackMesh;   // seen by others
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
	/** Mouse wheel: inventory open = flip through the backpack, otherwise switch weapon. Up = previous, down = next. */
	void Scroll(const FInputActionValue& Value);
	void Interact();
	void UseSelectedItem();
	void NextItem();
	void DropSelectedItem();
	void QuickMelee();

	void FireOnce();
	void SetSprinting(bool bNew);
	void ApplyWeaponVisuals();
	void ApplyAttachmentVisuals(const struct FGSWeaponSlot* W);
	void ApplyGearVisuals();
	/** Uses a real animated human (the Unreal mannequin from the Third Person content pack, or Fab/Lyra characters)
	 *  for the body others see, if one is in the project. The cylinder and sphere are the stand-in when none is found; hits land on HitZones. */
	void SetupHumanBody();
	bool bHumanBody = false;
	void UpdateFocusPickup();
	void UpdateLocalVisuals(float DeltaSeconds);
	void UpdateWeaponAnimation(float DeltaSeconds);
	void AddWeaponKick(float Strength, EGSWeaponClass Class);
	void DropMagazineFX();
	/** Brass (or a red shotgun shell) flies out of the ejection port and bounces (local, cosmetic). */
	void SpawnShellFX(EGSWeaponClass Class);
	/** Applies camera recoil smoothly over a few frames and drifts part of it back when you stop firing. */
	void TickRecoil(float Dt);
	/** Places the arms on the weapon (grip + handguard), animates the left hand during reloads. */
	void UpdateArms(float Dt, const struct FGSWeaponStats* S, float ReloadT, bool bShellReload);
	/** Eating, drinking, pills, injections, bandages: the item comes up in your hand. */
	void UpdateItemUse(float Dt);
	FName ItemAnimId;
	FVector LeftHandHome = FVector::ZeroVector;
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
	UFUNCTION(Server, Reliable) void ServerSetAiming(bool bNew);
	UFUNCTION(Server, Reliable) void ServerSelectSlot(int32 Slot);
	UFUNCTION(Server, Reliable) void ServerInteract(AGSPickup* Pickup);
	UFUNCTION(Server, Reliable) void ServerUseItem(int32 Index);
	UFUNCTION(Server, Reliable) void ServerCancelUse();
	UFUNCTION(Server, Reliable) void ServerDropItem(int32 Index);
	UFUNCTION(Server, Reliable) void ServerGrapple(FVector_NetQuantize Target);
	UFUNCTION(Server, Reliable) void ServerToggleFlashlight();
	UFUNCTION(NetMulticast, Unreliable) void MulticastShotFX(FVector_NetQuantize Start, FVector_NetQuantize End, uint8 Flags);   // Flags: bit 1 = suppressed, bit 2 = no tracer
	UFUNCTION(NetMulticast, Unreliable) void MulticastPelletFX(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, int32 Seed, int32 Pellets, float ConeDeg);
	UFUNCTION(NetMulticast, Unreliable) void MulticastMeleeFX();
	/** Everyone: this body was just hit here, from this direction (GSWound kind, strength 0..255). */
	UFUNCTION(NetMulticast, Unreliable) void MulticastWoundFX(FVector_NetQuantize Point, FVector_NetQuantizeNormal Dir, uint8 Kind, uint8 Strength);
	/** Local: push the ragdoll the way the last wound says (once; safe to call before or after the wound arrives). */
	void ApplyDeathImpulse();
	/** Local: the pool of blood under the body, once it has come to rest. */
	void SpawnDeathPool();
	UFUNCTION(Client, Unreliable) void ClientHitMarker(bool bKill, bool bHeadshot, bool bArmor);

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
	/** KillingProjectile: the grenade/mine/rocket/bolt that did it, if any (death card uses where it was thrown from). */
	void Die(AController* Killer, bool bHeadshot, const FString& WeaponName, const class AGSProjectile* KillingProjectile = nullptr);

	UPROPERTY(Replicated) bool bAiming = false;
	bool bWantsFire = false;
	bool bWantsSprint = false;
	bool bBinoculars = false;
	bool bNightVisionGoggles = false;
	float LastFireTime = -100.f;
	float ServerLastFireTime = -100.f;
	// Server: shots the weapon is allowed to fire right now (refills at the weapon's own rate)
	float ShotCredit = 1.f;
	float ShotCreditTime = -100.f;
	float TPAim = 0.f;        // others see the gun come up to the eye when you aim
	float LimpPhase = 0.f;    // local: the camera limps in step when a leg is hurt
	float ScrollAccum = 0.f;  // local: wheel/touchpad movement not yet turned into a whole step
	float ScrollLastTime = -100.f;
	float LastMeleeTime = -100.f;
	float LastHitMarkerTime = -100.f;
	uint8 LastHitKind = GSHitKind::Body;
	bool bLastKillHeadshot = false;
	float LastDamageTakenTime = -100.f;
	// Death cam (local only): when it started, where your eyes were, and the smoothed camera
	float DeathCamStart = -1.f;
	FVector DeathEyeLoc = FVector::ZeroVector;
	FRotator DeathEyeRot = FRotator::ZeroRotator;
	FVector DeathCamPos = FVector::ZeroVector;
	FRotator DeathCamRot = FRotator::ZeroRotator;
	float DeathCamArm = 1.f;   // 0..1 of the way from the body to the camera that is free of walls
	bool bDeathCamInit = false;
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
	bool bEmptyReload = false;    // reload from an empty mag: also racks the charging handle / slide
	float DryFireStart = -100.f;  // trigger pulled on an empty gun
	float WallBlend = 0.f;        // gun pulled back to high ready when you stand close to a wall
	float IdleTime = 0.f;         // seconds without moving or shooting (idle fidget)
	bool bAnimWasCrouched = false;
	float BarrelHeat = 0.f;       // rises per shot, barrel smokes after a long burst
	float LastBarrelSmoke = -100.f;
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
	float FlinchScale = 1.f;            // headshots you survive jerk the view harder
	float LastSeenHealth = 100.f;
	// Being hit (local only): burst of hits for the edge flash, sound de-duplication, heartbeat
	float HurtBurstLost = 0.f;          // health lost in the current burst (shotgun pellets arrive together)
	float LastHurtSoundTime = -100.f;
	int32 LastHurtSoundRank = -1;
	float NextHeartbeat = 0.f;
	float LastHeartbeatTime = -100.f;
	/** Low-health grey picture and vignette on top of the drug visuals (Visual 2/5 keep their own saturation). */
	void ApplyLowHealthPostProcess(int32 Visual, float Low);
	// Fire animation and recoil (local only)
	FVector2D RecoilPending = FVector2D::ZeroVector;  // pitch, yaw (degrees) still to apply
	FVector2D RecoilRecover = FVector2D::ZeroVector;  // part that drifts back after the burst
	int32 BurstShots = 0;
	int32 FireVariant = 0;
	/** Which melee swing plays next: slash, backhand, overhead chop (a stab for knives). */
	int32 MeleeVariant = 0;
	/** When you last climbed a ledge: the gun tucks down out of the way while you vault. */
	float MantleStart = -100.f;
	/** Scope sway: the crosshair drifts slowly through a scope; hold Shift to hold your breath for a few seconds. */
	float ScopeSwayAmp = 0.f;
	float BreathHeld = 0.f;
	FVector2D ScopeSwayPrev = FVector2D::ZeroVector;
	void TickScopeSway(float Dt);
	float FireAnimStart = -100.f;
	float LastShotTime = -100.f;
	float CycleStart = -100.f;   // pump or bolt cycle after a shot
	bool bCycleBolt = false;
	float TPSprint = 0.f;
	float TPReload = 0.f; // others see the gun tilted and lowered while you reload
	float LastRemoteShotTime = -100.f;
	float StepDistance = 0.f;
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
	// Local on every machine: the last wound this body took (ragdoll push, where the pool forms)
	FVector LastWoundPoint = FVector::ZeroVector;
	FVector LastWoundDir = FVector::ZeroVector;
	float LastWoundTime = -100.f;
	float LastWoundSoundTime = -100.f;
	uint8 LastWoundKind = 0;
	float LastWoundStrength = 0.f;
	bool bDeathImpulseDone = false;
	FTimerHandle DeathPoolTimer;
	// Server: at most 3 wound effects per frame per body (a shotgun blast is 8-9 hits at once)
	uint64 WoundFrame = 0;
	int32 WoundsThisFrame = 0;

	static constexpr float BodyLifeSeconds = 90.f;   // how long a dead body stays
	static constexpr float WalkSpeed = 420.f;
	static constexpr float SprintSpeed = 680.f;
	static constexpr float AimSpeed = 260.f;
};
