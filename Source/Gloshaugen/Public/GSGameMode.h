#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GSGameMode.generated.h"

/** Free roam deathmatch on Gloshaugen: run around, find weapons, shoot each other, respawn. */
UCLASS()
class GLOSHAUGEN_API AGSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGSGameMode();

	virtual void BeginPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	void OnPlayerKilled(AController* Killer, AController* Victim, const FString& WeaponName, bool bHeadshot);

	static constexpr float RespawnDelay = 4.f;

	virtual void Tick(float DeltaSeconds) override;
	/** Lobby: a player toggles ready. */
	void SetReady(AController* Who, bool bReady);
	/** Damage only counts while a match is running (the lobby is warm-up). */
	bool DamageEnabled() const;

protected:
	void StartCountdown();
	void StartMatch();
	void EndMatch();
	void BackToLobby();
	float LobbyStartedAt = 0.f;
	FTransform PickSpawnTransform() const;
	void SpawnPickups();
	/** PCs, 3D printers and security cameras in the buildings and around campus. */
	void SpawnTerminals();
};
