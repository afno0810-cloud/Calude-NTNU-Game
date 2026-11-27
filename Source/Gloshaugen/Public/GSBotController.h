#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GSBotTeam.h"
#include "GSBotController.generated.h"

class AGSCharacter;
class AGSPickup;
struct FGSBotSkill;

/**
 * Lag Bots: a computer player. It drives an ordinary AGSCharacter (the same body, weapons, damage and blood as a player)
 * only through the player's own actions (StartFire, Reload, SelectSlot ...; AGSCharacter makes us a friend for that).
 * It sees only what it really sees (field of view, range, walls; it looks through glass), remembers only enemies it has
 * seen, needs time to react, aims with an error that shrinks, holds its own recoil down and never shoots through solid
 * glass or a team mate. Server only. Sight 5 Hz, decisions 4 Hz, aim and movement every frame, staggered by BotIndex.
 */
UCLASS()
class GLOSHAUGEN_API AGSBotController : public AAIController
{
	GENERATED_BODY()

public:
	AGSBotController(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Destroyed() override;
	/** We turn the control rotation ourselves (yaw AND pitch); the engine's version zeroes the pitch without a focus. */
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;

	/** 0 Lett .. 3 Ekspert (AGSGameState::BotSkill). */
	uint8 Skill = 1;
	int32 BotIndex = 0;

	uint8 GetTeamId() const;
	AGSCharacter* GetBody() const;

	// Set by AGSBotTeam
	EGSBotRole Role = EGSBotRole::Attack;
	int32 ViaIndex = -1;       // the via point of this trip
	int32 DefIndex = -1;       // defence spot
	void SetRole(EGSBotRole NewRole);
	/** Start a new trip (new via point). */
	void NewTrip() { TripPhase = 0; ViaIndex = -1; }

protected:
	/** What the bot knows about one enemy. Only what it saw itself. */
	struct FSeen
	{
		FVector LastLoc = FVector::ZeroVector;
		FVector LastVel = FVector::ZeroVector;
		float LastSeen = -100.f;      // world time
		float FirstSeen = -100.f;     // start of this sighting (reaction time counts from here)
		float ReactAt = 0.f;          // may shoot from here
		bool bVisible = false;
		uint8 FreeMask = 0;           // 1 head, 2 chest, 4 hip
	};

	void Think(float Dt, float Now);
	void UpdateSight(float Now);
	void Decide(float Now);
	void UpdateAim(float Dt, float Now);
	void UpdateFire(float Now);
	void UpdateMove(float Now);
	void CheckStuck(float Now);
	void PickTarget(float Now);
	FVector AimPointFor(AGSCharacter* Target, const FSeen& S, bool bHead) const;
	bool LineOfFire(const FVector& To, AGSCharacter* Target) const;
	bool FindSidestep(const FVector& AimAt, FVector& Out) const;
	void GoTo(const FVector& Goal, float Accept = 80.f);
	void Halt();
	void SetSprint(bool bOn);
	void SetAimDown(bool bOn);
	void SetCrouch(bool bOn);
	void ReleaseTrigger();
	void SelectBestWeapon();
	void LookForAmmo(float Now);
	FVector EyeLoc() const;
	FVector GoalForRole(float Now, float& OutAccept, bool& bOutHold);
	void DrawDebug(float Now) const;

	const FGSBotSkill& Sk() const;

	TMap<TWeakObjectPtr<AGSCharacter>, FSeen> Memory;
	TWeakObjectPtr<AGSCharacter> Target;
	float TargetSince = -100.f;
	bool bHeadAim = false;
	float LeadFactor = 1.f;
	// Aim error: a fixed direction, halved every AimSettleHalfLife
	FRotator ErrorDir = FRotator::ZeroRotator;
	float ErrorStart = 0.f;
	float ErrorAt = 0.f;           // world time the error was (re)drawn
	// Recoil the bot holds down itself
	float RecoilOffset = 0.f;      // pitch (deg) still in the aim
	int32 LastAmmo = -1;
	FName LastWeapon;
	int32 ShotsInBurst = 0;
	float LastShotAt = -100.f;
	// The aim we turn toward (with error and recoil), and whether it is on a target
	FRotator LastDesired = FRotator::ZeroRotator;
	bool bDesiredOnTarget = false;
	float LastEnemySeenAt = -100.f;
	// Line of fire, checked before each burst and every 0.1 s while firing
	float LoFCheckedAt = -100.f;
	bool bLoFClear = false;
	// Trigger
	bool bTriggerDown = false;
	int32 BurstLeft = 0;
	float NextTriggerAt = 0.f;
	bool bAimDown = false;
	bool bSprintOn = false;
	bool bCrouchOn = false;
	// Look without a target
	float NextGlanceAt = 0.f;
	float GlanceYaw = 0.f;
	FVector LookLostAt = FVector::ZeroVector;
	float LookLostUntil = -100.f;
	// Timing (staggered)
	float NextSightAt = 0.f;
	float NextDecideAt = 0.f;
	float NextAmmoLookAt = 0.f;
	// Movement
	FVector MoveGoal = FVector::ZeroVector;
	bool bHasGoal = false;
	bool bHold = false;            // stand still here (defender at its spot, firing at a target)
	int32 TripPhase = 0;           // 0 to the via point, 1 to the goal
	int32 CarryAvoidVia = -1;      // carrying the flag home: not the via point it came in by
	FVector StuckRef = FVector::ZeroVector;
	float StuckSince = -1.f;
	float StuckCheckAt = 0.f;
	bool bJumpedThisStuck = false;
	float JumpReleaseAt = -1.f;
	float SidestepUntil = -100.f;
	FVector SidestepGoal = FVector::ZeroVector;
	float DefSwapAt = 0.f;
	float LastHealth = 100.f;
	float HurtAt = -100.f;
	TWeakObjectPtr<AGSPickup> AmmoPickup;
	bool bNoPathLogged = false;
	float NextSpikeLogAt = 0.f;
};
