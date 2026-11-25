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
	static void SpawnImpact(UWorld* World, const FVector& Location, const FVector& Normal);
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
};
