#include "GSCharacter.h"
#include "Engine/StaticMeshActor.h"
#include "GSPlayerController.h"
#include "GSGameMode.h"
#include "GSGameState.h"
#include "GSPlayerState.h"
#include "GSHUD.h"
#include "GSPickup.h"
#include "GSTerminal.h"
#include "GSProjectile.h"
#include "GSEffects.h"
#include "GSBallistics.h"
#include "GSUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "GSAudio.h"
#include "GSGameMode.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

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
	const FName FxPainkillers(TEXT("Smertestillende"));

	/** One hit box: where it sits on the capsule (cm from the capsule centre, X forward, Y right) and half its size,
	 *  standing and crouched. Standing capsule: feet at -92, crouched: feet at -58. */
	struct FGSZoneBox { uint8 Zone; FVector Stand; FVector StandExt; FVector Crouch; FVector CrouchExt; };
	const FGSZoneBox GSZoneBoxes[7] = {
		{ GSZone::Head,    FVector(3.f, 0.f, 77.f),     FVector(9.5f, 8.f, 12.f),    FVector(6.f, 0.f, 62.f),     FVector(9.5f, 8.f, 12.f) },
		{ GSZone::Chest,   FVector(0.f, 0.f, 49.f),     FVector(12.f, 18.f, 16.f),   FVector(3.f, 0.f, 35.f),     FVector(12.f, 18.f, 15.f) },
		{ GSZone::Stomach, FVector(0.f, 0.f, 15.5f),    FVector(11.f, 16.5f, 17.5f), FVector(0.f, 0.f, 6.f),      FVector(12.f, 16.5f, 14.f) },
		{ GSZone::Arm,     FVector(0.f, -24.5f, 28.f),  FVector(6.f, 5.5f, 32.f),    FVector(4.f, -24.5f, 22.f),  FVector(7.f, 5.5f, 26.f) },
		{ GSZone::Arm,     FVector(0.f, 24.5f, 28.f),   FVector(6.f, 5.5f, 32.f),    FVector(4.f, 24.5f, 22.f),   FVector(7.f, 5.5f, 26.f) },
		{ GSZone::Leg,     FVector(0.f, -9.5f, -47.f),  FVector(9.f, 8.5f, 45.f),    FVector(8.f, -9.5f, -33.f),  FVector(17.f, 8.5f, 25.f) },
		{ GSZone::Leg,     FVector(0.f, 9.5f, -47.f),   FVector(9.f, 8.5f, 45.f),    FVector(8.f, 9.5f, -33.f),   FVector(17.f, 8.5f, 25.f) },
	};

	TAutoConsoleVariable<int32> CVarShowHitZones(TEXT("gs.ShowHitZones"), 0, TEXT("1 = draw the hit zone boxes of every player"));

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
	// Bullets hit the zone boxes, not the capsule.
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
	auto SetupHittable = [](UPrimitiveComponent* C)
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

	auto FPPart = [this](const TCHAR* Name, USceneComponent* Parent)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Parent);
		C->SetOnlyOwnerSee(true);
		C->SetCastShadow(false);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		C->SetVisibility(false);
		return C;
	};
	FPArmR = FPPart(TEXT("FPArmR"), FPWeaponMesh);
	FPArmL = FPPart(TEXT("FPArmL"), FPWeaponMesh);
	FPMag = FPPart(TEXT("FPMag"), FPArmL);
	FPAttSight = FPPart(TEXT("FPAttSight"), FPWeaponMesh);
	FPAttMuzzle = FPPart(TEXT("FPAttMuzzle"), FPWeaponMesh);
	FPAttLaser = FPPart(TEXT("FPAttLaser"), FPWeaponMesh);
	FPAttGrip = FPPart(TEXT("FPAttGrip"), FPWeaponMesh);
	FPItemMesh = FPPart(TEXT("FPItem"), Camera);
	FPItemHand = FPPart(TEXT("FPItemHand"), FPItemMesh);
	FPLegs = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPLegs"));
	FPLegs->SetupAttachment(GetCapsuleComponent());
	FPLegs->SetOnlyOwnerSee(true);
	FPLegs->SetCastShadow(false);
	FPLegs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FPLegs->SetVisibility(false);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -14.f));
	BodyMesh->SetRelativeScale3D(FVector(0.62f, 0.62f, 1.56f));
	BodyMesh->SetOwnerNoSee(true);
	SetupVisual(BodyMesh);   // only a stand-in body to look at when the mannequin is missing; bullets hit the zone boxes

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	HeadMesh->SetupAttachment(GetCapsuleComponent());
	HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, 78.f));
	HeadMesh->SetRelativeScale3D(FVector(0.3f));
	HeadMesh->SetOwnerNoSee(true);
	SetupVisual(HeadMesh);

	// Hit zones: boxes the size of real body parts. Bullets, blades and arrows (ECC_Visibility) hit these.
	static const TCHAR* ZoneNames[7] = { TEXT("ZoneHead"), TEXT("ZoneChest"), TEXT("ZoneStomach"), TEXT("ZoneArmL"),
		TEXT("ZoneArmR"), TEXT("ZoneLegL"), TEXT("ZoneLegR") };
	for (int32 I = 0; I < 7; ++I)
	{
		UBoxComponent* B = CreateDefaultSubobject<UBoxComponent>(ZoneNames[I]);
		B->SetupAttachment(GetCapsuleComponent());
		SetupHittable(B);
		B->SetCanEverAffectNavigation(false);
		HitZones[I] = B;
	}
	ApplyHitZoneLayout(false);

	TPWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TPWeapon"));
	TPWeaponMesh->SetupAttachment(GetCapsuleComponent());
	TPWeaponMesh->SetRelativeLocation(FVector(40.f, 18.f, 30.f));
	TPWeaponMesh->SetOwnerNoSee(true);
	SetupVisual(TPWeaponMesh);
	auto TPPart = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(TPWeaponMesh);
		C->SetOwnerNoSee(true);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		C->SetVisibility(false);
		return C;
	};
	TPAttSight = TPPart(TEXT("TPAttSight"));
	TPAttMuzzle = TPPart(TEXT("TPAttMuzzle"));
	TPAttLaser = TPPart(TEXT("TPAttLaser"));
	TPAttGrip = TPPart(TEXT("TPAttGrip"));

	VestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Vest"));
	VestMesh->SetupAttachment(GetCapsuleComponent());
	VestMesh->SetRelativeLocation(FVector(0.f, 0.f, 28.f));
	VestMesh->SetOwnerNoSee(true);
	VestMesh->SetVisibility(false);
	SetupVisual(VestMesh);

	BackpackMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackpackWorn"));
	BackpackMesh->SetupAttachment(GetCapsuleComponent());
	BackpackMesh->SetRelativeLocation(FVector(-24.f, 0.f, -2.f));
	BackpackMesh->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	BackpackMesh->SetOwnerNoSee(true);
	BackpackMesh->SetVisibility(false);
	SetupVisual(BackpackMesh);

	HelmetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Helmet"));
	HelmetMesh->SetupAttachment(GetCapsuleComponent());
	HelmetMesh->SetRelativeLocation(FVector(0.f, 0.f, 82.f));
	HelmetMesh->SetOwnerNoSee(true);
	HelmetMesh->SetVisibility(false);
	SetupVisual(HelmetMesh);

	// Team armbands: placed on the upper arms once the body and the team are known (ApplyTeamLook).
	for (int32 I = 0; I < 2; ++I)
	{
		UStaticMeshComponent* Band = CreateDefaultSubobject<UStaticMeshComponent>(I == 0 ? TEXT("ArmbandL") : TEXT("ArmbandR"));
		Band->SetupAttachment(GetMesh());
		Band->SetOwnerNoSee(true);
		Band->SetVisibility(false);
		SetupVisual(Band);
		(I == 0 ? ArmbandL : ArmbandR) = Band;
	}

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
	ArmbandL->SetStaticMesh(CylF.Object);
	ArmbandR->SetStaticMesh(CylF.Object);
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
	DOREPLIFETIME(AGSCharacter, WornBackpack);
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
	DOREPLIFETIME(AGSCharacter, LegInjury);
	DOREPLIFETIME(AGSCharacter, ArmInjury);
	DOREPLIFETIME_CONDITION(AGSCharacter, bAiming, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AGSCharacter, SpreadKey, COND_OwnerOnly);
}

void AGSCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority()) SpreadKey = FMath::RandHelper(MAX_int32);   // FMath::Rand() has only 15 bits

	GSUtils::SetColor(HeadMesh, FLinearColor(0.85f, 0.68f, 0.55f));
	SetupHumanBody();
	// The body is in the team's colour (the team may not have arrived yet on a client: Tick applies it when it does).
	AppliedTeam = 255;
	ApplyTeamLook();

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

/** An open menu (ours or a GSMenu panel/dialog) stops every game action. Only actions that start something are
 *  stopped; releases (StopFire, StopAim, StopSprint, StopJump) always pass, so nothing hangs when a menu opens. */
static bool GSCharInputBlocked(const AGSCharacter* Ch, const TCHAR* What)
{
	static bool bWasBlocked = false;
	const AGSPlayerController* PC = Ch ? Cast<AGSPlayerController>(Ch->GetController()) : nullptr;
	const bool bBlocked = PC && PC->IsGameInputBlocked();
	if (bBlocked != bWasBlocked)
	{
		bWasBlocked = bBlocked;
		if (bBlocked)
		{
			UE_LOG(LogTemp, Log, TEXT("GSInput: handling sperret fordi en meny er åpen (%s, Kamp-meny %d)"), What, PC->bMenuOpen ? 1 : 0);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("GSInput: handlinger slipper gjennom igjen (%s)"), What);
		}
	}
	return bBlocked;
}

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
	EIC->BindAction(PC->ScrollAction, ETriggerEvent::Triggered, this, &AGSCharacter::Scroll);
	EIC->BindAction(PC->InteractAction, ETriggerEvent::Started, this, &AGSCharacter::Interact);
	EIC->BindAction(PC->UseItemAction, ETriggerEvent::Started, this, &AGSCharacter::UseSelectedItem);
	EIC->BindAction(PC->NextItemAction, ETriggerEvent::Started, this, &AGSCharacter::NextItem);
	EIC->BindAction(PC->DropItemAction, ETriggerEvent::Started, this, &AGSCharacter::DropSelectedItem);
	EIC->BindAction(PC->MeleeAction, ETriggerEvent::Started, this, &AGSCharacter::QuickMelee);
	EIC->BindAction(PC->InspectAction, ETriggerEvent::Started, this, &AGSCharacter::Inspect);
}

void AGSCharacter::Move(const FInputActionValue& Value)
{
	if (GSCharInputBlocked(this, TEXT("gå"))) return;
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
	if (GSCharInputBlocked(this, TEXT("se rundt"))) return;
	if (TerminalPC()) return;
	const FVector2D V = Value.Get<FVector2D>();
	float Sens = bAiming ? 0.55f : 1.f;
	if (bBinoculars) Sens = 0.2f;
	if (const AGSPlayerController* SPC = Cast<AGSPlayerController>(GetController()))
	{
		if (SPC->bMenuOpen) return;
		Sens *= SPC->MouseSensitivity;
	}
	AddControllerYawInput(V.X * Sens);
	AddControllerPitchInput(-V.Y * Sens);
	LookSwayInput += V * Sens;
}

void AGSCharacter::StartJump()
{
	if (GSCharInputBlocked(this, TEXT("hopp"))) return;
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
	MantleStart = GetWorld()->GetTimeSeconds();
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
	if (GSCharInputBlocked(this, TEXT("løp"))) return;
	bWantsSprint = true;
	if (!bAiming && !bIsCrouched && Stamina > 10.f && GetLegPain() < 0.5f) SetSprinting(true);
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
	bSprinting = bNew && Stamina > 1.f && !bIsCrouched && GetLegPain() < 0.5f;
	if (bSprinting) CancelUse();
}

void AGSCharacter::ToggleCrouch()
{
	if (GSCharInputBlocked(this, TEXT("huk"))) return;
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
	if (GSCharInputBlocked(this, TEXT("sikt"))) return;
	if (TerminalPC() || bDead) return;
	bAiming = true;
	SetSprinting(false);
	if (!HasAuthority()) ServerSetAiming(true);
}

void AGSCharacter::StopAim()
{
	bAiming = false;
	if (!HasAuthority()) ServerSetAiming(false);
	if (bWantsSprint) StartSprint();
}

void AGSCharacter::ServerSetAiming_Implementation(bool bNew)
{
	bAiming = bNew && !bDead;
	if (bAiming) bSprinting = false;
}

void AGSCharacter::SelectSlot(int32 Slot)
{
	if (GSCharInputBlocked(this, TEXT("våpenbytte"))) return;
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

void AGSCharacter::Scroll(const FInputActionValue& Value)
{
	if (GSCharInputBlocked(this, TEXT("musehjul"))) return;
	const float Wheel = Value.Get<float>();
	if (FMath::IsNearlyZero(Wheel)) return;
	// A touchpad or a smooth wheel sends fractions of a notch over many frames: add them up and take one step per
	// whole notch. The sum starts over when the direction turns or after a short pause.
	const float RealNow = GetWorld()->GetRealTimeSeconds();
	if (RealNow - ScrollLastTime > 0.3f || (Wheel > 0.f) != (ScrollAccum > 0.f)) ScrollAccum = 0.f;
	ScrollLastTime = RealNow;
	ScrollAccum += Wheel;
	const int32 Notches = FMath::Clamp(FMath::TruncToInt32(ScrollAccum), -3, 3);
	if (Notches == 0) return;
	ScrollAccum = FMath::Clamp(ScrollAccum - Notches, -1.f, 1.f);
	const int32 Step = -Notches;   // wheel up = previous (up the list), wheel down = next
	if (AGSPlayerController* TPC = TerminalPC())
	{
		// 3D printer: 1 = previous recipe, Q = next. Computer showing cameras: F = previous camera, Q = next.
		const AGSTerminal* Term = TPC->GetActiveTerminal();
		const TCHAR* Key = Step > 0 ? TEXT("Q") : (Term && Term->Kind == EGSTerminalKind::Printer ? TEXT("1") : TEXT("F"));
		for (int32 I = 0; I < FMath::Abs(Step); ++I) TPC->TerminalInput(Key);
		return;
	}
	const AGSPlayerController* PC = Cast<AGSPlayerController>(GetController());
	if (bDead || (PC && PC->bMenuOpen)) return;
	if (PC && PC->bShowInventory)
	{
		const int32 N = Backpack.Num();
		if (N == 0) { SelectedItem = 0; return; }
		SelectedItem = ((SelectedItem + Step) % N + N) % N;
		return;
	}
	const int32 N = Weapons.Num();
	if (N < 2) return;
	SelectSlot(((CurrentSlot + Step) % N + N) % N);
}

void AGSCharacter::ServerSelectSlot_Implementation(int32 Slot)
{
	if (!Weapons.IsValidIndex(Slot) || Slot == CurrentSlot) return;
	CurrentSlot = Slot;
	ShotCredit = FMath::Min(ShotCredit, 1.f);   // saved-up fire from a fast weapon does not follow to a slow one
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
	if (!bAiming) Spread += 1.5f * GetArmPain();
	return Spread * GetMods().Spread;
}

void AGSCharacter::StartFire()
{
	if (GSCharInputBlocked(this, TEXT("skyt"))) return;
	if (AGSPlayerController* TPC = TerminalPC()) { TPC->TerminalInput(TEXT("Fire")); return; }
	if (const AGSPlayerController* MPC = Cast<AGSPlayerController>(GetController())) { if (MPC->bMenuOpen) return; }
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

// Which bullets glow: tracer ammo, and every fifth round from a machine gun (a belt is loaded four ball, one tracer).
// Ordinary bullets cannot be seen in flight. Shotgun pellets never glow.
static bool GSIsTracer(const FGSWeaponStats& S, uint8 AmmoType, int32 ShotIndex)
{
	return AmmoType == GSAmmoType::Tracer || (S.Class == EGSWeaponClass::LMG && ShotIndex % 5 == 0);
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
		MeleeVariant = (MeleeVariant + 1) % 3; // alternate slash, backhand and overhead
		GSAudio::PlayAt(this, TEXT("Melee_Swing"), GetActorLocation(), 0.8f, 15.f, FMath::FRandRange(0.9f, 1.1f));
		ServerMelee(false);
		return;
	}
	if (W->Ammo <= 0)
	{
		// Dry fire: click and a small trigger twitch, then reload if there is ammo
		if (Now - DryFireStart > 0.35f)
		{
			DryFireStart = Now;
			GSAudio::Play2D(this, TEXT("Dry_Fire"), 0.8f, FMath::FRandRange(0.95f, 1.05f));
		}
		StopFire();
		Reload();
		return;
	}
	LastFireTime = Now;

	const FVector Start = Camera->GetComponentLocation();
	const FVector Aim = Camera->GetForwardVector();
	const int32 ShotIndex = ++LocalShotIndex;
	const int32 Seed = GSBallistics::ShotSeed(SpreadKey, ShotIndex);
	const FGSBallistic Ball = GSBallistics::ForWeapon(S);
	// The spread we use is sent along (0.01 degree steps); we use the packed value ourselves so both sides agree exactly.
	const uint16 SpreadQ = GSBallistics::PackSpread(S.Mode == EGSFireMode::Pellets ? (bAiming ? S.AimSpread : S.HipSpread) : GetCurrentSpread());
	const float Spread = GSBallistics::UnpackSpread(SpreadQ);
	const float MuzzleDist = GSItems::GetMesh(W->Id) ? S.MuzzleX : 50.f;
	const FVector MuzzleStart = FPWeaponMesh->GetComponentLocation() + Aim * MuzzleDist;

	// Local, instant feedback: our own copy of the bullet flies and shows where it lands (the server's decides the hit).
	if (S.Mode == EGSFireMode::Hitscan)
	{
		const FVector D = GSBallistics::ShotDirection(Aim, Spread, Seed, Ball);
		if (UGSBallistics* Ballistics = GetWorld()->GetSubsystem<UGSBallistics>()) Ballistics->FireShown(this, Start, D, Ball, GSIsTracer(S, W->AmmoType, ShotIndex));
	}
	else if (S.Mode == EGSFireMode::Pellets)
	{
		DrawPellets(Start, Aim, Seed, S.Pellets, Spread, Ball);
	}
	AGSShortFX::SpawnMuzzleFlash(GetWorld(), MuzzleStart, Aim, S.bSilent);
	BarrelHeat = FMath::Min(BarrelHeat + (S.Class == EGSWeaponClass::Shotgun || S.Class == EGSWeaponClass::Sniper ? 3.f : 1.f), 30.f);
	GSAudio::Play2D(this, GSAudio::ShotName(S.Class, S.bSilent), S.bSilent ? 0.6f : 1.f, FMath::FRandRange(0.96f, 1.04f));
	if (W->Ammo <= 1) GSAudio::Play2D(this, TEXT("Dry_Fire"), 0.5f);

	if (!HasAuthority()) W->Ammo--; // prediction, corrected by replication

	// Recoil: a per-weapon pattern that climbs during a burst and wanders sideways, applied smoothly
	// (TickRecoil), plus one of five fire animations on the weapon and a pump/bolt cycle where it has one.
	const float Recoil = S.RecoilPitch * Mods.Recoil * (1.f + 0.5f * GetArmPain());
	CurrentRecoil += Recoil;
	BurstShots = (Now - LastShotTime < S.FireInterval * 1.8f + 0.05f) ? BurstShots + 1 : 0;
	LastShotTime = Now;
	AddWeaponKick(Recoil, S.Class);
	const float AimFactor = bAiming ? 0.6f : 1.f;
	const float Climb = 1.f + 0.05f * FMath::Min(BurstShots, 12);
	const float Hash = float(GetTypeHash(W->Id) % 97) * 0.13f;
	const float Side = FMath::Sin(BurstShots * 0.85f + Hash) * 0.55f + FMath::FRandRange(-0.2f, 0.2f);
	RecoilPending += FVector2D(Recoil * AimFactor * Climb, Side * Recoil * AimFactor * 0.6f);
	FireVariant = (FireVariant + FMath::RandRange(1, 4)) % 5; // never the same twice in a row
	FireAnimStart = Now;
	const bool bPump = S.Class == EGSWeaponClass::Shotgun && !S.bAutomatic && S.FireInterval >= 0.5f;
	const bool bBolt = S.Class == EGSWeaponClass::Sniper && !S.bAutomatic && S.FireInterval >= 0.9f;
	if (bPump || bBolt)
	{
		CycleStart = Now;
		bCycleBolt = bBolt;
		const EGSWeaponClass Cls = S.Class;
		const TCHAR* First = bBolt ? TEXT("Reload_Out") : TEXT("Pump_Back");
		const TCHAR* Second = bBolt ? TEXT("Reload_In") : TEXT("Pump_Fwd");
		FTimerHandle H1, H2;
		GetWorldTimerManager().SetTimer(H1, FTimerDelegate::CreateWeakLambda(this, [this, First, Cls]()
		{
			GSAudio::Play2D(this, First, 0.7f, 1.05f);
			SpawnShellFX(Cls);
		}), bBolt ? 0.35f : 0.18f, false);
		GetWorldTimerManager().SetTimer(H2, FTimerDelegate::CreateWeakLambda(this, [this, Second]()
		{
			GSAudio::Play2D(this, Second, 0.7f, 0.9f);
		}), bBolt ? 0.7f : 0.38f, false);
	}
	else if (S.Class != EGSWeaponClass::Launcher && S.Class != EGSWeaponClass::Bow)
	{
		SpawnShellFX(S.Class);
	}

	// One costly shot must arrive (bolt rifle, shotgun, single-use launcher). A stream of them may lose one: fast
	// automatics send unreliably, so a long burst on a bad connection can never overflow the reliable buffer.
	if (S.Mode == EGSFireMode::Hitscan && S.bAutomatic && S.FireInterval < 0.15f) ServerFireFast(Start, Aim, ShotIndex, SpreadQ);
	else ServerFire(Start, Aim, ShotIndex, SpreadQ);

	if (!S.bAutomatic) GetWorldTimerManager().ClearTimer(AutoFireTimer);
}

void AGSCharacter::DrawPellets(const FVector& Start, const FVector& Dir, int32 Seed, int32 Pellets, float ConeDeg, const FGSBallistic& B)
{
	TArray<FVector> Dirs;
	GSBallistics::PelletDirections(Dir, ConeDeg, Seed, Pellets, B, Dirs);
	UGSBallistics* Ballistics = GetWorld()->GetSubsystem<UGSBallistics>();
	if (!Ballistics) return;
	for (const FVector& D : Dirs) Ballistics->FireShown(this, Start, D, B, false);   // pellets never glow
}

void AGSCharacter::ServerFire_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal AimDir, int32 ShotIndex, uint16 SpreadQ)
{
	HandleFire(Start, AimDir, ShotIndex, SpreadQ);
}

void AGSCharacter::ServerFireFast_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal AimDir, int32 ShotIndex, uint16 SpreadQ)
{
	HandleFire(Start, AimDir, ShotIndex, SpreadQ);
}

void AGSCharacter::HandleFire(const FVector& Start, const FVector& AimDir, int32 ShotIndex, uint16 SpreadQ)
{
	if (bDead || bReloading || !Weapons.IsValidIndex(CurrentSlot)) return;
	FGSWeaponSlot& W = Weapons[CurrentSlot];
	const FGSWeaponStats S = GetStats(W);
	const float Now = GetWorld()->GetTimeSeconds();
	if (S.Mode == EGSFireMode::Melee || W.Ammo <= 0) return;
	// The owner numbers his shots. Older or repeated numbers are dropped.
	const int64 Jump = int64(ShotIndex) - int64(LastServerShotIndex);
	if (Jump <= 0) return;
	const FVector Aim = FVector(AimDir).GetSafeNormal();
	if (Aim.IsNearlyZero()) return;
	// The shot must go roughly where we look (35 degrees leaves room for fast turns and network delay).
	if (!IsLocallyControlled() && FVector::DotProduct(Aim, GetBaseAimRotation().Vector()) < 0.82f)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("ServerFire: shot %d from %s rejected, %.0f degrees off"), ShotIndex, *GetName(),
			FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Aim, GetBaseAimRotation().Vector()), -1.0, 1.0))));
#endif
		return;
	}
	// Shot account: one shot is earned per FireInterval (5 % slack, the same the client allows itself). Up to
	// 0.15 s worth may be saved up, so shots that arrive bunched together after a network hiccup still count,
	// but the average rate can never beat the weapon's own.
	// The account always holds at least one and a half shots, and two or more for weapons faster than 400
	// rounds a minute: the server only sees shots when its own frame starts (33 ms on a 30 fps host) and the
	// net jitters, so two honest shots from a fast weapon can land in the same server frame.
	const float MaxCredit = 1.f + FMath::Clamp(0.15f / FMath::Max(S.FireInterval, 0.01f), 0.5f, 2.f);
	ShotCredit = FMath::Min(MaxCredit, ShotCredit + (Now - ShotCreditTime) / FMath::Max(S.FireInterval * 0.95f, 0.01f));
	ShotCreditTime = Now;
	if (ShotCredit < 1.f) return;
	ShotCredit -= 1.f;

	LastServerShotIndex = ShotIndex;   // only shots that passed every check move the number
	ServerLastFireTime = Now;
	W.Ammo--;
	CancelUse();
	const FGSEffectMods Mods = GetMods();
	// W points into Weapons, and flying a bullet ahead below can call TakeDamage: keep what we need as copies.
	const FName WeaponId = W.Id;
	const uint8 AmmoType = W.AmmoType;
	// Where it starts: the owner's eye as he saw it, if that is close to where we have him (120 cm = a sprint for
	// 0.2 s) and no wall lies between. Otherwise our own eye. The shot is never thrown away for this.
	const FVector Eye = Camera->GetComponentLocation();
	// Only the world blocks (walls, doors): an object trace, so a teammate beside you never counts as a wall.
	FCollisionQueryParams WallParams(SCENE_QUERY_STAT(GSFireStart), false, this);
	FCollisionObjectQueryParams WallTypes;
	WallTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	WallTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	FHitResult Wall;
	const bool bStartOk = FVector::DistSquared(Start, Eye) <= FMath::Square(120.f)
		&& !GetWorld()->LineTraceSingleByObjectType(Wall, Eye, Start, WallTypes, WallParams);
	const FVector From = bStartOk ? FVector(Start) : Eye;
	// Spread. The size: what the owner used, held within 15 % of what we get ourselves (so an honest player hits
	// exactly where he saw it, and a cheat gains at most 15 %). The host's own FireOnce already added this shot's recoil.
	const bool bPellets = S.Mode == EGSFireMode::Pellets;
	const float Own = bPellets ? (bAiming ? S.AimSpread : S.HipSpread) : GetCurrentSpread();
	const float Claimed = GSBallistics::UnpackSpread(SpreadQ);
	const float Spread = IsLocallyControlled() ? Claimed : FMath::Clamp(Claimed, Own * 0.85f, Own * 1.15f);
	if (!IsLocallyControlled()) CurrentRecoil += S.RecoilPitch * Mods.Recoil * (1.f + 0.5f * GetArmPain());   // same as FireOnce
	// The pattern: from the key and the shot number, so the owner sees the same. He picks the number himself, so
	// only small jumps (lost packets) are trusted; after a bigger jump we draw our own.
	const int32 Seed = Jump <= 3 ? GSBallistics::ShotSeed(SpreadKey, ShotIndex) : FMath::RandHelper(MAX_int32);
	const FGSBallistic Ball = GSBallistics::ForWeapon(S);
	// Half the round trip (the engine's ping is the whole round trip in milliseconds), at most 0.1 s. None for the host.
	const APlayerState* PS = GetPlayerState();
	const float Advance = IsLocallyControlled() || !PS ? 0.f : FMath::Min(PS->GetPingInMilliseconds() * 0.0005f, 0.1f);
	UGSBallistics* Ballistics = GetWorld()->GetSubsystem<UGSBallistics>();

	if (S.Mode == EGSFireMode::Projectile)
	{
		const FName Kind = S.Class == EGSWeaponClass::Bow ? FName(TEXT("arrow")) : (S.Radius > 550.f ? FName(TEXT("rocket")) : FName(TEXT("grenade40")));
		// It starts 80 cm ahead of the eye, but never on the far side of a wall.
		FVector Spawn = From + Aim * 80.f;
		if (GetWorld()->LineTraceSingleByObjectType(Wall, From, Spawn, WallTypes, WallParams)) Spawn =FVector(Wall.ImpactPoint) - Aim * 15.f;
		AGSProjectile::Launch(GetWorld(), this, Kind, WeaponId, Spawn, Aim * S.ProjSpeed, S.ProjGravity,
			S.Damage * Mods.Damage, S.Radius, 0.f);
		// No bullet (the projectile is its own). Launchers are loud; only bows are quiet. The launcher bit lets
		// others play the right sound even if a single-use tube is already gone from the weapon list when this arrives.
		MulticastShotFX(From, Aim, static_cast<uint8>(2 | (S.bSilent || S.Class == EGSWeaponClass::Bow ? 1 : 0)
			| (S.Class == EGSWeaponClass::Launcher ? 4 : 0)));
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

	if (bPellets)
	{
		// Every pellet is its own small bullet. The same directions are drawn on every machine (PelletDirections).
		TArray<FVector> Dirs;
		GSBallistics::PelletDirections(Aim, Spread, Seed, S.Pellets, Ball, Dirs);
		if (Ballistics)
		{
			for (const FVector& PD : Dirs) Ballistics->FireReal(this, From, PD, Ball, S.Damage * Mods.Damage, WeaponId, AmmoType, Advance);
		}
		MulticastPelletFX(From, Aim, Seed, S.Pellets, Spread);
		NoteShotForRadio(From);
		return;
	}

	const FVector D = GSBallistics::ShotDirection(Aim, Spread, Seed, Ball);
	if (Ballistics) Ballistics->FireReal(this, From, D, Ball, S.Damage * Mods.Damage, WeaponId, AmmoType, Advance);
	MulticastShotFX(From, D, static_cast<uint8>((S.bSilent || Mods.bSilent ? 1 : 0) | (GSIsTracer(S, AmmoType, ShotIndex) ? 8 : 0)));
}

void AGSCharacter::QuickMelee()
{
	if (GSCharInputBlocked(this, TEXT("slag"))) return;
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

	// The server only turns the camera of its own player up and down; for everyone else it stays level. The aim
	// rotation (the controller's, sent with every move) is right on the server for all players.
	const FVector Aim = GetBaseAimRotation().Vector();
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Aim * Reach;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSMelee), false, this);
	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(22.f), Params))
	{
		if (AGSCharacter* Victim = Cast<AGSCharacter>(Hit.GetActor()))
		{
			FPointDamageEvent Event(Damage, Hit, Aim, nullptr);
			Victim->bIncomingMelee = true;
			Victim->TakeDamage(Damage * GetMods().Damage, Event, GetController(), this);
			Victim->bIncomingMelee = false;
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
	if (IsLocallyControlled()) return;
	LastMeleeTime = GetWorld()->GetTimeSeconds();
	GSAudio::PlayAt(this, TEXT("Melee_Swing"), GetActorLocation(), 0.8f, 15.f, FMath::FRandRange(0.9f, 1.1f));
}

void AGSCharacter::MulticastWoundFX_Implementation(FVector_NetQuantize Point, FVector_NetQuantizeNormal Dir, uint8 Kind, uint8 Strength)
{
	if (GetNetMode() == NM_DedicatedServer) return;
	const float Now = GetWorld()->GetTimeSeconds();
	LastWoundPoint = Point;
	LastWoundDir = FVector(Dir);
	LastWoundKind = Kind;
	LastWoundStrength = Strength / 255.f;
	LastWoundTime = Now;
	const bool bOwnView = IsLocallyControlled() && !bDead;
	AGSShortFX::SpawnBloodHit(GetWorld(), LastWoundPoint, LastWoundDir, Kind, LastWoundStrength, this, bOwnView);
	// The one who was hit already has his own 2D sounds (ClientTookHit); everyone else hears it from the body.
	// Shotgun pellets landing together are one sound.
	if (!bOwnView && Now - LastWoundSoundTime > 0.06f)
	{
		LastWoundSoundTime = Now;
		const TCHAR* Snd = Kind == GSWound::Armor ? TEXT("Hit_Armor") : Kind == GSWound::Head ? TEXT("Hit_Head") : TEXT("Hit_Body");
		GSAudio::PlayAt(this, Snd, LastWoundPoint, 0.55f, 14.f, FMath::FRandRange(0.9f, 1.1f));
	}
	// The death may have reached this machine before the wound that caused it.
	if (bDead) ApplyDeathImpulse();
}

void AGSCharacter::MulticastShotFX_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, uint8 Flags)
{
	const bool bSilent = (Flags & 1) != 0;     // suppressed, or a bow
	const bool bNoTracer = (Flags & 2) != 0;   // no bullet: a launched projectile draws its own flight and impact
	if (!bSilent) NoteShotForRadio(Start);
	if (IsLocallyControlled()) return; // shooter already drew it
	LastRemoteShotTime = GetWorld()->GetTimeSeconds();
	{
		const FGSWeaponSlot* W = GetCurrentWeapon();
		const EGSWeaponClass Class = (Flags & 4) != 0 ? EGSWeaponClass::Launcher : (W ? GetStats(*W).Class : EGSWeaponClass::Rifle);
		// A bow has its own quiet sound (the string), not the "pfft" of a suppressed gun.
		GSAudio::PlayShotFor(this, Class, bSilent && !(bNoTracer && Class == EGSWeaponClass::Bow), Start);
	}
	if (bNoTracer) return;   // rockets, grenades and bolts fly as their own actors
	// Our own copy of the bullet shows where it lands. It flies as the gun the shooter holds right now.
	const FGSWeaponSlot* W = GetCurrentWeapon();
	const FGSWeaponStats S = W ? GetStats(*W) : GSItems::Weapon(NAME_None);
	if (UGSBallistics* Ballistics = GetWorld()->GetSubsystem<UGSBallistics>()) Ballistics->FireShown(this, Start, FVector(Dir), GSBallistics::ForWeapon(S), (Flags & 8) != 0);
}

void AGSCharacter::MulticastPelletFX_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, int32 Seed, int32 Pellets, float ConeDeg)
{
	if (IsLocallyControlled()) return;
	GSAudio::PlayShotFor(this, EGSWeaponClass::Shotgun, false, Start);
	const FGSWeaponSlot* W = GetCurrentWeapon();
	const FGSWeaponStats S = W ? GetStats(*W) : GSItems::Weapon(NAME_None);
	DrawPellets(Start, Dir, Seed, Pellets, ConeDeg,
		W &&S.Mode == EGSFireMode::Pellets ? GSBallistics::ForWeapon(S) : GSBallistics::ForCaliber(TEXT("12g")));
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
	if (GSCharInputBlocked(this, TEXT("lad"))) return;
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
	GetWorldTimerManager().SetTimer(ReloadTimer, this, &AGSCharacter::FinishReload, ReloadSeconds(S), false);
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

void AGSCharacter::ClientHitMarker_Implementation(bool bKill, bool bHeadshot, bool bArmor)
{
	const float Now = GetWorld()->GetTimeSeconds();
	const uint8 Kind = bKill ? GSHitKind::Kill : bHeadshot ? GSHitKind::Head : bArmor ? GSHitKind::Armor : GSHitKind::Body;
	// Shotgun pellets arrive as many markers: a later pellet must not turn a fresh kill marker back into a body hit.
	if (LastHitKind == GSHitKind::Kill && Kind != GSHitKind::Kill && Now - LastHitMarkerTime < 0.5f) return;
	LastHitKind = Kind;
	LastHitMarkerTime = Now;
	if (bKill) bLastKillHeadshot = bHeadshot;

	// Same sound, pitched per kind: deep for a kill, bright for a headshot, a thin "clink" off armor.
	switch (Kind)
	{
	case GSHitKind::Kill:
		GSAudio::Play2D(this, TEXT("Hit_Marker"), 0.9f, 0.8f);
		if (bHeadshot) GSAudio::Play2D(this, TEXT("Hit_Marker"), 0.85f, 1.45f);
		break;
	case GSHitKind::Head:  GSAudio::Play2D(this, TEXT("Hit_Marker"), 0.85f, 1.45f); break;
	case GSHitKind::Armor: GSAudio::Play2D(this, TEXT("Hit_Marker"), 0.55f, 1.7f); break;
	default:               GSAudio::Play2D(this, TEXT("Hit_Marker"), 0.6f, 1.f); break;
	}
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
	KickRotVel += FRotator(140.f * K, FMath::FRandRange(-15.f, 15.f) * K, FMath::FRandRange(-20.f, 20.f) * K);
}

void AGSCharacter::Inspect()
{
	if (GSCharInputBlocked(this, TEXT("se på våpenet"))) return;
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
		ReloadLength = W ? ReloadSeconds(S) : 2.f;
		bMagDropped = false;
		bEmptyReload = W && W->Ammo <= 0;
		// mag out, mag in, charging handle (timed to the procedural reload animation)
		const TCHAR* Steps[] = { TEXT("Reload_Out"), TEXT("Reload_In"), TEXT("Slide") };
		const float At[] = { 0.25f, 0.6f, 0.8f };
		for (int32 i = 0; i < 3; ++i)
		{
			FTimerHandle Handle;
			const FString Snd(Steps[i]);
			GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Snd]()
			{
				if (bReloading) GSAudio::Play2D(this, Snd, 0.7f, FMath::FRandRange(0.95f, 1.05f));
			}), FMath::Max(0.05f, ReloadLength * At[i]), false);
		}
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
				if (bEmptyReload)
				{
					Pos.X -= Bump(T, 0.72f, 0.86f) * 7.f;                     // charging handle / slide release
					Rot.Yaw += Bump(T, 0.72f, 0.86f) * 14.f;
				}
				else
				{
					// tactical reload: tap the new mag and check it, no rack
					Pos.Z += Bump(T, 0.7f, 0.78f) * 1.5f;
					Rot.Roll -= Bump(T, 0.74f, 0.9f) * 8.f;
				}
				if (!bMagDropped && T > 0.25f)
				{
					bMagDropped = true;
					DropMagazineFX();
				}
			}
		}

		// Jump: gun lags behind vertical motion
		if (bFalling)
		{
			const float Vz = float(GetVelocity().Z);
			Pos.Z += FMath::Clamp(-Vz * 0.005f, -3.f, 3.f) * (1.f - AimAlpha * 0.6f);
			Rot.Pitch += FMath::Clamp(Vz * 0.006f, -4.f, 4.f) * (1.f - AimAlpha * 0.6f);
		}
		// Strafe tilt and forward push from movement relative to the view
		{
			const FVector V = GetVelocity();
			const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
			const float Side = FVector::DotProduct(V, FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y));
			const float Fwd = FVector::DotProduct(V, FRotationMatrix(Yaw).GetUnitAxis(EAxis::X));
			Rot.Roll += FMath::Clamp(-Side * 0.008f, -5.f, 5.f) * (1.f - AimAlpha * 0.7f);
			Pos.X -= FMath::Clamp(Fwd * 0.003f, -1.5f, 1.5f) * (1.f - AimAlpha);
		}
		// Crouch / stand: a short dip of the gun
		{
			const bool bCr = bIsCrouched;
			if (bCr != bAnimWasCrouched) { KickVel += FVector(0.f, 0.f, bCr ? -140.f : 90.f); KickRotVel += FRotator(bCr ? -40.f : 30.f, 0.f, 0.f); }
			bAnimWasCrouched = bCr;
		}
		// Close to a wall: pull the gun back to high ready (muzzle up) so it never clips through
		{
			FHitResult WallHit;
			FCollisionQueryParams QP(SCENE_QUERY_STAT(GSWallCheck), false, this);
			const FVector From = Camera->GetComponentLocation();
			const bool bWall = !bAiming && GetWorld()->LineTraceSingleByChannel(WallHit, From, From + Camera->GetForwardVector() * 70.f, ECC_Visibility, QP);
			const float WallTarget = bWall ? FMath::Clamp(1.f - (WallHit.Distance - 25.f) / 45.f, 0.f, 1.f) : 0.f;
			WallBlend = FMath::FInterpTo(WallBlend, WallTarget, Dt, 10.f);
			Pos += FVector(-14.f, -3.f, -4.f) * WallBlend;
			Rot += FRotator(38.f, -12.f, -18.f) * WallBlend;
		}
		// Dry fire: trigger finger twitch, gun jolts a little
		{
			const float DT2 = Now - DryFireStart;
			if (DT2 >= 0.f && DT2 < 0.3f)
			{
				const float B = Bump(DT2, 0.f, 0.3f);
				Pos.X -= 0.6f * B;
				Rot.Roll += 3.f * B;
			}
		}
		// Hot barrel: thin smoke curls off the muzzle after a long burst
		BarrelHeat = FMath::Max(0.f, BarrelHeat - Dt * 2.f);
		if (BarrelHeat > 6.f && Now - LastShotTime > 0.2f && Now - LastBarrelSmoke > 0.07f && !IsScopedIn())
		{
			LastBarrelSmoke = Now;
			const float MD = GSItems::GetMesh(W->Id) ? S.MuzzleX : 50.f;
			AGSShortFX::SpawnBarrelSmoke(GetWorld(), FPWeaponMesh->GetComponentLocation() + FPWeaponMesh->GetForwardVector() * MD, FPWeaponMesh->GetForwardVector());
		}
		// Idle fidget: after standing still for a while, glance at the gun
		{
			const bool bIdle = GetVelocity().Size2D() < 10.f && !bAiming && !bWantsFire && !bReloading && UseItemId.IsNone();
			IdleTime = bIdle ? IdleTime + Dt : 0.f;
			if (IdleTime > 14.f) { IdleTime = 0.f; if (Now - InspectStart > 3.2f) InspectStart = Now; }
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

		// Fire: five different shot animations (picked per shot), heavier for big guns, smaller when aiming.
		const float FT = Now - FireAnimStart;
		if (FT >= 0.f && FT < 0.6f)
		{
			float Heavy = 1.f, Rate = 16.f;
			switch (S.Class)
			{
			case EGSWeaponClass::Pistol:   Heavy = 1.2f; Rate = 18.f; break;
			case EGSWeaponClass::SMG:      Heavy = 0.6f; Rate = 26.f; break;
			case EGSWeaponClass::LMG:      Heavy = 0.8f; Rate = 22.f; break;
			case EGSWeaponClass::Sniper:   Heavy = 2.2f; Rate = 8.f; break;
			case EGSWeaponClass::Shotgun:  Heavy = 2.f;  Rate = 9.f; break;
			case EGSWeaponClass::Launcher: Heavy = 2.6f; Rate = 6.f; break;
			default: break;
			}
			const float Env = Ease(Phase01(FT, 0.f, 0.025f)) * FMath::Exp(-FT * Rate) * Heavy;
			const float A = 1.f - AimBlend * 0.6f;
			switch (FireVariant)
			{
			case 0: Pos.X -= 3.f * Env; Rot.Pitch += 4.f * Env * A; break;                                         // straight back, muzzle up
			case 1: Pos.X -= 2.5f * Env; Rot.Pitch += 3.f * Env * A; Rot.Roll += 5.f * Env * A; break;            // twist right
			case 2: Pos.X -= 2.5f * Env; Rot.Pitch += 3.f * Env * A; Rot.Roll -= 4.f * Env * A; Rot.Yaw -= 2.f * Env * A; break; // twist left
			case 3: Pos.X -= 1.5f * Env; Pos.Z += 1.5f * Env; Rot.Pitch += 6.f * Env * A; break;                 // muzzle snap
			default: Pos.X -= 3.5f * Env; Pos.Z -= 1.f * Env; Rot.Pitch += 2.f * Env * A; Rot.Yaw += 1.5f * Env * A; break; // into the shoulder
			}
			if (S.Class == EGSWeaponClass::Pistol) Rot.Pitch += 5.f * Env * A; // wrist flips up
		}
		// Pump-action and bolt-action cycle after the shot
		const float CT = Now - CycleStart;
		if (CT >= 0.f && CT < 1.1f && !bReloading)
		{
			if (bCycleBolt)
			{
				const float B = Bump(CT, 0.2f, 1.0f);
				Rot.Roll += 22.f * B * (1.f - AimBlend * 0.5f);
				Pos.Y -= 4.f * B;
				Pos.X -= 4.f * Bump(CT, 0.4f, 0.75f);
				Rot.Pitch -= 4.f * B;
			}
			else
			{
				const float B = Bump(CT, 0.12f, 0.5f);
				Pos.X -= 6.f * B;
				Rot.Roll += 8.f * B;
				Rot.Pitch -= 3.f * B;
			}
		}

		// Mantle: tuck the gun down and to the side while the free hand pulls you over, then bring it back up
		const float MT = Now - MantleStart;
		if (MT < 0.7f)
		{
			const float Tuck = Ease(Phase01(MT, 0.f, 0.15f)) * (1.f - Ease(Phase01(MT, 0.4f, 0.7f)));
			Pos += FVector(-8.f * Tuck, 6.f * Tuck, -20.f * Tuck);
			Rot += FRotator(-25.f * Tuck, 20.f * Tuck, 35.f * Tuck);
		}

		// Melee: wind-up, fast swing, follow-through. Three swings in turn; light blades stab on the third.
		const float SW = Now - LastMeleeTime;
		if (SW < 0.45f)
		{
			const float Wind = Ease(Phase01(SW, 0.f, 0.08f)) * (1.f - Phase01(SW, 0.08f, 0.16f));
			const float Swing = Ease(Phase01(SW, 0.08f, 0.2f)) * (1.f - Ease(Phase01(SW, 0.22f, 0.45f)));
			const FGSWeaponSlot* MW = GetCurrentWeapon();
			const bool bBlade = MW && (MW->Id == FName(TEXT("Knife")) || MW->Id == FName(TEXT("PrintKnife")) || MW->Id == FName(TEXT("PistolKnife")));
			switch (MeleeVariant)
			{
			case 1: // backhand: from the left across to the right
				Rot += FRotator(20.f * Wind - 30.f * Swing, -30.f * Wind + 65.f * Swing, 20.f * Wind - 35.f * Swing);
				Pos += FVector(-6.f * Wind + 28.f * Swing, -14.f * Wind + 22.f * Swing, 6.f * Wind - 2.f * Swing);
				break;
			case 2:
				if (bBlade)
				{
					// stab: pull back, thrust straight forward
					Rot += FRotator(-8.f * Wind + 6.f * Swing, 6.f * Wind - 4.f * Swing, 0.f);
					Pos += FVector(-12.f * Wind + 36.f * Swing, 2.f * Wind - 6.f * Swing, 3.f * Wind - 2.f * Swing);
				}
				else
				{
					// overhead chop: raise high, bring it down hard
					Rot += FRotator(45.f * Wind - 70.f * Swing, -5.f * Wind - 10.f * Swing, 5.f * Wind);
					Pos += FVector(-8.f * Wind + 22.f * Swing, -4.f * Wind - 8.f * Swing, 22.f * Wind - 18.f * Swing);
				}
				break;
			default: // slash from the right across to the left
				Rot += FRotator(25.f * Wind - 40.f * Swing, 25.f * Wind - 70.f * Swing, -15.f * Wind + 30.f * Swing);
				Pos += FVector(-6.f * Wind + 30.f * Swing, 10.f * Wind - 25.f * Swing, 8.f * Wind - 4.f * Swing);
				break;
			}
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

	{
		const float RT = (bReloading && ReloadLength > 0.f) ? (Now - ReloadStart) / ReloadLength : -1.f;
		const bool bShellR = W && (S.Class == EGSWeaponClass::Shotgun || (S.Class == EGSWeaponClass::Sniper && S.MagSize <= 5));
		UpdateArms(Dt, W ? &S : nullptr, RT, bShellR);
		UpdateItemUse(Dt);
	}
	// Looking through a scope: the HUD draws it, the gun and arms are hidden.
	FPWeaponMesh->SetHiddenInGame(IsScopedIn() && AimBlend > 0.85f, true);
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, TargetFOV, Dt, 12.f));
	FPWeaponMesh->SetRelativeLocation(FMath::VInterpTo(FPWeaponMesh->GetRelativeLocation(), Pos, Dt, 40.f));
	FPWeaponMesh->SetRelativeRotation(FMath::RInterpTo(FPWeaponMesh->GetRelativeRotation(), Rot, Dt, 40.f));
}

static UStaticMesh* FPModel(const TCHAR* Name)
{
	static TMap<FString, TWeakObjectPtr<UStaticMesh>> Cache;
	if (TWeakObjectPtr<UStaticMesh>* F = Cache.Find(Name)) { if (F->IsValid()) return F->Get(); }
	UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Models/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (M) Cache.Add(Name, M);
	return M;
}

// Piecewise path through keyframes (time 0..1 -> position), eased between keys.
struct FGSKey { float T; FVector V; };
static FVector KeyPath(float T, const TArray<FGSKey>& Keys)
{
	if (Keys.Num() == 0) return FVector::ZeroVector;
	if (T <= Keys[0].T) return Keys[0].V;
	for (int32 i = 1; i < Keys.Num(); ++i)
	{
		if (T <= Keys[i].T)
		{
			const float A = (T - Keys[i - 1].T) / FMath::Max(Keys[i].T - Keys[i - 1].T, 0.001f);
			return FMath::Lerp(Keys[i - 1].V, Keys[i].V, A * A * (3.f - 2.f * A));
		}
	}
	return Keys.Last().V;
}

static UStaticMesh* GSFXMesh(const TCHAR* Name); // defined next to SpawnShellFX

void AGSCharacter::UpdateArms(float Dt, const FGSWeaponStats* S, float ReloadT, bool bShellReload)
{
	const FGSWeaponSlot* W = GetCurrentWeapon();
	UStaticMesh* ArmR = FPModel(TEXT("SM_FPArm_R"));
	UStaticMesh* ArmL = FPModel(TEXT("SM_FPArm_L"));
	const bool bModel = W && GSItems::GetMesh(W->Id) != nullptr;
	const bool bShow = S && ArmR && ArmL && bModel && FPWeaponMesh->IsVisible() && UseItemId.IsNone();
	FPArmR->SetVisibility(bShow);
	if (!bShow)
	{
		FPArmL->SetVisibility(false);
		FPMag->SetVisibility(false);
		return;
	}
	if (FPArmR->GetStaticMesh() != ArmR) FPArmR->SetStaticMesh(ArmR);
	if (FPArmL->GetStaticMesh() != ArmL) FPArmL->SetStaticMesh(ArmL);

	// Where the left hand rests (weapon space, cm). Knives and one-handed melee: no left hand.
	const float MX = S->MuzzleX;
	bool bLeft = true;
	FVector Home(MX * 0.45f, 0.f, -2.5f);
	FRotator HomeRot = FRotator::ZeroRotator;
	switch (S->Class)
	{
	case EGSWeaponClass::Pistol:   Home = FVector(-1.f, 0.5f, -7.5f); HomeRot = FRotator(-20.f, 0.f, 25.f); break;
	case EGSWeaponClass::Shotgun:  Home = FVector(MX * 0.58f, 0.f, -3.f); break;
	case EGSWeaponClass::Launcher: Home = FVector(MX * 0.25f, 0.f, -7.f); break;
	case EGSWeaponClass::Bow:      Home = FVector(-12.f, 0.f, -4.f); break;
	case EGSWeaponClass::Melee:    bLeft = MX > 45.f; Home = FVector(-14.f, 0.f, 0.f); break;
	default: break;
	}
	LeftHandHome = Home;
	FVector L = Home;
	FRotator LR = HomeRot;
	bool bMag = false;
	if (ReloadT >= 0.f && bLeft)
	{
		const FVector Well = S->Class == EGSWeaponClass::Pistol ? FVector(-4.f, 0.f, -12.f) : FVector(8.f, 0.f, -9.f);
		const FVector Pouch(-18.f, 14.f, -40.f);
		if (bShellReload)
		{
			// one shell at a time from the belt into the loading port
			const int32 N = FMath::Max(2, S->MagSize / 2);
			const float Cyc = FMath::Frac(FMath::Clamp((ReloadT - 0.12f) / 0.76f, 0.f, 1.f) * N);
			const FVector Port(6.f, 0.f, -6.f);
			const float In = FMath::Clamp(ReloadT / 0.12f, 0.f, 1.f) * FMath::Clamp((1.f - ReloadT) / 0.12f, 0.f, 1.f);
			L = FMath::Lerp(Home, KeyPath(Cyc, { {0.f, Port}, {0.45f, Pouch * 0.6f}, {1.f, Port} }), In);
			bMag = Cyc > 0.45f && In > 0.5f;
		}
		else
		{
			const FVector Charge = S->Class == EGSWeaponClass::Pistol ? FVector(-5.f, 0.f, 4.f) : FVector(-2.f, -4.f, 5.f);
			L = KeyPath(ReloadT, {
				{0.f, Home}, {0.12f, Well}, {0.28f, Well + FVector(0.f, 0.f, -14.f)}, {0.42f, Pouch},
				{0.56f, Well + FVector(0.f, 0.f, -14.f)}, {0.64f, Well}, {0.72f, Charge}, {0.82f, Charge + FVector(-9.f, 0.f, 0.f)},
				{0.95f, Home} });
			LR = FRotator(0.f, 0.f, 30.f * FMath::Sin(FMath::Clamp(ReloadT, 0.f, 1.f) * PI));
			// old magazine in the hand going out, new one coming in
			bMag = (ReloadT > 0.14f && ReloadT < 0.27f) || (ReloadT > 0.42f && ReloadT < 0.64f);
		}
	}
	FPArmL->SetVisibility(bLeft);
	FPArmL->SetRelativeLocation(FMath::VInterpTo(FPArmL->GetRelativeLocation(), L, Dt, 25.f));
	FPArmL->SetRelativeRotation(FMath::RInterpTo(FPArmL->GetRelativeRotation(), LR, Dt, 25.f));
	FPMag->SetVisibility(bMag);
	if (bMag)
	{
		const bool bShell = bShellReload;
		const bool bPistolMag = S && S->Class == EGSWeaponClass::Pistol;
		UStaticMesh* Real = GSFXMesh(bShell ? (S && S->Class == EGSWeaponClass::Shotgun ? TEXT("SM_FX_ShellShotgun") : TEXT("SM_FX_CasingRifle"))
			: bPistolMag ? TEXT("SM_FX_MagPistol") : TEXT("SM_FX_MagRifle"));
		if (Real)
		{
			if (FPMag->GetStaticMesh() != Real)
			{
				FPMag->SetStaticMesh(Real);
				FPMag->SetRelativeScale3D(FVector(1.f));
				FPMag->EmptyOverrideMaterials();
			}
			FPMag->SetRelativeLocation(bShell ? FVector(1.f, 0.f, 1.f) : FVector(0.f, 0.f, -6.f));
		}
		else if (FPMag->GetStaticMesh() != GSUtils::Cube())
		{
			FPMag->SetStaticMesh(GSUtils::Cube());
			FPMag->SetRelativeLocation(bShell ? FVector(1.f, 0.f, 1.f) : FVector(0.f, 0.f, -8.f));
			FPMag->SetRelativeScale3D(bShell ? FVector(0.065f, 0.02f, 0.02f) : FVector(0.035f, 0.025f, 0.17f));
			GSUtils::SetColor(FPMag, bShell ? FLinearColor(0.5f, 0.04f, 0.03f) : FLinearColor(0.03f, 0.03f, 0.035f));
		}
	}
}

void AGSCharacter::UpdateItemUse(float Dt)
{
	const FGSItemDef* Def = UseItemId.IsNone() ? nullptr : GSItems::Find(UseItemId);
	UStaticMesh* Model = Def ? GSItems::GetMesh(UseItemId) : nullptr;
	if (!Model)
	{
		FPItemMesh->SetVisibility(false);
		FPItemHand->SetVisibility(false);
		ItemAnimId = NAME_None;
		return;
	}
	if (ItemAnimId != UseItemId)
	{
		ItemAnimId = UseItemId;
		FPItemMesh->SetStaticMesh(Model);
		const float Size = Model->GetBounds().BoxExtent.GetMax() * 2.f;
		FPItemMesh->SetRelativeScale3D(FVector(FMath::Clamp(14.f / FMath::Max(Size, 1.f), 0.3f, 3.f)));
		FPItemMesh->SetRelativeLocation(FVector(30.f, 14.f, -40.f));
		if (UStaticMesh* Hand = FPModel(TEXT("SM_FPArm_R"))) FPItemHand->SetStaticMesh(Hand);
	}
	FPItemMesh->SetVisibility(true);
	FPItemHand->SetVisibility(FPItemHand->GetStaticMesh() != nullptr);
	// the hand keeps its real size even when the item is scaled up or down
	const float Inv = 1.f / FMath::Max(FPItemMesh->GetRelativeScale3D().X, 0.01f);
	FPItemHand->SetRelativeScale3D(FVector(Inv));
	FPItemHand->SetRelativeLocation(FVector(-2.f, 0.f, -4.f) * Inv);

	const float Now = GetServerTime();
	const float Len = FMath::Max(UseEndTime - UseStartTime, 0.1f);
	const float T = FMath::Clamp((Now - UseStartTime) / Len, 0.f, 1.f);
	const float In = FMath::Clamp(T / 0.15f, 0.f, 1.f) * FMath::Clamp((1.f - T) / 0.1f, 0.f, 1.f);
	const FString Id = UseItemId.ToString();
	const FVector Low(30.f, 14.f, -40.f);
	const FVector Hold(28.f, 8.f, -16.f);
	const FVector Mouth(14.f, 2.f, -7.f);
	FVector P = Hold;
	FRotator R(0.f, -20.f, 0.f);
	const float Clock = Now - UseStartTime;
	const bool bDrink = Def->Thirst > 0.f && Def->Hunger <= 0.f;
	const bool bPills = Id.Contains(TEXT("Pill")) || Id.Contains(TEXT("Painkillers")) || Id.Contains(TEXT("Snus")) || Id.Contains(TEXT("Nicotine"));
	const bool bWrap = Id.Contains(TEXT("Bandage")) || Id.Contains(TEXT("Plaster"));
	if (Def->Cat == EGSItemCat::Throwable)
	{
		// pull the pin (small jerk), cock the arm back over the shoulder, then the throw goes
		const float Pin = Bump(T, 0.15f, 0.35f);
		const float Back = Ease(Phase01(T, 0.45f, 0.85f));
		const float Fwd = Ease(Phase01(T, 0.88f, 1.f));
		P = Hold + FVector(-2.f, 0.f, 3.f) * Pin + FVector(-14.f, 10.f, 20.f) * Back + FVector(30.f, -14.f, -6.f) * Fwd;
		R = FRotator(-35.f * Back + 50.f * Fwd, -20.f, 20.f * Pin);
	}
	else if (Def->Cat == EGSItemCat::Food && bDrink)
	{
		// bottle up to the mouth and tip it back, small sips
		const float Up = FMath::Clamp((T - 0.15f) / 0.15f, 0.f, 1.f) * FMath::Clamp((0.92f - T) / 0.12f, 0.f, 1.f);
		P = FMath::Lerp(Hold, Mouth, Up);
		R = FRotator(55.f * Up + 6.f * FMath::Sin(Clock * 6.f) * Up, -10.f, 0.f);
	}
	else if (Def->Cat == EGSItemCat::Food)
	{
		// bites: to the mouth, chew, again
		const float Bite = FMath::Max(0.f, FMath::Sin(Clock * PI / 0.7f));
		P = FMath::Lerp(Hold, Mouth, Bite * 0.85f);
		R = FRotator(15.f * Bite, -25.f, 10.f * Bite);
	}
	else if (bPills)
	{
		const float Up = FMath::Clamp((T - 0.3f) / 0.2f, 0.f, 1.f) * FMath::Clamp((0.8f - T) / 0.15f, 0.f, 1.f);
		P = FMath::Lerp(Hold, Mouth, Up);
		R = FRotator(40.f * Up, -15.f, 20.f * FMath::Sin(Clock * 12.f) * (1.f - Up));
	}
	else if (bWrap || Def->Cat == EGSItemCat::Medicine)
	{
		// wrapping / working on the wound low in front of you
		const float A = Clock * (bWrap ? 7.f : 4.f);
		P = FVector(30.f, -4.f, -24.f) + FVector(0.f, FMath::Cos(A) * 6.f, FMath::Sin(A) * 4.f);
		R = FRotator(-20.f, -30.f + FMath::Sin(A) * 15.f, 30.f);
	}
	else
	{
		// injection into the other arm: aim, push, hold
		const float Push = FMath::Clamp((T - 0.35f) / 0.1f, 0.f, 1.f) * FMath::Clamp((0.85f - T) / 0.1f, 0.f, 1.f);
		P = FVector(24.f, -12.f + 4.f * Push, -24.f);
		R = FRotator(-10.f, -70.f, 0.f);
	}
	P = FMath::Lerp(Low, P, In);
	FPItemMesh->SetRelativeLocation(FMath::VInterpTo(FPItemMesh->GetRelativeLocation(), P, Dt, 14.f));
	FPItemMesh->SetRelativeRotation(FMath::RInterpTo(FPItemMesh->GetRelativeRotation(), R, Dt, 14.f));
}

bool AGSCharacter::IsScopedIn() const
{
	const FGSWeaponSlot* W = GetCurrentWeapon();
	if (!W || !bAiming || bReloading) return false;
	if (W->Attachments & (GSAttach::Scope | GSAttach::Scope8x)) return true;
	return GetStats(*W).AimFOV <= 30.f;
}

namespace
{
	/** Which hurt kind wins when several hits land at once (shotgun pellets): the most serious one. */
	int32 HurtRank(uint8 Kind)
	{
		switch (Kind)
		{
		case GSHurtKind::ArmorBroke: return 4;
		case GSHurtKind::Head:       return 3;
		case GSHurtKind::HeadArmor:  return 2;
		case GSHurtKind::Body:       return 1;
		default:                     return 0;   // Armor
		}
	}
}

void AGSCharacter::ClientTookHit_Implementation(FVector_NetQuantize From, bool bHasFrom, float HealthLost, uint8 Kind)
{
	const uint8 Zone = Kind >> 4;   // GSZone in the high bits
	Kind &= 0x0F;
	const float Now = GetWorld()->GetTimeSeconds();
	HealthLost = FMath::Max(0.f, HealthLost);

	// Direction wedge: hits from (nearly) the same direction share one wedge, up to 4 directions at once.
	if (bHasFrom)
	{
		LastDamageFrom = From;
		LastDamageFromTime = Now;
		RecentHits.RemoveAll([Now](const FGSRecentHit& H) { return Now - H.Time > 2.f; });
		const FVector Me = GetActorLocation();
		const float Yaw = (FVector(From) - Me).Rotation().Yaw;
		FGSRecentHit* Same = nullptr;
		for (FGSRecentHit& H : RecentHits)
		{
			if (FMath::Abs(FRotator::NormalizeAxis((H.From - Me).Rotation().Yaw - Yaw)) <= 25.f) { Same = &H; break; }
		}
		if (Same)
		{
			// Pellets from one shot (same instant) add up; a later separate hit keeps half of the old size.
			const bool bSameBurst = Now - Same->Time < 0.1f;
			Same->BurstLost = bSameBurst ? Same->BurstLost + HealthLost : HealthLost;
			const float NewStrength = FMath::Clamp(Same->BurstLost / 35.f, 0.35f, 1.f);
			Same->Strength = FMath::Max(bSameBurst ? Same->Strength : Same->Strength * 0.5f, NewStrength);
			Same->From = From;
			Same->Time = Now;
		}
		else
		{
			if (RecentHits.Num() >= 4)
			{
				int32 Oldest = 0;
				for (int32 i = 1; i < RecentHits.Num(); ++i)
				{
					if (RecentHits[i].Time < RecentHits[Oldest].Time) Oldest = i;
				}
				RecentHits.RemoveAt(Oldest);
			}
			FGSRecentHit& H = RecentHits.AddDefaulted_GetRef();
			H.From = From;
			H.Time = Now;
			H.Strength = FMath::Clamp(HealthLost / 35.f, 0.35f, 1.f);
			H.BurstLost = HealthLost;
		}
	}

	// Nothing got through and no armor was hit (e.g. a damage-immunity boost): only the wedge.
	if (HealthLost <= 0.f && (Kind == GSHurtKind::Body || Kind == GSHurtKind::Head)) return;

	// Screen-edge flash: hits in the same instant add up, and the most serious kind decides the look.
	const bool bBurst = Now - LastHurtFlashTime < 0.1f;
	HurtBurstLost = bBurst ? HurtBurstLost + HealthLost : HealthLost;
	const float Strength = FMath::Clamp(HurtBurstLost / 40.f, 0.25f, 1.f);
	LastHurtFlashStrength = bBurst ? FMath::Max(LastHurtFlashStrength, Strength) : Strength;
	LastHurtBlood = FMath::Clamp(HurtBurstLost / 40.f, 0.f, 1.f);
	if (Kind != GSHurtKind::Armor && Kind != GSHurtKind::HeadArmor) LastHurtBlood = FMath::Max(LastHurtBlood, 0.25f);
	if (!bBurst || HurtRank(Kind) >= HurtRank(LastHurtKind)) LastHurtKind = Kind;
	LastHurtFlashTime = Now;

	// Headshots you survive jerk the view harder (the normal flinch starts from the health change).
	const bool bHead = Zone == GSZone::Head || Kind == GSHurtKind::Head || Kind == GSHurtKind::HeadArmor;
	if (bHead)
	{
		FlinchScale = 2.2f;
		FlinchStart = Now;
		FlinchSign = FMath::RandBool() ? 1.f : -1.f;
	}
	// You feel where it hit: the knee gives way, or the gun is knocked in your hands.
	// Once per instant: nine shotgun pellets in the leg are one buckle, not nine.
	const bool bJolt = HealthLost > 0.f && (Zone == GSZone::Leg || Zone == GSZone::Arm) && Now - LastLimbJoltTime > 0.06f;
	if (bJolt) LastLimbJoltTime = Now;
	if (bJolt && Zone == GSZone::Leg) CamDipVel -= 90.f + 140.f * FMath::Clamp(HealthLost / 30.f, 0.f, 1.f);
	if (bJolt && Zone == GSZone::Arm)
	{
		const FGSWeaponSlot* W = GetCurrentWeapon();
		AddWeaponKick(2.5f, W ? GetStats(*W).Class : EGSWeaponClass::Rifle);
	}

	// One sound per instant: nine shotgun pellets must not play nine sounds on top of each other,
	// but a more serious hit in the same instant (the vest breaking) is still heard.
	const int32 Rank = HurtRank(Kind);
	if (Now - LastHurtSoundTime < 0.04f && Rank <= LastHurtSoundRank) return;
	LastHurtSoundTime = Now;
	LastHurtSoundRank = Rank;
	switch (Kind)
	{
	case GSHurtKind::Armor:
		GSAudio::Play2D(this, TEXT("Hit_Armor"), 0.7f, FMath::FRandRange(0.95f, 1.05f));
		if (HealthLost > 1.f) GSAudio::Play2D(this, TEXT("Hit_Body"), 0.3f, FMath::FRandRange(0.9f, 1.1f));
		break;
	case GSHurtKind::Head:
		GSAudio::Play2D(this, TEXT("Hit_Head"), 0.9f, 1.f);
		GSAudio::Play2D(this, TEXT("Ear_Ring"), 0.45f, 1.f);
		GSAudio::Play2D(this, TEXT("Hit_Body"), 0.6f, FMath::FRandRange(0.9f, 1.1f));
		break;
	case GSHurtKind::HeadArmor:
		GSAudio::Play2D(this, TEXT("Hit_Armor"), 0.8f, 1.25f);
		GSAudio::Play2D(this, TEXT("Ear_Ring"), 0.3f, 1.f);
		break;
	case GSHurtKind::ArmorBroke:
		GSAudio::Play2D(this, TEXT("Armor_Break"), 0.9f, 1.f);
		if (Zone == GSZone::Head) GSAudio::Play2D(this, TEXT("Ear_Ring"), 0.4f, 1.f);
		GSAudio::Play2D(this, TEXT("Hit_Body"), 0.5f, FMath::FRandRange(0.9f, 1.1f));
		break;
	default:
		GSAudio::Play2D(this, TEXT("Hit_Body"), 0.8f, FMath::FRandRange(0.9f, 1.1f));
		break;
	}
}

float AGSCharacter::GetLowHealthAlpha() const
{
	if (bDead) return 0.f;
	const float F = Health / FMath::Max(MaxHealth, 1.f);
	return FMath::Clamp((0.35f - F) / 0.35f, 0.f, 1.f);
}

void AGSCharacter::TickRecoil(float Dt)
{
	const FVector2D Step = RecoilPending * (1.f - FMath::Exp(-Dt * 35.f));
	RecoilPending -= Step;
	AddControllerPitchInput(-Step.X);
	AddControllerYawInput(Step.Y);
	RecoilRecover += Step * 0.6f;
	if (GetWorld()->GetTimeSeconds() - LastShotTime > 0.15f)
	{
		const FVector2D Back = RecoilRecover * (1.f - FMath::Exp(-Dt * 5.f));
		RecoilRecover -= Back;
		AddControllerPitchInput(Back.X);
		AddControllerYawInput(-Back.Y);
	}
}

// Casing / magazine props from Tools/Blender/fx_models.py (nullptr until imported; callers fall back to shapes)
static UStaticMesh* GSFXMesh(const TCHAR* Name)
{
	static TMap<FString, TWeakObjectPtr<UStaticMesh>> Cache;
	TWeakObjectPtr<UStaticMesh>& Slot = Cache.FindOrAdd(Name);
	if (!Slot.IsValid())
	{
		Slot = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Models/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	return Slot.Get();
}

void AGSCharacter::SpawnShellFX(EGSWeaponClass Class)
{
	UWorld* World = GetWorld();
	if (!World || GetNetMode() == NM_DedicatedServer) return;
	const FVector Port = FPWeaponMesh->GetComponentLocation() + Camera->GetForwardVector() * 8.f + Camera->GetRightVector() * 3.f + Camera->GetUpVector() * 4.f;
	AStaticMeshActor* Shell = World->SpawnActor<AStaticMeshActor>(Port, Camera->GetComponentRotation() + FRotator(0.f, 90.f, 90.f));
	if (!Shell) return;
	Shell->SetMobility(EComponentMobility::Movable);
	UStaticMeshComponent* C = Shell->GetStaticMeshComponent();
	const bool bShotgun = Class == EGSWeaponClass::Shotgun;
	const bool bBig = Class == EGSWeaponClass::Sniper || Class == EGSWeaponClass::LMG;
	const bool bPistolCase = Class == EGSWeaponClass::Pistol || Class == EGSWeaponClass::SMG;
	if (UStaticMesh* Real = GSFXMesh(bShotgun ? TEXT("SM_FX_ShellShotgun") : bPistolCase ? TEXT("SM_FX_CasingPistol") : TEXT("SM_FX_CasingRifle")))
	{
		C->SetStaticMesh(Real);
		C->SetWorldScale3D(FVector(bBig ? 1.35f : 1.f));
	}
	else
	{
		C->SetStaticMesh(GSUtils::Cylinder());
		const float R = bShotgun ? 0.02f : bBig ? 0.012f : 0.009f;
		const float L = bShotgun ? 0.065f : bBig ? 0.07f : Class == EGSWeaponClass::Pistol ? 0.02f : 0.045f;
		C->SetWorldScale3D(FVector(R, R, L));
		GSUtils::SetColor(C, bShotgun ? FLinearColor(0.5f, 0.04f, 0.03f) : FLinearColor(0.75f, 0.52f, 0.18f));
	}
	C->SetCastShadow(false);
	C->SetCollisionProfileName(TEXT("PhysicsActor"));
	C->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	C->SetSimulatePhysics(true);
	C->SetPhysicsLinearVelocity(GetVelocity() + Camera->GetRightVector() * FMath::FRandRange(150.f, 230.f)
		+ Camera->GetUpVector() * FMath::FRandRange(90.f, 160.f) - Camera->GetForwardVector() * FMath::FRandRange(0.f, 40.f));
	C->SetPhysicsAngularVelocityInDegrees(FVector(FMath::FRandRange(-900.f, 900.f), FMath::FRandRange(-900.f, 900.f), 0.f));
	Shell->SetLifeSpan(4.f);
	// tinkle when it lands
	FTimerHandle H;
	TWeakObjectPtr<AStaticMeshActor> WeakShell(Shell);
	World->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this, WeakShell]()
	{
		if (WeakShell.IsValid()) GSAudio::PlayAt(this, GSAudio::Variant(TEXT("Shell"), 3), WeakShell->GetActorLocation(), 0.4f, 12.f, FMath::FRandRange(0.9f, 1.2f));
	}), FMath::FRandRange(0.45f, 0.7f), false);
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
	const FGSWeaponSlot* MW = GetCurrentWeapon();
	const bool bPistolMag = MW && GSItems::Weapon(MW->Id).Class == EGSWeaponClass::Pistol;
	if (UStaticMesh* Real = GSFXMesh(bPistolMag ? TEXT("SM_FX_MagPistol") : TEXT("SM_FX_MagRifle")))
	{
		C->SetStaticMesh(Real);
	}
	else
	{
		C->SetStaticMesh(GSUtils::Cube());
		C->SetWorldScale3D(FVector(0.03f, 0.07f, 0.16f));
		GSUtils::SetColor(C, FLinearColor(0.04f, 0.04f, 0.045f));
	}
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
	if (!bHas)
	{
		ApplyAttachmentVisuals(nullptr);
		return;
	}
	const FGSWeaponStats& S = GSItems::Weapon(W->Id);
	const FVector Fallback = S.Class == EGSWeaponClass::Pistol ? FVector(0.22f, 0.04f, 0.12f)
		: S.Class == EGSWeaponClass::Melee ? FVector(0.7f, 0.03f, 0.05f) : FVector(0.95f, 0.05f, 0.11f);
	ShowItemOn(FPWeaponMesh, W->Id, Fallback, 1.f);
	ShowItemOn(TPWeaponMesh, W->Id, Fallback, 1.f, true);
	ApplyAttachmentVisuals(W);
}

void AGSCharacter::ApplyAttachmentVisuals(const FGSWeaponSlot* W)
{
	UStaticMeshComponent* Parts[] = { FPAttSight, FPAttMuzzle, FPAttLaser, FPAttGrip, TPAttSight, TPAttMuzzle, TPAttLaser, TPAttGrip };
	for (UStaticMeshComponent* P : Parts) if (P) P->SetVisibility(false);
	if (!W || W->Id.IsNone() || bDead || W->Attachments == 0) return;
	const FGSWeaponStats& S = GSItems::Weapon(W->Id);
	if (S.Mode == EGSFireMode::Melee) return;
	const bool bPistol = S.Class == EGSWeaponClass::Pistol;
	// Each attachment goes on both guns: the first-person one you see and the third-person one others see
	auto Show = [](UStaticMeshComponent* FP, UStaticMeshComponent* TP, FName ItemId, const FVector& Loc)
	{
		UStaticMesh* M = GSItems::GetMesh(ItemId);
		if (!M) return;
		for (UStaticMeshComponent* C : { FP, TP })
		{
			if (!C) continue;
			C->SetStaticMesh(M);
			C->SetRelativeLocation(Loc);
			C->SetRelativeRotation(FRotator::ZeroRotator);
			C->SetRelativeScale3D(FVector(1.f));
			C->SetVisibility(true);
		}
	};
	// Optics sit on the iron-sight line (their optical axis is this high above their base, in cm)
	const float SightX = bPistol ? 1.f : 3.f;
	if (W->Attachments & GSAttach::Scope8x)     Show(FPAttSight, TPAttSight, TEXT("Scope8x"), FVector(SightX, 0.f, S.SightZ - 5.0f));
	else if (W->Attachments & GSAttach::Scope)  Show(FPAttSight, TPAttSight, TEXT("Scope"), FVector(SightX, 0.f, S.SightZ - 4.0f));
	else if (W->Attachments & GSAttach::Holo)   Show(FPAttSight, TPAttSight, TEXT("Holo"), FVector(SightX, 0.f, S.SightZ - 3.7f));
	else if (W->Attachments & GSAttach::RedDot) Show(FPAttSight, TPAttSight, TEXT("RedDot"), FVector(SightX, 0.f, S.SightZ - 3.2f));
	// Muzzle devices on the bore axis, mount over the muzzle
	if (W->Attachments & GSAttach::Suppressor)        Show(FPAttMuzzle, TPAttMuzzle, TEXT("Suppressor"), FVector(S.MuzzleX + 9.5f, 0.f, S.BoreZ));
	else if (W->Attachments & GSAttach::Compensator)  Show(FPAttMuzzle, TPAttMuzzle, TEXT("Compensator"), FVector(S.MuzzleX + 2.5f, 0.f, S.BoreZ - 1.3f));
	// Under the barrel: laser forward, grip behind it
	if (W->Attachments & GSAttach::Laser)
	{
		Show(FPAttLaser, TPAttLaser, TEXT("Laser"), bPistol ? FVector(S.MuzzleX * 0.6f, 0.f, S.BoreZ - 4.5f) : FVector(S.MuzzleX * 0.62f, 0.f, S.BoreZ - 6.f));
	}
	if ((W->Attachments & GSAttach::Grip) && !bPistol)
	{
		Show(FPAttGrip, TPAttGrip, TEXT("Grip"), FVector(S.MuzzleX * 0.45f, 0.f, S.BoreZ - 4.f - 10.75f));
	}
}

void AGSCharacter::ApplyGearVisuals()
{
	VestMesh->SetVisibility(!BodyArmor.IsNone() && !bDead);
	if (!BodyArmor.IsNone()) ShowItemOn(VestMesh, BodyArmor, FVector(0.68f, 0.68f, 0.5f), 1.6f);
	HelmetMesh->SetVisibility(!Helmet.IsNone() && !bDead);
	if (!Helmet.IsNone()) ShowItemOn(HelmetMesh, Helmet, FVector(0.32f, 0.32f, 0.18f), 1.15f);
	BackpackMesh->SetVisibility(!WornBackpack.IsNone() && !bDead);
	if (!WornBackpack.IsNone())
	{
		ShowItemOn(BackpackMesh, WornBackpack, FVector(0.2f, 0.32f, 0.45f), 1.f);
		BackpackMesh->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	}
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
		WornBackpack = Def->Id;
		ApplyGearVisuals();
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
	if (GSCharInputBlocked(this, TEXT("bruk/plukk opp"))) return;
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
	if (GSCharInputBlocked(this, TEXT("neste ting"))) return;
	if (AGSPlayerController* TPC = TerminalPC()) { TPC->TerminalInput(TEXT("Q")); return; }
	if (Backpack.Num() == 0) { SelectedItem = 0; return; }
	SelectedItem = (SelectedItem + 1) % Backpack.Num();
}

void AGSCharacter::UseSelectedItem()
{
	if (GSCharInputBlocked(this, TEXT("bruk ting"))) return;
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
		if (Def->Cat == EGSItemCat::Medicine && Def->Heal > 0.f)
		{
			// Dressing the wound: a plaster helps a little, a bandage a lot, a first-aid kit sets the limb right.
			const float Treat = Def->Heal / 60.f;
			LegInjury = FMath::Max(0.f, LegInjury - Treat);
			ArmInjury = FMath::Max(0.f, ArmInjury - Treat);
		}
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
				ApplyWeaponVisuals(); // listen-server host does not get OnRep
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
	// Aim rotation, not the camera: on the server a client's camera never looks up or down (see DoMeleeHit).
	const FVector Fwd = GetBaseAimRotation().Vector();
	const bool bPlace = Def.Kind == FName(TEXT("mine")) || Def.Kind == FName(TEXT("c4"));
	const FVector Vel = Fwd * (bPlace ? 700.f : 1500.f) + FVector(0.f, 0.f, bPlace ? 100.f : 250.f);
	AGSProjectile::Launch(GetWorld(), this, Def.Kind, Def.Id, Camera->GetComponentLocation() + Fwd * 50.f, Vel + GetVelocity() * 0.5f,
		1.f, Def.ThrowDamage, Def.ThrowRadius, Def.Fuse);
}

void AGSCharacter::DropSelectedItem()
{
	if (GSCharInputBlocked(this, TEXT("slipp ting"))) return;
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

uint8 AGSCharacter::GetTeam() const
{
	const AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	return PS ? PS->Team : GSTeam::None;
}

void AGSCharacter::PlaceArmband(UStaticMeshComponent* Band, FName UpperArm, FName LowerArm)
{
	USkeletalMeshComponent* Skel = GetMesh();
	const USkeletalMesh* Asset = Skel ? Skel->GetSkeletalMeshAsset() : nullptr;
	if (!Band || !Asset) return;
	const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
	const int32 Lower = Ref.FindBoneIndex(LowerArm);
	if (Ref.FindBoneIndex(UpperArm) == INDEX_NONE || Lower == INDEX_NONE) return;
	// Where the elbow sits in the upper arm's own space tells which way the arm points (the right arm is mirrored).
	const FVector ToElbow = FVector(Ref.GetRefBonePose()[Lower].GetTranslation());
	const float Length = ToElbow.Size();
	if (Length < 5.f) return;
	const FVector Dir = ToElbow / Length;
	Band->AttachToComponent(Skel, FAttachmentTransformRules::SnapToTargetNotIncludingScale, UpperArm);
	Band->SetRelativeLocation(Dir * Length * 0.5f);
	Band->SetRelativeRotation(FRotationMatrix::MakeFromZ(Dir).Rotator());   // the cylinder's axis along the arm
	Band->SetRelativeScale3D(FVector(0.135f, 0.135f, 0.085f));              // 13.5 cm across, 8.5 cm wide
}

void AGSCharacter::ApplyTeamLook()
{
	if (GetNetMode() == NM_DedicatedServer) return;
	const uint8 Team = GetTeam();
	if (Team == AppliedTeam) return;
	AppliedTeam = Team;
	// Sober cloth colours (GSTeam::Color), plain grey before the team is known. Nothing glows.
	const FLinearColor Cloth = GSTeam::Color(Team);
	const FLinearColor Jacket = Team == GSTeam::None ? FLinearColor(0.22f, 0.22f, 0.21f) : FMath::Lerp(Cloth, FLinearColor(0.12f, 0.12f, 0.12f), 0.25f);
	GSUtils::SetColor(BodyMesh, Jacket);   // stand-in body when no mannequin is found
	if (bHumanBody)
	{
		static const FName PaintTint(TEXT("Paint Tint"));
		for (USkeletalMeshComponent* Skel : { GetMesh(), FPLegs.Get() })
		{
			if (!Skel || !Skel->GetSkeletalMeshAsset()) continue;
			for (int32 Slot = 0; Slot < Skel->GetNumMaterials(); ++Slot)
			{
				if (UMaterialInstanceDynamic* MID = Skel->CreateDynamicMaterialInstance(Slot))
				{
					MID->SetVectorParameterValue(PaintTint, Jacket);
				}
			}
		}
		if (ArmbandL->GetAttachSocketName().IsNone())
		{
			PlaceArmband(ArmbandL, TEXT("upperarm_l"), TEXT("lowerarm_l"));
			PlaceArmband(ArmbandR, TEXT("upperarm_r"), TEXT("lowerarm_r"));
		}
	}
	const bool bBands = bHumanBody && Team != GSTeam::None && !ArmbandL->GetAttachSocketName().IsNone();
	for (UStaticMeshComponent* Band : { ArmbandL.Get(), ArmbandR.Get() })
	{
		// A little brighter than the jacket so the band reads against it, still plain cloth.
		if (bBands) GSUtils::SetColor(Band, FMath::Lerp(Cloth, FLinearColor::White, 0.18f));
		Band->SetVisibility(bBands);
	}
}

bool AGSCharacter::IsSparedFrom(const AController* From) const
{
	if (!From || From == GetController()) return false;
	const AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>();
	if (!GS || GS->bFriendlyFire) return false;
	const AGSPlayerState* Mine = GetPlayerState<AGSPlayerState>();
	const AGSPlayerState* Theirs = From->GetPlayerState<AGSPlayerState>();
	return Mine && Theirs && Mine->Team != GSTeam::None && Mine->Team == Theirs->Team;
}

void AGSCharacter::ApplyFlash(const FVector& Location, float Radius, const AController* Thrower)
{
	if (bDead || IsSparedFrom(Thrower)) return;   // a teammate's flashbang does not blind you (your own does)
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
	// Lobby / countdown / results: warm-up, nobody gets hurt.
	if (const AGSGameMode* GM = GetWorld()->GetAuthGameMode<AGSGameMode>())
	{
		if (!GM->DamageEnabled()) return 0.f;
	}
	// Same team and friendly fire off: nothing happens at all (no damage, limb injury, blood, hit marker or wedge).
	// The controller is used, not the pawn, so a grenade from a teammate who has died since is covered too.
	if (IsSparedFrom(EventInstigator)) return 0.f;

	bool bHeadshot = false;
	uint8 Zone = GSZone::None;
	bool bFace = false;
	// Where and from which way the body was hit, for the blood everyone sees (MulticastWoundFX below)
	FVector WoundPoint = GetActorLocation() + FVector(0.f, 0.f, 30.f);
	FVector WoundDir = FVector::ZeroVector;
	bool bPointHit = false;
	bool bBurn = false;
	float Damage = DamageAmount;
	AGSCharacter* Shooter = Cast<AGSCharacter>(DamageCauser);
	FString WeaponName = TEXT("Verden");
	if (Shooter) WeaponName = Shooter->CurrentWeaponName();
	const bool bBullet = !IncomingWeaponId.IsNone();   // a bullet from GSBallistics: it knows its own gun and where it was fired
	if (bBullet) WeaponName = GSItems::Name(IncomingWeaponId);
	// Grenades, mines, rockets, bolts: the thrower may have died and got a new body since. Shooter is then his new body
	// (hit marker goes to him), but the direction and the life steal only count the body that actually threw it.
	const AGSProjectile* Proj = Cast<AGSProjectile>(DamageCauser);
	bool bShooterThrewIt = true;
	if (Proj)
	{
		WeaponName = GSItems::Name(Proj->ItemId);
		Shooter = Proj->GetThrowerNow(bShooterThrewIt);
		bBurn = Proj->Kind == FName(TEXT("fire"));   // standing in molotov fire: burns, does not bleed
	}

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PE = static_cast<const FPointDamageEvent&>(DamageEvent);
		WoundPoint = PE.HitInfo.ImpactPoint;
		WoundDir = FVector(PE.ShotDirection);
		bPointHit = true;
		Zone = ZoneOf(PE.HitInfo.GetComponent());
		if (Zone == GSZone::None) Zone = GSZone::Chest;   // a point hit that found no zone box counts as the torso
		if (Zone == GSZone::Head)
		{
			// The helmet covers the skull, not the face: the lower part of the head, hit from the front, goes past it.
			const float LocalZ = PE.HitInfo.GetComponent()->GetComponentTransform().InverseTransformPosition(PE.HitInfo.ImpactPoint).Z;
			bFace = LocalZ < -2.5f && FVector::DotProduct(FVector(PE.ShotDirection), GetActorForwardVector()) < -0.2f;
		}
		if (Zone == GSZone::Head)
		{
			bHeadshot = true;
			if (bBullet) Damage *= GSItems::Weapon(IncomingWeaponId).HeadshotMultiplier;
			else if (const AGSCharacter* S = Cast<AGSCharacter>(DamageCauser))
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

	// Arms and legs are seldom deadly on their own; the belly a little less than the chest.
	if (Zone == GSZone::Arm) Damage *= 0.55f;
	else if (Zone == GSZone::Leg) Damage *= 0.65f;
	else if (Zone == GSZone::Stomach) Damage *= 0.9f;

	Damage *= GetMods().Taken;

	// Special rounds
	const uint8 Ammo = IncomingAmmoType;
	// Armor only covers what it covers: the helmet the skull, the vest the torso (and it takes part of a blast).
	const bool bHelmetCovers = bHeadshot && !bFace;
	const bool bVestCovers = !bHeadshot && Zone != GSZone::Arm && Zone != GSZone::Leg;
	const bool bArmored = (bHelmetCovers && !Helmet.IsNone()) || (bVestCovers && !BodyArmor.IsNone());
	float AbsorbMul = 1.f;
	if (Ammo == GSAmmoType::ArmorPiercing) { AbsorbMul = 0.35f; Damage *= 0.95f; }
	else if (Ammo == GSAmmoType::HollowPoint) Damage *= bArmored ? 0.75f : 1.3f;
	else if (Ammo == GSAmmoType::Incendiary) AddEffect(TEXT("Brennende"));
	else if (Ammo == GSAmmoType::Shock) AddEffect(TEXT("Sjokk"));

	// Armor soaks up part of the damage until it breaks.
	const float HealthBefore = Health;
	bool bArmorHit = false;
	bool bArmorBroke = false;
	if (bHelmetCovers && !Helmet.IsNone())
	{
		const FGSItemDef* H = GSItems::Find(Helmet);
		const float Soak = FMath::Min(Damage * (H ? H->Absorb : 0.3f) * AbsorbMul, HelmetPoints);
		HelmetPoints -= Soak;
		Damage -= Soak;
		bArmorHit = Soak > 0.f;
		if (HelmetPoints <= 0.5f) { Helmet = NAME_None; HelmetPoints = 0.f; bArmorBroke = true; ApplyGearVisuals(); ClientToast(TEXT("Hjelmen gikk i stykker!")); }
	}
	else if (bVestCovers && !BodyArmor.IsNone())
	{
		const FGSItemDef* A = GSItems::Find(BodyArmor);
		const float Soak = FMath::Min(Damage * (A ? A->Absorb : 0.3f) * AbsorbMul, ArmorPoints);
		ArmorPoints -= Soak;
		Damage -= Soak;
		bArmorHit = Soak > 0.f;
		if (ArmorPoints <= 0.5f) { BodyArmor = NAME_None; ArmorPoints = 0.f; bArmorBroke = true; ApplyGearVisuals(); ClientToast(TEXT("Vesten er \u00F8delagt!")); }
	}

	Health = FMath::Max(0.f, Health - Damage);
	// A hit that draws blood from a limb leaves it hurt until it is treated.
	if (Health > 0.f && HealthBefore > Health)
	{
		if (Zone == GSZone::Leg) LegInjury = FMath::Min(1.f, LegInjury + (HealthBefore - Health) / 45.f);
		else if (Zone == GSZone::Arm) ArmInjury = FMath::Min(1.f, ArmInjury + (HealthBefore - Health) / 40.f);
	}
	LastDamageTakenTime = GetWorld()->GetTimeSeconds();
	// Tell the one who was hit where it came from, how hard, and whether the vest/helmet took it. Sent before
	// OnRep_Health below so a listen-server host does not also play the generic hurt sound.
	{
		const uint8 Kind = bArmorBroke ? GSHurtKind::ArmorBroke
			: bHeadshot ? (bArmorHit ? GSHurtKind::HeadArmor : GSHurtKind::Head)
			: bArmorHit ? GSHurtKind::Armor : GSHurtKind::Body;
		// Projectiles: from where it went off (blast) or where it was fired from (bolt, rocket hitting you), never
		// from the thrower's new body or his corpse.
		const bool bOwnProjectile = Proj && (Proj->GetOwner() == this || Shooter == this);
		const bool bHasFrom = Proj ? !bOwnProjectile : (bBullet || (Shooter && Shooter != this));
		const FVector From = !bHasFrom ? GetActorLocation()
			: Proj ? (bPointHit ? Proj->LaunchLocation : Proj->GetActorLocation())
			: bBullet ? IncomingOrigin : Shooter->GetActorLocation();
		ClientTookHit(From, bHasFrom, HealthBefore - Health, static_cast<uint8>(Kind | (Zone << 4)));   // zone in the high bits
	}
	// Everyone nearby sees the wound. Only real hits: hunger, thirst and poison never come through here, and fire
	// burns without blood. Sent before Die() below, so a listen-server host knows the wound when the body falls.
	if ((DamageCauser || bBullet) && !bBurn)   // a bullet whose shooter has left has no causer, but still wounds
	{
		const float Lost = HealthBefore - Health;
		if (!bPointHit && DamageCauser) WoundDir = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
		if (WoundDir.IsNearlyZero()) WoundDir = FVector::DownVector;   // own grenade at your feet
		uint8 Wound = GSWound::Blast;
		if (bArmorHit && Lost < 12.f) Wound = GSWound::Armor;
		else if (bHeadshot) Wound = GSWound::Head;
		else if (bIncomingMelee) Wound = GSWound::Blade;
		else if (bPointHit) Wound = Lost >= 18.f ? GSWound::Through : GSWound::Flesh;
		if (Lost > 0.f || bArmorHit)
		{
			if (WoundFrame != GFrameCounter) { WoundFrame = GFrameCounter; WoundsThisFrame = 0; }
			if (++WoundsThisFrame <= 3)
			{
				MulticastWoundFX(WoundPoint, WoundDir, Wound, static_cast<uint8>(FMath::Clamp(Lost / 60.f, 0.f, 1.f) * 255.f));
			}
		}
	}
	LastDamageWeapon = WeaponName;
	LastDamageInstigator = EventInstigator;
	OnRep_Health();

	// Blodtorst: the shooter heals from damage dealt (not a new body healing from a mine the old one laid).
	if (Shooter && Shooter != this && bShooterThrewIt && !Shooter->bDead)
	{
		const float Steal = Shooter->GetMods().Lifesteal;
		if (Steal > 0.f)
		{
			Shooter->Health = FMath::Min(Shooter->MaxHealth, Shooter->Health + Damage * Steal);
			Shooter->OnRep_Health();   // a listen-server host sees the change like a client does
		}
	}

	bool bKilled = Health <= 0.f;
	if (bKilled && GetMods().bPhoenix)
	{
		// Foniks: get back up once.
		Health = 50.f;
		OnRep_Health();
		Effects.RemoveAll([](const FGSActiveEffect& E) { return E.Id == FxFoniks; });
		AddEffect(FName(TEXT("Adrenalin")));
		ClientToast(TEXT("F\u00D8NIKS! Du reiste deg fra de d\u00F8de."));
		bKilled = false;
	}
	if (Shooter && Shooter != this)
	{
		Shooter->ClientHitMarker(bKilled, bHeadshot, bArmorHit);
	}
	if (bKilled)
	{
		Die(EventInstigator, bHeadshot, WeaponName, Proj);
	}
	return Damage;
}

void AGSCharacter::OnRep_Health()
{
	if (IsLocallyControlled())
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (Health < LastSeenHealth - 0.5f)
		{
			LastDamageTakenTime = Now;
			// Weapon hits already played their own sound and flash (ClientTookHit). This is hunger, thirst,
			// poison and the like: a softer thud and a faint red edge.
			if (Now - LastHurtFlashTime >= 0.3f)
			{
				GSAudio::Play2D(this, TEXT("Hit_Body"), 0.4f, FMath::FRandRange(0.9f, 1.1f));
				LastHurtFlashTime = Now;
				LastHurtKind = GSHurtKind::Body;
				LastHurtFlashStrength = 0.25f;
				LastHurtBlood = 0.25f;
				HurtBurstLost = 0.f;   // a weapon hit arriving right after this adds its own damage, not this
			}
		}
		else if (Health > LastSeenHealth + 4.f)
		{
			LastHealTime = Now;   // food, bandage, syringe: short green edge
		}
	}
	LastSeenHealth = Health;
}

void AGSCharacter::Die(AController* Killer, bool bHeadshot, const FString& WeaponName, const AGSProjectile* KillingProjectile)
{
	if (bDead) return;
	bDead = true;
	StopFire();
	bAiming = false;
	CancelUse();

	if (AGSGameMode* GM = GetWorld()->GetAuthGameMode<AGSGameMode>())
	{
		GM->OnPlayerKilled(Killer, GetController(), WeaponName, bHeadshot);
	}
	DropEverything();
	bFlashlightOn = false;

	// Death cam and killer card for the player who died (server has the real health/armor of the killer,
	// even though those are only replicated to their owner).
	AGSPlayerController* VictimPC = Cast<AGSPlayerController>(GetController());
	APawn* KillerPawn = Killer ? Killer->GetPawn() : nullptr;
	if (KillerPawn == this) KillerPawn = nullptr;
	// A grenade or mine from a body that has died since: his new body is not the one who killed you (it may be
	// across campus). Show where it was thrown from instead, without his health and armor.
	if (KillingProjectile && KillerPawn && KillerPawn != KillingProjectile->GetOwner()) KillerPawn = nullptr;
	FGSDeathInfo Info;
	Info.Weapon = WeaponName;
	Info.bHeadshot = bHeadshot;
	// The name is shown even if the killer has no pawn any more (his bullet or grenade got you after he died);
	// killing yourself (own grenade) leaves it empty.
	if (Killer && Killer != GetController())
	{
		Info.Killer = Killer->PlayerState ? Killer->PlayerState->GetPlayerName() : FString(TEXT("?"));
	}
	if (KillerPawn)
	{
		Info.KillerLocation = KillerPawn->GetActorLocation();
		Info.bHasKillerLocation = true;
		Info.DistanceM = FVector::Dist(KillerPawn->GetActorLocation(), GetActorLocation()) / 100.f;
		if (const AGSCharacter* K = Cast<AGSCharacter>(KillerPawn))
		{
			Info.KillerHealth = K->Health;
			Info.KillerMaxHealth = K->MaxHealth;
			Info.KillerArmor = K->ArmorPoints + K->HelmetPoints;
			Info.bHasKillerHealth = true;
		}
	}
	else if (KillingProjectile && !Info.Killer.IsEmpty())
	{
		Info.KillerLocation = KillingProjectile->LaunchLocation;
		Info.bHasKillerLocation = true;
		Info.DistanceM = FVector::Dist(KillingProjectile->LaunchLocation, GetActorLocation()) / 100.f;
	}

	OnRep_Dead();
	if (VictimPC) VictimPC->DeathCamBody = this;   // keeps the view on this body when the controller lets go of it
	DetachFromControllerPendingDestroy();
	if (VictimPC)
	{
		VictimPC->SetViewTarget(this);
		VictimPC->ClientDeathCam(this, KillerPawn, Info);
	}
	// Never more than 12 bodies lying around: the oldest goes first.
	{
		int32 Bodies = 0;
		AGSCharacter* Oldest = nullptr;
		for (TActorIterator<AGSCharacter> It(GetWorld()); It; ++It)
		{
			if (!It->bDead || *It == this || It->GetLifeSpan() <= 0.f) continue;
			++Bodies;
			if (!Oldest || It->GetLifeSpan() < Oldest->GetLifeSpan()) Oldest = *It;
		}
		if (Bodies >= 12 && Oldest) Oldest->SetLifeSpan(0.5f);
	}
	SetLifeSpan(BodyLifeSeconds);
}

void AGSCharacter::ApplyHitZoneLayout(bool bCrouch)
{
	for (int32 I = 0; I < 7; ++I)
	{
		if (!HitZones[I]) continue;
		const FGSZoneBox& Z = GSZoneBoxes[I];
		HitZones[I]->SetRelativeLocation(bCrouch ? Z.Crouch : Z.Stand);
		HitZones[I]->SetBoxExtent(bCrouch ? Z.CrouchExt : Z.StandExt, false);
	}
}

uint8 AGSCharacter::ZoneOf(const UPrimitiveComponent* Comp) const
{
	for (int32 I = 0; I < 7; ++I)
	{
		if (Comp && HitZones[I] == Comp) return GSZoneBoxes[I].Zone;
	}
	return GSZone::None;
}

void AGSCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	ApplyHitZoneLayout(true);
	FixBodyMeshOffset(HalfHeightAdjust);
}

void AGSCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	ApplyHitZoneLayout(false);
	FixBodyMeshOffset(0.f);
}

float AGSCharacter::GetLegPain() const
{
	const bool bNumb = Effects.ContainsByPredicate([](const FGSActiveEffect& E) { return E.Id == FxPainkillers; });
	return LegInjury * (bNumb ? 0.5f : 1.f);
}

float AGSCharacter::GetArmPain() const
{
	const bool bNumb = Effects.ContainsByPredicate([](const FGSActiveEffect& E) { return E.Id == FxPainkillers; });
	return ArmInjury * (bNumb ? 0.5f : 1.f);
}

float AGSCharacter::ReloadSeconds(const FGSWeaponStats& S) const
{
	return S.ReloadTime * GetMods().Reload * (1.f + 0.6f * GetArmPain());   // a shot arm fumbles the magazine
}

void AGSCharacter::FixBodyMeshOffset(float HalfHeightAdjust)
{
	if (!bHumanBody || bDead) return;
	const float StandHalf = GetDefault<AGSCharacter>()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const FVector Loc(0.f, 0.f, -StandHalf + HalfHeightAdjust);   // feet on the floor (the capsule centre drops when crouched)
	const FRotator Rot(0.f, -90.f, 0.f);
	GetMesh()->SetRelativeLocationAndRotation(Loc, Rot);
	CacheInitialMeshOffset(Loc, Rot);   // the engine's network smoothing and crouch code start from this
}

void AGSCharacter::SetupHumanBody()
{
	// Paths of the standard Unreal characters (Third Person template / content pack, older templates).
	static const TCHAR* MeshPaths[] = {
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"),
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"),
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn.SKM_Quinn"),
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"),
		TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"),
	};
	static const TCHAR* AnimPaths[] = {
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"),
		TEXT("/Game/Characters/Mannequins/Anims/ABP_Manny.ABP_Manny_C"),
		TEXT("/Game/Characters/Mannequins/Anims/ABP_Quinn.ABP_Quinn_C"),
		TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"),
		TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn.ABP_Quinn_C"),
		TEXT("/Game/Mannequin/Animations/ThirdPerson_AnimBP.ThirdPerson_AnimBP_C"),
	};
	USkeletalMesh* Body = nullptr;
	// Mix Manny and Quinn between players
	const int32 First = (GetTypeHash(GetName()) % 2 == 0) ? 0 : 2;
	const int32 NumMeshes = static_cast<int32>(UE_ARRAY_COUNT(MeshPaths));
	for (int32 I = 0; I < NumMeshes && !Body; ++I)
	{
		Body = LoadObject<USkeletalMesh>(nullptr, MeshPaths[(First + I) % NumMeshes], nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	UClass* Anim = nullptr;
	for (const TCHAR* Path : AnimPaths)
	{
		Anim = LoadClass<UAnimInstance>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (Anim) break;
	}
	USkeletalMeshComponent* Skel = GetMesh();
	if (!Body || !Skel) return;

	Skel->SetSkeletalMesh(Body);
	if (Anim) Skel->SetAnimInstanceClass(Anim);
	Skel->SetRelativeLocation(FVector(0.f, 0.f, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
	Skel->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Skel->SetOwnerNoSee(true);
	Skel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Skel->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Skel->SetVisibility(true);
	bHumanBody = true;
	FixBodyMeshOffset(bIsCrouched ? GetDefault<AGSCharacter>()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() : 0.f);
	// Your own legs when you look down: same body, upper body hidden, a little behind the camera.
	if (IsLocallyControlled() && FPLegs)
	{
		FPLegs->SetSkeletalMesh(Body);
		if (Anim) FPLegs->SetAnimInstanceClass(Anim);
		FPLegs->SetRelativeLocation(FVector(-14.f, 0.f, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
		FPLegs->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		FPLegs->HideBoneByName(TEXT("spine_03"), EPhysBodyOp::PBO_None);
		FPLegs->SetVisibility(true);
	}
	// the old shapes are only the stand-in body when no mannequin is found
	BodyMesh->SetVisibility(false);
	HeadMesh->SetVisibility(false);
}

void AGSCharacter::OnRep_Dead()
{
	if (!bDead) return;
	// Death cam starts from where your eyes were
	DeathCamStart = GetWorld()->GetTimeSeconds();
	DeathEyeLoc = Camera->GetComponentLocation();
	DeathEyeRot = Camera->GetComponentRotation();
	bDeathCamInit = false;
	// The death cam looks at your own body: show it to you too, and hide the first-person arms, gun and item.
	if (bHumanBody) { GetMesh()->SetOwnerNoSee(false); ArmbandL->SetOwnerNoSee(false); ArmbandR->SetOwnerNoSee(false); }
	else { BodyMesh->SetOwnerNoSee(false); HeadMesh->SetOwnerNoSee(false); }
	for (UPrimitiveComponent* FP : TInlineComponentArray<UPrimitiveComponent*>(this))
	{
		if (FP && FP->bOnlyOwnerSee) FP->SetVisibility(false);
	}
	if (FPLegs) FPLegs->SetVisibility(false);
	// The death cam keeps this camera's post-process: drop the low-health grey (no more local ticks may come
	// once the controller lets go of the body).
	{
		const FGSEffectMods Mods = GetMods();
		ApplyLowHealthPostProcess(bNightVisionGoggles || Mods.bNightVision ? 7 : Mods.Visual, 0.f);
	}
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	for (UBoxComponent* Z : HitZones) { if (Z) Z->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
	if (bHumanBody)
	{
		// Real ragdoll: the body falls the way physics says (local only, it is just for looks)
		USkeletalMeshComponent* Skel = GetMesh();
		Skel->SetCollisionProfileName(TEXT("Ragdoll"));
		Skel->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		Skel->SetAllBodiesSimulatePhysics(true);
		Skel->SetSimulatePhysics(true);
		Skel->WakeAllRigidBodies();
		ApplyDeathImpulse();
	}
	// Simple "fall over" for the old shapes (hidden when the human body is used)
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -80.f));
	BodyMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	HeadMesh->SetRelativeLocation(FVector(95.f, 0.f, -80.f));
	ApplyWeaponVisuals();
	ApplyGearVisuals();
	// The pool forms once the body has come to rest.
	if (GetNetMode() != NM_DedicatedServer)
	{
		GetWorldTimerManager().SetTimer(DeathPoolTimer, this, &AGSCharacter::SpawnDeathPool, 1.6f, false);
	}
}

void AGSCharacter::ApplyDeathImpulse()
{
	// A bullet does not throw a body: a small push where it hit, and the body collapses. Blasts push harder.
	if (!bDead || !bHumanBody || bDeathImpulseDone) return;
	if (GetWorld()->GetTimeSeconds() - LastWoundTime > 0.8f) return;   // no hit (hunger...) or the wound has not arrived yet
	bDeathImpulseDone = true;
	USkeletalMeshComponent* Skel = GetMesh();
	if (LastWoundKind == GSWound::Blast)
	{
		Skel->AddImpulse((LastWoundDir + FVector(0.f, 0.f, 0.35f)) * (3000.f + 9000.f * LastWoundStrength), NAME_None, false);
		return;
	}
	// The part that was hit takes as much of the push as it can without being flung (a head snaps back, an arm
	// is knocked away); the rest goes into the body as a whole.
	const float Push = LastWoundKind == GSWound::Blade ? 2200.f : 2800.f;
	float BonePush = 0.f;
	const FName Bone = Skel->FindClosestBone(LastWoundPoint, nullptr, 0.f, true);
	if (!Bone.IsNone())
	{
		BonePush = FMath::Min(Push, Skel->GetBoneMass(Bone) * 400.f);
		if (BonePush > 0.f) Skel->AddImpulseAtLocation(LastWoundDir * BonePush, LastWoundPoint, Bone);
	}
	if (Push - BonePush > 1.f) Skel->AddImpulseAtLocation(LastWoundDir * (Push - BonePush), LastWoundPoint);
}

void AGSCharacter::SpawnDeathPool()
{
	if (!bDead || GetNetMode() == NM_DedicatedServer) return;
	if (GetWorld()->GetTimeSeconds() - LastWoundTime > 8.f) return;   // not a violent death: no blood
	FVector From = GetActorLocation() - FVector(0.f, 0.f, 60.f);
	if (bHumanBody) From = GetMesh()->GetSocketLocation(LastWoundKind == GSWound::Head ? TEXT("head") : TEXT("spine_02"));
	FHitResult Floor;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSDeathPool), true, this);
	if (GetWorld()->LineTraceSingleByChannel(Floor, From + FVector(0.f, 0.f, 30.f), From - FVector(0.f, 0.f, 160.f), ECC_Visibility, Params)
		&& !Cast<APawn>(Floor.GetActor()) && Floor.ImpactNormal.Z > 0.5f)
	{
		AGSShortFX::SpawnBloodPool(GetWorld(), Floor.ImpactPoint, Floor.ImpactNormal, FMath::FRandRange(95.f, 135.f), this);
	}
}

void AGSCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	Super::CalcCamera(DeltaTime, OutResult);   // keeps post-process from the camera component
	if (!bDead || DeathCamStart < 0.f) return;

	const UWorld* World = GetWorld();
	const float T = World->GetTimeSeconds() - DeathCamStart;
	const float Dt = FMath::Clamp(DeltaTime, 0.f, 0.05f);
	constexpr float FallTime = 0.35f;   // phase 1: you drop where you stand
	constexpr float PullTime = 1.6f;    // phase 2: the camera pulls back over the body; phase 3: turn to the killer / orbit
	constexpr float Back = 280.f, Up = 180.f;

	// Look at the ragdoll's hips (or where the old shapes lie)
	const FVector Focus = bHumanBody ? GetMesh()->GetSocketLocation(TEXT("pelvis")) : GetActorLocation() - FVector(0.f, 0.f, 60.f);
	const FVector Facing = FRotator(0.f, DeathEyeRot.Yaw, 0.f).Vector();   // the way you were looking

	const float FallA = FMath::Clamp(T / FallTime, 0.f, 1.f);
	const FVector FallLoc = DeathEyeLoc - FVector(0.f, 0.f, 45.f * FallA * FallA);
	const FRotator FallRot(DeathEyeRot.Pitch - 20.f * FallA, DeathEyeRot.Yaw, 18.f * FallA);

	// Turn toward the killer: his live position if your game can see his pawn, otherwise where he stood when
	// you died (he was too far away to be replicated to you).
	APawn* Killer = DeathCamKiller.Get();
	const AGSCharacter* KillerChar = Cast<AGSCharacter>(Killer);
	bool bLookAtKiller = false;
	FVector KillerLook = FVector::ZeroVector;
	if (Killer)
	{
		bLookAtKiller = !(KillerChar && KillerChar->bDead);
		KillerLook = Killer->GetActorLocation();
	}
	else if (bDeathCamHasKillerLoc)
	{
		bLookAtKiller = true;
		KillerLook = DeathCamKillerLoc;
	}

	FVector Desired = FallLoc;
	if (T >= FallTime)
	{
		const float PullA = FMath::SmoothStep(0.f, 1.f, (T - FallTime) / (PullTime - FallTime));
		FVector Target = Focus - Facing * Back + FVector(0.f, 0.f, Up);
		if (T >= PullTime)
		{
			const float After = T - PullTime;
			if (bLookAtKiller)
			{
				Target.Z += 50.f * FMath::SmoothStep(0.f, 1.f, After / 0.8f);
			}
			else
			{
				// Nobody to look at: slow orbit around the body
				Target = Focus - FRotator(0.f, DeathEyeRot.Yaw + 10.f * After, 0.f).Vector() * Back + FVector(0.f, 0.f, Up);
			}
		}
		Desired = FMath::Lerp(FallLoc, Target, PullA);
	}

	if (!bDeathCamInit)
	{
		bDeathCamInit = true;
		DeathCamPos = Desired;
		DeathCamRot = FallRot;
		DeathCamArm = 1.f;
	}
	DeathCamPos = FMath::VInterpTo(DeathCamPos, Desired, Dt, 10.f);

	// Never through walls, roofs or the ground: sweep from the body out to the camera, snap in at once when
	// something is in the way, ease back out when it is clear again.
	FVector Final = DeathCamPos;
	if (T >= FallTime)
	{
		const FVector Start = Focus + FVector(0.f, 0.f, 30.f);
		const FVector Arm = DeathCamPos - Start;
		const float Len = Arm.Size();
		float Free = 1.f;
		if (Len > 1.f)
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(GSDeathCam), false, this);
			if (Killer) Params.AddIgnoredActor(Killer);
			FHitResult Hit;
			if (World->SweepSingleByChannel(Hit, Start, DeathCamPos, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(12.f), Params)
				&& !Hit.bStartPenetrating)
			{
				Free = FMath::Clamp(Hit.Time - 8.f / Len, 0.f, 1.f);
			}
		}
		DeathCamArm = Free < DeathCamArm ? Free : FMath::FInterpTo(DeathCamArm, Free, Dt, 4.f);
		Final = Start + Arm * DeathCamArm;
	}

	// Rotation: fall forward with a roll, then turn to look at the body; after that turn slowly toward the killer.
	FRotator Rot;
	if (T < FallTime)
	{
		Rot = FallRot;
	}
	else if (T < PullTime)
	{
		const float PullA = FMath::SmoothStep(0.f, 1.f, (T - FallTime) / (PullTime - FallTime));
		Rot = FQuat::Slerp(FQuat(FallRot), FQuat((Focus - Final).Rotation()), PullA).Rotator();
	}
	else
	{
		const FVector LookAt = bLookAtKiller ? KillerLook + FVector(0.f, 0.f, 60.f) : Focus;
		Rot = FMath::RInterpTo(DeathCamRot, (LookAt - Final).Rotation(), Dt, 2.5f);
	}
	DeathCamRot = Rot;

	OutResult.Location = Final;
	OutResult.Rotation = Rot;
	OutResult.FOV = DefaultFOV;   // not zoomed in even if you died looking through a scope
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
	OnRep_Health();   // a listen-server host hears and sees hunger/thirst damage like a client does
	if (Health <= 0.f)
	{
		if (Mods.bPhoenix) { Health = 50.f; OnRep_Health(); Effects.RemoveAll([](const FGSActiveEffect& E) { return E.Id == FxFoniks; }); return; }
		const FString Cause = Hunger <= 0.f ? TEXT("Sult") : Thirst <= 0.f ? TEXT("T\u00F8rst") : TEXT("Overdose");
		Die(nullptr, false, Cause);
	}
}

void AGSCharacter::TickScopeSway(float Dt)
{
	AController* C = GetController();
	if (!C) return;
	const bool bScoped = IsScopedIn() && !bDead;
	// Holding breath (Shift while scoped) steadies the aim for 4 seconds, then you have to breathe and it shakes more
	const bool bHolding = bScoped && bWantsSprint && BreathHeld < 4.f;
	if (bHolding) BreathHeld += Dt;
	else BreathHeld = FMath::Max(0.f, BreathHeld - Dt * (bWantsSprint ? 0.f : 1.5f));
	float Target = 0.f;
	if (bScoped)
	{
		Target = bIsCrouched ? 0.18f : 0.32f;
		if (GetVelocity().Size2D() > 50.f) Target *= 1.8f;
		if (bHolding) Target *= 0.12f;
		else if (BreathHeld >= 4.f) Target *= 1.7f;
	}
	// A shot arm cannot hold the gun still: the sights wander and tremble with any weapon. Holding your breath does not help.
	const float ArmPain = bDead ? 0.f : GetArmPain();
	const float Shake = bAiming ? 0.07f * ArmPain : 0.f;
	if (bAiming) Target += 0.5f * ArmPain;
	ScopeSwayAmp = FMath::FInterpTo(ScopeSwayAmp, Target, Dt, 3.f);
	const float T = GetWorld()->GetTimeSeconds();
	const FVector2D Off(FMath::Sin(T * 0.9f) * ScopeSwayAmp + FMath::Sin(T * 6.3f) * Shake,
		FMath::Sin(T * 1.7f + 0.8f) * ScopeSwayAmp * 0.6f + FMath::Sin(T * 7.9f + 1.3f) * Shake);
	const FVector2D Delta = Off - ScopeSwayPrev;
	ScopeSwayPrev = Off;
	if (!Delta.IsNearlyZero(1e-5f))
	{
		FRotator R = C->GetControlRotation();
		R.Yaw += Delta.X;
		R.Pitch += Delta.Y;
		C->SetControlRotation(R);
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

	// Low health: greyer picture, darker edges and a heartbeat that speeds up the lower you get.
	const float Low = GetLowHealthAlpha();
	ApplyLowHealthPostProcess(Visual, Low);
	if (Low > 0.f && T >= NextHeartbeat)
	{
		GSAudio::Play2D(this, TEXT("Heartbeat"), 0.35f + 0.5f * Low, 1.f);
		LastHeartbeatTime = T;
		NextHeartbeat = T + FMath::Lerp(1.05f, 0.55f, Low);
	}

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

	// Camera animation on top of the view (does not move the aim): lean into strafes, flinch when hit,
	// dip on landing (harder falls dip more), slow breathing that gets heavier when hurt or out of breath.
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	const FVector Vel = GetVelocity();
	const float Right = FVector::DotProduct(Vel, GetActorRightVector()) / FMath::Max(WalkSpeed, 1.f);
	CamLean = FMath::FInterpTo(CamLean, bDead ? 0.f : FMath::Clamp(Right, -1.5f, 1.5f) * 1.4f, Dt, 6.f);

	if (LastDamageTakenTime > FlinchSeenTime + 0.01f)
	{
		FlinchSeenTime = LastDamageTakenTime;
		// A headshot has just started a harder flinch (ClientTookHit): let it play instead of restarting it.
		if (FlinchScale <= 1.f || T - FlinchStart > 0.15f)
		{
			FlinchStart = T;
			FlinchSign = FMath::RandBool() ? 1.f : -1.f;
			FlinchScale = 1.f;
		}
	}
	const float FA = T - FlinchStart;
	const float Flinch = FA < 0.6f ? FMath::Exp(-FA * 7.f) * FMath::Sin(FA * 30.f) * FlinchScale : 0.f;

	const bool bFalling = GetCharacterMovement()->IsFalling();
	if (bFalling) CamFallSpeed = FMath::Max(CamFallSpeed, -Vel.Z);
	if (bCamWasFalling && !bFalling)
	{
		CamDipVel -= FMath::Clamp(CamFallSpeed * 0.25f, 40.f, 260.f);
		GSAudio::PlayAt(this, TEXT("Land"), GetActorLocation() - FVector(0.f, 0.f, 90.f), FMath::Clamp(CamFallSpeed / 900.f, 0.3f, 1.f), 20.f);
		CamFallSpeed = 0.f;
	}
	bCamWasFalling = bFalling;
	CamDipVel += (-CamDip * 180.f - CamDipVel * 18.f) * Dt;
	CamDip += CamDipVel * Dt;

	const float Hurt = 1.f - FMath::Clamp(Health / FMath::Max(MaxHealth, 1.f), 0.f, 1.f);
	const float Tired = 1.f - Stamina / 100.f;
	const float BreathRate = 1.4f + 1.6f * FMath::Max(Hurt, Tired);
	const float BreathAmp = (bAiming ? 0.05f : 0.15f) * (1.f + 2.f * FMath::Max(Hurt, Tired));
	const float Breath = FMath::Sin(T * BreathRate) * BreathAmp;

	// A hurt leg: the view drops and tips on every second step.
	const float LimpAmt = bDead || bFalling ? 0.f : GetLegPain() * FMath::Clamp(Vel.Size2D() / 200.f, 0.f, 1.f);
	LimpPhase = FMath::Frac(LimpPhase + Vel.Size2D() * Dt / 350.f);   // one cycle = two steps
	const float Limp = LimpAmt * FMath::Max(0.f, FMath::Sin(LimpPhase * 2.f * PI));

	Camera->ClearAdditiveOffset();
	if (!bDead)
	{
		const FRotator CamRot(Breath + Flinch * 2.5f, Flinch * FlinchSign * 1.2f, CamLean + Flinch * FlinchSign * 3.f + Limp * 2.2f);
		Camera->AddAdditiveOffset(FTransform(CamRot, FVector(0.f, 0.f, CamDip - Limp * 7.f)), 0.f);
	}
}

void AGSCharacter::ApplyLowHealthPostProcess(int32 Visual, float Low)
{
	FPostProcessSettings& PP = Camera->PostProcessSettings;
	const bool bLow = Low > 0.01f;
	// Drug visuals 2 and 5 set their own saturation: leave those alone. Otherwise switch the override off
	// again explicitly, or the picture would stay grey after you are healed.
	if (Visual != 2 && Visual != 5)
	{
		const float S = 1.f - 0.6f * Low;
		PP.bOverride_ColorSaturation = bLow;
		PP.ColorSaturation = FVector4(S, S, S, 1.f);
	}
	if (Visual == 0)
	{
		PP.bOverride_VignetteIntensity = bLow;
		PP.VignetteIntensity = bLow ? 0.4f + 0.6f * Low : 0.6f;
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

	// Team colours on the body: a cheap guard that only does work when the team changed (it arrives after the
	// body on clients, changes with L in the lobby and when the teams are evened out at match start).
	// A dead body keeps the colours it died in (it loses its player state when the player gets a new body).
	if (!bDead && AppliedTeam != GetTeam()) ApplyTeamLook();

	// Carrying the flag (the player state is replicated to everyone, so all machines agree).
	const AGSPlayerState* FlagPS = bDead ? nullptr : GetPlayerState<AGSPlayerState>();
	const bool bCarriesFlag = FlagPS && FlagPS->CarriedFlagTeam != GSTeam::None;

	// Stamina (server is authoritative, owner predicts)
	const bool bMoving = GetVelocity().Size2D() > 50.f;
	if (bSprinting && bMoving)
	{
		// Running with the pole on your back wears you out faster: the carrier cannot sprint for long.
		Stamina = FMath::Max(0.f, Stamina - 14.f * Mods.StaminaDrain * (bCarriesFlag ? 1.6f : 1.f) * DeltaSeconds);
		if (Stamina <= 0.f) SetSprinting(false);
	}
	else
	{
		const float HungerFactor = (Hunger < 20.f || Thirst < 20.f) ? 0.5f : 1.f;
		Stamina = FMath::Min(100.f, Stamina + 9.f * Mods.StaminaRegen * HungerFactor * DeltaSeconds);
	}

	UCharacterMovementComponent* Move = GetCharacterMovement();
	const float LegPain = bDead ? 0.f : GetLegPain();
	// A badly shot leg cannot run. Server and owner both apply it (bSprinting is predicted by the owner).
	if (bSprinting && LegPain >= 0.5f && (HasAuthority() || IsLocallyControlled())) SetSprinting(false);
	float Speed = bSprinting ? SprintSpeed : (bAiming ? AimSpeed : WalkSpeed);
	if (!UseItemId.IsNone()) Speed = FMath::Min(Speed, AimSpeed);
	const float FlagSpeed = bCarriesFlag ? 0.85f : 1.f;   // carrying the flag slows you a little
	Move->MaxWalkSpeed = Speed * Mods.Speed * GearSpeed * (1.f - 0.45f * LegPain) * FlagSpeed;
	// The same limp while crouched (a slide sets its own speed and puts 200 back when it ends).
	if (!bSliding) Move->MaxWalkSpeedCrouched = 200.f * (1.f - 0.45f * LegPain) * FlagSpeed;
	Move->JumpZVelocity = 430.f * Mods.Jump * (1.f - 0.4f * LegPain);
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

	// Footsteps (every machine that renders): one step per stride, grass sounds soft, hard floors click.
	if (GetNetMode() != NM_DedicatedServer && !bDead && !Move->IsFalling())
	{
		const float Speed2D = GetVelocity().Size2D();
		if (Speed2D > 120.f)
		{
			StepDistance += Speed2D * DeltaSeconds;
			const float Stride = bSprinting ? 230.f : bIsCrouched ? 120.f : 175.f;
			if (StepDistance >= Stride)
			{
				StepDistance = 0.f;
				const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
				FHitResult Hit;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(GSStep), false, this);
				bool bGrass = false;
				if (GetWorld()->LineTraceSingleByChannel(Hit, Feet + FVector(0.f, 0.f, 20.f), Feet - FVector(0.f, 0.f, 60.f), ECC_Visibility, Params))
				{
					if (const UPrimitiveComponent* C = Hit.GetComponent())
					{
						const UMaterialInterface* Mat = C->GetMaterial(0);
						bGrass = Mat && Mat->GetName().Contains(TEXT("Grass"));
					}
				}
				const float Vol = bIsCrouched ? 0.25f : bSprinting ? 1.f : 0.6f;
				GSAudio::PlayAt(this, GSAudio::Variant(bGrass ? TEXT("Step_Grass") : TEXT("Step_Hard"), 4), Feet, Vol, bSprinting ? 30.f : 18.f,
					FMath::FRandRange(0.92f, 1.08f));
			}
		}
		else
		{
			StepDistance = 100.f; // first step comes quickly when you start moving
		}
	}

	// Ghost boost: others only see you flicker.
	if (!IsLocallyControlled() && !bDead)
	{
		const bool bVisible = !Mods.bGhost || FMath::Frac(GetWorld()->GetTimeSeconds() * 2.5f) < 0.12f;
		BodyMesh->SetVisibility(bVisible && !bHumanBody);
		HeadMesh->SetVisibility(bVisible && !bHumanBody);
		if (bHumanBody) GetMesh()->SetVisibility(bVisible);
		const bool bBands = bVisible && bHumanBody && AppliedTeam != GSTeam::None && AppliedTeam != 255 && !ArmbandL->GetAttachSocketName().IsNone();
		ArmbandL->SetVisibility(bBands);
		ArmbandR->SetVisibility(bBands);
		TPWeaponMesh->SetHiddenInGame(!bVisible, true); // hides its attachments with it
		VestMesh->SetVisibility(bVisible && !BodyArmor.IsNone());
		HelmetMesh->SetVisibility(bVisible && !Helmet.IsNone());
		BackpackMesh->SetVisibility(bVisible && !WornBackpack.IsNone());
	}

	// Other players' weapons: held at the shoulder, pointing where they aim (up/down), lowered when
	// sprinting, kicking back when they shoot, bobbing with their steps.
	if (!IsLocallyControlled() && !bDead && GetNetMode() != NM_DedicatedServer)
	{
		const float AimPitch = FRotator::NormalizeAxis(GetBaseAimRotation().Pitch);
		const float Spd = GetVelocity().Size2D();
		TPSprint = FMath::FInterpTo(TPSprint, bSprinting ? 1.f : 0.f, DeltaSeconds, 6.f);
		const float Since = GetWorld()->GetTimeSeconds() - LastRemoteShotTime;
		const float Kick = Since < 0.3f ? FMath::Exp(-Since * 14.f) : 0.f;
		const float Bob = FMath::Sin(GetWorld()->GetTimeSeconds() * (bSprinting ? 13.f : 8.5f)) * FMath::Min(Spd / 600.f, 1.f);
		TPAim = FMath::FInterpTo(TPAim, bAiming && !bSprinting && !bReloading ? 1.f : 0.f, DeltaSeconds, 9.f);
		const FVector Shoulder(5.f + 4.f * TPAim, 18.f - 9.f * TPAim, (bIsCrouched ? 10.f : 48.f) + 9.f * TPAim);
		FRotator Aim(FMath::Lerp(AimPitch, -35.f, TPSprint) + 8.f * Kick, FMath::Lerp(0.f, -40.f, TPSprint), 0.f);
		// Reloading seen by others: gun canted and lowered toward the chest, with a tug when the mag seats
		TPReload = FMath::FInterpTo(TPReload, bReloading ? 1.f : 0.f, DeltaSeconds, 8.f);
		if (TPReload > 0.01f)
		{
			const float Tug = FMath::Sin(GetWorld()->GetTimeSeconds() * 9.f);
			Aim.Pitch -= TPReload * (18.f + 3.f * Tug);
			Aim.Yaw -= TPReload * 25.f;
			Aim.Roll += TPReload * 35.f;
		}
		// Melee swing seen by others: raise to the right, sweep across in front of the body
		const float MSince = GetWorld()->GetTimeSeconds() - LastMeleeTime;
		float Reach = 0.f;
		if (MSince < 0.45f)
		{
			const float Up = FMath::Clamp(MSince / 0.08f, 0.f, 1.f);
			const float Sweep = FMath::Clamp((MSince - 0.08f) / 0.14f, 0.f, 1.f);
			const float Back = FMath::Clamp((MSince - 0.22f) / 0.23f, 0.f, 1.f);
			const float W = (1.f - Back);
			Aim.Pitch += W * (25.f * Up - 45.f * Sweep);
			Aim.Yaw += W * (50.f * Up - 110.f * Sweep);
			Aim.Roll += W * 30.f * Sweep;
			Reach = W * 12.f * FMath::Sin(Sweep * PI);
		}
		TPWeaponMesh->SetRelativeLocation(Shoulder + Aim.RotateVector(FVector(30.f - 6.f * Kick + Reach, 0.f, -6.f)) + FVector(0.f, 0.f, Bob * 2.f));
		TPWeaponMesh->SetRelativeRotation(Aim);
	}

	if (IsLocallyControlled())
	{
		UpdateFocusPickup();
		UpdateLocalVisuals(DeltaSeconds);
		if (SelectedItem >= Backpack.Num()) SelectedItem = FMath::Max(0, Backpack.Num() - 1);
		if (bBinoculars && !HasItemKind(KindZoom)) bBinoculars = false;
		if (bNightVisionGoggles && !HasItemKind(KindNightVision)) bNightVisionGoggles = false;

		TickRecoil(DeltaSeconds);
		TickScopeSway(DeltaSeconds);
		// Dead: the first-person arms, gun and item stay hidden (the client can still control the body for a few frames)
		if (!bDead) UpdateWeaponAnimation(DeltaSeconds);
	}

#if !UE_BUILD_SHIPPING
	if (CVarShowHitZones.GetValueOnGameThread() != 0 && !bDead && GetNetMode() != NM_DedicatedServer)
	{
		static const FColor ZoneColors[6] = { FColor::White, FColor::Red, FColor::Orange, FColor::Yellow, FColor::Cyan, FColor::Green };
		for (int32 I = 0; I < 7; ++I)
		{
			if (const UBoxComponent* Z = HitZones[I])
			{
				DrawDebugBox(GetWorld(), Z->GetComponentLocation(), Z->GetScaledBoxExtent(), Z->GetComponentQuat(),
					ZoneColors[GSZoneBoxes[I].Zone], false, -1.f, 0, 0.6f);
			}
		}
	}
#endif
}
