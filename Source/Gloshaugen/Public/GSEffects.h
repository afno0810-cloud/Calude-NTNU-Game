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

protected:
	UPROPERTY() TObjectPtr<class UStaticMeshComponent> Mesh;
};
