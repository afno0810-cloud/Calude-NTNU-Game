#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "GSBotTeam.generated.h"

class AGSBotController;
class AGSFlag;

/** What a bot does for its team right now. */
enum class EGSBotRole : uint8 { Attack, Defend, Retrieve, CarryHome };

namespace GSBotRole
{
	inline const TCHAR* Name(EGSBotRole R)
	{
		switch (R)
		{
		case EGSBotRole::Defend: return TEXT("forsvarer");
		case EGSBotRole::Retrieve: return TEXT("henter flagget");
		case EGSBotRole::CarryHome: return TEXT("bærer hjem");
		default: return TEXT("angriper");
		}
	}
}

/**
 * Lag Bots: the brain of one team's bots (server only, one per team). Every 2 s, and at once when a flag changes, it
 * hands out the roles: defenders by team size, half the attackers go for the own flag when it is away, a bot carrying
 * the enemy flag goes home another way, and the rest attack on different via points (at most 2 on the same one).
 * It knows only what the team's people know: the flags' state as the HUD shows it, and the radio report of the carrier.
 */
UCLASS(NotPlaceable)
class GLOSHAUGEN_API AGSBotTeam : public AInfo
{
	GENERATED_BODY()

public:
	AGSBotTeam();
	virtual void Tick(float DeltaSeconds) override;

	uint8 Team = 0;

	void Register(AGSBotController* Bot);
	void Unregister(AGSBotController* Bot);

	/** A via point for this bot's next trip (high 2 bots on one). Avoid: the one it came by (carry home: also prefer the
	 *  other side or floor). -1 when there are none. */
	int32 ClaimVia(AGSBotController* Bot, int32 Avoid, bool bPreferOther);
	/** A defence spot for this bot (another one than Avoid when there is one). -1 when there are none. */
	int32 ClaimDefense(AGSBotController* Bot, int32 Avoid);

	AGSFlag* OwnFlag() const;
	AGSFlag* EnemyFlag() const;
	/** Where the team believes its own flag is (dropped: where it lies; carried and reported: where the carrier was seen). */
	bool OwnFlagBelief(FVector& Out) const;
	/** Where an attacker goes for the enemy flag (its base, or where it lies when dropped). */
	FVector EnemyFlagGoal() const;
	/** Another bot (or player) of ours carries the enemy flag. */
	bool TeamCarriesEnemyFlag() const;

	void RequestAssign() { bAssignNow = true; }

protected:
	void Assign();
	void WatchFlags();

	TArray<TWeakObjectPtr<AGSBotController>> Bots;
	TMap<TWeakObjectPtr<AGSBotController>, int32> ViaOf;
	TMap<TWeakObjectPtr<AGSBotController>, int32> DefOf;
	mutable TWeakObjectPtr<AGSFlag> CachedOwn, CachedEnemy;
	float AssignAccum = 0.f;
	float WatchAccum = 0.f;
	bool bAssignNow = true;
	uint8 LastFlagState[3] = { 255, 255, 255 };
	TWeakObjectPtr<APlayerState> LastCarrier[3];
};
