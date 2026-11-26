#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GSGameMode.generated.h"

class AGSPlayerState;

/** Capture the Flag on Gloshaugen: two teams, two bases, take the enemy flag and carry it home. */
UCLASS()
class GLOSHAUGEN_API AGSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGSGameMode();

	virtual void BeginPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	/** Cleans the name (letters, digits, space and - _ . only, at most 20) and makes it unique among the players. */
	virtual void ChangeName(AController* Controller, const FString& NewName, bool bNameChange) override;

	void OnPlayerKilled(AController* Killer, AController* Victim, const FString& WeaponName, bool bHeadshot);

	static constexpr float RespawnDelay = 4.f;
	static constexpr int32 MaxPerTeam = 10;
	/** Seconds between two accepted team switches for one player. */
	static constexpr float TeamSwitchCooldown = 2.f;
	/** Seconds between two name changes for one player (a name cannot be swapped in a stream to confuse). */
	static constexpr float NameChangeCooldown = 2.f;

	virtual void Tick(float DeltaSeconds) override;
	/** Lobby: a player toggles ready. */
	void SetReady(AController* Who, bool bReady);
	/** Damage only counts while a match is running (the lobby is warm-up). */
	bool DamageEnabled() const;

	/** Lobby: switch team, only if it does not make the teams more uneven. */
	void RequestTeam(AGSPlayerState* PS, uint8 NewTeam);
	/** A flag was brought home by ScoringTeam (called by AGSFlag). */
	void OnFlagCaptured(uint8 ScoringTeam);
	/** Where team 1 (red) / 2 (blue) has its flag, in meters. The one place that knows where the bases are. */
	static FVector BasePointM(uint8 Team);

protected:
	void StartCountdown(bool bForced);
	void StartMatch();
	void EndMatch();
	void BackToLobby();
	float LobbyStartedAt = 0.f;
	/** The countdown started because the lobby timer ran out: one player cannot stop it. */
	bool bCountdownForced = false;
	int32 LobbyPlayersLast = 0;
	/** The match was started by a single player (stopped when a friend joins). */
	bool bSoloMatch = false;
	FTransform PickSpawnTransform(const AController* For) const;
	void SpawnPickups();
	/** PCs, 3D printers and security cameras in the buildings and around campus. */
	void SpawnTerminals();
	/** One flag at each base. */
	void SpawnFlags();
	/** Puts a player without a team on the smaller team. */
	void AssignTeam(AGSPlayerState* PS);
	int32 TeamCount(uint8 Team) const;
	/** Server world time of each player's last accepted team switch. */
	TMap<TWeakObjectPtr<AGSPlayerState>, float> LastTeamSwitch;
	/** Server world time of each player's last accepted name change (from the player, not the join URL). */
	TMap<TWeakObjectPtr<AController>, float> LastNameChange;
	/** Match start: remove mines, fire, smoke, dropped items and bodies; refill all loot; flags home. */
	void CleanWorldForMatch();
};
