#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "InputCoreTypes.h"
#include "GSTypes.h"
#include "GSExplore.generated.h"

class APlayerController;
class AGSCharacter;
class UInputAction;
class UInputMappingContext;

/**
 * "Test kartet": walk freely around the map with no match and no damage, fly through walls (N), jump to the bases,
 * set clock and weather. Exists only in a game started with Mode=Explore, which is always alone on one machine
 * (NM_Standalone), so everything here runs locally with authority. The menu calls the functions in namespace GSExplore.
 */
UCLASS()
class GLOSHAUGEN_API AGSExplore : public AInfo
{
	GENERATED_BODY()

public:
	AGSExplore();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool bFlying = false;
	/** Where the player last stood on the ground (flying ends here when nothing else is free). */
	FVector LastSafe = FVector::ZeroVector;
	bool bHasLastSafe = false;
	/** Where the player stood before the last move from the menu ("Der du sto sist"). */
	FVector PrevSpot = FVector::ZeroVector;
	float PrevYaw = 0.f;
	bool bHasPrevSpot = false;

	void StartFlying(APlayerController* PC, AGSCharacter* C);
	/** Puts the body down on a floor, never inside a wall. bMoveToFloor false: only ends the flight where it is. */
	void StopFlying(APlayerController* PC, AGSCharacter* C, bool bMoveToFloor = true);

protected:
	float NextFoodAt = 0.f;
	float NextSafeAt = 0.f;
	float NextModeFixAt = 0.f;
	bool bModeFixLogged = false;

	/**
	 * While flying, the jump and crouch keys go to this context instead (higher priority, consumes the keys), so the
	 * game's own jump, climb, crouch and slide do not also run in the air. Flying up and down reads the raw key state.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> FlyBlockContext;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> FlyBlockAction;
	TArray<FKey> BlockedKeys;
	/** bOn: the context is there and holds the current jump/crouch keys (rebinding is followed); off: removed. */
	void UpdateKeyBlock(APlayerController* PC, bool bOn);
};

namespace GSExplore
{
	/** N in the test: fly through walls on / off. */
	GLOSHAUGEN_API void ToggleFly(APlayerController* PC);
	GLOSHAUGEN_API bool IsFlying(const APlayerController* PC);
	/** Moves the player to Target (cm; a floor near it is looked for), facing Yaw. Remembers where he stood. */
	GLOSHAUGEN_API void Teleport(APlayerController* PC, const FVector& Target, float Yaw);
	/** "Der du sto sist": back to where the last move started. */
	GLOSHAUGEN_API bool HasPreviousSpot(const APlayerController* PC);
	GLOSHAUGEN_API void TeleportBack(APlayerController* PC);
	/** The in-game clock jumps to Hours (0-24) and runs on from there. */
	GLOSHAUGEN_API void SetClock(APlayerController* PC, float Hours);
	/** The weather changes at once and stays. */
	GLOSHAUGEN_API void SetWeather(APlayerController* PC, EGSWeather Weather);
}
