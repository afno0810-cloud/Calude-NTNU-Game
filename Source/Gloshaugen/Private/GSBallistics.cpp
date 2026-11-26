#include "GSBallistics.h"
#include "GSCharacter.h"
#include "GSItems.h"
#include "GSEffects.h"
#include "GSCampus.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Controller.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

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

uint8 GSBallistics::PackSpread(float Deg)
{
	return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Deg * 10.f), 0, 255));
}

float GSBallistics::UnpackSpread(uint8 Q)
{
	return Q * 0.1f;
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
		if (Oldest != INDEX_NONE) Bullets.RemoveAtSwap(Oldest);
	}
	Bullets.Add(B);
}

void UGSBallistics::Add(const FGSBullet& B)
{
	if (bStepping) Pending.Add(B);
	else AddNow(B);
}

void UGSBallistics::FireShown(const AGSCharacter* Shooter, const FVector& Start, const FVector& Dir, const FGSBallistic& B)
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer) return;
	FGSBullet Bullet;
	Bullet.Pos = Bullet.Origin = Start;
	Bullet.Vel = Dir.GetSafeNormal() * B.MuzzleVelocity * 100.f;
	Bullet.Ball = B;
	Bullet.Shooter = const_cast<AGSCharacter*>(Shooter);
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

	// Fly it ahead by half the shooter's ping, in ordinary steps, so it keeps pace with the bullet he sees.
	// The bullet is a local here, so nothing TakeDamage does can pull it out from under us.
	float Left = FMath::Clamp(AdvanceSeconds, 0.f, MaxAdvance);
	if (Left > 0.f)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBulletAdvance), true, Shooter);
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
	const float Dt = FMath::Min(DeltaTime, 0.1f);
	if (Dt <= 0.f) return;
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
			bAlive = Step(B, Dt, Params);   // does no damage: one trace per frame is enough
		}
		if (bAlive) Bullets[I] = B;
		else Bullets.RemoveAtSwap(I);
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
	const FVector NewVel = B.Vel + (FVector(0.f, 0.f, -GravityCm) - B.Vel * (DragCm * Speed)) * Dt;
	const FVector NewPos = B.Pos + (B.Vel + NewVel) * 0.5f * Dt;
	// A line trace on the visibility channel: that is where the hit zone boxes of the bodies are.
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, B.Pos, NewPos, ECC_Visibility, Params);
	const FVector End = bHit ? FVector(Hit.ImpactPoint) : NewPos;
#if ENABLE_DRAW_DEBUG
	if (B.bReal && CVarShowBullets.GetValueOnGameThread() != 0)
	{
		DrawDebugLine(World, B.Pos, End, bHit ? FColor::Red : FColor::Yellow, false, 4.f, 0, 1.5f);
	}
#endif
	if (bHit)
	{
		AGSCharacter* Victim = Cast<AGSCharacter>(Hit.GetActor());
		if (B.bReal)
		{
			if (Victim)
			{
				const float DistM = (B.Travelled + FVector::Dist(B.Pos, End)) / 100.f;
				const float Dmg = B.Damage * GSBallistics::DamageScale(B.Ball, DistM);
				FPointDamageEvent Event(Dmg, Hit, B.Vel.GetSafeNormal(), nullptr);
				Victim->IncomingAmmoType = B.AmmoType;
				Victim->IncomingWeaponId = B.WeaponId;
				Victim->IncomingOrigin = B.Origin;
				Victim->TakeDamage(Dmg, Event, B.Instigator.Get(), B.Shooter.Get());
				Victim->IncomingAmmoType = GSAmmoType::Normal;
				Victim->IncomingWeaponId = NAME_None;
			}
		}
		else if (!Victim)
		{
			// Dust and a hole where the bullet lands. Blood on a body comes from the server (MulticastWoundFX).
			AGSShortFX::SpawnImpact(World, Hit.ImpactPoint, Hit.ImpactNormal);
		}
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
