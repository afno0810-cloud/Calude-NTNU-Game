#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CollisionQueryParams.h"
#include "GSBallistics.generated.h"

class AGSCharacter;
class AController;
struct FGSWeaponStats;

/** How a cartridge flies. */
struct FGSBallistic
{
	float MuzzleVelocity = 400.f;  // m/s
	float Drag = 0.0015f;          // 1/m: the speed falls as v0 * exp(-Drag * distance)
	float ZeroM = 25.f;            // the sights meet the bullet at this distance; full damage out to here
};

namespace GSBallistics
{
	GLOSHAUGEN_API FGSBallistic ForCaliber(FName Caliber);
	GLOSHAUGEN_API FGSBallistic ForWeapon(const FGSWeaponStats& S);
	GLOSHAUGEN_API float TimeTo(const FGSBallistic& B, float DistM);
	GLOSHAUGEN_API float ZeroAngle(const FGSBallistic& B);                  // radians the barrel points above the sight line
	GLOSHAUGEN_API float HoldUnder(const FGSBallistic& B, float DistM);     // radians below the sight line at DistM (5b: scope marks)
	GLOSHAUGEN_API int32 ShotSeed(int32 Key, int32 ShotIndex);
	GLOSHAUGEN_API FVector Lift(const FVector& Dir, float ZeroRad);         // tilt Dir up by the zero angle
	/** One bullet: AimDir, a random point in a cone of SpreadDeg (full angle), lifted by the zero angle. */
	GLOSHAUGEN_API FVector ShotDirection(const FVector& AimDir, float SpreadDeg, int32 Seed, const FGSBallistic& B);
	/** A shotgun blast. The one place pellet directions are drawn, for server and every client alike. */
	GLOSHAUGEN_API void PelletDirections(const FVector& AimDir, float ConeDeg, int32 Seed, int32 Pellets, const FGSBallistic& B, TArray<FVector>& Out);
	/** Damage left after DistM metres: 1 out to the zero distance, then with the energy left, never below 0.2. */
	GLOSHAUGEN_API float DamageScale(const FGSBallistic& B, float DistM);
	/** Spread in 0.1 degree steps for the fire RPC. Owner and server both use the unpacked value. */
	GLOSHAUGEN_API uint8 PackSpread(float Deg);
	GLOSHAUGEN_API float UnpackSpread(uint8 Q);
}

struct FGSBullet
{
	FVector Pos = FVector::ZeroVector;
	FVector Vel = FVector::ZeroVector;    // cm/s
	FVector Origin = FVector::ZeroVector; // where it was fired from (hit direction, also if the shooter is gone)
	FGSBallistic Ball;
	float Age = 0.f;
	float Travelled = 0.f;                // cm
	float Damage = 0.f;                   // at the muzzle
	FName WeaponId;
	uint8 AmmoType = 0;
	TWeakObjectPtr<AGSCharacter> Shooter;
	TWeakObjectPtr<AController> Instigator;
	bool bReal = false;                   // server bullet that does damage; otherwise it only shows
};

/** Every bullet in flight in this world. Server: real bullets. Each player's machine: bullets that only show. */
UCLASS()
class GLOSHAUGEN_API UGSBallistics : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	/** Server. AdvanceSeconds: fly this far at once (half the shooter's ping), so it keeps pace with what he sees. */
	void FireReal(AGSCharacter* Shooter, const FVector& Start, const FVector& Dir, const FGSBallistic& B, float Damage, FName WeaponId, uint8 AmmoType, float AdvanceSeconds);
	/** Any machine with a screen: a bullet that does no damage, only shows where it lands. */
	void FireShown(const AGSCharacter* Shooter, const FVector& Start, const FVector& Dir, const FGSBallistic& B);
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGSBallistics, STATGROUP_Tickables); }
protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
	{
		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
	}
private:
	TArray<FGSBullet> Bullets;
	TArray<FGSBullet> Pending;   // bullets fired while Tick walks Bullets
	bool bStepping = false;
	void Add(const FGSBullet& B);
	void AddNow(const FGSBullet& B);
	/** Moves one bullet Dt seconds. False when it is done (hit something, spent, too old, left the map). */
	bool Step(FGSBullet& B, float Dt, const FCollisionQueryParams& Params);
};
