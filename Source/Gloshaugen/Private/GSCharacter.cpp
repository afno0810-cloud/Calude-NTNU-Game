#include "GSCharacter.h"
#include "GSPlayerController.h"
#include "GSGameMode.h"
#include "GSEffects.h"
#include "GSUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylF(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphF(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	FPWeaponMesh->SetStaticMesh(CubeF.Object);
	TPWeaponMesh->SetStaticMesh(CubeF.Object);
	BodyMesh->SetStaticMesh(CylF.Object);
	HeadMesh->SetStaticMesh(SphF.Object);
}

void AGSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSCharacter, Health);
	DOREPLIFETIME(AGSCharacter, MaxHealth);
	DOREPLIFETIME_CONDITION(AGSCharacter, Stamina, COND_OwnerOnly);
	DOREPLIFETIME(AGSCharacter, Weapons);
	DOREPLIFETIME(AGSCharacter, CurrentSlot);
	DOREPLIFETIME(AGSCharacter, bReloading);
	DOREPLIFETIME_CONDITION(AGSCharacter, bSprinting, COND_SkipOwner);
	DOREPLIFETIME(AGSCharacter, bDead);
	DOREPLIFETIME_CONDITION(AGSCharacter, bSliding, COND_SkipOwner);
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
		GiveWeapon(EGSWeaponId::Pistol);
	}
	ApplyWeaponVisuals();
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
	EIC->BindAction(PC->NextWeaponAction, ETriggerEvent::Started, this, &AGSCharacter::NextWeapon);
}

void AGSCharacter::Move(const FInputActionValue& Value)
{
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
	const FVector2D V = Value.Get<FVector2D>();
	const float Sens = bAiming ? 0.55f : 1.f;
	AddControllerYawInput(V.X * Sens);
	AddControllerPitchInput(-V.Y * Sens);
}

void AGSCharacter::StartJump()
{
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
	if (bNew) StopFire();
	if (!HasAuthority()) ServerSetSprinting(bNew);
}

void AGSCharacter::ServerSetSprinting_Implementation(bool bNew)
{
	bSprinting = bNew && Stamina > 1.f && !bIsCrouched;
}

void AGSCharacter::ToggleCrouch()
{
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
	bAiming = true;
	SetSprinting(false);
}

void AGSCharacter::StopAim()
{
	bAiming = false;
	if (bWantsSprint) StartSprint();
}

void AGSCharacter::SelectSlot1() { if (Weapons.IsValidIndex(0)) { CurrentSlot = 0; ApplyWeaponVisuals(); ServerSelectSlot(0); } }
void AGSCharacter::SelectSlot2() { if (Weapons.IsValidIndex(1)) { CurrentSlot = 1; ApplyWeaponVisuals(); ServerSelectSlot(1); } }

void AGSCharacter::NextWeapon()
{
	if (Weapons.Num() < 2) return;
	const int32 Next = (CurrentSlot + 1) % Weapons.Num();
	CurrentSlot = Next;
	ApplyWeaponVisuals();
	ServerSelectSlot(Next);
}

void AGSCharacter::ServerSelectSlot_Implementation(int32 Slot)
{
	if (!Weapons.IsValidIndex(Slot) || Slot == CurrentSlot) return;
	CurrentSlot = Slot;
	bReloading = false;
	GetWorldTimerManager().ClearTimer(ReloadTimer);
	ApplyWeaponVisuals();
}

// ---------------------------------------------------------------- Weapons

bool AGSCharacter::GiveWeapon(EGSWeaponId Id)
{
	if (!HasAuthority() || Id == EGSWeaponId::None) return false;
	const FGSWeaponStats& S = GSWeapons::Get(Id);

	for (FGSWeaponSlot& Slot : Weapons)
	{
		if (Slot.Id == Id)
		{
			if (Slot.Reserve >= S.MaxReserve) return false;
			Slot.Reserve = FMath::Min(Slot.Reserve + S.MagSize * 2, S.MaxReserve);
			return true;
		}
	}

	FGSWeaponSlot New;
	New.Id = Id;
	New.Ammo = S.MagSize;
	New.Reserve = S.StartReserve;
	const int32 Index = Weapons.Add(New);
	CurrentSlot = Index; // switch to what you just picked up
	bReloading = false;
	ApplyWeaponVisuals();
	return true;
}

float AGSCharacter::GetCurrentSpread() const
{
	const FGSWeaponSlot* W = GetCurrentWeapon();
	if (!W) return 0.f;
	const FGSWeaponStats& S = GSWeapons::Get(W->Id);
	float Spread = bAiming ? S.AimSpread : S.HipSpread;
	const float Speed = GetVelocity().Size2D();
	Spread += FMath::GetMappedRangeValueClamped(FVector2D(0.f, SprintSpeed), FVector2D(0.f, bAiming ? 1.f : 3.f), Speed);
	if (GetCharacterMovement()->IsFalling()) Spread += 4.f;
	if (bIsCrouched) Spread *= 0.7f;
	Spread += CurrentRecoil * 0.4f;
	return Spread;
}

void AGSCharacter::StartFire()
{
	bWantsFire = true;
	FireOnce();
	const FGSWeaponSlot* W = GetCurrentWeapon();
	if (W && GSWeapons::Get(W->Id).bAutomatic)
	{
		GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AGSCharacter::FireOnce, GSWeapons::Get(W->Id).FireInterval, true);
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
	const FGSWeaponStats& S = GSWeapons::Get(W->Id);

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFireTime < S.FireInterval * 0.95f) return;
	if (W->Ammo <= 0)
	{
		StopFire();
		Reload();
		return;
	}
	if (bSprinting) SetSprinting(false);
	LastFireTime = Now;

	const FVector Start = Camera->GetComponentLocation();
	const float SpreadRad = FMath::DegreesToRadians(GetCurrentSpread() * 0.5f);
	const FVector Dir = FMath::VRandCone(Camera->GetForwardVector(), SpreadRad);

	// Local, instant feedback (server decides what actually got hit).
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSFireLocal), true, this);
	const FVector End = Start + Dir * S.Range;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	const FVector MuzzleStart = FPWeaponMesh->GetComponentLocation() + Camera->GetForwardVector() * (S.MeshScale.X * 50.f);
	SpawnTracer(MuzzleStart, bHit ? Hit.ImpactPoint : End);
	if (bHit && !Cast<AGSCharacter>(Hit.GetActor()))
	{
		AGSShortFX::SpawnImpact(GetWorld(), Hit.ImpactPoint, Hit.ImpactNormal);
	}

	if (!HasAuthority()) W->Ammo--; // prediction, corrected by replication

	// Recoil
	CurrentRecoil += S.RecoilPitch;
	const float AimFactor = bAiming ? 0.6f : 1.f;
	AddControllerPitchInput(-S.RecoilPitch * AimFactor);
	AddControllerYawInput(FMath::FRandRange(-0.25f, 0.25f) * S.RecoilPitch * AimFactor);

	ServerFire(Start, Dir, 0);

	if (!S.bAutomatic) GetWorldTimerManager().ClearTimer(AutoFireTimer);
}

void AGSCharacter::ServerFire_Implementation(FVector_NetQuantize Start, FVector_NetQuantizeNormal Dir, int32 Seed)
{
	if (bDead || bReloading || !Weapons.IsValidIndex(CurrentSlot)) return;
	FGSWeaponSlot& W = Weapons[CurrentSlot];
	const FGSWeaponStats& S = GSWeapons::Get(W.Id);
	const float Now = GetWorld()->GetTimeSeconds();
	if (W.Ammo <= 0 || Now - ServerLastFireTime < S.FireInterval * 0.7f) return;
	// Basic anti-cheat: the shot must start close to where we really are.
	if (FVector::DistSquared(Start, Camera->GetComponentLocation()) > FMath::Square(250.f)) return;

	ServerLastFireTime = Now;
	W.Ammo--;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSFireServer), true, this);
	const FVector End = FVector(Start) + FVector(Dir).GetSafeNormal() * S.Range;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	if (bHit)
	{
		if (AGSCharacter* Victim = Cast<AGSCharacter>(Hit.GetActor()))
		{
			FPointDamageEvent Event(S.Damage, Hit, Dir, nullptr);
			Victim->TakeDamage(S.Damage, Event, GetController(), this);
		}
	}
	MulticastShotFX(Start, bHit ? FVector(Hit.ImpactPoint) : End);
}

void AGSCharacter::MulticastShotFX_Implementation(FVector_NetQuantize Start, FVector_NetQuantize End)
{
	if (IsLocallyControlled()) return; // shooter already drew it
	const FVector From = TPWeaponMesh->GetComponentLocation();
	SpawnTracer(From, End);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSFxTrace), true, this);
	const FVector Dir = (FVector(End) - FVector(Start)).GetSafeNormal();
	if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(End) - Dir * 20.f, FVector(End) + Dir * 20.f, ECC_Visibility, Params)
		&& !Cast<AGSCharacter>(Hit.GetActor()))
	{
		AGSShortFX::SpawnImpact(GetWorld(), Hit.ImpactPoint, Hit.ImpactNormal);
	}
}

void AGSCharacter::SpawnTracer(const FVector& Start, const FVector& End) const
{
	AGSShortFX::SpawnTracer(GetWorld(), Start, End);
}

void AGSCharacter::Reload()
{
	const FGSWeaponSlot* W = GetCurrentWeapon();
	if (!W || bReloading || bDead) return;
	if (W->Ammo >= GSWeapons::Get(W->Id).MagSize || W->Reserve <= 0) return;
	StopFire();
	ServerReload();
}

void AGSCharacter::ServerReload_Implementation()
{
	if (!Weapons.IsValidIndex(CurrentSlot) || bReloading || bDead) return;
	const FGSWeaponSlot& W = Weapons[CurrentSlot];
	const FGSWeaponStats& S = GSWeapons::Get(W.Id);
	if (W.Ammo >= S.MagSize || W.Reserve <= 0) return;
	bReloading = true;
	GetWorldTimerManager().SetTimer(ReloadTimer, this, &AGSCharacter::FinishReload, S.ReloadTime, false);
}

void AGSCharacter::FinishReload()
{
	bReloading = false;
	if (!Weapons.IsValidIndex(CurrentSlot)) return;
	FGSWeaponSlot& W = Weapons[CurrentSlot];
	const int32 Need = GSWeapons::Get(W.Id).MagSize - W.Ammo;
	const int32 Take = FMath::Min(Need, W.Reserve);
	W.Ammo += Take;
	W.Reserve -= Take;
}

void AGSCharacter::ClientHitMarker_Implementation(bool bKill)
{
	LastHitMarkerTime = GetWorld()->GetTimeSeconds();
}

void AGSCharacter::ApplyWeaponVisuals()
{
	const FGSWeaponSlot* W = GetCurrentWeapon();
	const bool bHas = W && W->Id != EGSWeaponId::None && !bDead;
	FPWeaponMesh->SetVisibility(bHas);
	TPWeaponMesh->SetVisibility(bHas);
	if (!bHas) return;
	const FGSWeaponStats& S = GSWeapons::Get(W->Id);
	FPWeaponMesh->SetRelativeScale3D(S.MeshScale);
	TPWeaponMesh->SetRelativeScale3D(S.MeshScale);
	GSUtils::SetColor(FPWeaponMesh, S.Color);
	GSUtils::SetColor(TPWeaponMesh, S.Color);
}

void AGSCharacter::OnRep_Weapons() { ApplyWeaponVisuals(); }
void AGSCharacter::OnRep_CurrentSlot() { ApplyWeaponVisuals(); }

// ---------------------------------------------------------------- Health

float AGSCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || bDead) return 0.f;

	bool bHeadshot = false;
	float Damage = DamageAmount;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PE = static_cast<const FPointDamageEvent&>(DamageEvent);
		if (PE.HitInfo.GetComponent() == HeadMesh)
		{
			bHeadshot = true;
			if (const AGSCharacter* Shooter = Cast<AGSCharacter>(DamageCauser))
			{
				if (const FGSWeaponSlot* W = Shooter->GetCurrentWeapon())
				{
					Damage *= GSWeapons::Get(W->Id).HeadshotMultiplier;
				}
			}
		}
	}

	Health = FMath::Max(0.f, Health - Damage);
	LastDamageTakenTime = GetWorld()->GetTimeSeconds();
	OnRep_Health();

	const bool bKilled = Health <= 0.f;
	if (AGSCharacter* Shooter = Cast<AGSCharacter>(DamageCauser))
	{
		Shooter->ClientHitMarker(bKilled);
	}
	if (bKilled)
	{
		Die(EventInstigator, bHeadshot);
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

void AGSCharacter::Die(AController* Killer, bool bHeadshot)
{
	if (bDead) return;
	bDead = true;
	StopFire();

	FString WeaponName = TEXT("?");
	if (Killer)
	{
		if (const AGSCharacter* KC = Cast<AGSCharacter>(Killer->GetPawn()))
		{
			if (const FGSWeaponSlot* W = KC->GetCurrentWeapon()) WeaponName = GSWeapons::Get(W->Id).Name;
		}
	}
	if (AGSGameMode* GM = GetWorld()->GetAuthGameMode<AGSGameMode>())
	{
		GM->OnPlayerKilled(Killer, GetController(), WeaponName, bHeadshot);
	}

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
}

// ---------------------------------------------------------------- Tick

void AGSCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Stamina (server is authoritative, owner predicts)
	const bool bMoving = GetVelocity().Size2D() > 50.f;
	if (bSprinting && bMoving)
	{
		Stamina = FMath::Max(0.f, Stamina - 14.f * DeltaSeconds);
		if (Stamina <= 0.f) SetSprinting(false);
	}
	else
	{
		Stamina = FMath::Min(100.f, Stamina + 9.f * DeltaSeconds);
	}

	GetCharacterMovement()->MaxWalkSpeed = bSprinting ? SprintSpeed : (bAiming ? AimSpeed : WalkSpeed);

	CurrentRecoil = FMath::FInterpTo(CurrentRecoil, 0.f, DeltaSeconds, 6.f);

	if (IsLocallyControlled())
	{
		float TargetFOV = DefaultFOV;
		FVector WeaponPos(38.f, 16.f, -16.f);
		if (const FGSWeaponSlot* W = GetCurrentWeapon())
		{
			if (bAiming)
			{
				TargetFOV = GSWeapons::Get(W->Id).AimFOV;
				WeaponPos = FVector(32.f, 0.f, -9.f);
			}
		}
		if (bSprinting) TargetFOV += 6.f;
		if (bSliding) TargetFOV += 10.f;
		if (bReloading) WeaponPos += FVector(-8.f, 0.f, -14.f);
		Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, TargetFOV, DeltaSeconds, 12.f));
		FPWeaponMesh->SetRelativeLocation(FMath::VInterpTo(FPWeaponMesh->GetRelativeLocation(), WeaponPos, DeltaSeconds, 14.f));
	}
}
