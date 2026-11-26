#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GSPlayerState.generated.h"

/** Capture the Flag teams. 0 = none yet. */
namespace GSTeam
{
	constexpr uint8 None = 0, Red = 1, Blue = 2;
	inline uint8 Other(uint8 T) { return T == Red ? Blue : (T == Blue ? Red : None); }
	inline const TCHAR* Name(uint8 T) { return T == Red ? TEXT("Rødt lag") : (T == Blue ? TEXT("Blått lag") : TEXT("Uten lag")); }
	inline const TCHAR* FlagName(uint8 T) { return T == Red ? TEXT("det røde flagget") : TEXT("det blå flagget"); }
	/** Sober cloth colours, not neon. */
	inline FLinearColor Color(uint8 T) { return T == Red ? FLinearColor(0.55f, 0.07f, 0.06f) : (T == Blue ? FLinearColor(0.07f, 0.17f, 0.50f) : FLinearColor(0.5f, 0.5f, 0.5f)); }
}

UCLASS()
class GLOSHAUGEN_API AGSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadOnly) int32 Kills = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 Deaths = 0;
	/** Lobby: this player pressed Enter and is ready for the match. */
	UPROPERTY(Replicated, BlueprintReadOnly) bool bReady = false;

	/** Capture the Flag team (GSTeam: 0 none, 1 red, 2 blue). Set by the server before the pawn spawns. */
	UPROPERTY(Replicated, BlueprintReadOnly) uint8 Team = 0;
	/** Team of the flag this player carries, 0 = carries nothing. */
	UPROPERTY(Replicated, BlueprintReadOnly) uint8 CarriedFlagTeam = 0;
	/** Flags brought home to the own base this match. */
	UPROPERTY(Replicated, BlueprintReadOnly) int32 Captures = 0;
	/** Own dropped flags walked back home this match. */
	UPROPERTY(Replicated, BlueprintReadOnly) int32 Returns = 0;

	/** Lobby: ask to switch team (allowed only when it does not make the teams more uneven). */
	UFUNCTION(Server, Reliable) void ServerRequestTeam(uint8 NewTeam);
};
