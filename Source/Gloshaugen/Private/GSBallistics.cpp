#include "GSBallistics.h"
#include "GSCharacter.h"
#include "GSItems.h"
#include "GSEffects.h"
#include "GSCampus.h"
#include "GSAudio.h"
#include "GSGameState.h"
#include "GSPlayerState.h"
#include "GSMapState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Controller.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

namespace
{
	// Reserve table per caliber until the item catalog carries the muzzle velocity itself (ordered from Spillbarhet).
	struct FCalRow { const TCHAR* Caliber; float V0; float Drag; float ZeroM; };
	const FCalRow CalTable[] = {
		{ TEXT("9mm"),     360.f, 0.0015f,  25.f },
		{ TEXT("45acp"),   260.f, 0.0009f,  25.f },   // subsonic
		{ TEXT("357"),     440.f, 0.0022f,  25.f },
		{ TEXT("57"),      715.f, 0.0018f,  50.f },
		{ TEXT("556"),     940.f, 0.0009f, 100.f },
		{ TEXT("762x39"),  730.f, 0.0013f, 100.f },
		{ TEXT("762nato"), 840.f, 0.00074f, 100.f },
		{ TEXT("762x54"),  830.f, 0.00075f, 100.f },
		{ TEXT("50bmg"),   890.f, 0.00034f, 100.f },
		{ TEXT("12g"),     400.f, 0.0064f,  25.f },   // one 00 buckshot pellet: slows fast
		{ TEXT("ball"),    450.f, 0.0035f,  50.f },   // musket ball
	};
	constexpr float GravityCm = 980.f;
	constexpr float MaxLife = 1.5f;           // s
	constexpr float MaxTravelCm = 60000.f;    // 600 m: the map is 330 x 265 m
	constexpr float MaxHeightCm = 15000.f;    // 150 m up: gone
	constexpr float AreaMarginM = 50.f;       // outside the play area by this much: gone
	constexpr float MinSpeedCm = 8000.f;      // 80 m/s: spent
	constexpr float StepSeconds = 1.f / 60.f;
	constexpr float MaxAdvance = 0.1f;        // s: the most a bullet is flown ahead for the shooter's ping
	constexpr int32 MaxBullets = 600;         // oldest shown bullets go first
	constexpr int32 MaxStreaks = 40;          // glowing tracer streaks on one machine at once
	constexpr float TracerLightCm = 1000.f;   // a tracer lights 10 m from the muzzle, like the real compound
	constexpr float SoundSpeedCm = 34300.f;   // faster than this: a sharp crack; slower: a whiz
	constexpr float CrackRadiusCm = 300.f;
	constexpr float WhizRadiusCm = 150.f;
	TAutoConsoleVariable<int32> CVarShowBullets(TEXT("gs.ShowBullets"), 0, TEXT("1 = draw the path of every real bullet (server)"));
}

FGSBallistic GSBallistics::ForCaliber(FName Caliber)
{
	FGSBallistic B;
	B.MuzzleVelocity = 400.f; B.Drag = 0.002f; B.ZeroM = 25.f;
	for (const FCalRow& Row : CalTable)
	{
		if (Caliber == FName(Row.Caliber))
		{
			B.MuzzleVelocity = Row.V0; B.Drag = Row.Drag; B.ZeroM = Row.ZeroM;
			break;
		}
	}
	return B;
}

FGSBallistic GSBallistics::ForWeapon(const FGSWeaponStats& S)
{
	FGSBallistic B = ForCaliber(S.Caliber);
	// When the catalog carries it: if (S.MuzzleVelocity > 0.f) B.MuzzleVelocity = S.MuzzleVelocity;
	return B;
}

float GSBallistics::TimeTo(const FGSBallistic& B, float DistM)
{
	const float V0 = FMath::Max(B.MuzzleVelocity, 1.f);
	return B.Drag > 0.f ? (FMath::Exp(B.Drag * DistM) - 1.f) / (B.Drag * V0) : DistM / V0;
}

float GSBallistics::ZeroAngle(const FGSBallistic& B)
{
	const float ZeroM = FMath::Max(B.ZeroM, 1.f);
	const float T = TimeTo(B, ZeroM);
	return FMath::Atan(0.5f * 9.81f * T * T / ZeroM);
}

float GSBallistics::HoldUnder(const FGSBallistic& B, float DistM)
{
	if (DistM <= 0.f) return 0.f;
	const float T = TimeTo(B, DistM);
	const float Drop = 0.5f * 9.81f * T * T - FMath::Tan(ZeroAngle(B)) * DistM;
	return FMath::Atan(Drop / DistM);
}

int32 GSBallistics::ShotSeed(int32 Key, int32 ShotIndex)
{
	return static_cast<int32>(HashCombine(GetTypeHash(Key), GetTypeHash(ShotIndex)));
}

FVector GSBallistics::Lift(const FVector& Dir, float ZeroRad)
{
	const FVector Up = (FVector::UpVector - Dir * FVector::DotProduct(Dir, FVector::UpVector)).GetSafeNormal();
	return Up.IsNearlyZero() ? Dir : (Dir + Up * FMath::Tan(ZeroRad)).GetSafeNormal();
}

FVector GSBallistics::ShotDirection(const FVector& AimDir, float SpreadDeg, int32 Seed, const FGSBallistic& B)
{
	FRandomStream Rng(Seed);
	return Lift(Rng.VRandCone(AimDir, FMath::DegreesToRadians(FMath::Max(SpreadDeg, 0.f) * 0.5f)), ZeroAngle(B));
}

void GSBallistics::PelletDirections(const FVector& AimDir, float ConeDeg, int32 Seed, int32 Pellets, const FGSBallistic& B, TArray<FVector>& Out)
{
	Out.Reset();
	FRandomStream Rng(Seed);
	const FVector Axis = Lift(AimDir, ZeroAngle(B));
	const float HalfRad = FMath::DegreesToRadians(FMath::Max(ConeDeg, 0.f) * 0.5f);
	for (int32 I = 0; I < Pellets; ++I)
	{
		Out.Add(Rng.VRandCone(Axis, HalfRad));
	}
}

float GSBallistics::DamageScale(const FGSBallistic& B, float DistM)
{
	// From the distance, not from the bullet's computed speed: exactly 1 out to the zero distance, and small
	// errors in the flight cannot turn 100 into 99.5. The physics is the same: the speed is v0 * exp(-Drag * x),
	// the energy goes with its square.
	const float R = FMath::Exp(-B.Drag * FMath::Max(0.f, DistM - B.ZeroM));
	return FMath::Clamp(0.2f + 0.8f * R * R, 0.2f, 1.f);
}

uint16 GSBallistics::PackSpread(float Deg)
{
	// 0.01 degree steps: the L96 scoped and crouched (0.035) still keeps its own cone, and up to 655 degrees fit.
	return static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(Deg * 100.f), 0, 65535));
}

float GSBallistics::UnpackSpread(uint16 Q)
{
	return Q * 0.01f;
}

void UGSBallistics::EndBullet(FGSBullet& B)
{
	// Counted by whether it ever got one, not by whether the actor still exists, so the count can never drift.
	if (!B.Streak.IsExplicitlyNull())
	{
		if (AGSShortFX* Streak = B.Streak.Get()) Streak->Destroy();
		LiveStreaks = FMath::Max(LiveStreaks - 1, 0);
		B.Streak.Reset();
	}
}

void UGSBallistics::AddNow(const FGSBullet& B)
{
	if (Bullets.Num() >= MaxBullets)
	{
		// Full: the oldest bullet that only shows goes. Real bullets are never thrown away.
		int32 Oldest = INDEX_NONE;
		for (int32 I = 0; I < Bullets.Num(); ++I)
		{
			if (!Bullets[I].bReal && (Oldest == INDEX_NONE || Bullets[I].Age > Bullets[Oldest].Age)) Oldest = I;
		}
		if (Oldest != INDEX_NONE)
		{
			EndBullet(Bullets[Oldest]);
			Bullets.RemoveAtSwap(Oldest);
		}
	}
	Bullets.Add(B);
}

void UGSBallistics::Add(const FGSBullet& B)
{
	if (bStepping) Pending.Add(B);
	else AddNow(B);
}

void UGSBallistics::FireShown(const AGSCharacter* Shooter, const FVector& Start, const FVector& Dir, const FGSBallistic& B, bool bTracer)
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer) return;
	FGSBullet Bullet;
	Bullet.Pos = Bullet.Origin = Start;
	Bullet.Vel = Dir.GetSafeNormal() * B.MuzzleVelocity * 100.f;
	Bullet.Ball = B;
	Bullet.Shooter = const_cast<AGSCharacter*>(Shooter);
	Bullet.bTracer = bTracer;
	Add(Bullet);
}

void UGSBallistics::FireReal(AGSCharacter* Shooter, const FVector& Start, const FVector& Dir, const FGSBallistic& B, float Damage, FName WeaponId, uint8 AmmoType, float AdvanceSeconds)
{
	if (!GetWorld()) return;
	FGSBullet Bullet;
	Bullet.Pos = Bullet.Origin = Start;
	Bullet.Vel = Dir.GetSafeNormal() * B.MuzzleVelocity * 100.f;
	Bullet.Ball = B;
	Bullet.Shooter = Shooter;
	Bullet.bReal = true;
	Bullet.Damage = Damage;
	Bullet.WeaponId = WeaponId;
	Bullet.AmmoType = AmmoType;
	Bullet.Instigator = Shooter ? Shooter->GetController() : nullptr;
	Bullet.Team = Shooter ? Shooter->GetTeam() : GSTeam::None;

	// Fly it ahead by half the shooter's ping, in ordinary steps, so it keeps pace with the bullet he sees.
	// The bullet is a local here, so nothing TakeDamage does can pull it out from under us.
	float Left = FMath::Clamp(AdvanceSeconds, 0.f, MaxAdvance);
	if (Left > 0.f)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBulletAdvance), true, Shooter);
		Params.bReturnPhysicalMaterial = true;   // impact sound by material
		while (Left > KINDA_SMALL_NUMBER)
		{
			const float Dt = FMath::Min(Left, StepSeconds);
			Left -= Dt;
			if (!Step(Bullet, Dt, Params)) return;   // it already hit something or left the map
		}
	}
	Add(Bullet);
}

void UGSBallistics::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Bullets.Num() == 0) return;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBullet), true);   // once per tick; only the ignored shooter changes
	Params.bReturnPhysicalMaterial = true;   // impact sound by material
	const float Dt = FMath::Min(DeltaTime, 0.1f);
	if (Dt <= 0.f) return;
	UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;
	// The local player's ear (his camera, so a death cam or spectator hears where it looks), once per tick.
	const APlayerController* LocalPC = World && World->GetNetMode() != NM_DedicatedServer ? World->GetFirstPlayerController() : nullptr;
	const bool bHasEar = LocalPC && LocalPC->IsLocalController() && LocalPC->PlayerCameraManager;
	const FVector Ear = bHasEar ? LocalPC->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
	const APawn* EarPawn = bHasEar ? LocalPC->GetPawn() : nullptr;
	bStepping = true;
	for (int32 I = Bullets.Num() - 1; I >= 0; --I)
	{
		// A copy: TakeDamage runs inside Step, and nothing it does may leave us holding a reference into the array.
		FGSBullet B = Bullets[I];
		Params.ClearIgnoredSourceObjects();
		if (const AGSCharacter* Shooter = B.Shooter.Get()) Params.AddIgnoredActor(Shooter);
		bool bAlive = true;
		if (B.bReal)
		{
			float Left = Dt;
			while (bAlive && Left > KINDA_SMALL_NUMBER)
			{
				const float Sub = FMath::Min(Left, StepSeconds);
				Left -= Sub;
				bAlive = Step(B, Sub, Params);
			}
		}
		else
		{
			// Shown bullets only (the host has a real and a shown one for the same shot; this keeps it to one sound
			// and one streak).
			const FVector Prev = B.Pos;
			const float Speed = B.Vel.Size();
			bAlive = Step(B, Dt, Params);   // does no damage: one trace per frame is enough

			// Crack or whiz once per bullet when it passes close to the local player's head. Never his own bullets.
			if (bHasEar && !B.bPassedEar && EarPawn != B.Shooter.Get())
			{
				const bool bSupersonic = Speed > SoundSpeedCm;
				if (FMath::PointDistToSegment(Ear, Prev, B.Pos) < (bSupersonic ? CrackRadiusCm : WhizRadiusCm))
				{
					B.bPassedEar = true;
					// A shotgun blast going past is one crack, and never more than 20 a second.
					if (Now - LastCrackTime > 0.05f)
					{
						LastCrackTime = Now;
						GSAudio::PlayAt(this, bSupersonic ? TEXT("Bullet_Crack") : TEXT("Bullet_Whiz"),
							FMath::ClosestPointOnSegment(Ear, Prev, B.Pos), bSupersonic ? 0.9f : 0.6f, 30.f, FMath::FRandRange(0.92f, 1.08f));
					}
				}
			}

			// The tracer: lights 10 m out, then a streak as long as the bullet goes in 1/50 s (how the eye sees one).
			// Moved here every frame; never a new actor or material per frame.
			if (bAlive && B.bTracer && B.Travelled > TracerLightCm)
			{
				if (B.Streak.IsExplicitlyNull() && LiveStreaks < MaxStreaks)
				{
					B.Streak = AGSShortFX::SpawnBulletStreak(World);
					if (!B.Streak.IsExplicitlyNull()) ++LiveStreaks;
				}
				if (AGSShortFX* Streak = B.Streak.Get())
				{
					const float Len = FMath::Min(FMath::Clamp(B.Vel.Size() * 0.02f, 200.f, 1500.f), B.Travelled - TracerLightCm);
					if (Len > 1.f) Streak->SetStreak(B.Pos - B.Vel.GetSafeNormal() * Len, B.Pos);
				}
			}
		}
		if (bAlive) Bullets[I] = B;
		else
		{
			EndBullet(B);
			Bullets.RemoveAtSwap(I);
		}
	}
	bStepping = false;
	for (const FGSBullet& B : Pending) AddNow(B);
	Pending.Reset();
}

bool UGSBallistics::Step(FGSBullet& B, float Dt, const FCollisionQueryParams& Params)
{
	UWorld* World = GetWorld();
	if (!World) return false;
	const float Speed = B.Vel.Size();
	const float DragCm = B.Ball.Drag / 100.f;
	// Air drag (with the square of the speed) and gravity.
	FVector NewVel = B.Vel + (FVector(0.f, 0.f, -GravityCm) - B.Vel * (DragCm * Speed)) * Dt;
	const FVector NewPos = B.Pos + (B.Vel + NewVel) * 0.5f * Dt;
	// A line trace on the visibility channel: that is where the hit zone boxes of the bodies are.
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, B.Pos, NewPos, ECC_Visibility, Params);
	const FVector End = bHit ? FVector(Hit.ImpactPoint) : NewPos;
	// Glass is not on the visibility channel (the trace above goes through it): Interaktiv finds the panes on this
	// stretch, breaks them (real bullets on the server) and slows the bullet. Real and shown bullets call it alike, so
	// the path is the same on every machine. Only up to End: glass behind the wall that stopped the bullet stays whole.
	const float GlassF = GSMapState::BulletSegment(World, B.Pos, End, B.Vel, B.Damage, B.bReal);
	if (GlassF < 1.f)
	{
		if (B.bReal)
		{
			UE_LOG(LogTemp, Log, TEXT("GS kule gjennom glass: GlassF %.3f, fart %.0f -> %.0f m/s, skade %.1f -> %.1f"),
				GlassF, NewVel.Size() / 100.f, NewVel.Size() * GlassF / 100.f, B.Damage, B.Damage * GlassF);
		}
		NewVel *= GlassF;
		B.Damage *= GlassF;
	}
#if ENABLE_DRAW_DEBUG
	if (B.bReal && CVarShowBullets.GetValueOnGameThread() != 0)
	{
		DrawDebugLine(World, B.Pos, End, bHit ? FColor::Red : FColor::Yellow, false, 4.f, 0, 1.5f);
	}
#endif
	if (bHit)
	{
		AGSCharacter* Victim = Cast<AGSCharacter>(Hit.GetActor());
		const float DistM = (B.Travelled + FVector::Dist(B.Pos, End)) / 100.f;
		if (B.bReal)
		{
			// The shooter has left the game while it flew: TakeDamage cannot tell his team any more, so the team
			// stored at the shot decides. A teammate stops the bullet and takes nothing (friendly fire off).
			bool bSpared = false;
			if (Victim && !B.Instigator.IsValid() && B.Team != GSTeam::None && Victim->GetTeam() == B.Team)
			{
				const AGSGameState* GS = World->GetGameState<AGSGameState>();
				bSpared = GS && !GS->bFriendlyFire;
			}
			if (Victim && !bSpared)
			{
				const float Dmg = B.Damage * GSBallistics::DamageScale(B.Ball, DistM);
				FPointDamageEvent Event(Dmg, Hit, B.Vel.GetSafeNormal(), nullptr);
				Victim->IncomingAmmoType = B.AmmoType;
				Victim->IncomingWeaponId = B.WeaponId;
				Victim->IncomingOrigin = B.Origin;
				Victim->TakeDamage(Dmg, Event, B.Instigator.Get(), B.Shooter.Get());
				Victim->IncomingAmmoType = GSAmmoType::Normal;
				Victim->IncomingWeaponId = NAME_None;
			}
			else if (!Victim)
			{
				// Doors and code locks (Interaktiv): a lock that is hit breaks.
				GSMapState::ApplyBulletHit(World, Hit, B.Damage * GSBallistics::DamageScale(B.Ball, DistM));
			}
		}
		else if (!Victim)
		{
			// A lasting hole by surface (Interaktiv's pool), then dust and sound where the bullet lands. Blood on a body
			// comes from the server (MulticastWoundFX).
			GSMapState::AddBulletMark(World, Hit);
			AGSShortFX::SpawnImpact(World, Hit.ImpactPoint, Hit.ImpactNormal, GSAudio::SurfaceAt(Hit));
		}
		B.Pos = End;   // the last stretch is known to Tick (a crack from a bullet that hit the wall beside you)
		return false;
	}
	B.Travelled += FVector::Dist(B.Pos, NewPos);
	B.Pos = NewPos;
	B.Vel = NewVel;
	B.Age += Dt;
	if (B.Age >= MaxLife || B.Travelled >= MaxTravelCm || NewVel.SizeSquared() < FMath::Square(MinSpeedCm)) return false;
	if (NewPos.Z > MaxHeightCm || NewPos.Z < -2000.f) return false;
	static const FBox2D Area = GSCampus::PlayArea().ExpandBy(AreaMarginM);   // metres
	return Area.IsInside(FVector2D(NewPos.X, NewPos.Y) / 100.f);
}

// ---------------------------------------------------------------------------------------------------------------------
// Headless check (plan 13, K1):  gs.Test.Glasskudd  -> a real 5.56 bullet through a whole glass wall in Helgasetr (not
// the test room), fired 80 cm in front of it; after 0.5 s the log says whether the pane broke. Server only.

namespace GSBallisticsTest
{
	void CmdGlassShot(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogTemp, Warning, TEXT("gs.Test.Glasskudd: bare på verten i spillet"));
			return;
		}
		AGSMapState* MS = AGSMapState::Get(World);
		if (!MS || !MS->IsRegistryBuilt() || MS->GetGlass().Num() == 0)
		{
			// Started from the command line (-ExecCmds): wait for the map and its glass
			static int32 Tries = 0;
			if (++Tries > 60)
			{
				UE_LOG(LogTemp, Warning, TEXT("gs.Test.Glasskudd: FEIL  ingen glassruter etter 60 s"));
				return;
			}
			FTimerHandle H;
			TWeakObjectPtr<UWorld> WW(World);
			const TArray<FString> ArgsCopy = Args;
			World->GetTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([WW, ArgsCopy]()
			{
				if (UWorld* W2 = WW.Get()) CmdGlassShot(ArgsCopy, W2);
			}), 1.f, false);
			return;
		}
		// A whole tempered glass wall from the map (one hit breaks it); a railing if there is none
		int32 Key = 0;
		const AGSMapState::FGlassPane* Pane = nullptr;
		for (int32 Pass = 0; Pass < 2 && !Pane; ++Pass)
		{
			for (const auto& Pair : MS->GetGlass())
			{
				const AGSMapState::FGlassPane& P = Pair.Value;
				if (P.State != 0 || P.Def.Id >= GSPart::TestIdBase) continue;
				if (Pass == 0 && P.Def.Type != EGSGlass::Wall) continue;
				Key = Pair.Key;
				Pane = &P;
				break;
			}
		}
		UGSBallistics* Bal = World->GetSubsystem<UGSBallistics>();
		if (!Pane || !Bal)
		{
			UE_LOG(LogTemp, Warning, TEXT("gs.Test.Glasskudd: FEIL  fant ingen hel glassrute i kartet (%d ruter i alt)"), MS->GetGlass().Num());
			return;
		}
		const FVector Center = Pane->Def.CenterCm;
		const FVector N = FRotator(0.f, Pane->Def.YawDeg + 90.f, 0.f).Vector();
		const FVector Start = Center + N * 80.f;
		const int32 Type = int32(Pane->Def.Type);
		UE_LOG(LogTemp, Log, TEXT("gs.Test.Glasskudd: skyter 5,56 mot rute %d (type %d, %.0f x %.0f cm) ved %s fra 80 cm"),
			GSPart::IdOf(Key), Type, Pane->Def.LengthCm, Pane->Def.HeightCm, *Center.ToCompactString());
		Bal->FireReal(nullptr, Start, -N, GSBallistics::ForCaliber(TEXT("556")), 30.f, NAME_None, 0, 0.f);
		FTimerHandle H;
		TWeakObjectPtr<UWorld> WW(World);
		World->GetTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([WW, Key, Type]()
		{
			UWorld* W = WW.Get();
			AGSMapState* M = W ? AGSMapState::Get(W) : nullptr;
			if (!M) return;
			FGSPartState S;
			M->GetPart(Key, S);
			const bool bOk = Type == int32(EGSGlass::Wall) ? S.State == 2 : S.State >= 1;
			UE_LOG(LogTemp, Log, TEXT("gs.Test.Glasskudd: %s  ruta etter skuddet: tilstand %d (0 hel, 1 sprukket, 2 knust), %d ruter knust i kartet"),
				bOk ? TEXT("OK  ") : TEXT("FEIL"), S.State, M->NumGlassBroken());
		}), 0.5f, false);
	}

	FAutoConsoleCommandWithWorldAndArgs GCmdGlassShot(TEXT("gs.Test.Glasskudd"),
		TEXT("Skyter en ekte kule gjennom en hel glassvegg i Helgasetr og logger om den knuses (hodeløs test)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CmdGlassShot));
}
