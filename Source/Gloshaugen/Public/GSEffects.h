#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSEffects.generated.h"

/** Short-lived visual: bullet tracer or impact puff. Purely cosmetic, never replicated. */
UCLASS(NotPlaceable)
class GLOSHAUGEN_API AGSShortFX : public AActor
{
	GENERATED_BODY()

public:
	AGSShortFX();

	static void SpawnTracer(UWorld* World, const FVector& Start, const FVector& End);
	/** A glowing streak that lives until it is destroyed. GSBallistics owns one per tracer bullet and moves it every frame. */
	static AGSShortFX* SpawnBulletStreak(UWorld* World);
	void SetStreak(const FVector& Tail, const FVector& Head);
	/** Surface: what was hit (GSAudio::SurfaceAt) for the sound; None = concrete. */
	static void SpawnImpact(UWorld* World, const FVector& Location, const FVector& Normal, FName Surface = NAME_None);
	static void SpawnMuzzleFlash(UWorld* World, const FVector& Location, const FVector& Dir, bool bSilent);
	static void SpawnExplosion(UWorld* World, const FVector& Location, float Radius);
	/** A plain colored sphere (fallback when the FX materials are missing). */
	static AGSShortFX* SpawnBlob(UWorld* World, const FVector& Location, const FVector& Scale, const FLinearColor& Color, float LifeSeconds);
	/**
	 * Soft camera-facing sprite (smoke, dust, fire, flash) that grows, drifts and fades.
	 * Size/EndSize are diameters in cm. Additive sprites glow (fire, flashes, sparks); the others are smoke.
	 */
	static AGSShortFX* SpawnSprite(UWorld* World, const FVector& Location, float Size, float EndSize, const FLinearColor& Color,
		float Opacity, float LifeSeconds, const FVector& Velocity = FVector::ZeroVector, bool bAdditive = false,
		float Glow = 1.f, float Rise = 0.f, float Drag = 1.5f);
	/** Smoke column / cloud made of many sprites (smoke grenade, burning things). */
	static void SpawnSmokeCloud(UWorld* World, const FVector& Location, float Radius, const FLinearColor& Color, float LifeSeconds, int32 Count);
	/** Thin wisp of smoke from a hot barrel after a long burst. */
	static void SpawnBarrelSmoke(UWorld* World, const FVector& Location, const FVector& Dir);
	/** Blood stain lying flat on a surface. Grows to Size (cm, across) over GrowSeconds, dries, erodes away at the
	 *  end of LifeSeconds. Stretch > 1 makes it long along Along (spatter that hit at an angle). Delay = seconds
	 *  before it shows (a falling drop). */
	static AGSShortFX* SpawnBloodStain(UWorld* World, const FVector& Location, const FVector& Normal, float Size,
		float GrowSeconds, float LifeSeconds, const FVector& Along = FVector::ZeroVector, float Stretch = 1.f, float Delay = 0.f,
		bool bTrail = false);
	/** One drop from someone who is bleeding: falls straight down from From and lands as a small stain. Trail drops
	 *  have their own limit per machine, so a trail never takes the place of spatter on walls or pools. */
	static void SpawnBloodDrip(UWorld* World, const FVector& From, float Size, const AActor* Victim);
	/** A blood patch (Size cm across) owned by Owner, for sticking to the body where a bullet went in. The caller
	 *  places and attaches it; it goes when the body goes. No collision, no shadow, no tick. */
	static class UStaticMeshComponent* MakeBodyWound(AActor* Owner, float Size);
	/** A hit on a body (GSWound kind): mist and drops at the wound, spatter behind, drops on the floor. Strength 0..1.
	 *  bOwnView: the local player is the victim (no mist in his own face). */
	static void SpawnBloodHit(UWorld* World, const FVector& Point, const FVector& Dir, uint8 Kind, float Strength,
		const AActor* Victim, bool bOwnView);
	/** Pool that slowly spreads under a body. Shrinks to fit the floor it lies on (ledges, stairs). */
	static void SpawnBloodPool(UWorld* World, const FVector& Location, const FVector& Normal, float Size, const AActor* Body);

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY() TObjectPtr<class UStaticMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> MID;
	bool bSprite = false;
	bool bAdd = false;
	float Age = 0.f;
	float Life = 1.f;
	float Size0 = 10.f;
	float Size1 = 10.f;
	float Opacity0 = 1.f;
	float Glow0 = 1.f;
	float SpriteSpin = 0.f;
	float SpinRate = 0.f;
	float RiseAccel = 0.f;
	float DragK = 1.5f;
	FVector Vel = FVector::ZeroVector;
	// Blood stain (lies on a surface instead of facing the camera)
	bool bStain = false;
	float GrowTime = 0.1f;
	float StretchK = 1.f;
	float ErodeTime = 5.f;
	/** Stain: erode away now (the oldest stains make room for new ones). */
	void RetireStain();
};
