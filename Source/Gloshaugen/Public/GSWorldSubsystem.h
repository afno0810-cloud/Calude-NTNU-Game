#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "GSWorldSubsystem.generated.h"

class AGSCampusBuilder;

/** Builds the map (Helgasetr or the old campus) and the sky on every machine when a game world starts. */
UCLASS()
class GLOSHAUGEN_API UGSWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	/** The map is built when the world's actors are initialized: that is BEFORE the engine spawns the local players
	 *  (UEngine::LoadMap and PIE both call InitializeActorsForPlay, then SpawnPlayActor, then BeginPlay), so the first
	 *  spawn already finds the floors. OnWorldBeginPlay builds it if this did not run. */
	void HandleActorsInitialized(const FActorsInitializedParams& Params);
	void BuildMap(UWorld& InWorld);

	TWeakObjectPtr<AGSCampusBuilder> Campus;
	FDelegateHandle ActorsInitializedHandle;
	bool bBuilt = false;
};
