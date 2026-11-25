#include "GSCharacter.h"
#include "Engine/StaticMeshActor.h"
#include "GSPlayerController.h"
#include "GSGameMode.h"
#include "GSHUD.h"
#include "GSPickup.h"
#include "GSTerminal.h"
#include "GSProjectile.h"
#include "GSEffects.h"
#include "GSUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName KindZoom(TEXT("zoom"));
	const FName KindNightVision(TEXT("nightvision"));
	const FName KindDrone(TEXT("drone"));
	const FName KindGrapple(TEXT("grapple"));
	const FName KindParachute(TEXT("parachute"));
	const FName KindKey(TEXT("key"));
	const FName KindLockpick(TEXT("lockpick"));
	const FName KindLight(TEXT("light"));
	const FName KindRadio(TEXT("radio"));
	const FName KindAmmo(TEXT("ammo"));
	const FName KindPlate(TEXT("plate"));
	const FName KindBackpack(TEXT("backpack"));
	const FName KindCompass(TEXT("compass"));
	const FName KindJoke(TEXT("joke"));
	const FName KindDefib(TEXT("defib"));
	const FName AttSuppressor(TEXT("att_suppressor"));
	const FName AttRedDot(TEXT("att_reddot"));
	const FName AttScope(TEXT("att_scope"));
	const FName AttExtMag(TEXT("att_extmag"));
	const FName AttLaser(TEXT("att_laser"));
	const FName AttHolo(TEXT("att_holo"));
	const FName AttScope8x(TEXT("att_scope8x"));
	const FName AttDrum(TEXT("att_drum"));
	const FName AttComp(TEXT("att_comp"));
	const FName AttGrip(TEXT("att_grip"));
	const FName KindCaliber(TEXT("caliber"));
	const FName KindFilament(TEXT("filament"));

	/** Item that gives each attachment bit (so a replaced attachment goes back in the sack). */
	FName AttachmentItem(uint16 Bit)
	{
		switch (Bit)
		{
		case GSAttach::Suppressor: return TEXT("Suppressor");
		case GSAttach::RedDot: return TEXT("RedDot");
		case GSAttach::Scope: return TEXT("Scope");
		case GSAttach::ExtMag: return TEXT("ExtMag");
		case GSAttach::Laser: return TEXT("Laser");
		case GSAttach::Holo: return TEXT("Holo");
		case GSAttach::Scope8x: return TEXT("Scope8x");
		case GSAttach::Drum: return TEXT("Drum");
		case GSAttach::Compensator: return TEXT("Compensator");
		case GSAttach::Grip: return TEXT("Grip");
		default: return NAME_None;
		}
	}

	FName AmmoBoxFor(FName Caliber) { return Caliber.IsNone() ? FName() : FName(*(FString(TEXT("Ammo_")) + Caliber.ToString())); }
	const FName FxFoniks(TEXT("Foniks"));
	const FName FxGlide(TEXT("Glide"));
	const FName FxDrone(TEXT("Avslort"));
	const FName FxFlash(TEXT("Blendet"));
	const FName FxPoison(TEXT("Matforgiftning"));

	bool IsLocalOnlyGadget(const FGSItemDef& D)
	{
		return D.Cat == EGSItemCat::Gadget && (D.Kind == KindZoom || D.Kind == KindNightVision || D.Kind == KindRadio
			|| D.Kind == KindCompass || D.Kind == KindJoke || D.Kind == KindKey || D.Kind == KindLockpick
			|| D.Kind == KindCaliber || D.Kind == KindFilament);
	}
}

AGSCharacter::AGSCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetNetUpdateFrequency(60.f);

	GetCapsuleComponent()->InitCapsuleSize(40.f, 92.f);
	// Bullets hit the body/head meshes, not the capsule.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = false;
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MaxWalkSpeedCrouched = 200.f;
	Move->JumpZVelocity = 430.f;
	Move->AirControl = 0.25f;
	Move->BrakingDecelerationWalking = 1800.f;
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	Move->SetCrouchedHalfHeight(58.f);
	Move->MaxStepHeight = 50.f;
	Move->SetWalkableFloorAngle(50.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 66.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(DefaultFOV);

	auto SetupVisual = [](UStaticMeshComponent* C)
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		C->CanCharacterStepUpOn = ECB_No;
	};
	auto SetupHittable = [](UStaticMeshComponent* C)
	{
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		C->SetCollisionObjectType(ECC_Pawn);
		C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		C->SetGenerateOverlapEvents(false);
		C->CanCharacterStepUpOn = ECB_No;
	};

	FPWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FPWeapon"));
	FPWeaponMesh->SetupAttachment(Camera);
	FPWeaponMesh->SetRelativeLocation(FVector(38.f, 16.f, -16.f));
	FPWeaponMesh->SetOnlyOwnerSee(true);
	FPWeaponMesh->SetCastShadow(false);
	SetupVisual(FPWeaponMesh);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -14.f));
	BodyMesh->SetRelativeScale3D(FVector(0.62f, 0.62f, 1.56f));
	BodyMesh->SetOwnerNoSee(true);
	SetupHittable(BodyMesh);

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	HeadMesh->SetupAttachment(GetCapsuleComponent());
	HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, 78.f));
	HeadMesh->SetRelativeScale3D(FVector(0.3f));
	HeadMesh->SetOwnerNoSee(true);
	SetupHittable(HeadMesh);

	TPWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TPWeapon"));
	TPWeaponMesh->SetupAttachment(GetCapsuleComponent());
	TPWeaponMesh->SetRelativeLocation(FVector(40.f, 18.f, 30.f));
	TPWeaponMesh->SetOwnerNoSee(true);
	SetupVisual(TPWeaponMesh);

	VestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Vest"));
	VestMesh->SetupAttachment(GetCapsuleComponent());
	VestMesh->SetRelativeLocation(FVector(0.f, 0.f, 28.f));
	VestMesh->SetOwnerNoSee(true);
	VestMesh->SetVisibility(false);
	SetupVisual(VestMesh);

	HelmetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Helmet"));
	HelmetMesh->SetupAttachment(GetCapsuleComponent());
	HelmetMesh->SetRelativeLocation(FVector(0.f, 0.f, 82.f));
	HelmetMesh->SetOwnerNoSee(true);
	HelmetMesh->SetVisibility(false);
	SetupVisual(HelmetMesh);

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(Camera);
	Flashlight->SetRelativeLocation(FVector(10.f, 15.f, -10.f));
	Flashlight->SetIntensity(30000.f);
	Flashlight->SetAttenuationRadius(3500.f);
	Flashlight->SetOuterConeAngle(24.f);
	Flashlight->SetInnerConeAngle(10.f);
	Flashlight->SetCastShadows(false);
	Flashlight->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylF(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphF(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	FPWeaponMesh->SetStaticMesh(CubeF.Object);
	TPWeaponMesh->SetStaticMesh(CubeF.Object);
	VestMesh->SetStaticMesh(CubeF.Object);
	HelmetMesh->SetStaticMesh(SphF.Object);
	BodyMesh->SetStaticMesh(CylF.Object);
	HeadMesh->SetStaticMesh(SphF.Object);
}

void AGSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSCharacter, Health);
	DOREPLIFETIME(AGSCharacter, MaxHealth);
	DOREPLIFETIME_CONDITION(AGSCharacter, Stamina, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AGSCharacter, Hunger, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AGSCharacter, Thirst, COND_OwnerOnly);
	DOREPLIFETIME(AGSCharacter, Weapons);
	DOREPLIFETIME(AGSCharacter, CurrentSlot);
	DOREPLIFETIME_CONDITION(AGSCharacter, Backpack, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AGSCharacter, BackpackBonus, COND_OwnerOnly);
	DOREPLIFETIME(AGSCharacter, BodyArmor);
	DOREPLIFETIME_CONDITION(AGSCharacter, ArmorPoints, COND_OwnerOnly);
	DOREPLIFETIME(AGSCharacter, Helmet);
	DOREPLIFETIME_CONDITION(AGSCharacter, HelmetPoints, COND_OwnerOnly);
	DOREPLIFETIME(AGSCharacter, Effects);
	DOREPLIFETIME_CONDITION(AGSCharacter, UseItemId, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AGSCharacter, UseStartTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AGSCharacter, UseEndTime, COND_OwnerOnly);
	DOREPLIFETIME(AGSCharacter, bReloading);
	DOREPLIFETIME_CONDITION(AGSCharacter, bSprinting, COND_SkipOwner);
	DOREPLIFETIME(AGSCharacter, bDead);
	DOREPLIFETIME_CONDITION(AGSCharacter, bSliding, COND_SkipOwner);
	DOREPLIFETIME(AGSCharacter, bFlashlightOn);
}

void AGSCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Each player gets a body color from their name so you can tell people apart.
	const uint32 Hash = GetTypeHash(GetName());
	const FLinearColor BodyColor = FLinearColor::MakeFromHSV8(uint8(Hash % 255), 120, 150);
	GSUtils::SetColor(BodyMesh, BodyColor);
	GSUtils::SetColor(HeadMesh, FLinearColor(0.85f, 0.68f, 0.55f));

	if (HasAuthority() && Weapons.Num() == 0)
	{
		GiveWeapon(GSItems::Pistol);
		AddToBackpack(FName(TEXT("Plaster")), 2);
		AddToBackpack(FName(TEXT("Water")), 1);
	}
	ApplyWeaponVisuals();
	ApplyGearVisuals();
}

float AGSCharacter::GetServerTime() const
{
	const UWorld* W = GetWorld();
	if (!W) return 0.f;
	const AGameStateBase* GS = W->GetGameState();
	return GS ? GS->GetServerWorldTimeSeconds() : W->GetTimeSeconds();
}

// ---------------------------------------------------------------- Input

void AGSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	AGSPlayerController* PC = Cast<AGSPlayerController>(GetController());
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!PC || !EIC)
	{
		UE_LOG(LogTemp, Error, TEXT("Gloshaugen: Enhanced Input is not set up (check Config/DefaultInput.ini)."));
		return;
	}
	PC->EnsureInputCreated();

	EIC->BindAction(PC->MoveAction, ETriggerEvent::Triggered, this, &AGSCharacter::Move);
	EIC->BindAction(PC->LookAction, ETriggerEvent::Triggered, this, &AGSCharacter::Look);
	EIC->BindAction(PC->JumpAction, ETriggerEvent::Started, this, &AGSCharacter::StartJump);
	EIC->BindAction(PC->JumpAction, ETriggerEvent::Completed, this, &AGSCharacter::StopJump);
	EIC->BindAction(PC->SprintAction, ETriggerEvent::Started, this, &AGSCharacter::StartSprint);
	EIC->BindAction(PC->SprintAction, ETriggerEvent::Completed, this, &AGSCharacter::StopSprint);
	EIC->BindAction(PC->CrouchAction, ETriggerEvent::Started, this, &AGSCharacter::ToggleCrouch);
	EIC->BindAction(PC->FireAction, ETriggerEvent::Started, this, &AGSCharacter::StartFire);
	EIC->BindAction(PC->FireAction, ETriggerEvent::Completed, this, &AGSCharacter::StopFire);
	EIC->BindAction(PC->AimAction, ETriggerEvent::Started, this, &AGSCharacter::StartAim);
	EIC->BindAction(PC->AimAction, ETriggerEvent::Completed, this, &AGSCharacter::StopAim);
	EIC->BindAction(PC->ReloadAction, ETriggerEvent::Started, this, &AGSCharacter::Reload);
	EIC->BindAction(PC->Slot1Action, ETriggerEvent::Started, this, &AGSCharacter::SelectSlot1);
	EIC->BindAction(PC->Slot2Action, ETriggerEvent::Started, this, &AGSCharacter::SelectSlot2);
	EIC->BindAction(PC->Slot3Action, ETriggerEvent::Started, this, &AGSCharacter::SelectSlot3);
	EIC->BindAction(PC->Slot4Action, ETriggerEvent::Started, this, &AGSCharacter::SelectSlot4);
	EIC->BindAction(PC->NextWeaponAction, ETriggerEvent::Started, this, &AGSCharacter::NextWeapon);
	EIC->BindAction(PC->InteractAction, ETriggerEvent::Started, this, &AGSCharacter::Interact);
	EIC->BindAction(PC->UseItemAction, ETriggerEvent::Started, this, &AGSCharacter::UseSelectedItem);
	EIC->BindAction(PC->NextItemAction, ETriggerEvent::Started, this, &AGSCharacter::NextItem);
	EIC->BindAction(PC->DropItemAction, ETriggerEvent::Started, this, &AGSCharacter::DropSelectedItem);
	EIC->BindAction(PC->MeleeAction, ETriggerEvent::Started, this, &AGSCharacter::QuickMelee);
	EIC->BindAction(PC->InspectAction, ETriggerEvent::Started, this, &AGSCharacter::Inspect);
}

void AGSCharacter::Move(const FInputActionValue& Value)
{
	if (TerminalPC()) return;
	if (bDead) return;
	const FVector2D V = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), V.Y);
	AddMovementInput(GetActorRightVector(), V.X);
	if (bSprinting && V.Y <= 0.1f)
	{
		SetSprinting(false);
	}
}

void AGSCharacter::Look(const FInputActionValue& Value)
{
	if (TerminalPC()) return;
	const FVector2D V = Value.Get<FVector2D>();
	float Sens = bAiming ? 0.55f : 1.f;
	if (bBinoculars) Sens = 0.2f;
	AddControllerYawInput(V.X * Sens);
	AddControllerPitchInput(-V.Y * Sens);
	LookSwayInput += V * Sens;
}

void AGSCharacter::StartJump()
{
	if (TerminalPC()) return;
	if (bDead) return;
	if (bSliding) EndSlide();
	if (!TryMantle()) Jump();
}

bool AGSCharacter::TryMantle()
{
	// Climb onto ledges and low walls in front of you (60 - 220 cm high).
	const FVector Fwd = GetActorForwardVector();
	const float HalfH = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, HalfH);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSMantle), false, this);

	FHitResult Wall;
	if (!GetWorld()->LineTraceSingleByChannel(Wall, Feet + FVector(0.f, 0.f, 70.f), Feet + FVector(0.f, 0.f, 70.f) + Fwd * 80.f, ECC_WorldStatic, Params))
	{
		return false;
	}
	FHitResult Top;
	const FVector Down0 = Wall.ImpactPoint + Fwd * 30.f + FVector(0.f, 0.f, 200.f);
	if (!GetWorld()->LineTraceSingleByChannel(Top, Down0, Down0 - FVector(0.f, 0.f, 200.f), ECC_WorldStatic, Params)
		|| Top.bStartPenetrating || Top.ImpactNormal.Z < 0.7f)
	{
		return false;
	}
	const float LedgeHeight = Top.ImpactPoint.Z - Feet.Z;
	if (LedgeHeight < 55.f || LedgeHeight > 220.f) return false;

	// Room to stand on top?
	if (GetWorld()->OverlapAnyTestByChannel(Top.ImpactPoint + FVector(0.f, 0.f, HalfH + 5.f), FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.9f, HalfH), Params))
	{
		return false;
	}

	const float Gravity = FMath::Abs(GetCharacterMovement()->GetGravityZ());
	const float Up = FMath::Sqrt(2.f * Gravity * (LedgeHeight + 45.f));
	const FVector Launch = Fwd * 260.f + FVector(0.f, 0.f, Up);
	LaunchCharacter(Launch, true, true);
	if (!HasAuthority()) ServerMantle(Launch);
	return true;
}

void AGSCharacter::ServerMantle_Implementation(FVector_NetQuantize10 LaunchVelocity)
{
	if (bDead || LaunchVelocity.Size() > 1200.f) return;
	LaunchCharacter(LaunchVelocity, true, true);
}

void AGSCharacter::StartSlide()
{
	if (bSliding || GetCharacterMovement()->IsFalling()) return;
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastSlideTime < 1.2f) return;
	LastSlideTime = Now;
	bSliding = true;
	bSprinting = false;
	Crouch();
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->GroundFriction = 0.4f;
	Move->BrakingDecelerationWalking = 250.f;
	Move->MaxWalkSpeedCrouched = SprintSpeed + 250.f;
	Move->Velocity += GetActorForwardVector() * 320.f;
	GetWorldTimerManager().SetTimer(SlideTimer, this, &AGSCharacter::EndSlide, 0.85f, false);
	if (!HasAuthority()) ServerSlide();
}

void AGSCharacter::ServerSlide_Implementation()
{
	StartSlide();
}

void AGSCharacter::EndSlide()
{
	if (!bSliding) return;
	bSliding = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->GroundFriction = 8.f;
	Move->BrakingDecelerationWalking = 1800.f;
	Move->MaxWalkSpeedCrouched = 200.f;
	GetWorldTimerManager().ClearTimer(SlideTimer);
}
void AGSCharacter::StopJump() { StopJumping(); }

void AGSCharacter::StartSprint()
{
	bWantsSprint = true;
	if (!bAiming && !bIsCrouched && Stamina > 10.f) SetSprinting(true);
}

void AGSCharacter::StopSprint()
{
	bWantsSprint = false;
	SetSprinting(false);
}

void AGSCharacter::SetSprinting(bool bNew)
{
	if (bSprinting == bNew) return;
	bSprinting = bNew;
	if (bNew)
	{
		StopFire();
		if (!UseItemId.IsNone()) ServerCancelUse();
	}
	if (!HasAuthority()) ServerSetSprinting(bNew);
}

void AGSCharacter::ServerSetSprinting_Implementation(bool bNew)
{
	bSprinting = bNew && Stamina > 1.f && !bIsCrouched;
	if (bSprinting) CancelUse();
}

void AGSCharacter::ToggleCrouch()
{
	if (TerminalPC()) return;
	if (bDead) return;
	// Crouch while sprinting = slide
	if (bSprinting && GetVelocity().Size2D() > WalkSpeed + 50.f)
	{
		SetSprinting(false);
		StartSlide();
		return;
	}
	if (bSliding) { EndSlide(); UnCrouch(); return; }
	if (bIsCrouched) UnCrouch(); else { SetSprinting(false); Crouch(); }
}

void AGSCharacter::StartAim()
{
	if (TerminalPC()) return;
	bAiming = true;
	SetSprinting(false);
}

void AGSCharacter::StopAim()
{
	bAiming = false;
	if (bWantsSprint) StartSprint();
}

void AGSCharacter::SelectSlot(int32 Slot)
{
	if (AGSPlayerController* TPC = TerminalPC()) { TPC->TerminalInput(*FString::FromInt(Slot + 1)); return; }
	if (!Weapons.IsValidIndex(Slot) || bDead) return;
	StopFire();
	CurrentSlot = Slot;
	bBinoculars = false;
	ApplyWeaponVisuals();
	ServerSelectSlot(Slot);
}

void AGSCharacter::NextWeapon()
{
	if (AGSPlayerController* TPC = TerminalPC()) { TPC->TerminalInput(TEXT("Q")); return; }
	if (Weapons.Num() < 2) return;
	SelectSlot((CurrentSlot + 1) % Weapons.Num());
}

void AGSCharacter::ServerSelectSlot_Implementation(int32 Slot)
{
	if (!Weapons.IsValidIndex(Slot) || Slot == CurrentSlot) return;
	CurrentSlot = Slot;
	bReloading = false;
	GetWorldTimerManager().ClearTimer(ReloadTimer);
	CancelUse();
	ApplyWeaponVisuals();
}

// ---------------------------------------------------------------- Weapons

FGSWeaponStats AGSCharacter::GetStats(const FGSWeaponSlot& Slot) const
{
	FGSWeaponStats S = GSItems::Weapon(Slot.Id);
	const uint16 A = Slot.Attachments;
	if (A & GSAttach::ExtMag) S.MagSize = FMath::CeilToInt(S.MagSize * 1.5f);
	if (A & GSAttach::Drum) { S.MagSize *= 2; S.ReloadTime *= 1.25f; }
	if (A & GSAttach::RedDot) S.AimSpread *= 0.7f;
	if (A & GSAttach::Holo) { S.AimSpread *= 0.65f; S.AimFOV = FMath::Min(S.AimFOV, 50.f); }
	if (A & GSAttach::Laser) S.HipSpread *= 0.7f;
	if (A & GSAttach::Scope) { S.AimFOV = FMath::Min(S.AimFOV, 25.f); S.AimSpread *= 0.6f; }
	if (A & GSAttach::Scope8x) { S.AimFOV = FMath::Min(S.AimFOV, 11.f); S.AimSpread *= 0.4f; }
	if (A & GSAttach::Suppressor) { S.bSilent = true; S.Damage *= 0.92f; }
	if (A & GSAttach::Compensator) S.RecoilPitch *= 0.7f;
	if (A & GSAttach::Grip) { S.RecoilPitch *= 0.85f; S.HipSpread *= 0.8f; }
	if (Slot.AmmoType == GSAmmoType::Tracer) { S.HipSpread *= 0.85f; S.AimSpread *= 0.85f; }
	return S;
}

bool AGSCharacter::GiveWeapon(FName Id, int32 Ammo, int32 Reserve, uint16 Attachments)
{
	const FGSItemDef* Def = GSItems::Find(Id);
	if (!HasAuthority() || !Def || !Def->IsWeapon()) return false;
	const FGSWeaponStats& S = Def->W;

	for (FGSWeaponSlot& Slot : Weapons)
	{
		if (Slot.Id == Id)
		{
			if (!S.UsesAmmo() || Slot.Reserve >= S.MaxReserve) return false;
			Slot.Reserve = FMath::Min(Slot.Reserve + FMath::Max(S.MagSize * 2, 2), S.MaxReserve);
			ClientToast(FString::Printf(TEXT("+ammo til %s"), *Def->Name));
			return true;
		}
	}

	FGSWeaponSlot New;
	New.Id = Id;
	New.Ammo = Ammo >= 0 ? Ammo : S.MagSize;
	New.Reserve = Reserve >= 0 ? Reserve : S.StartReserve;
	New.Attachments = Attachments;

	if (Weapons.Num() >= MaxWeapons)
	{
		// Hands full: swap with the weapon you hold, and drop that one.
		const FGSWeaponSlot Old = Weapons[CurrentSlot];
		AGSPickup::SpawnDropped(GetWorld(), GetActorLocation() + GetActorForwardVector() * 60.f, Old.Id, 1, Old.Ammo, Old.Reserve, Old.Attachments);
		Weapons[CurrentSlot] = New;
	}
	else
	{
		CurrentSlot = Weapons.Add(New); // switch to what you just picked up
	}
	bReloading = false;
	GetWorldTimerManager().ClearTimer(ReloadTimer);
	ApplyWeaponVisuals();
	return true;
}

float AGSCharacter::GetCurrentSpread() const
{
	const FGSWeaponSlot* W = GetCurrentWeapon();
	if (!W) return 0.f;
	const FGSWeaponStats S = GetStats(*W);
	float Spread = bAiming ? S.AimSpread : S.HipSpread;
	const float Speed = GetVelocity().Size2D();
	Spread += FMath::GetMappedRangeValueClamped(FVector2D(0.f, SprintSpeed), FVector2D(0.f, bAiming ? 1.f : 3.f), Speed);
	if (GetCharacterMovement()->IsFalling()) Spread += 4.f;
	if (bIsCrouched) Spread *= 0.7f;
	Spread += CurrentRecoil * 0.4f;
	return Spread * GetMods().Spread;
}

void AGSCharacter::StartFire()
{
	if (AGSPlayerController* TPC = TerminalPC()) { TPC->TerminalInput(TEXT("Fire")); return; }
	if (bBinoculars) { bBinoculars = false; return; }
	if (!UseItemId.IsNone()) ServerCancelUse();
	bWantsFire = true;
	FireOnce();
	const FGSWeaponSlot* W = GetCurrentWeapon();
	if (W)
	{
		const FGSWeaponStats S = GetStats(*W);
		if (S.bAutomatic)
		{
			GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AGSCharacter::FireOnce, S.FireInterval, true);
		}
	}
}

void AGSCharacter::StopFire()
{
	bWantsFire = false;
	GetWorldTimerManager().ClearTimer(AutoFireTimer);
}

void AGSCharacter::FireOnce()
{
	if (bDead || !IsLocallyControlled()) return;
	FGSWeaponSlot* W = Weapons.IsValidIndex(CurrentSlot) ? &Weapons[CurrentSlot] : nullptr;
	if (!W || bReloading) return;
	const FGSWeaponStats S = GetStats(*W);
	const FGSEffectMods Mods = GetMods();

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFireTime < S.FireInterval * 0.95f) return;
	if (bSprinting) SetSprinting(false);

	if (S.Mode == EGSFireMode::Melee)
	{
		LastFireTime = Now;
		LastMeleeTime = Now;
		ServerMelee(false);
		return;
	}
	if (W->Ammo <= 0)
	{
		StopFire();
		Reload();
		return;
	}
	LastFireTime = Now;

	const FVector Start = Camera->GetComponentLocation();
	const float SpreadRad = FMath::DegreesToRadians(GetCurrentSpread() * 0.5f);
	const FVector Dir = FMath::VRandCone(Camera->GetForwardVector(), SpreadRad);
	const int32 Seed = FMath::Rand();
	const float MuzzleDist = GSItems::GetMesh(W->Id) ? S.MuzzleX : 50.f;
	const FVector MuzzleStart = FPWeaponMesh->GetComponentLocation() + Camera->GetForwardVector() * MuzzleDist;

	// Local, instant feedback (the server decides what actually got hit).
	if (S.Mode == EGSFireMode::Hitscan)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSFireLocal), true, this);
		const FVector End = Start + Dir * S.Range;
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
		AGSShortFX::SpawnTracer(GetWorld(), MuzzleStart, bHit ? Hit.ImpactPoint : End);
		if (bHit && !Cast<AGSCharacter>(Hit.GetActor()))
		{
			AGSShortFX::SpawnImpact(GetWorld(), Hit.ImpactPoint, Hit.ImpactNormal);
		}
	}
	else if (S.Mode == EGSFireMode::Pellets)
	{
		DrawPellets(Start, Camera->GetForwardVector(), Seed, S.Pellets, bAiming ? S.AimSpread : S.HipSpread, true);
	}
	AGSShortFX::SpawnMuzzleFlash(GetWorld(), MuzzleStart, Camera->GetForwardVector(), S.bSilent);

	if (!HasAuthority()) W->Ammo--; // prediction, corrected by replication

	// Recoil
	const float Recoil = S.RecoilPitch * Mods.Recoil;
	CurrentRecoil += Recoil;
	AddWeaponKick(Recoil, S.Class);
	const float AimFactor = bAiming ? 0.6f : 1.f;
	AddControllerPitchInput(-Recoil * AimFactor);
	AddControllerYawInput(FMath::FRandRange(-0.25f, 0.25f) * Recoil * AimFactor);

	ServerFire(Start, S.Mode == EGSFireMode::Hitscan ? Dir : Camera->GetForwardVector(), Seed);

	if (!S.bAutomatic) GetWorldTimerManager().ClearTimer(AutoFireTimer);
}

void AGSCharacter::DrawPellets(const FVector& Start, const FVector& Dir, int32 Seed, int32 Pellets, float ConeDeg, bool bFromMuzzle)
{
	FRandomStream Rng(Seed);
	const FVector From = bFromMuzzle ? FPWeaponMesh->GetComponentLocation() + Dir * 60.f : TPWeaponMesh->GetComponentLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSPelletFx), true, this);
	for (int32 I = 0; I < Pellets; ++I)
	{
		const FVector D = Rng.VRandCone(Dir, FMath::DegreesToRadians(ConeDeg * 0.5f));
		FHitResult Hit;
		const FVector End = Start + D * 4000.f;
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
		AGSShortFX::SpawnTracer(GetWorld(), From, bHit ? Hit.ImpactPoint : End);
		if (bHit && !Cast<AGSCharacter>(Hit.GetActor())) AGSShortFX::SpawnImpact(GetWorld(), Hit.ImpactPoint, Hit.ImpactNormal);
	}
}

void AGSCharacter::ServerFire_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, int32 Seed)
{
	if (bDead || bReloading || !Weapons.IsValidIndex(CurrentSlot)) return;
	FGSWeaponSlot& W = Weapons[CurrentSlot];
	const FGSWeaponStats S = GetStats(W);
	const float Now = GetWorld()->GetTimeSeconds();
	if (S.Mode == EGSFireMode::Melee || W.Ammo <= 0 || Now - ServerLastFireTime < S.FireInterval * 0.7f) return;
	// Basic anti-cheat: the shot must start close to where we really are.
	if (FVector::DistSquared(Start, Camera->GetComponentLocation()) > FMath::Square(250.f)) return;

	ServerLastFireTime = Now;
	W.Ammo--;
	CancelUse();
	const FGSEffectMods Mods = GetMods();
	const FVector D = FVector(Dir).GetSafeNormal();

	if (S.Mode == EGSFireMode::Projectile)
	{
		const FName Kind = S.Class == EGSWeaponClass::Bow ? FName(TEXT("arrow")) : (S.Radius > 550.f ? FName(TEXT("rocket")) : FName(TEXT("grenade40")));
		AGSProjectile::Launch(GetWorld(), this, Kind, W.Id, FVector(Start) + D * 80.f, D * S.ProjSpeed, S.ProjGravity,
			S.Damage * Mods.Damage, S.Radius, 0.f);
		if (!S.bSilent) MulticastShotFX(Start, FVector(Start) + D * 200.f, true);
		if (W.Ammo <= 0 && W.Reserve <= 0 && S.MaxReserve <= 0 && S.Caliber.IsNone())
		{
			// Single-use launcher: the empty tube is thrown away.
			const FName Spent = W.Id;
			Weapons.RemoveAt(CurrentSlot);
			CurrentSlot = FMath::Clamp(CurrentSlot, 0, FMath::Max(0, Weapons.Num() - 1));
			ApplyWeaponVisuals();
			ClientToast(FString::Printf(TEXT("%s er tomt og kastet"), *GSItems::Name(Spent)));
		}
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSFireServer), true, this);
	if (S.Mode == EGSFireMode::Pellets)
	{
		FRandomStream Rng(Seed);
		const float Cone = bAiming ? S.AimSpread : S.HipSpread;
		for (int32 I = 0; I < S.Pellets; ++I)
		{
			const FVector PD = Rng.VRandCone(D, FMath::DegreesToRadians(Cone * 0.5f));
			FHitResult Hit;
			if (GetWorld()->LineTraceSingleByChannel(Hit, Start, FVector(Start) + PD * S.Range, ECC_Visibility, Params))
			{
				if (AGSCharacter* Victim = Cast<AGSCharacter>(Hit.GetActor()))
				{
					FPointDamageEvent Event(S.Damage, Hit, PD, nullptr);
					Victim->IncomingAmmoType = W.AmmoType;
					Victim->TakeDamage(S.Damage * Mods.Damage, Event, GetController(), this);
					Victim->IncomingAmmoType = GSAmmoType::Normal;
				}
			}
		}
		MulticastPelletFX(Start, D, Seed, S.Pellets, Cone);
		NoteShotForRadio(Start);
		return;
	}

	FHitResult Hit;
	const FVector End = FVector(Start) + D * S.Range;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	if (bHit)
	{
		if (AGSCharacter* Victim = Cast<AGSCharacter>(Hit.GetActor()))
		{
			FPointDamageEvent Event(S.Damage, Hit, D, nullptr);
			Victim->IncomingAmmoType = W.AmmoType;
			Victim->TakeDamage(S.Damage * Mods.Damage, Event, GetController(), this);
			Victim->IncomingAmmoType = GSAmmoType::Normal;
		}
	}
	MulticastShotFX(Start, bHit ? FVector(Hit.ImpactPoint) : End, S.bSilent || Mods.bSilent);
}

void AGSCharacter::QuickMelee()
{
	if (TerminalPC()) return;
	if (bDead || bBinoculars) return;
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastMeleeTime < 0.8f) return;
	LastMeleeTime = Now;
	ServerMelee(true);
}

void AGSCharacter::ServerMelee_Implementation(bool bQuick)
{
	if (bDead) return;
	const float Now = GetWorld()->GetTimeSeconds();
	const FGSWeaponSlot* W = GetCurrentWeapon();
	const FGSWeaponStats S = W ? GetStats(*W) : GSItems::Weapon(NAME_None);
	const float Interval = bQuick ? 0.7f : S.FireInterval * 0.8f;
	if (Now - ServerLastFireTime < Interval) return;
	ServerLastFireTime = Now;
	CancelUse();
	DoMeleeHit(bQuick);
}

void AGSCharacter::DoMeleeHit(bool bQuick)
{
	const FGSWeaponSlot* W = GetCurrentWeapon();
	const FGSWeaponStats S = W ? GetStats(*W) : GSItems::Weapon(NAME_None);
	const bool bMeleeWeapon = S.Mode == EGSFireMode::Melee;
	const float Reach = bMeleeWeapon ? S.Range : 160.f;
	const float Damage = (bQuick && !bMeleeWeapon) ? S.MeleeDamage : S.Damage;

	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * Reach;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSMelee), false, this);
	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(22.f), Params))
	{
		if (AGSCharacter* Victim = Cast<AGSCharacter>(Hit.GetActor()))
		{
			FPointDamageEvent Event(Damage, Hit, Camera->GetForwardVector(), nullptr);
			Victim->TakeDamage(Damage * GetMods().Damage, Event, GetController(), this);
		}
		else
		{
			AGSShortFX::SpawnImpact(GetWorld(), Hit.ImpactPoint, Hit.ImpactNormal);
		}
	}
	MulticastMeleeFX();
}

void AGSCharacter::MulticastMeleeFX_Implementation()
{
	if (!IsLocallyControlled()) LastMeleeTime = GetWorld()->GetTimeSeconds();
}

void AGSCharacter::MulticastShotFX_Implementation(FVector_NetQuantize Start, FVector_NetQuantize End, bool bSilent)
{
	if (!bSilent) NoteShotForRadio(Start);
	if (IsLocallyControlled()) return; // shooter already drew it
	const FVector From = TPWeaponMesh->GetComponentLocation();
	if (!bSilent) AGSShortFX::SpawnTracer(GetWorld(), From, End);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSFxTrace), true, this);
	const FVector Dir = (FVector(End) - FVector(Start)).GetSafeNormal();
	if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(End) - Dir * 20.f, FVector(End) + Dir * 20.f, ECC_Visibility, Params)
		&& !Cast<AGSCharacter>(Hit.GetActor()))
	{
		AGSShortFX::SpawnImpact(GetWorld(), Hit.ImpactPoint, Hit.ImpactNormal);
	}
}

void AGSCharacter::MulticastPelletFX_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, int32 Seed, int32 Pellets, float ConeDeg)
{
	if (IsLocallyControlled()) return;
	DrawPellets(Start, Dir, Seed, Pellets, ConeDeg, false);
}

void AGSCharacter::NoteShotForRadio(const FVector& Where)
{
	// Every machine: tell the local player's HUD (it only draws it if they carry a walkie-talkie).
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (AGSHUD* HUD = Cast<AGSHUD>(PC->GetHUD()))
			{
				if (PC->GetPawn() != this) HUD->AddShotMarker(Where);
			}
		}
	}
}

void AGSCharacter::Reload()
{
	if (TerminalPC()) return;
	const FGSWeaponSlot* W = GetCurrentWeapon();
	if (!W || bReloading || bDead) return;
	const FGSWeaponStats S = GetStats(*W);
	const bool bHaveBox = W->Reserve <= 0 && CountItem(AmmoBoxFor(S.Caliber)) > 0;
	if (!S.UsesAmmo() || W->Ammo >= S.MagSize || (W->Reserve <= 0 && !bHaveBox)) return;
	StopFire();
	ServerReload();
}

void AGSCharacter::ServerReload_Implementation()
{
	if (!Weapons.IsValidIndex(CurrentSlot) || bReloading || bDead) return;
	FGSWeaponSlot& W = Weapons[CurrentSlot];
	const FGSWeaponStats S = GetStats(W);
	if (W.Reserve <= 0)
	{
		// Open an ammo box from the sack
		const FName Box = AmmoBoxFor(S.Caliber);
		const FGSItemDef* BoxDef = GSItems::Find(Box);
		if (BoxDef && BoxDef->Rounds > 0 && RemoveItem(Box, 1)) W.Reserve += BoxDef->Rounds;
	}
	if (!S.UsesAmmo() || W.Ammo >= S.MagSize || W.Reserve <= 0) return;
	bReloading = true;
	CancelUse();
	GetWorldTimerManager().SetTimer(ReloadTimer, this, &AGSCharacter::FinishReload, S.ReloadTime * GetMods().Reload, false);
}

void AGSCharacter::FinishReload()
{
	bReloading = false;
	if (!Weapons.IsValidIndex(CurrentSlot)) return;
	FGSWeaponSlot& W = Weapons[CurrentSlot];
	const int32 Need = GetStats(W).MagSize - W.Ammo;
	const int32 Take = FMath::Clamp(Need, 0, W.Reserve);
	W.Ammo += Take;
	W.Reserve -= Take;
}

void AGSCharacter::ClientHitMarker_Implementation(bool bKill)
{
	LastHitMarkerTime = GetWorld()->GetTimeSeconds();
}

FString AGSCharacter::CurrentWeaponName() const
{
	const FGSWeaponSlot* W = GetCurrentWeapon();
	return W ? GSItems::Name(W->Id) : FString(TEXT("Knyttneve"));
}

static void ShowItemOn(UStaticMeshComponent* C, FName Id, const FVector& FallbackScale, float ModelScale, bool bApplyRotation = false)
{
	const FGSItemDef* Def = GSItems::Find(Id);
	if (UStaticMesh* Model = GSItems::GetMesh(Id))
	{
		C->SetStaticMesh(Model);
		C->SetRelativeScale3D(FVector(ModelScale * GSItems::MeshScale(Id)));
		if (bApplyRotation) C->SetRelativeRotation(GSItems::MeshRotation(Id));
		C->EmptyOverrideMaterials();
	}
	else
	{
		C->SetStaticMesh(GSUtils::Cube());
		C->SetRelativeScale3D(FallbackScale);
		GSUtils::SetColor(C, Def ? GSItems::RarityColor(Def->Rarity) * 0.5f : FLinearColor::Gray);
	}
}

// ---------------------------------------------------------------- First-person weapon animation (procedural)
// Works for every weapon without animation assets: equip, idle breathing, walk/sprint bob, mouse sway,
// recoil kick with spring, landing dip, reload in phases (mag out, mag in, charge), inspect and melee swings.

void AGSCharacter::AddWeaponKick(float Strength, EGSWeaponClass Class)
{
	const float Big = Class == EGSWeaponClass::Sniper || Class == EGSWeaponClass::Shotgun || Class == EGSWeaponClass::Launcher ? 1.8f : 1.f;
	const float K = FMath::Clamp(Strength, 0.2f, 4.f) * Big * (bAiming ? 0.55f : 1.f);
	KickVel += FVector(-120.f * K, FMath::FRandRange(-12.f, 12.f) * K, 18.f * K);
	KickRotVel += FRotator(140.f * K, FMath::FRandRange(-30.f, 30.f) * K, FMath::FRandRange(-60.f, 60.f) * K);
}

void AGSCharacter::Inspect()
{
	if (bDead || bReloading || bAiming || !GetCurrentWeapon()) return;
	InspectStart = GetWorld()->GetTimeSeconds();
}

static float Phase01(float T, float A, float B) { return FMath::Clamp((T - A) / FMath::Max(B - A, 0.001f), 0.f, 1.f); }
static float Ease(float X) { return X * X * (3.f - 2.f * X); }
static float Bump(float T, float A, float B) { const float P = Phase01(T, A, B); return FMath::Sin(P * PI); }

void AGSCharacter::UpdateWeaponAnimation(float Dt)
{
	const float Now = GetWorld()->GetTimeSeconds();
	float TargetFOV = DefaultFOV;
	FVector Pos(38.f, 16.f, -16.f);
	FRotator Rot = FRotator::ZeroRotator;
	const FGSWeaponSlot* W = GetCurrentWeapon();
	FGSWeaponStats S;
	if (W) S = GetStats(*W);
	const bool bMeleeWeapon = W && S.Mode == EGSFireMode::Melee;

	// Weapon changed -> equip animation (comes up from below, rotated)
	const FName CurId = W ? W->Id : NAME_None;
	if (CurId != AnimWeaponId) { AnimWeaponId = CurId; EquipStart = Now; InspectStart = -100.f; }
	// Reload started -> remember when, and drop the old magazine
	if (bReloading && !bWasReloading)
	{
		ReloadStart = Now;
		ReloadLength = W ? S.ReloadTime * GetMods().Reload : 2.f;
		bMagDropped = false;
	}
	bWasReloading = bReloading;
	// Landing dip
	const bool bFalling = GetCharacterMovement() && GetCharacterMovement()->IsFalling();
	if (bWasFalling && !bFalling) { KickVel += FVector(0.f, 0.f, -260.f); KickRotVel += FRotator(-90.f, 0.f, 0.f); }
	bWasFalling = bFalling;

	if (W)
	{
		Pos = S.FPHip;
		const float AimAlpha = FMath::FInterpTo(AimBlend, (bAiming && !bMeleeWeapon) ? 1.f : 0.f, Dt, 14.f);
		AimBlend = AimAlpha;
		Pos = FMath::Lerp(S.FPHip, S.FPAim, AimAlpha);
		TargetFOV = FMath::Lerp(DefaultFOV, S.AimFOV, AimAlpha);

		// Idle breathing
		const float Breath = (1.f - AimAlpha * 0.85f);
		Pos.Z += FMath::Sin(Now * 1.6f) * 0.35f * Breath;
		Rot.Pitch += FMath::Sin(Now * 1.6f + 0.6f) * 0.4f * Breath;

		// Walk / sprint bob (figure eight)
		const float Speed01 = FMath::Min(float(GetVelocity().Size2D()) / 600.f, 1.f) * (bFalling ? 0.f : 1.f);
		const float Freq = bSprinting ? 13.f : 8.5f;
		BobTime += Dt * Freq * FMath::Max(Speed01, 0.05f);
		const float BobAmt = Speed01 * (bSprinting ? 2.4f : 1.1f) * (1.f - AimAlpha * 0.8f);
		Pos.Y += FMath::Sin(BobTime) * BobAmt;
		Pos.Z += FMath::Abs(FMath::Cos(BobTime)) * BobAmt * 0.8f - BobAmt * 0.4f;
		Rot.Roll += FMath::Sin(BobTime) * BobAmt * 1.2f;

		// Sprint pose: weapon tilted down and across the chest
		SprintBlend = FMath::FInterpTo(SprintBlend, (bSprinting && !bReloading) ? 1.f : 0.f, Dt, 9.f);
		Pos += FVector(-6.f, -6.f, -6.f) * SprintBlend;
		Rot += FRotator(-18.f, 32.f, -12.f) * SprintBlend;
		TargetFOV += 6.f * SprintBlend;

		// Equip
		const float E = Ease(Phase01(Now - EquipStart, 0.f, 0.35f));
		Pos += FVector(-10.f, 4.f, -32.f) * (1.f - E);
		Rot += FRotator(-45.f, 10.f, 40.f) * (1.f - E);

		// Reload in phases
		if (bReloading && ReloadLength > 0.f)
		{
			const float T = (Now - ReloadStart) / ReloadLength; // 0..1
			const bool bShell = S.Class == EGSWeaponClass::Shotgun || (S.Class == EGSWeaponClass::Sniper && S.MagSize <= 5);
			const float In = Ease(Phase01(T, 0.f, 0.15f)) * (1.f - Ease(Phase01(T, 0.88f, 1.f)));
			Pos += FVector(-6.f, -2.f, -6.f) * In;
			Rot += FRotator(8.f, -12.f, 38.f) * In;
			if (bShell)
			{
				// one shell at a time: rhythmic pushes
				const float Push = FMath::Max(0.f, FMath::Sin(T * PI * 2.f * FMath::Max(2, S.MagSize / 2)));
				Pos.Z += Push * 2.5f * In;
				Rot.Pitch += Push * 4.f * In;
			}
			else
			{
				Pos.Z -= Bump(T, 0.15f, 0.42f) * 5.f;                         // mag out
				Rot.Roll += Bump(T, 0.15f, 0.42f) * 10.f;
				Pos.Z += Bump(T, 0.52f, 0.66f) * 4.f;                         // mag slammed in
				Rot.Pitch += Bump(T, 0.55f, 0.66f) * 6.f;
				Pos.X -= Bump(T, 0.72f, 0.86f) * 7.f;                         // charging handle
				Rot.Yaw += Bump(T, 0.72f, 0.86f) * 14.f;
				if (!bMagDropped && T > 0.25f)
				{
					bMagDropped = true;
					DropMagazineFX();
				}
			}
		}

		// Inspect: turn the weapon to look at the side, then the top
		const float IT = Now - InspectStart;
		if (IT >= 0.f && IT < 3.2f)
		{
			if (bAiming || bWantsFire || bReloading) InspectStart = -100.f;
			const float A = Ease(Phase01(IT, 0.f, 0.5f)) * (1.f - Ease(Phase01(IT, 2.6f, 3.2f)));
			const float Mid = Ease(Phase01(IT, 1.2f, 1.8f));
			Pos += FVector(-4.f, -10.f, 4.f) * A;
			Rot += FRotator(10.f * Mid, 55.f * (1.f - Mid) * A + 20.f * Mid * A, -35.f * (1.f - Mid) * A + 60.f * Mid * A);
		}

		// Melee: wind-up, fast swing, follow-through
		const float SW = Now - LastMeleeTime;
		if (SW < 0.45f)
		{
			const float Wind = Ease(Phase01(SW, 0.f, 0.08f)) * (1.f - Phase01(SW, 0.08f, 0.16f));
			const float Swing = Ease(Phase01(SW, 0.08f, 0.2f)) * (1.f - Ease(Phase01(SW, 0.22f, 0.45f)));
			Rot += FRotator(25.f * Wind - 40.f * Swing, 25.f * Wind - 70.f * Swing, -15.f * Wind + 30.f * Swing);
			Pos += FVector(-6.f * Wind + 30.f * Swing, 10.f * Wind - 25.f * Swing, 8.f * Wind - 4.f * Swing);
		}
	}
	else
	{
		AimBlend = 0.f;
	}

	// Mouse sway: weapon lags behind where you look
	SwayRot = FMath::RInterpTo(SwayRot, FRotator(LookSwayInput.Y * 1.2f, -LookSwayInput.X * 1.4f, -LookSwayInput.X * 2.f) * (1.f - AimBlend * 0.75f), Dt, 10.f);
	SwayRot.Pitch = FMath::Clamp(SwayRot.Pitch, -6.f, 6.f);
	SwayRot.Yaw = FMath::Clamp(SwayRot.Yaw, -7.f, 7.f);
	SwayRot.Roll = FMath::Clamp(SwayRot.Roll, -8.f, 8.f);
	LookSwayInput = FVector2D::ZeroVector;
	Rot += SwayRot;

	// Recoil spring (critically damped-ish)
	const float K = 260.f, D = 24.f;
	KickVel += (-KickPos * K - KickVel * D) * Dt;
	KickPos += KickVel * Dt;
	KickRotVel += (KickRot * -K - KickRotVel * D) * Dt;
	KickRot += KickRotVel * Dt;
	Pos += KickPos * 0.06f;
	Rot += KickRot * 0.06f;

	if (bBinoculars) TargetFOV = 15.f;
	if (bSliding) { TargetFOV += 10.f; Rot.Roll -= 12.f; }
	if (!UseItemId.IsNone()) Pos += FVector(-10.f, 0.f, -30.f);

	// Fab / Megascans replacement models can need a fixed turn and offset (Config/GSMeshOverrides.ini)
	if (W)
	{
		Rot = (FQuat(Rot) * FQuat(GSItems::MeshRotation(W->Id))).Rotator();
		Pos += GSItems::MeshOffset(W->Id);
	}

	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, TargetFOV, Dt, 12.f));
	FPWeaponMesh->SetRelativeLocation(FMath::VInterpTo(FPWeaponMesh->GetRelativeLocation(), Pos, Dt, 22.f));
	FPWeaponMesh->SetRelativeRotation(FMath::RInterpTo(FPWeaponMesh->GetRelativeRotation(), Rot, Dt, 22.f));
}

void AGSCharacter::DropMagazineFX()
{
	// A small magazine falls out and bounces on the floor (client-only, cosmetic).
	UWorld* World = GetWorld();
	if (!World || GetNetMode() == NM_DedicatedServer) return;
	AStaticMeshActor* Mag = World->SpawnActor<AStaticMeshActor>(FPWeaponMesh->GetComponentLocation() + FVector(0.f, 0.f, -12.f), FPWeaponMesh->GetComponentRotation());
	if (!Mag) return;
	Mag->SetMobility(EComponentMobility::Movable);
	UStaticMeshComponent* C = Mag->GetStaticMeshComponent();
	C->SetStaticMesh(GSUtils::Cube());
	C->SetWorldScale3D(FVector(0.03f, 0.07f, 0.16f));
	GSUtils::SetColor(C, FLinearColor(0.04f, 0.04f, 0.045f));
	C->SetCollisionProfileName(TEXT("PhysicsActor"));
	C->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	C->SetSimulatePhysics(true);
	C->SetPhysicsLinearVelocity(GetVelocity() + Camera->GetRightVector() * 60.f + FVector(0.f, 0.f, -100.f));
	C->SetPhysicsAngularVelocityInDegrees(FVector(FMath::FRandRange(-300.f, 300.f), 200.f, 0.f));
	Mag->SetLifeSpan(8.f);
}

void AGSCharacter::ApplyWeaponVisuals()
{
	const FGSWeaponSlot* W = GetCurrentWeapon();
	const bool bHas = W && !W->Id.IsNone() && !bDead;
	FPWeaponMesh->SetVisibility(bHas && !bBinoculars);
	TPWeaponMesh->SetVisibility(bHas);
	if (!bHas) return;
	const FGSWeaponStats& S = GSItems::Weapon(W->Id);
	const FVector Fallback = S.Class == EGSWeaponClass::Pistol ? FVector(0.22f, 0.04f, 0.12f)
		: S.Class == EGSWeaponClass::Melee ? FVector(0.7f, 0.03f, 0.05f) : FVector(0.95f, 0.05f, 0.11f);
	ShowItemOn(FPWeaponMesh, W->Id, Fallback, 1.f);
	ShowItemOn(TPWeaponMesh, W->Id, Fallback, 1.f, true);
}

void AGSCharacter::ApplyGearVisuals()
{
	VestMesh->SetVisibility(!BodyArmor.IsNone() && !bDead);
	if (!BodyArmor.IsNone()) ShowItemOn(VestMesh, BodyArmor, FVector(0.68f, 0.68f, 0.5f), 1.6f);
	HelmetMesh->SetVisibility(!Helmet.IsNone() && !bDead);
	if (!Helmet.IsNone()) ShowItemOn(HelmetMesh, Helmet, FVector(0.32f, 0.32f, 0.18f), 1.15f);
	Flashlight->SetVisibility(bFlashlightOn && !bDead);
}

void AGSCharacter::OnRep_Weapons() { ApplyWeaponVisuals(); }
void AGSCharacter::OnRep_CurrentSlot() { ApplyWeaponVisuals(); }
void AGSCharacter::OnRep_Gear() { ApplyGearVisuals(); }

// ---------------------------------------------------------------- Inventory

int32 AGSCharacter::CountItem(FName Id) const
{
	int32 N = 0;
	for (const FGSInvStack& S : Backpack) if (S.Id == Id) N += S.Count;
	return N;
}

bool AGSCharacter::HasItemKind(FName Kind) const
{
	for (const FGSInvStack& S : Backpack)
	{
		const FGSItemDef* D = GSItems::Find(S.Id);
		if (D && D->Kind == Kind) return true;
	}
	return false;
}

int32 AGSCharacter::AddToBackpack(FName Id, int32 Count)
{
	const FGSItemDef* Def = GSItems::Find(Id);
	if (!HasAuthority() || !Def || Count <= 0) return 0;
	int32 Left = Count;
	for (FGSInvStack& S : Backpack)
	{
		if (S.Id != Id || S.Count >= Def->MaxStack) continue;
		const int32 Add = FMath::Min(Left, Def->MaxStack - S.Count);
		S.Count += Add;
		Left -= Add;
		if (Left == 0) return Count;
	}
	while (Left > 0 && Backpack.Num() < GetBackpackSlots())
	{
		FGSInvStack S;
		S.Id = Id;
		S.Count = FMath::Min(Left, Def->MaxStack);
		Left -= S.Count;
		Backpack.Add(S);
	}
	return Count - Left;
}

bool AGSCharacter::RemoveItem(FName Id, int32 Count)
{
	for (int32 I = Backpack.Num() - 1; I >= 0 && Count > 0; --I)
	{
		if (Backpack[I].Id != Id) continue;
		const int32 Take = FMath::Min(Count, Backpack[I].Count);
		Backpack[I].Count -= Take;
		Count -= Take;
		if (Backpack[I].Count <= 0) Backpack.RemoveAt(I);
	}
	return Count == 0;
}

int32 AGSCharacter::TakeFromPickup(AGSPickup* Pickup)
{
	const FGSItemDef* Def = Pickup ? GSItems::Find(Pickup->ItemId) : nullptr;
	if (!Def || !HasAuthority()) return 0;

	switch (Def->Cat)
	{
	case EGSItemCat::Weapon:
		return GiveWeapon(Def->Id, Pickup->Ammo, Pickup->Reserve, Pickup->Attachments) ? 1 : 0;

	case EGSItemCat::Armor:
	case EGSItemCat::Helmet:
	{
		const bool bBody = Def->Cat == EGSItemCat::Armor;
		FName& Slot = bBody ? BodyArmor : Helmet;
		float& Points = bBody ? ArmorPoints : HelmetPoints;
		if (!Slot.IsNone())
		{
			AGSPickup::SpawnDropped(GetWorld(), GetActorLocation() + GetActorForwardVector() * 60.f, Slot, 1);
		}
		Slot = Def->Id;
		Points = Def->Armor;
		ClientToast(FString::Printf(TEXT("Tok p\u00E5 %s (%d panser)"), *Def->Name, FMath::RoundToInt(Def->Armor)));
		ApplyGearVisuals();
		return 1;
	}
	default:
		break;
	}

	if (Def->Kind == KindCaliber && Def->Rounds > 0)
	{
		// Ammo boxes go straight into the guns that use them; whole boxes that do not fit wait in the sack.
		int32 Rounds = Def->Rounds * Pickup->Count;
		const int32 Before = Rounds;
		for (FGSWeaponSlot& W : Weapons)
		{
			const FGSWeaponStats S = GetStats(W);
			if (S.Caliber != Def->Caliber || S.MaxReserve <= 0) continue;
			const int32 Add = FMath::Clamp(S.MaxReserve - W.Reserve, 0, Rounds);
			W.Reserve += Add;
			Rounds -= Add;
		}
		const int32 UsedBoxes = (Before - Rounds) / Def->Rounds;
		const int32 LeftBoxes = Pickup->Count - UsedBoxes;
		const int32 Stored = LeftBoxes > 0 ? AddToBackpack(Def->Id, LeftBoxes) : 0;
		const int32 Taken = UsedBoxes + Stored;
		if (Before - Rounds > 0) ClientToast(FString::Printf(TEXT("+%d skudd %s"), Before - Rounds, *Def->Name));
		else if (Stored > 0) ClientToast(FString::Printf(TEXT("%s lagt i sekken"), *Def->Name));
		else ClientToast(TEXT("Sekken er full"));
		return FMath::Max(Taken, (Before - Rounds) > 0 ? 1 : 0);
	}

	if (Def->Kind == KindBackpack)
	{
		if (Def->Slots <= BackpackBonus)
		{
			ClientToast(TEXT("Du har allerede en like stor sekk"));
			return 0;
		}
		BackpackBonus = Def->Slots;
		ClientToast(FString::Printf(TEXT("%s: %d plasser i sekken"), *Def->Name, GetBackpackSlots()));
		return 1;
	}

	const int32 Added = AddToBackpack(Def->Id, Pickup->Count);
	if (Added == 0) ClientToast(TEXT("Sekken er full"));
	return Added;
}

void AGSCharacter::UpdateFocusPickup()
{
	FocusPickup = nullptr;
	FocusTerminal = nullptr;
	const FVector Eye = Camera->GetComponentLocation();
	const FVector Fwd = Camera->GetForwardVector();
	float Best = -1.f;
	for (TActorIterator<AGSTerminal> It(GetWorld()); It; ++It)
	{
		AGSTerminal* T = *It;
		const FVector To = T->GetActorLocation() + FVector(0.f, 0.f, 40.f) - Eye;
		const float Dist = To.Size();
		if (Dist > 240.f) continue;
		const float Dot = FVector::DotProduct(To / FMath::Max(Dist, 1.f), Fwd);
		const float Score = Dot - Dist / 1000.f;
		if (Dot > 0.8f && Score > Best)
		{
			Best = Score;
			FocusTerminal = T;
		}
	}
	if (FocusTerminal.IsValid()) return;
	for (TActorIterator<AGSPickup> It(GetWorld()); It; ++It)
	{
		AGSPickup* P = *It;
		if (!P->bAvailable) continue;
		const FVector To = P->GetActorLocation() - Eye;
		const float Dist = To.Size();
		if (Dist > 280.f) continue;
		const float Dot = FVector::DotProduct(To / FMath::Max(Dist, 1.f), Fwd);
		const float Score = Dot - Dist / 1000.f;
		if ((Dot > 0.75f || Dist < 110.f) && Score > Best)
		{
			Best = Score;
			FocusPickup = P;
		}
	}
}

void AGSCharacter::Interact()
{
	if (bDead) return;
	AGSPlayerController* PC = Cast<AGSPlayerController>(GetController());
	if (PC && PC->IsOnTerminal()) { PC->TerminalInput(TEXT("E")); return; }
	if (AGSTerminal* T = FocusTerminal.Get())
	{
		if (PC) PC->OpenTerminal(T);
		return;
	}
	if (AGSPickup* P = FocusPickup.Get()) ServerInteract(P);
}

AGSPlayerController* AGSCharacter::TerminalPC() const
{
	AGSPlayerController* PC = Cast<AGSPlayerController>(GetController());
	return PC && PC->IsOnTerminal() ? PC : nullptr;
}

bool AGSCharacter::ConsumeItem(FName Id, int32 Count)
{
	if (!HasAuthority() || CountItem(Id) < Count) return false;
	return Count <= 0 || RemoveItem(Id, Count);
}

void AGSCharacter::ServerInteract_Implementation(AGSPickup* Pickup)
{
	if (bDead || !Pickup || !Pickup->bAvailable) return;
	if (FVector::Dist(Pickup->GetActorLocation(), GetActorLocation()) > 350.f) return;

	if (Pickup->bLocked)
	{
		const FGSItemDef* Key = nullptr;
		for (const FGSInvStack& S : Backpack)
		{
			const FGSItemDef* D = GSItems::Find(S.Id);
			if (D && D->Kind == KindKey) { Key = D; break; }
		}
		if (Key)
		{
			RemoveItem(Key->Id, 1);
			Pickup->Unlock();
			ClientToast(TEXT("Adgangskort godkjent. Kassen er \u00E5pen!"));
			return;
		}
		for (int32 I = 0; I < Backpack.Num(); ++I)
		{
			const FGSItemDef* D = GSItems::Find(Backpack[I].Id);
			if (D && D->Kind == KindLockpick)
			{
				UseTargetPickup = Pickup;
				UseIndex = I;
				UseItemId = D->Id;
				UseStartTime = GetServerTime();
				UseEndTime = UseStartTime + D->UseTime;
				ClientToast(TEXT("Dirker opp l\u00E5sen ..."));
				return;
			}
		}
		ClientToast(TEXT("L\u00E5st. Du trenger NTNU-adgangskort eller dirkesett."));
		return;
	}

	const int32 Taken = TakeFromPickup(Pickup);
	if (Taken > 0) Pickup->OnTaken(Taken);
}

void AGSCharacter::NextItem()
{
	if (AGSPlayerController* TPC = TerminalPC()) { TPC->TerminalInput(TEXT("Q")); return; }
	if (Backpack.Num() == 0) { SelectedItem = 0; return; }
	SelectedItem = (SelectedItem + 1) % Backpack.Num();
}

void AGSCharacter::UseSelectedItem()
{
	if (AGSPlayerController* TPC = TerminalPC()) { TPC->TerminalInput(TEXT("F")); return; }
	if (bDead || !Backpack.IsValidIndex(SelectedItem)) return;
	if (!UseItemId.IsNone()) { ServerCancelUse(); return; }
	const FGSItemDef* Def = GSItems::Find(Backpack[SelectedItem].Id);
	if (!Def) return;

	if (IsLocalOnlyGadget(*Def))
	{
		if (Def->Kind == KindZoom) { bBinoculars = !bBinoculars; StopFire(); ApplyWeaponVisuals(); }
		else if (Def->Kind == KindNightVision) { bNightVisionGoggles = !bNightVisionGoggles; }
		else if (Def->Kind == KindKey || Def->Kind == KindLockpick) { ToastText = TEXT("Se p\u00E5 en l\u00E5st kasse og trykk E"); ToastTime = GetWorld()->GetTimeSeconds(); }
		else { ToastText = Def->Desc; ToastTime = GetWorld()->GetTimeSeconds(); }
		return;
	}
	if (Def->Kind == KindLight) { ServerToggleFlashlight(); return; }
	if (Def->Kind == KindGrapple)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSGrapple), false, this);
		const FVector Start = Camera->GetComponentLocation();
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Camera->GetForwardVector() * 3500.f, ECC_WorldStatic, Params))
		{
			ServerGrapple(Hit.ImpactPoint);
		}
		else
		{
			ToastText = TEXT("For langt unna (maks 35 m)");
			ToastTime = GetWorld()->GetTimeSeconds();
		}
		return;
	}
	ServerUseItem(SelectedItem);
}

void AGSCharacter::ServerToggleFlashlight_Implementation()
{
	if (!HasItemKind(KindLight)) return;
	bFlashlightOn = !bFlashlightOn;
	ApplyGearVisuals();
}

void AGSCharacter::ServerGrapple_Implementation(FVector_NetQuantize Target)
{
	if (bDead || !HasItemKind(KindGrapple)) return;
	const FVector To = FVector(Target) - GetActorLocation();
	if (To.Size() > 3700.f) return;
	for (const FGSInvStack& S : Backpack)
	{
		const FGSItemDef* D = GSItems::Find(S.Id);
		if (D && D->Kind == KindGrapple) { RemoveItem(S.Id, 1); break; }
	}
	// Fly towards the hook point, with some extra lift so you land on top.
	const FVector Launch = To.GetSafeNormal() * FMath::Clamp(float(To.Size()) * 1.6f, 900.f, 2600.f) + FVector(0.f, 0.f, 450.f);
	LaunchCharacter(Launch, true, true);
	AGSShortFX::SpawnTracer(GetWorld(), Camera->GetComponentLocation(), Target);
}

void AGSCharacter::ServerUseItem_Implementation(int32 Index)
{
	if (bDead || !Backpack.IsValidIndex(Index) || !UseItemId.IsNone()) return;
	const FGSItemDef* Def = GSItems::Find(Backpack[Index].Id);
	if (!Def || IsLocalOnlyGadget(*Def)) return;
	UseIndex = Index;
	UseItemId = Def->Id;
	UseTargetPickup = nullptr;
	UseStartTime = GetServerTime();
	UseEndTime = UseStartTime + Def->UseTime;
	if (Def->UseTime <= 0.01f) CompleteUse();
}

void AGSCharacter::ServerCancelUse_Implementation() { CancelUse(); }

void AGSCharacter::CancelUse()
{
	if (!HasAuthority()) return;
	UseItemId = NAME_None;
	UseIndex = INDEX_NONE;
	UseTargetPickup = nullptr;
}

void AGSCharacter::CompleteUse()
{
	const FGSItemDef* Def = GSItems::Find(UseItemId);
	const FName Id = UseItemId;
	AGSPickup* Target = UseTargetPickup.Get();
	CancelUse();
	if (!Def || CountItem(Id) <= 0) return;

	// Lock picking a crate
	if (Def->Kind == KindLockpick)
	{
		if (Target && Target->bLocked && FVector::Dist(Target->GetActorLocation(), GetActorLocation()) < 400.f)
		{
			RemoveItem(Id, 1);
			Target->Unlock();
			ClientToast(TEXT("Kassen er dirket opp!"));
		}
		return;
	}

	bool bConsume = true;
	switch (Def->Cat)
	{
	case EGSItemCat::Food:
	case EGSItemCat::Drug:
	case EGSItemCat::Medicine:
	{
		Health = FMath::Min(MaxHealth, Health + Def->Heal);
		Hunger = FMath::Clamp(Hunger + Def->Hunger, 0.f, 100.f);
		Thirst = FMath::Clamp(Thirst + Def->Thirst, 0.f, 100.f);
		Stamina = FMath::Clamp(Stamina + Def->StaminaGain, 0.f, 100.f);
		if (!Def->Effect.IsNone())
		{
			if (Def->Poison <= 0.f || FMath::FRand() < Def->Poison)
			{
				AddEffect(Def->Effect);
				if (Def->Effect == FxPoison) ClientToast(TEXT("Uff. Den pizzaen var ikke god ..."));
			}
		}
		OnRep_Health();
		break;
	}
	case EGSItemCat::Throwable:
		ThrowItem(*Def);
		break;
	case EGSItemCat::Gadget:
	{
		const FName K = Def->Kind;
		if (K == KindDrone) { AddEffect(FxDrone); ClientToast(TEXT("Dronen skanner omr\u00E5det ...")); }
		else if (K == KindParachute) { AddEffect(FxGlide); GlideStartTime = GetWorld()->GetTimeSeconds(); ClientToast(TEXT("Fallskjerm klar. Hopp!")); }
		else if (K == KindDefib) { AddEffect(Def->Effect); ClientToast(TEXT("Hjertestarteren er ladet. Du st\u00E5r opp igjen hvis du d\u00F8r.")); }
		else if (K == KindAmmo)
		{
			for (FGSWeaponSlot& W : Weapons)
			{
				const FGSWeaponStats S = GetStats(W);
				if (S.UsesAmmo()) W.Reserve = FMath::Min(S.MaxReserve, W.Reserve + FMath::Max(S.MagSize * 2, 3));
			}
			ClientToast(TEXT("Ammo fylt p\u00E5"));
		}
		else if (K == KindPlate)
		{
			const FGSItemDef* Vest = GSItems::Find(BodyArmor);
			if (!Vest) { ClientToast(TEXT("Du har ingen vest \u00E5 reparere")); bConsume = false; }
			else ArmorPoints = FMath::Min(Vest->Armor, ArmorPoints + 40.f);
		}
		else if (K.ToString().StartsWith(TEXT("att_")))
		{
			FGSWeaponSlot* W = Weapons.IsValidIndex(CurrentSlot) ? &Weapons[CurrentSlot] : nullptr;
			const FGSWeaponStats* S = W ? &GSItems::Weapon(W->Id) : nullptr;
			const bool bGun = S && S->Mode != EGSFireMode::Melee && S->Class != EGSWeaponClass::Launcher;
			uint16 Bit = 0;
			if (K == AttSuppressor) Bit = GSAttach::Suppressor;
			else if (K == AttRedDot) Bit = GSAttach::RedDot;
			else if (K == AttScope) Bit = GSAttach::Scope;
			else if (K == AttExtMag) Bit = GSAttach::ExtMag;
			else if (K == AttLaser) Bit = GSAttach::Laser;
			else if (K == AttHolo) Bit = GSAttach::Holo;
			else if (K == AttScope8x) Bit = GSAttach::Scope8x;
			else if (K == AttDrum) Bit = GSAttach::Drum;
			else if (K == AttComp) Bit = GSAttach::Compensator;
			else if (K == AttGrip) Bit = GSAttach::Grip;
			const bool bLongGun = S && (S->Class == EGSWeaponClass::Rifle || S->Class == EGSWeaponClass::Sniper || S->Class == EGSWeaponClass::LMG || S->Class == EGSWeaponClass::Bow);
			const bool bScopeOk = (Bit != GSAttach::Scope && Bit != GSAttach::Scope8x) || bLongGun;
			const bool bMagOk = (Bit != GSAttach::ExtMag && Bit != GSAttach::Drum) || (S && S->MagSize > 1 && S->Mode != EGSFireMode::Projectile);
			if (!bGun || Bit == 0 || !bScopeOk || !bMagOk || (W->Attachments & Bit))
			{
				ClientToast(TEXT("Passer ikke p\u00E5 v\u00E5penet du holder"));
				bConsume = false;
			}
			else
			{
				// Only one sight, one magazine and one muzzle device at a time: the old one goes back in the sack.
				for (const uint16 Group : { GSAttach::SightGroup, GSAttach::MagGroup, GSAttach::MuzzleGroup })
				{
					if (!(Group & Bit)) continue;
					const uint16 Old = W->Attachments & Group;
					for (uint16 B = 1; B != 0 && B <= Old; B <<= 1)
					{
						if (Old & B)
						{
							W->Attachments &= ~B;
							const FName OldItem = AttachmentItem(B);
							if (!OldItem.IsNone() && AddToBackpack(OldItem, 1) == 0)
							{
								AGSPickup::SpawnDropped(GetWorld(), GetActorLocation() + GetActorForwardVector() * 60.f, OldItem, 1);
							}
						}
					}
				}
				W->Attachments |= Bit;
				W->Ammo = FMath::Min(W->Ammo, GetStats(*W).MagSize);
				ClientToast(FString::Printf(TEXT("%s satt p\u00E5 %s"), *Def->Name, *GSItems::Name(W->Id)));
			}
		}
		else if (K.ToString().StartsWith(TEXT("ammotype_")))
		{
			FGSWeaponSlot* W = Weapons.IsValidIndex(CurrentSlot) ? &Weapons[CurrentSlot] : nullptr;
			const FGSWeaponStats* S = W ? &GSItems::Weapon(W->Id) : nullptr;
			if (!S || S->Mode == EGSFireMode::Melee || S->Mode == EGSFireMode::Projectile)
			{
				ClientToast(TEXT("Hold en pistol, et gev\u00E6r eller en hagle f\u00F8rst"));
				bConsume = false;
			}
			else
			{
				const FString T = K.ToString();
				W->AmmoType = T.EndsWith(TEXT("_ap")) ? GSAmmoType::ArmorPiercing : T.EndsWith(TEXT("_hp")) ? GSAmmoType::HollowPoint
					: T.EndsWith(TEXT("_fire")) ? GSAmmoType::Incendiary : T.EndsWith(TEXT("_tracer")) ? GSAmmoType::Tracer : GSAmmoType::Shock;
				W->Ammo = GetStats(*W).MagSize; // a fresh magazine of the special rounds
				ClientToast(FString::Printf(TEXT("%s ladet med %s"), *GSItems::Name(W->Id), *Def->Name));
			}
		}
		}
		else bConsume = false;
		break;
	}
	default:
		bConsume = false;
		break;
	}
	if (bConsume) RemoveItem(Id, 1);
}

void AGSCharacter::ThrowItem(const FGSItemDef& Def)
{
	const FVector Fwd = Camera->GetForwardVector();
	const bool bPlace = Def.Kind == FName(TEXT("mine")) || Def.Kind == FName(TEXT("c4"));
	const FVector Vel = Fwd * (bPlace ? 700.f : 1500.f) + FVector(0.f, 0.f, bPlace ? 100.f : 250.f);
	AGSProjectile::Launch(GetWorld(), this, Def.Kind, Def.Id, Camera->GetComponentLocation() + Fwd * 50.f, Vel + GetVelocity() * 0.5f,
		1.f, Def.ThrowDamage, Def.ThrowRadius, Def.Fuse);
}

void AGSCharacter::DropSelectedItem()
{
	if (TerminalPC()) return;
	if (bDead || !Backpack.IsValidIndex(SelectedItem)) return;
	ServerDropItem(SelectedItem);
}

void AGSCharacter::ServerDropItem_Implementation(int32 Index)
{
	if (bDead || !Backpack.IsValidIndex(Index)) return;
	const FGSInvStack S = Backpack[Index];
	Backpack.RemoveAt(Index);
	if (S.Id == UseItemId) CancelUse();
	AGSPickup::SpawnDropped(GetWorld(), GetActorLocation() + GetActorForwardVector() * 80.f, S.Id, S.Count);
}

void AGSCharacter::DropEverything()
{
	TArray<TTuple<FName, int32, FGSWeaponSlot>> Drops;
	for (const FGSWeaponSlot& W : Weapons)
	{
		if (W.Id != GSItems::Pistol) Drops.Add(MakeTuple(W.Id, 1, W));
	}
	for (const FGSInvStack& S : Backpack) Drops.Add(MakeTuple(S.Id, S.Count, FGSWeaponSlot()));
	if (!BodyArmor.IsNone() && ArmorPoints > 5.f) Drops.Add(MakeTuple(BodyArmor, 1, FGSWeaponSlot()));
	if (!Helmet.IsNone() && HelmetPoints > 5.f) Drops.Add(MakeTuple(Helmet, 1, FGSWeaponSlot()));

	const FVector Center = GetActorLocation() - FVector(0.f, 0.f, 40.f);
	for (int32 I = 0; I < Drops.Num(); ++I)
	{
		const float Angle = I * 2.f * PI / FMath::Max(1, Drops.Num());
		const FVector Loc = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (70.f + 15.f * (I / 8));
		const FGSWeaponSlot& W = Drops[I].Get<2>();
		AGSPickup::SpawnDropped(GetWorld(), Loc, Drops[I].Get<0>(), Drops[I].Get<1>(),
			W.Id.IsNone() ? -1 : W.Ammo, W.Id.IsNone() ? -1 : W.Reserve, W.Attachments);
	}
	Weapons.Reset();
	Backpack.Reset();
}

// ---------------------------------------------------------------- Effects

FGSEffectMods AGSCharacter::GetMods() const
{
	FGSEffectMods M;
	const float Now = GetServerTime();
	for (const FGSActiveEffect& E : Effects)
	{
		if (E.EndTime < Now) continue;
		if (const FGSEffectDef* Def = GSItems::FindEffect(E.Id)) M.Combine(Def->Mods);
	}
	return M;
}

void AGSCharacter::AddEffect(FName EffectId)
{
	const FGSEffectDef* Def = GSItems::FindEffect(EffectId);
	if (!Def || !HasAuthority()) return;
	const float End = GetServerTime() + Def->Duration;
	for (FGSActiveEffect& E : Effects)
	{
		if (E.Id == EffectId) { E.EndTime = End; return; }
	}
	FGSActiveEffect E;
	E.Id = EffectId;
	E.EndTime = End;
	Effects.Add(E);
}

void AGSCharacter::TickEffects()
{
	const float Now = GetServerTime();
	TArray<FName> Starting;
	for (int32 I = Effects.Num() - 1; I >= 0; --I)
	{
		if (Effects[I].EndTime > Now) continue;
		if (const FGSEffectDef* Def = GSItems::FindEffect(Effects[I].Id))
		{
			if (!Def->After.IsNone()) Starting.Add(Def->After);
		}
		Effects.RemoveAt(I);
	}
	for (const FName& Id : Starting) AddEffect(Id);

	// Parachute ends when you land.
	if (GetWorld()->GetTimeSeconds() - GlideStartTime > 2.f && !GetCharacterMovement()->IsFalling())
	{
		Effects.RemoveAll([](const FGSActiveEffect& E) { return E.Id == FxGlide; });
	}
}

void AGSCharacter::ApplyFlash(const FVector& Location, float Radius)
{
	if (bDead) return;
	const FVector Eye = Camera->GetComponentLocation();
	const float Dist = FVector::Dist(Location, Eye);
	if (Dist > Radius || GetMods().bFlashImmune) return;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSFlash), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Location, Eye, ECC_WorldStatic, Params)) return; // behind a wall
	const FVector View = GetControlRotation().Vector();
	const float Facing = FVector::DotProduct(View, (Location - Eye).GetSafeNormal());
	if (Facing > 0.2f || Dist < 400.f) AddEffect(FxFlash);
}

void AGSCharacter::ClientToast_Implementation(const FString& Message)
{
	ToastText = Message;
	ToastTime = GetWorld()->GetTimeSeconds();
}

// ---------------------------------------------------------------- Health

float AGSCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || bDead) return 0.f;

	bool bHeadshot = false;
	float Damage = DamageAmount;
	AGSCharacter* Shooter = Cast<AGSCharacter>(DamageCauser);
	FString WeaponName = TEXT("Verden");
	if (Shooter) WeaponName = Shooter->CurrentWeaponName();
	if (const AGSProjectile* Proj = Cast<AGSProjectile>(DamageCauser))
	{
		WeaponName = GSItems::Name(Proj->ItemId);
		Shooter = Cast<AGSCharacter>(Proj->GetOwner());
	}

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PE = static_cast<const FPointDamageEvent&>(DamageEvent);
		if (PE.HitInfo.GetComponent() == HeadMesh)
		{
			bHeadshot = true;
			if (const AGSCharacter* S = Cast<AGSCharacter>(DamageCauser))
			{
				if (const FGSWeaponSlot* W = S->GetCurrentWeapon()) Damage *= GSItems::Weapon(W->Id).HeadshotMultiplier;
				else Damage *= 1.5f;
			}
			else
			{
				Damage *= 2.f; // arrows
			}
		}
	}

	Damage *= GetMods().Taken;

	// Special rounds
	const uint8 Ammo = IncomingAmmoType;
	const bool bArmored = bHeadshot ? !Helmet.IsNone() : !BodyArmor.IsNone();
	float AbsorbMul = 1.f;
	if (Ammo == GSAmmoType::ArmorPiercing) { AbsorbMul = 0.35f; Damage *= 0.95f; }
	else if (Ammo == GSAmmoType::HollowPoint) Damage *= bArmored ? 0.75f : 1.3f;
	else if (Ammo == GSAmmoType::Incendiary) AddEffect(TEXT("Brennende"));
	else if (Ammo == GSAmmoType::Shock) AddEffect(TEXT("Sjokk"));

	// Armor soaks up part of the damage until it breaks.
	if (bHeadshot && !Helmet.IsNone())
	{
		const FGSItemDef* H = GSItems::Find(Helmet);
		const float Soak = FMath::Min(Damage * (H ? H->Absorb : 0.3f) * AbsorbMul, HelmetPoints);
		HelmetPoints -= Soak;
		Damage -= Soak;
		if (HelmetPoints <= 0.5f) { Helmet = NAME_None; HelmetPoints = 0.f; ApplyGearVisuals(); ClientToast(TEXT("Hjelmen gikk i stykker!")); }
	}
	else if (!bHeadshot && !BodyArmor.IsNone())
	{
		const FGSItemDef* A = GSItems::Find(BodyArmor);
		const float Soak = FMath::Min(Damage * (A ? A->Absorb : 0.3f) * AbsorbMul, ArmorPoints);
		ArmorPoints -= Soak;
		Damage -= Soak;
		if (ArmorPoints <= 0.5f) { BodyArmor = NAME_None; ArmorPoints = 0.f; ApplyGearVisuals(); ClientToast(TEXT("Vesten er \u00F8delagt!")); }
	}

	Health = FMath::Max(0.f, Health - Damage);
	LastDamageTakenTime = GetWorld()->GetTimeSeconds();
	LastDamageWeapon = WeaponName;
	LastDamageInstigator = EventInstigator;
	OnRep_Health();

	// Blodtorst: the shooter heals from damage dealt.
	if (Shooter && Shooter != this)
	{
		const float Steal = Shooter->GetMods().Lifesteal;
		if (Steal > 0.f) Shooter->Health = FMath::Min(Shooter->MaxHealth, Shooter->Health + Damage * Steal);
	}

	bool bKilled = Health <= 0.f;
	if (bKilled && GetMods().bPhoenix)
	{
		// Foniks: get back up once.
		Health = 50.f;
		Effects.RemoveAll([](const FGSActiveEffect& E) { return E.Id == FxFoniks; });
		AddEffect(FName(TEXT("Adrenalin")));
		ClientToast(TEXT("F\u00D8NIKS! Du reiste deg fra de d\u00F8de."));
		bKilled = false;
	}
	if (Shooter && Shooter != this)
	{
		Shooter->ClientHitMarker(bKilled);
	}
	if (bKilled)
	{
		Die(EventInstigator, bHeadshot, WeaponName);
	}
	return Damage;
}

void AGSCharacter::OnRep_Health()
{
	if (IsLocallyControlled())
	{
		LastDamageTakenTime = GetWorld()->GetTimeSeconds();
	}
}

void AGSCharacter::Die(AController* Killer, bool bHeadshot, const FString& WeaponName)
{
	if (bDead) return;
	bDead = true;
	StopFire();
	CancelUse();

	if (AGSGameMode* GM = GetWorld()->GetAuthGameMode<AGSGameMode>())
	{
		GM->OnPlayerKilled(Killer, GetController(), WeaponName, bHeadshot);
	}
	DropEverything();
	bFlashlightOn = false;

	OnRep_Dead();
	DetachFromControllerPendingDestroy();
	SetLifeSpan(15.f);
}

void AGSCharacter::OnRep_Dead()
{
	if (!bDead) return;
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Simple "fall over" until we have real ragdolls.
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -80.f));
	BodyMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	HeadMesh->SetRelativeLocation(FVector(95.f, 0.f, -80.f));
	ApplyWeaponVisuals();
	ApplyGearVisuals();
}

// ---------------------------------------------------------------- Tick

void AGSCharacter::TickSurvival(float DeltaSeconds)
{
	const FGSEffectMods Mods = GetMods();
	const float Effort = bSprinting ? 1.6f : 1.f;
	Hunger = FMath::Max(0.f, Hunger - 100.f / 1500.f * Mods.Hunger * Effort * DeltaSeconds);   // ~25 min
	Thirst = FMath::Max(0.f, Thirst - 100.f / 1080.f * Mods.Thirst * Effort * DeltaSeconds);   // ~18 min

	float HealthDelta = Mods.Regen - Mods.Drain;
	if (Hunger <= 0.f) HealthDelta -= 1.f;
	if (Thirst <= 0.f) HealthDelta -= 1.f;
	if (HealthDelta == 0.f) return;

	StarveAccumulator += HealthDelta * DeltaSeconds;
	if (FMath::Abs(StarveAccumulator) < 1.f) return; // replicate whole points only
	const float Change = FMath::TruncToFloat(StarveAccumulator);
	StarveAccumulator -= Change;
	Health = FMath::Clamp(Health + Change, 0.f, MaxHealth);
	if (Health <= 0.f)
	{
		if (Mods.bPhoenix) { Health = 50.f; Effects.RemoveAll([](const FGSActiveEffect& E) { return E.Id == FxFoniks; }); return; }
		const FString Cause = Hunger <= 0.f ? TEXT("Sult") : Thirst <= 0.f ? TEXT("T\u00F8rst") : TEXT("Overdose");
		Die(nullptr, false, Cause);
	}
}

void AGSCharacter::UpdateLocalVisuals(float DeltaSeconds)
{
	const FGSEffectMods Mods = GetMods();
	FPostProcessSettings& PP = Camera->PostProcessSettings;
	const float T = GetWorld()->GetTimeSeconds();
	int32 Visual = Mods.Visual;
	if (bNightVisionGoggles || Mods.bNightVision) Visual = 7;

	PP.bOverride_SceneFringeIntensity = Visual != 0;
	PP.bOverride_VignetteIntensity = Visual != 0;
	PP.bOverride_BloomIntensity = Visual != 0;
	PP.bOverride_ColorGain = Visual == 2 || Visual == 4 || Visual == 5 || Visual == 7;
	PP.bOverride_ColorSaturation = Visual == 2 || Visual == 5;
	PP.bOverride_AutoExposureBias = Visual == 6 || Visual == 7;
	PP.bOverride_FilmGrainIntensity = Visual == 7;
	PP.SceneFringeIntensity = Visual == 1 ? 4.f : Visual == 5 ? 5.f : 1.5f;
	PP.VignetteIntensity = Visual == 1 || Visual == 4 ? 1.f : 0.6f;
	PP.BloomIntensity = Visual == 6 ? 4.f : Visual == 4 ? 2.f : 1.f;
	if (Visual == 7) { PP.ColorGain = FVector4(0.35f, 1.6f, 0.35f, 1.f); PP.AutoExposureBias = 3.5f; PP.FilmGrainIntensity = 0.6f; }
	else if (Visual == 4) PP.ColorGain = FVector4(0.85f, 1.15f, 0.8f, 1.f);
	else if (Visual == 2) { PP.ColorGain = FVector4(1.2f, 0.9f, 1.2f, 1.f); PP.ColorSaturation = FVector4(1.8f, 1.8f, 1.8f, 1.6f); }
	else if (Visual == 5)
	{
		const FLinearColor Hue = FLinearColor::MakeFromHSV8(uint8(FMath::Fmod(T * 60.f, 255.f)), 160, 255);
		PP.ColorGain = FVector4(0.6f + Hue.R, 0.6f + Hue.G, 0.6f + Hue.B, 1.f);
		PP.ColorSaturation = FVector4(2.f, 2.f, 2.f, 2.f);
	}
	if (Visual == 6) PP.AutoExposureBias = 1.5f;

	// Shaky hands / swaying world (the camera follows the controller, so nudge that)
	if (Visual == 3)
	{
		AddControllerPitchInput(FMath::Sin(T * 23.f) * 0.05f);
		AddControllerYawInput(FMath::Cos(T * 17.f) * 0.05f);
	}
	else if (Visual == 5)
	{
		AddControllerYawInput(FMath::Sin(T * 0.9f) * 0.03f);
	}
}

void AGSCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FGSEffectMods Mods = GetMods();
	const FGSItemDef* Vest = GSItems::Find(BodyArmor);
	const FGSItemDef* Hat = GSItems::Find(Helmet);
	const float GearSpeed = (Vest ? Vest->SpeedMul : 1.f) * (Hat ? Hat->SpeedMul : 1.f);

	if (HasAuthority() && !bDead)
	{
		TickSurvival(DeltaSeconds);
		TickEffects();
		if (!UseItemId.IsNone() && GetServerTime() >= UseEndTime) CompleteUse();
	}

	// Stamina (server is authoritative, owner predicts)
	const bool bMoving = GetVelocity().Size2D() > 50.f;
	if (bSprinting && bMoving)
	{
		Stamina = FMath::Max(0.f, Stamina - 14.f * Mods.StaminaDrain * DeltaSeconds);
		if (Stamina <= 0.f) SetSprinting(false);
	}
	else
	{
		const float HungerFactor = (Hunger < 20.f || Thirst < 20.f) ? 0.5f : 1.f;
		Stamina = FMath::Min(100.f, Stamina + 9.f * Mods.StaminaRegen * HungerFactor * DeltaSeconds);
	}

	UCharacterMovementComponent* Move = GetCharacterMovement();
	float Speed = bSprinting ? SprintSpeed : (bAiming ? AimSpeed : WalkSpeed);
	if (!UseItemId.IsNone()) Speed = FMath::Min(Speed, AimSpeed);
	Move->MaxWalkSpeed = Speed * Mods.Speed * GearSpeed;
	Move->JumpZVelocity = 430.f * Mods.Jump;
	if (Mods.bGlide && Move->IsFalling())
	{
		Move->AirControl = 0.9f;
		Move->Velocity.Z = FMath::Max(Move->Velocity.Z, -260.0);
	}
	else
	{
		Move->AirControl = 0.25f;
	}

	CurrentRecoil = FMath::FInterpTo(CurrentRecoil, 0.f, DeltaSeconds, 6.f);

	// Ghost boost: others only see you flicker.
	if (!IsLocallyControlled() && !bDead)
	{
		const bool bVisible = !Mods.bGhost || FMath::Frac(GetWorld()->GetTimeSeconds() * 2.5f) < 0.12f;
		BodyMesh->SetVisibility(bVisible);
		HeadMesh->SetVisibility(bVisible);
		TPWeaponMesh->SetVisibility(bVisible && GetCurrentWeapon() != nullptr);
		VestMesh->SetVisibility(bVisible && !BodyArmor.IsNone());
		HelmetMesh->SetVisibility(bVisible && !Helmet.IsNone());
	}

	if (IsLocallyControlled())
	{
		UpdateFocusPickup();
		UpdateLocalVisuals(DeltaSeconds);
		if (SelectedItem >= Backpack.Num()) SelectedItem = FMath::Max(0, Backpack.Num() - 1);
		if (bBinoculars && !HasItemKind(KindZoom)) bBinoculars = false;
		if (bNightVisionGoggles && !HasItemKind(KindNightVision)) bNightVisionGoggles = false;

		UpdateWeaponAnimation(DeltaSeconds);
	}
}
