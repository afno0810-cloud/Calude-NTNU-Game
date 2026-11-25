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
	/** A plain colored sphere (smoke puffs, fire, flashes). */
	static AGSShortFX* SpawnBlob(UWorld* World, const FVector& Location, const FVector& Scale, const FLinearColor& Color, float LifeSeconds);

protected:
	UPROPERTY() TObjectPtr<class UStaticMeshComponent> Mesh;
};
