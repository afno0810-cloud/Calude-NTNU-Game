#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GSWorldSubsystem.generated.h"

/** Builds the campus and the sky on every machine when a game world starts. */
UCLASS()
class GLOSHAUGEN_API UGSWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
};
