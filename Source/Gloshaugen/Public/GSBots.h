#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GSBots.generated.h"

class AGSBotController;
class AGSBotTeam;
class AGSPlayerState;

DECLARE_STATS_GROUP(TEXT("GSBots"), STATGROUP_GSBots, STATCAT_Advanced);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Bot think"), STAT_GSBotThink, STATGROUP_GSBots, GLOSHAUGEN_API);

/**
 * How good a bot is (lag Bots). Only skill differs between the levels: never health, damage or what a bot may know.
 * The fields marked 01b are used from round 01b (cover, medicine, hearing); they are in the table now so the header stays.
 */
struct FGSBotSkill
{
	const TCHAR* Name;
	float ReactMin, ReactMax;      // s from first sight to the first shot
	float TurnDegPerSec;           // how fast the aim turns
	float AimErrorDeg;             // aim error at first sight ...
	float AimSettleHalfLife;       // ... halved this often (s)
	float LeadError;               // 0 = leads a moving target perfectly
	float RecoilControl;           // part of each kick pulled back down
	float HeadChance;              // aims for the head
	float ViewFovDeg, SightM;      // field of view, how far it sees
	float BurstShortM;             // closer than this: holds the trigger
	float HearM;                   // 01b
	float CoverUse;                // 01b
	float RetreatHealth;           // 01b
};

namespace GSBots
{
	/** 0 Lett, 1 Middels, 2 Vanskelig, 3 Ekspert. */
	GLOSHAUGEN_API const FGSBotSkill& Skill(uint8 Level);
	/** gs.bots.debug: draw roles, goals, paths and via points. */
	GLOSHAUGEN_API bool IsDebug();
	/** gs.bots.think 0: the bots stand still and do not think (performance measurement). */
	GLOSHAUGEN_API bool IsThinking();
}

/**
 * Makes the bots in a bot match ("Spill mot bots", always alone on one machine) and keeps the numbers the menu chose:
 * BotAllies on the human's team, BotEnemies on the other. Each bot is an AGSBotController with its own AGSPlayerState
 * (marked as a bot) and spawns through AGSGameMode::RestartPlayer like a player, so the match, the respawns and the
 * scoreboard treat it like one.
 */
UCLASS()
class GLOSHAUGEN_API UGSBotsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	static UGSBotsSubsystem* Get(const UObject* Ctx);

	/** The team brain of one team (made the first time). */
	AGSBotTeam* GetTeam(uint8 Team);
	const TArray<TWeakObjectPtr<AGSBotController>>& GetBots() const { return Bots; }
	/** Shared sight budget: at most 48 sight traces per frame for all bots together. */
	bool TakeSightTraces(int32 N);
	/** Time spent thinking this frame (all bots), for the log every 30 s. */
	void AddThinkTime(double Seconds) { ThinkThisWindow += Seconds; ThinkMaxBot = FMath::Max(ThinkMaxBot, Seconds); }
	/** A bot fired a shot (for the 30 s log line). */
	void CountShot() { ++ShotsWindow; }
	/** gs.bots.fill: bots also in "Test kartet" (they stand and defend, no match). */
	void ForceFill(int32 Allies, int32 Enemies, int32 Level);
	/** The bots are wanted in this world (bot match, or gs.bots.fill). */
	bool IsActive() const;

protected:
	void Step();
	AGSBotController* SpawnBot(uint8 Team);
	void RemoveBot(uint8 Team);
	FString PickName() const;
	void LogSkillTable() const;
	void LogThinkTime();

	TArray<TWeakObjectPtr<AGSBotController>> Bots;
	UPROPERTY() TObjectPtr<AGSBotTeam> Teams[3];
	FTimerHandle StepTimer;
	int32 NextIndex = 0;
	bool bForced = false;
	bool bNavAsked = false;
	bool bLoggedTable = false;
	bool bFlagLogBound = false;
	uint8 LastPhase = 255;
	// Sight budget
	uint64 BudgetFrame = 0;
	int32 BudgetUsed = 0;
	// Think time
	double ThinkThisWindow = 0.0;
	double ThinkMaxBot = 0.0;
	double WindowStart = 0.0;
	uint64 WindowFrame = 0;
	// What the bots did in the window (for headless tests): shots, and where each one was at the last log
	int32 ShotsWindow = 0;
	TMap<TWeakObjectPtr<AGSBotController>, FVector> LastPos;
};
