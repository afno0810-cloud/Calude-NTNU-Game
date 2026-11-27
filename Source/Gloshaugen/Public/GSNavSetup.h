#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CollisionQueryParams.h"
#include "GSNavSetup.generated.h"

class ANavMeshBoundsVolume;
class UBoxComponent;

/** A point between the bases the attackers route through (lag Bots). Centimetres, on the nav mesh. */
struct FGSViaPoint
{
	FVector Loc = FVector::ZeroVector;
	int32 Level = 0;   // GSHelgasetr::LevelAt (0 = Plan 1)
	int32 Side = 0;    // +1 / -1: left / right of the line red flag -> blue flag
};

/** A place a defender stands: sees the flag, has a wall at its back, and looks at the entrance it sees best. */
struct FGSDefSpot
{
	FVector Loc = FVector::ZeroVector;
	FVector LookAt = FVector::ZeroVector;
	int32 Entrances = 0;   // how many of the base's entrances it sees
};

/**
 * Lag Bots: the navigation mesh for the map that is built in code (Helgasetr), made at run time, and the routes the bots
 * use (via points between the bases, defence spots per base). Nothing here has coordinates of its own: everything comes
 * from the map data (GSHelgasetr::NavBoundsM, the flags, the spawn points, the door slots).
 * Server only (the bots live there); a client never makes a nav system (bAllowClientSideNavigation=False).
 */
namespace GSNavSetup
{
	/** Makes the bounds volume (once) so the nav mesh is generated. Safe to call again. */
	GLOSHAUGEN_API void EnsureNavMesh(UWorld* World);
	/** The nav mesh has tiles and is not building. */
	GLOSHAUGEN_API bool IsNavReady(const UWorld* World);
	/** The self-test, via points and defence spots are done. */
	GLOSHAUGEN_API bool AreRoutesReady(const UWorld* World);
	/** Something that changes where one can walk (a wall blown, a door unlocked): rebuild that box. */
	GLOSHAUGEN_API void NotifyMapChanged(UWorld* World, const FBox& BoxCm);
	/** Projects a FLOOR point onto the nav mesh (extent 100 x 100 x 250 cm). Never project a point in the air. */
	GLOSHAUGEN_API bool ToNav(UWorld* World, const FVector& In, FVector& Out);
	/** A full (not partial) path From -> To on the nav mesh: its length (cm) and, if asked, its points. */
	GLOSHAUGEN_API bool FindPath(UWorld* World, const FVector& From, const FVector& To, float& OutLenCm, TArray<FVector>* OutPoints = nullptr);
	GLOSHAUGEN_API const TArray<FGSViaPoint>& ViaPoints(const UWorld* World);
	GLOSHAUGEN_API const TArray<FGSDefSpot>& DefenseSpots(const UWorld* World, uint8 Team);
	/** The flag of a team projected to the nav mesh (false before the routes are ready). */
	GLOSHAUGEN_API bool FlagNav(const UWorld* World, uint8 Team, FVector& Out);
	/**
	 * A line From -> To: true when nothing blocks it. bObjects: only the world (WorldStatic objects) blocks, else Channel.
	 * bSkipGlass: components tagged Mat_Glass are looked through (the trace goes on from the hit, at most 3 times).
	 * OutHit gets the first blocking hit (when it returns false).
	 */
	GLOSHAUGEN_API bool ClearLine(const UWorld* World, const FVector& From, const FVector& To, const FCollisionQueryParams& Params,
		bool bObjects, ECollisionChannel Channel, bool bSkipGlass, FHitResult* OutHit = nullptr);
}

/** Holds the nav mesh state of one game world, runs the readiness check, the self-test and the route sampling. */
UCLASS()
class GLOSHAUGEN_API UGSNavSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	void Ensure();
	bool IsReady() const { return bNavReady; }
	bool RoutesReady() const { return Stage == EStage::Done; }

	TArray<FGSViaPoint> Via;
	TArray<FGSDefSpot> Defense[3];   // by team (1 red, 2 blue)
	FVector FlagNavLoc[3];
	bool bFlagNav[3] = { false, false, false };

protected:
	enum class EStage : uint8 { Idle, Waiting, SelfTest, ViaSample, ViaPaths, ViaPick, Defense, Done, Failed };

	void CheckReady();
	void RunSelfTest();
	void SampleVia();
	void StepViaPaths();
	void PickVia();
	void BuildDefense();
	void SyncLockedDoors();
	void OnWalkableChanged(UWorld* InWorld, const FBox& BoxCm);
	bool GetFlagFloor(uint8 Team, FVector& Out) const;
	void GetSpawnFloors(uint8 Team, TArray<FVector>& Out) const;

	TWeakObjectPtr<ANavMeshBoundsVolume> Volume;
	EStage Stage = EStage::Idle;
	bool bNavReady = false;
	bool bRebuildKicked = false;
	double EnsureTime = 0.0;
	float CheckAccum = 0.f;
	int32 LastTiles = -1;
	int32 StableChecks = 0;
	float DoorAccum = 0.f;

	struct FViaCand { FVector Loc; int32 Level; int32 Side; bool bChecked = false; bool bOk = false; };
	TArray<FViaCand> ViaCands;
	TArray<int32> ViaQueue;     // candidate indexes to path-check (at most 40)
	int32 ViaQueuePos = 0;

	/** Locked, closed doors: a box that cuts the opening out of the nav mesh (NavArea_Null), by door key. */
	UPROPERTY() TObjectPtr<AActor> DoorBlockHolder;
	UPROPERTY() TMap<int32, TObjectPtr<UBoxComponent>> DoorBlocks;
	FDelegateHandle WalkableHandle;
};
