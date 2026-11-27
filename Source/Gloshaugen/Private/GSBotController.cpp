#include "GSBotController.h"
#include "GSBots.h"
#include "GSBotTeam.h"
#include "GSNavSetup.h"
#include "GSCharacter.h"
#include "GSPlayerState.h"
#include "GSPickup.h"
#include "GSItems.h"
#include "GSBallistics.h"
#include "GSMapState.h"
#include "GSDoor.h"
#include "GSFlag.h"
#include "GSHelgasetr.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogGSBot, Log, All);

namespace GSBotImpl
{
	/** How much a bot wants this weapon in its hands (0 = never by choice). */
	int32 WeaponRank(const FGSWeaponStats& S)
	{
		if (S.Mode == EGSFireMode::Melee || S.Mode == EGSFireMode::Projectile) return 0;
		switch (S.Class)
		{
		case EGSWeaponClass::Rifle: case EGSWeaponClass::LMG: return 5;
		case EGSWeaponClass::SMG: return 4;
		case EGSWeaponClass::Shotgun: case EGSWeaponClass::Sniper: return 3;
		case EGSWeaponClass::Pistol: return 2;
		default: return 0;
		}
	}

	float HalfHeight(const AGSCharacter* C) { return C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(); }
}

AGSBotController::AGSBotController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	bWantsPlayerState = true;
	bSetControlRotationFromPawnOrientation = false;
	PrimaryActorTick.bCanEverTick = true;
}

const FGSBotSkill& AGSBotController::Sk() const { return GSBots::Skill(Skill); }

uint8 AGSBotController::GetTeamId() const
{
	const AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	return PS ? PS->Team : 0;
}

AGSCharacter* AGSBotController::GetBody() const { return Cast<AGSCharacter>(GetPawn()); }

FVector AGSBotController::EyeLoc() const
{
	const AGSCharacter* C = GetBody();
	return C ? C->Camera->GetComponentLocation() : FVector::ZeroVector;
}

void AGSBotController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ClearFocus(EAIFocusPriority::Gameplay);   // we turn the view ourselves
	AGSCharacter* C = GetBody();
	if (!C) return;
	C->GetCharacterMovement()->bUseRVOAvoidance = false;   // the crowd avoids the other bots
	SetControlRotation(C->GetActorRotation());
	const float Now = GetWorld()->GetTimeSeconds();
	NextSightAt = Now + (BotIndex % 5) * 0.04f;
	NextDecideAt = Now + (BotIndex % 4) * 0.0625f;
	NextAmmoLookAt = Now + 1.f + (BotIndex % 7) * 0.1f;
	Memory.Reset();
	Target = nullptr;
	LastHealth = C->Health;
	LastAmmo = -1;
	LastWeapon = NAME_None;
	RecoilOffset = 0.f;
	bTriggerDown = bAimDown = bSprintOn = bCrouchOn = false;
	bHasGoal = bHold = false;
	StuckSince = -1.f;
	SidestepUntil = -100.f;
	LookLostUntil = -100.f;
	AmmoPickup = nullptr;
	NewTrip();
	SelectBestWeapon();
}

void AGSBotController::OnUnPossess()
{
	Super::OnUnPossess();
	bTriggerDown = bAimDown = bSprintOn = bCrouchOn = false;
	bHasGoal = bHold = false;
	Memory.Reset();
	Target = nullptr;
}

void AGSBotController::Destroyed()
{
	if (UGSBotsSubsystem* S = UGSBotsSubsystem::Get(this))
	{
		if (AGSBotTeam* T = S->GetTeam(GetTeamId())) T->Unregister(this);
	}
	Super::Destroyed();
}

void AGSBotController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	// Keep our own control rotation (with pitch); only turn the body's yaw to it.
	APawn* P = GetPawn();
	if (P && bUpdatePawn) P->FaceRotation(GetControlRotation(), DeltaTime);
}

void AGSBotController::SetRole(EGSBotRole NewRole)
{
	if (NewRole == Role) return;
	if (Role == EGSBotRole::Defend) SetCrouch(false);
	if (NewRole == EGSBotRole::CarryHome) CarryAvoidVia = ViaIndex;   // home another way than it came
	Role = NewRole;
	DefIndex = -1;
	NewTrip();
	bHasGoal = false;
}

void AGSBotController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority()) return;
	AGSCharacter* C = GetBody();
	if (!C || C->bDead) return;
	if (!GSBots::IsThinking())
	{
		ReleaseTrigger();
		Halt();
		return;
	}
	const double T0 = FPlatformTime::Seconds();
	{
		SCOPE_CYCLE_COUNTER(STAT_GSBotThink);
		Think(DeltaSeconds, GetWorld()->GetTimeSeconds());
	}
	if (UGSBotsSubsystem* S = UGSBotsSubsystem::Get(this)) S->AddThinkTime(FPlatformTime::Seconds() - T0);
}

void AGSBotController::Think(float Dt, float Now)
{
	AGSCharacter* C = GetBody();
	if (JumpReleaseAt > 0.f && Now >= JumpReleaseAt) { C->StopJump(); JumpReleaseAt = -1.f; }
	if (C->Health < LastHealth - 0.5f) HurtAt = Now;
	LastHealth = C->Health;

	// Each part timed: a single think over 5 ms is logged with its slowest part (at most every 10 s per bot).
	double Part[4] = { 0.0, 0.0, 0.0, 0.0 };
	double T = FPlatformTime::Seconds();
	if (Now >= NextSightAt)
	{
		NextSightAt = Now + 0.2f;
		UpdateSight(Now);
		PickTarget(Now);
	}
	Part[0] = FPlatformTime::Seconds() - T; T += Part[0];
	if (Now >= NextDecideAt)
	{
		NextDecideAt = Now + 0.25f;
		Decide(Now);
		CheckStuck(Now);
	}
	Part[1] = FPlatformTime::Seconds() - T; T += Part[1];
	UpdateAim(Dt, Now);
	Part[2] = FPlatformTime::Seconds() - T; T += Part[2];
	UpdateFire(Now);
	Part[3] = FPlatformTime::Seconds() - T;
	const double Sum = Part[0] + Part[1] + Part[2] + Part[3];
	if (Sum > 0.005 && Now >= NextSpikeLogAt)
	{
		NextSpikeLogAt = Now + 10.f;
		static const TCHAR* Names[4] = { TEXT("syn"), TEXT("beslutning/sti"), TEXT("sikte"), TEXT("skudd/skuddlinje") };
		int32 Worst = 0;
		for (int32 I = 1; I < 4; ++I) if (Part[I] > Part[Worst]) Worst = I;
		const AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
		UE_LOG(LogGSBot, Log, TEXT("Bot %s: lang tenking %.2f ms (mest %s %.2f ms)"), PS ? *PS->GetPlayerName() : TEXT("?"), Sum * 1000.0, Names[Worst], Part[Worst] * 1000.0);
	}
#if ENABLE_DRAW_DEBUG
	if (GSBots::IsDebug()) DrawDebug(Now);
#endif
}

// ---------------------------------------------------------------------------------------------------------------------
// Sight: only what the bot really sees

void AGSBotController::UpdateSight(float Now)
{
	AGSCharacter* C = GetBody();
	UWorld* W = GetWorld();
	UGSBotsSubsystem* Sub = UGSBotsSubsystem::Get(this);
	const FGSBotSkill& S = Sk();
	const uint8 Mine = GetTeamId();
	const FVector Eye = EyeLoc();
	const FVector Fwd = GetControlRotation().Vector();
	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(S.ViewFovDeg * 0.5f));
	const float Range = S.SightM * 100.f;

	TArray<AGSCharacter*> Enemies;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBotSight), false, C);
	for (TActorIterator<AGSCharacter> It(W); It; ++It)
	{
		AGSCharacter* E = *It;
		if (E == C) continue;
		const uint8 T = E->GetTeam();
		if (T == Mine) Params.AddIgnoredActor(E);   // team mates never block the view
		else if (T != GSTeam::None && !E->bDead) Enemies.Add(E);
	}

	for (auto It = Memory.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || It.Key()->bDead) It.RemoveCurrent();
	}

	for (AGSCharacter* E : Enemies)
	{
		FSeen& M = Memory.FindOrAdd(E);
		const FVector To = E->GetActorLocation() - Eye;
		const float Dist = To.Size();
		const bool bInView = Dist <= Range && (Dist < 300.f || FVector::DotProduct(To / FMath::Max(Dist, 1.f), Fwd) >= CosHalf);
		if (!bInView) { M.bVisible = false; continue; }
		if (Sub && !Sub->TakeSightTraces(3)) continue;   // budget spent this frame: keep what we knew, try next time

		const float H = GSBotImpl::HalfHeight(E);
		const FVector Base = E->GetActorLocation();
		const FVector Pts[3] = { Base + FVector(0.f, 0.f, H * 0.82f), Base + FVector(0.f, 0.f, H * 0.45f), Base };
		uint8 Mask = 0;
		for (int32 I = 0; I < 3; ++I)
		{
			FHitResult Hit;
			if (GSNavSetup::ClearLine(W, Eye, Pts[I], Params, false, ECC_Visibility, true, &Hit) || Hit.GetActor() == E) Mask |= (1 << I);
		}
		const bool bVis = Mask != 0;
		if (bVis)
		{
			if (!M.bVisible && Now - M.LastSeen > 2.f)
			{
				// A new sighting: the reaction time starts now.
				M.FirstSeen = Now;
				M.ReactAt = Now + FMath::FRandRange(S.ReactMin, S.ReactMax);
			}
			const FVector Vel = E->GetVelocity();
			if (Target.Get() == E && (Vel - M.LastVel).Size() > 300.f)
			{
				// The target changed direction sharply: the aim has to settle again (half the first error).
				const float Cur = ErrorStart * FMath::Pow(0.5f, (Now - ErrorAt) / FMath::Max(S.AimSettleHalfLife, 0.05f));
				ErrorStart = FMath::Max(Cur, S.AimErrorDeg * 0.5f);
				ErrorAt = Now;
			}
			M.LastVel = Vel;
			M.LastLoc = Base;
			M.LastSeen = Now;
			M.FreeMask = Mask;
			LastEnemySeenAt = Now;
		}
		M.bVisible = bVis;
	}
}

void AGSBotController::PickTarget(float Now)
{
	const FGSBotSkill& S = Sk();
	const FVector Eye = EyeLoc();
	AGSCharacter* Best = nullptr;
	float BestScore = 0.f;
	for (const auto& Pair : Memory)
	{
		AGSCharacter* E = Pair.Key.Get();
		if (!E || !Pair.Value.bVisible || E->bDead) continue;
		const FVector To = Eye - E->GetActorLocation();
		float Score = 1.f / FMath::Max(To.Size(), 100.f);
		if (FVector::DotProduct(E->GetBaseAimRotation().Vector(), To.GetSafeNormal()) > 0.9f) Score *= 1.5f;   // looks at me
		const AGSPlayerState* PS = E->GetPlayerState<AGSPlayerState>();
		if (PS && PS->CarriedFlagTeam != 0) Score *= 2.f;                                                    // has a flag
		if (E == Target.Get()) Score *= 1.3f;                                                                 // stay on it
		if (Score > BestScore) { BestScore = Score; Best = E; }
	}
	if (Best != Target.Get())
	{
		if (!Best && Target.IsValid())
		{
			// Lost it: look where it was for 2 s, but never shoot at a remembered place.
			if (const FSeen* M = Memory.Find(Target)) { LookLostAt = M->LastLoc + FVector(0.f, 0.f, 60.f); LookLostUntil = Now + 2.f; }
		}
		Target = Best;
		TargetSince = Now;
		if (Best)
		{
			// A fresh aim error in a random direction, halved every AimSettleHalfLife.
			const float A = FMath::FRandRange(0.f, 2.f * PI);
			ErrorDir = FRotator(FMath::Sin(A), FMath::Cos(A), 0.f);
			ErrorStart = S.AimErrorDeg;
			ErrorAt = Now;
			bHeadAim = FMath::FRand() < S.HeadChance;
			LeadFactor = 1.f - S.LeadError * FMath::FRand();
		}
	}
}

FVector AGSBotController::AimPointFor(AGSCharacter* T, const FSeen& S, bool bHead) const
{
	const float H = GSBotImpl::HalfHeight(T);
	const FVector Base = T->GetActorLocation();
	float Z = H * 0.45f;                                   // chest
	if (bHead || S.FreeMask == 1) Z = H * 0.82f;            // the head (always when only the head shows)
	else if (!(S.FreeMask & 2) && (S.FreeMask & 4)) Z = 0.f; // only the hip shows
	FVector P = Base + FVector(0.f, 0.f, Z);
	// Lead a moving target by the bullet's flight time (with this bot's lead error).
	const AGSCharacter* C = GetBody();
	if (const FGSWeaponSlot* W = C ? C->GetCurrentWeapon() : nullptr)
	{
		const FGSWeaponStats St = C->GetStats(*W);
		const float DistM = FVector::Dist(EyeLoc(), P) / 100.f;
		const float Flight = St.Mode == EGSFireMode::Projectile && St.ProjSpeed > 0.f
			? DistM * 100.f / St.ProjSpeed : GSBallistics::TimeTo(GSBallistics::ForWeapon(St), DistM);
		P += T->GetVelocity() * Flight * LeadFactor;
	}
	return P;
}

// ---------------------------------------------------------------------------------------------------------------------
// Aim and fire

void AGSBotController::UpdateAim(float Dt, float Now)
{
	AGSCharacter* C = GetBody();
	const FGSBotSkill& S = Sk();
	const FVector Eye = EyeLoc();
	const FRotator Cur = GetControlRotation();
	FRotator Desired = Cur;
	float Rate = S.TurnDegPerSec;
	bDesiredOnTarget = false;

	const FSeen* M = Target.IsValid() ? Memory.Find(Target) : nullptr;
	if (M && M->bVisible)
	{
		Desired = (AimPointFor(Target.Get(), *M, bHeadAim) - Eye).Rotation();
		const float Err = ErrorStart * FMath::Pow(0.5f, (Now - ErrorAt) / FMath::Max(S.AimSettleHalfLife, 0.05f));
		Desired.Pitch += ErrorDir.Pitch * Err + RecoilOffset;
		Desired.Yaw += ErrorDir.Yaw * Err;
		bDesiredOnTarget = true;
	}
	else if (Now < LookLostUntil)
	{
		Desired = (LookLostAt - Eye).Rotation();
	}
	else
	{
		Rate *= 0.5f;   // calm when nobody is there
		FVector LookAt = FVector::ZeroVector;
		bool bHave = false;
		if (bHold && Role == EGSBotRole::Defend)
		{
			const TArray<FGSDefSpot>& Spots = GSNavSetup::DefenseSpots(GetWorld(), GetTeamId());
			if (Spots.IsValidIndex(DefIndex)) { LookAt = Spots[DefIndex].LookAt; bHave = true; }
		}
		if (!bHave)
		{
			// Along the path: the first point more than 2 m ahead, at eye height over the floor.
			const UPathFollowingComponent* PF = GetPathFollowingComponent();
			const FNavPathSharedPtr Path = PF ? PF->GetPath() : nullptr;
			if (Path.IsValid() && PF->GetStatus() == EPathFollowingStatus::Moving)
			{
				const TArray<FNavPathPoint>& Pts = Path->GetPathPoints();
				for (int32 I = FMath::Max<int32>(PF->GetNextPathIndex(), 0); I < Pts.Num(); ++I)
				{
					if (FVector::Dist2D(Pts[I].Location, C->GetActorLocation()) > 200.f || I == Pts.Num() - 1)
					{
						LookAt = Pts[I].Location + FVector(0.f, 0.f, 150.f);
						bHave = true;
						break;
					}
				}
			}
		}
		if (bHave) Desired = (LookAt - Eye).Rotation();
		else Desired.Pitch = 0.f;
		// Calm looks to the sides now and then.
		if (Now >= NextGlanceAt)
		{
			NextGlanceAt = Now + FMath::FRandRange(2.f, 4.f);
			GlanceYaw = GlanceYaw != 0.f ? 0.f : FMath::FRandRange(-25.f, 25.f);
		}
		Desired.Yaw += GlanceYaw;
	}

	// The part of the recoil the bot did not hold down fades once it stops shooting.
	if (Now - LastShotAt > 0.35f) RecoilOffset = FMath::FInterpTo(RecoilOffset, 0.f, Dt, 4.f);

	Desired = Desired.GetNormalized();
	FRotator New = FMath::RInterpConstantTo(Cur.GetNormalized(), Desired, Dt, Rate);
	New.Pitch = FMath::Clamp(FRotator::NormalizeAxis(New.Pitch), -80.f, 80.f);
	New.Roll = 0.f;
	SetControlRotation(New);
	LastDesired = Desired;
}

bool AGSBotController::LineOfFire(const FVector& To, AGSCharacter* T) const
{
	const AGSCharacter* C = GetBody();
	const FVector Eye = EyeLoc();
	const uint8 Mine = GetTeamId();
	// A team mate near the line (closer than the target): never shoot.
	const float TDist = FVector::Dist(Eye, To);
	for (TActorIterator<AGSCharacter> It(GetWorld()); It; ++It)
	{
		const AGSCharacter* O = *It;
		if (O == C || O->bDead || O->GetTeam() != Mine) continue;
		const FVector P = O->GetActorLocation();
		if (FVector::Dist(Eye, P) > TDist) continue;
		const float H = GSBotImpl::HalfHeight(O);
		const FVector Closest = FMath::ClosestPointOnSegment(P, Eye, To);
		if (FMath::Abs(Closest.Z - P.Z) < H + 10.f && FVector::Dist2D(Closest, P) < 60.f) return false;
	}
	// The bullet's channel, glass NOT looked through: solid glass, a wall or a team mate first = no shot.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBotFireLine), false, C);
	const FVector End = To + (To - Eye).GetSafeNormal() * 40.f;
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Eye, End, ECC_Visibility, Params)) return true;
	const AGSCharacter* HitChar = Cast<AGSCharacter>(Hit.GetActor());
	return HitChar && (HitChar == T || (HitChar->GetTeam() != Mine && HitChar->GetTeam() != GSTeam::None));
}

bool AGSBotController::FindSidestep(const FVector& AimAt, FVector& Out) const
{
	const AGSCharacter* C = GetBody();
	UWorld* W = GetWorld();
	const FVector Feet = C->GetActorLocation() - FVector(0.f, 0.f, GSBotImpl::HalfHeight(C));
	const FVector EyeUp = EyeLoc() - Feet;
	const FVector Side = FVector::CrossProduct((AimAt - Feet).GetSafeNormal2D(), FVector::UpVector);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBotSidestep), false, C);
	for (const float Off : { 150.f, -150.f, 250.f, -250.f })
	{
		FVector P;
		if (!GSNavSetup::ToNav(W, Feet + Side * Off + FVector(0.f, 0.f, 30.f), P)) continue;
		FHitResult Hit;
		const FVector From = P + EyeUp;
		if (!W->LineTraceSingleByChannel(Hit, From, AimAt, ECC_Visibility, Params) || Cast<AGSCharacter>(Hit.GetActor()))
		{
			Out = P;
			return true;
		}
	}
	return false;
}

void AGSBotController::ReleaseTrigger()
{
	if (!bTriggerDown) return;
	bTriggerDown = false;
	if (AGSCharacter* C = GetBody()) C->StopFire();
}

void AGSBotController::UpdateFire(float Now)
{
	AGSCharacter* C = GetBody();
	const FGSBotSkill& S = Sk();
	const FGSWeaponSlot* W = C->GetCurrentWeapon();
	if (!W) { ReleaseTrigger(); return; }
	const FGSWeaponStats St = C->GetStats(*W);

	// Shots fired since last frame: the kick goes into the aim, and the bot pulls part of it down again by turning.
	if (W->Id != LastWeapon) { LastWeapon = W->Id; LastAmmo = W->Ammo; }
	if (W->Ammo < LastAmmo)
	{
		for (int32 I = W->Ammo; I < LastAmmo; ++I)
		{
			ShotsInBurst = Now - LastShotAt < St.FireInterval * 1.8f + 0.05f ? ShotsInBurst + 1 : 0;
			LastShotAt = Now;
			const float Kick = St.RecoilPitch * (1.f + 0.05f * FMath::Min(ShotsInBurst, 12));
			FRotator R = GetControlRotation();
			R.Pitch += Kick;
			SetControlRotation(R);
			RecoilOffset += Kick * (1.f - S.RecoilControl);
			--BurstLeft;
			if (UGSBotsSubsystem* Sub = UGSBotsSubsystem::Get(this)) Sub->CountShot();
		}
	}
	LastAmmo = W->Ammo;

	if (!St.UsesAmmo()) { ReleaseTrigger(); return; }
	if (C->bReloading) { ReleaseTrigger(); return; }
	if (W->Ammo <= 0)
	{
		ReleaseTrigger();
		C->Reload();
		return;
	}

	bool bCan = false;
	float DistM = 0.f;
	const FSeen* M = Target.IsValid() ? Memory.Find(Target) : nullptr;
	if (M && M->bVisible && bDesiredOnTarget && Now >= M->ReactAt)
	{
		const FVector Eye = EyeLoc();
		const FVector AimP = AimPointFor(Target.Get(), *M, bHeadAim);
		DistM = FVector::Dist(Eye, AimP) / 100.f;
		const float MaxAngle = DistM < 6.f ? 8.f : 3.f;
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(GetControlRotation().Vector(), LastDesired.Vector()), -1.f, 1.f)));
		if (Angle < MaxAngle)
		{
			// The line of fire, before each burst and every 0.1 s while the trigger is held.
			if (!bTriggerDown || Now - LoFCheckedAt > 0.1f)
			{
				LoFCheckedAt = Now;
				bLoFClear = LineOfFire(AimP, Target.Get());
				if (!bLoFClear && Now > SidestepUntil)
				{
					FVector Step;
					if (FindSidestep(AimP, Step)) { SidestepGoal = Step; SidestepUntil = Now + 2.f; NextDecideAt = Now; }
					else SidestepUntil = Now + 1.f;   // nowhere better: wait
				}
			}
			bCan = bLoFClear;
		}
	}
	SetAimDown(bCan && DistM > 12.f && C->GetVelocity().Size2D() < 300.f);
	if (!bCan) { ReleaseTrigger(); return; }

	if (St.bAutomatic)
	{
		if (!bTriggerDown)
		{
			if (Now >= NextTriggerAt)
			{
				SetSprint(false);
				C->StartFire();
				bTriggerDown = true;
				BurstLeft = DistM < S.BurstShortM ? 1000 : FMath::RandRange(3, 5);
			}
		}
		else if (BurstLeft <= 0)
		{
			ReleaseTrigger();
			NextTriggerAt = Now + FMath::FRandRange(0.3f, 0.6f);
		}
	}
	else if (Now >= NextTriggerAt)
	{
		SetSprint(false);
		C->StartFire();
		C->StopFire();
		NextTriggerAt = Now + St.FireInterval * FMath::FRandRange(1.3f, 2.0f);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Decisions and movement

void AGSBotController::SelectBestWeapon()
{
	AGSCharacter* C = GetBody();
	if (!C || C->bReloading) return;
	int32 Best = -1, BestRank = 0;
	for (int32 I = 0; I < C->Weapons.Num(); ++I)
	{
		const FGSWeaponSlot& W = C->Weapons[I];
		if (W.Ammo + W.Reserve <= 0) continue;
		const int32 Rank = GSBotImpl::WeaponRank(C->GetStats(W));
		if (Rank > BestRank) { BestRank = Rank; Best = I; }
	}
	if (Best >= 0 && Best != C->CurrentSlot)
	{
		ReleaseTrigger();
		C->SelectSlot(Best);
	}
}

void AGSBotController::LookForAmmo(float Now)
{
	AGSCharacter* C = GetBody();
	const FGSWeaponSlot* W = C->GetCurrentWeapon();
	if (!W) return;
	const FGSWeaponStats St = C->GetStats(*W);
	if (!St.UsesAmmo() || W->Reserve >= St.MagSize) { AmmoPickup = nullptr; return; }
	if (AmmoPickup.IsValid() && AmmoPickup->bAvailable) return;
	AmmoPickup = nullptr;
	const FName Box = St.Caliber.IsNone() ? NAME_None : FName(*(FString(TEXT("Ammo_")) + St.Caliber.ToString()));
	const FVector P = C->GetActorLocation();
	float BestD = 600.f;
	for (TActorIterator<AGSPickup> It(GetWorld()); It; ++It)
	{
		AGSPickup* K = *It;
		if (!K->bAvailable || K->bLocked) continue;
		if (K->ItemId != W->Id && (Box.IsNone() || K->ItemId != Box)) continue;
		const FVector L = K->GetActorLocation();
		if (FMath::Abs(L.Z - P.Z) > 200.f) continue;
		const float D = FVector::Dist2D(L, P);
		if (D < BestD) { BestD = D; AmmoPickup = K; }
	}
}

FVector AGSBotController::GoalForRole(float Now, float& OutAccept, bool& bOutHold)
{
	AGSCharacter* C = GetBody();
	UWorld* W = GetWorld();
	const uint8 Mine = GetTeamId();
	UGSBotsSubsystem* Sub = UGSBotsSubsystem::Get(this);
	AGSBotTeam* Team = Sub ? Sub->GetTeam(Mine) : nullptr;
	OutAccept = 80.f;
	bOutHold = false;
	const FVector Pos = C->GetActorLocation();
	const TArray<FGSViaPoint>& Via = GSNavSetup::ViaPoints(W);
	FVector OwnFlag;
	if (!GSNavSetup::FlagNav(W, Mine, OwnFlag))
	{
		const AGSFlag* F = Team ? Team->OwnFlag() : nullptr;
		OwnFlag = F ? F->GetActorLocation() : Pos;
	}

	// Through a via point first (phase 0), then on to the goal (phase 1).
	auto ViaThen = [&](const FVector& Goal) -> FVector
	{
		if (TripPhase == 0 && Via.IsValidIndex(ViaIndex))
		{
			const FVector V = Via[ViaIndex].Loc;
			if (FVector::Dist2D(Pos, V) < 300.f && FMath::Abs(Pos.Z - V.Z) < 250.f) TripPhase = 1;
			else return V;
		}
		TripPhase = 1;
		return Goal;
	};

	switch (Role)
	{
	case EGSBotRole::Defend:
	{
		const TArray<FGSDefSpot>& Spots = GSNavSetup::DefenseSpots(W, Mine);
		// A new spot after 30-60 s, or when an enemy has found it (hit while standing there).
		if (Team && Spots.Num() > 0 && (!Spots.IsValidIndex(DefIndex) || Now >= DefSwapAt || (bHold && Now - HurtAt < 0.5f)))
		{
			DefIndex = Team->ClaimDefense(this, DefIndex);
			DefSwapAt = Now + FMath::FRandRange(30.f, 60.f);
		}
		if (Spots.IsValidIndex(DefIndex))
		{
			OutAccept = 50.f;
			bOutHold = FVector::Dist2D(Pos, Spots[DefIndex].Loc) < 100.f && FMath::Abs(Pos.Z - Spots[DefIndex].Loc.Z) < 200.f;
			return Spots[DefIndex].Loc;
		}
		bOutHold = FVector::Dist(Pos, OwnFlag) < 600.f;
		return OwnFlag;
	}
	case EGSBotRole::Retrieve:
	{
		FVector Belief = OwnFlag;
		if (Team) Team->OwnFlagBelief(Belief);
		OutAccept = 50.f;
		return Belief;
	}
	case EGSBotRole::CarryHome:
	{
		if (TripPhase == 0 && ViaIndex < 0 && Team) ViaIndex = Team->ClaimVia(this, CarryAvoidVia, true);
		OutAccept = 50.f;
		return ViaThen(OwnFlag);
	}
	default:
	{
		// A team mate has the enemy flag: go home and be there for the capture.
		if (Team && Team->TeamCarriesEnemyFlag()) return OwnFlag;
		if (TripPhase == 0 && ViaIndex < 0 && Team) ViaIndex = Team->ClaimVia(this, -1, false);
		const FVector Goal = Team ? Team->EnemyFlagGoal() : Pos;
		OutAccept = 50.f;
		return ViaThen(Goal.IsZero() ? Pos : Goal);
	}
	}
}

void AGSBotController::Decide(float Now)
{
	AGSCharacter* C = GetBody();
	SelectBestWeapon();
	if (Now >= NextAmmoLookAt)
	{
		NextAmmoLookAt = Now + 1.f;
		LookForAmmo(Now);
	}

	float Accept = 80.f;
	bool bHoldHere = false;
	FVector Goal = GoalForRole(Now, Accept, bHoldHere);
	const bool bCombat = Target.IsValid();

	if (Now < SidestepUntil && !SidestepGoal.IsZero())
	{
		Goal = SidestepGoal;
		Accept = 40.f;
		bHoldHere = FVector::Dist2D(C->GetActorLocation(), SidestepGoal) < 60.f;
	}
	else if (AmmoPickup.IsValid() && !bCombat)
	{
		AGSPickup* K = AmmoPickup.Get();
		if (FVector::Dist(K->GetActorLocation(), C->GetActorLocation()) < 200.f)
		{
			C->ServerInteract(K);   // the same request a player's use key sends
			AmmoPickup = nullptr;
		}
		else
		{
			Goal = K->GetActorLocation();
			Accept = 60.f;
		}
	}
	else if (bCombat && Role != EGSBotRole::CarryHome && Role != EGSBotRole::Retrieve)
	{
		// Seen an enemy: a defender stands and fights; an attacker stops only when it is close (under 20 m) and otherwise
		// keeps walking its route while it shoots (cover is round 01b). The carrier and the one fetching the flag keep going.
		const float D = FVector::Dist(Target->GetActorLocation(), C->GetActorLocation());
		if (Role == EGSBotRole::Defend || D < FMath::Min(Sk().SightM, 20.f) * 100.f) bHoldHere = true;
	}

	if (bHoldHere)
	{
		Halt();
		bHold = true;
	}
	else
	{
		bHold = false;
		GoTo(Goal, Accept);
	}
	SetCrouch(bHold && Role == EGSBotRole::Defend);

	// Sprint: nobody seen for 4 s, far to go, and looking where it runs.
	bool bSprint = false;
	if (!bHold && !bCombat && Now - LastEnemySeenAt > 4.f && bHasGoal && FVector::Dist(C->GetActorLocation(), MoveGoal) > 1500.f)
	{
		const FVector V = C->GetVelocity().GetSafeNormal2D();
		bSprint = V.IsNearlyZero() || FVector::DotProduct(V, FRotator(0.f, GetControlRotation().Yaw, 0.f).Vector()) > 0.866f;
	}
	SetSprint(bSprint);
}

void AGSBotController::GoTo(const FVector& Goal, float Accept)
{
	const UPathFollowingComponent* PF = GetPathFollowingComponent();
	const bool bMoving = PF && PF->GetStatus() != EPathFollowingStatus::Idle;
	if (bHasGoal && bMoving && FVector::Dist(Goal, MoveGoal) < 100.f) return;
	const EPathFollowingRequestResult::Type R = MoveToLocation(Goal, Accept, true, true, true, true, nullptr, true);
	MoveGoal = Goal;
	bHasGoal = true;
	if (R == EPathFollowingRequestResult::Failed)
	{
		bHasGoal = false;
		if (!bNoPathLogged)
		{
			bNoPathLogged = true;
			const AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
			UE_LOG(LogGSBot, Warning, TEXT("Bot %s: ingen sti fra %s til %s (%s)"), PS ? *PS->GetPlayerName() : TEXT("?"),
				*GetPawn()->GetActorLocation().ToCompactString(), *Goal.ToCompactString(), GSBotRole::Name(Role));
		}
		if (Role == EGSBotRole::Attack || Role == EGSBotRole::CarryHome) NewTrip();
	}
}

void AGSBotController::Halt()
{
	if (!bHasGoal) return;
	StopMovement();
	bHasGoal = false;
}

void AGSBotController::SetSprint(bool bOn)
{
	if (bOn == bSprintOn) return;
	AGSCharacter* C = GetBody();
	if (!C) return;
	bSprintOn = bOn;
	if (bOn) C->StartSprint(); else C->StopSprint();
}

void AGSBotController::SetAimDown(bool bOn)
{
	if (bOn == bAimDown) return;
	AGSCharacter* C = GetBody();
	if (!C) return;
	bAimDown = bOn;
	if (bOn) { SetSprint(false); C->StartAim(); } else C->StopAim();
}

void AGSBotController::SetCrouch(bool bOn)
{
	AGSCharacter* C = GetBody();
	if (!C) return;
	bCrouchOn = bOn;
	// Only when nothing is pending, so one toggle is never undone by the next.
	const bool bWants = C->GetCharacterMovement()->bWantsToCrouch;
	if (C->bIsCrouched == bOn || bWants != C->bIsCrouched) return;
	SetSprint(false);
	C->ToggleCrouch();
}

void AGSBotController::CheckStuck(float Now)
{
	AGSCharacter* C = GetBody();
	const UPathFollowingComponent* PF = GetPathFollowingComponent();
	if (!bHasGoal || bHold || !PF || PF->GetStatus() != EPathFollowingStatus::Moving)
	{
		StuckSince = -1.f;
		return;
	}
	const FVector Pos = C->GetActorLocation();
	if (StuckSince < 0.f || FVector::Dist(Pos, StuckRef) > 50.f)
	{
		StuckRef = Pos;
		StuckSince = Now;
		bJumpedThisStuck = false;
		return;
	}
	const float For = Now - StuckSince;
	if (For > 1.5f && !bJumpedThisStuck)
	{
		bJumpedThisStuck = true;
		// (a) A closed door right ahead: open it like a player would (a locked one is cut out of the nav mesh).
		const FVector Ahead = (MoveGoal - Pos).GetSafeNormal2D();
		bool bDoor = false;
		GSMapState::ForEachDoor(GetWorld(), [&](const FGSDoorInfo& D)
		{
			if (bDoor || D.bOpen || !D.Door || D.Lock == GSLock::Locked) return;
			const FVector To = D.LocationCm - Pos;
			if (To.Size2D() > 150.f || FMath::Abs(To.Z + GSBotImpl::HalfHeight(C)) > 150.f) return;
			if (FVector::DotProduct(To.GetSafeNormal2D(), Ahead) < 0.3f) return;
			D.Door->ServerOpenAway(Pos, GSCause::Bot, false, false);
			bDoor = true;
		});
		// (b) Otherwise one jump.
		if (!bDoor)
		{
			C->StartJump();
			JumpReleaseAt = Now + 0.3f;
		}
		return;
	}
	if (For > 3.f)
	{
		// (c) Still stuck: another route.
		const AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
		UE_LOG(LogGSBot, Log, TEXT("Bot %s sitter fast ved (%.1f, %.1f, %.1f) m, plan %d (%s) - velger en annen rute"),
			PS ? *PS->GetPlayerName() : TEXT("?"), Pos.X / 100.f, Pos.Y / 100.f, Pos.Z / 100.f,
			GSHelgasetr::LevelAt(Pos.Z / 100.f) + 1, GSBotRole::Name(Role));
		StuckSince = -1.f;
		if (Role == EGSBotRole::Attack || Role == EGSBotRole::CarryHome)
		{
			if (UGSBotsSubsystem* Sub = UGSBotsSubsystem::Get(this))
			{
				if (AGSBotTeam* T = Sub->GetTeam(GetTeamId())) { const int32 Old = ViaIndex; TripPhase = 0; ViaIndex = T->ClaimVia(this, Old, false); }
			}
		}
		else if (Role == EGSBotRole::Defend)
		{
			DefSwapAt = Now;
		}
		Halt();
	}
}

// ---------------------------------------------------------------------------------------------------------------------

void AGSBotController::DrawDebug(float Now) const
{
#if ENABLE_DRAW_DEBUG
	const AGSCharacter* C = GetBody();
	UWorld* W = GetWorld();
	const AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	const FColor Col = GetTeamId() == GSTeam::Red ? FColor(255, 80, 60) : FColor(80, 140, 255);
	FString Text = FString::Printf(TEXT("%s: %s"), PS ? *PS->GetPlayerName() : TEXT("?"), GSBotRole::Name(Role));
	if (Role == EGSBotRole::Attack || Role == EGSBotRole::CarryHome) Text += FString::Printf(TEXT(" via %d fase %d"), ViaIndex, TripPhase);
	if (Target.IsValid()) Text += TEXT(" [SER FIENDE]");
	DrawDebugString(W, C->GetActorLocation() + FVector(0.f, 0.f, 120.f), Text, nullptr, Col, 0.f, true);
	if (bHasGoal) DrawDebugLine(W, C->GetActorLocation(), MoveGoal, Col, false, -1.f, 0, 1.f);
	if (const UPathFollowingComponent* PF = GetPathFollowingComponent())
	{
		const FNavPathSharedPtr Path = PF->GetPath();
		if (Path.IsValid())
		{
			const TArray<FNavPathPoint>& Pts = Path->GetPathPoints();
			for (int32 I = 1; I < Pts.Num(); ++I) DrawDebugLine(W, Pts[I - 1].Location + FVector(0, 0, 20), Pts[I].Location + FVector(0, 0, 20), FColor::Yellow, false, -1.f, 0, 2.f);
		}
	}
	if (Target.IsValid()) DrawDebugLine(W, EyeLoc(), Target->GetActorLocation(), FColor::Red, false, -1.f, 0, 1.f);
	if (BotIndex == 0)
	{
		const TArray<FGSViaPoint>& Via = GSNavSetup::ViaPoints(W);
		for (int32 I = 0; I < Via.Num(); ++I)
		{
			DrawDebugSphere(W, Via[I].Loc + FVector(0, 0, 50), 40.f, 8, FColor::Green, false, -1.f);
			DrawDebugString(W, Via[I].Loc + FVector(0, 0, 110), FString::Printf(TEXT("via %d (P%d)"), I, Via[I].Level + 1), nullptr, FColor::Green, 0.f, true);
		}
		for (uint8 T = GSTeam::Red; T <= GSTeam::Blue; ++T)
		{
			for (const FGSDefSpot& S : GSNavSetup::DefenseSpots(W, T))
			{
				DrawDebugBox(W, S.Loc + FVector(0, 0, 50), FVector(25.f), T == GSTeam::Red ? FColor::Red : FColor::Blue, false, -1.f);
				DrawDebugLine(W, S.Loc + FVector(0, 0, 150), S.LookAt, FColor::Cyan, false, -1.f);
			}
		}
	}
#endif
}
