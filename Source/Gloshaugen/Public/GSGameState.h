#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GSTypes.h"
#include "GSGameState.generated.h"

UCLASS()
class GLOSHAUGEN_API AGSGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AGSGameState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Current in-game clock, 0-24. Same on every machine. */
	float GetTimeOfDayHours() const;
	/** 0..1 blend from the previous weather to the current one. */
	float GetWeatherBlend() const;

	void AddKill(const FString& Killer, const FString& Victim, const FString& Weapon, bool bHeadshot);

	UPROPERTY(Replicated) EGSWeather Weather = EGSWeather::Clear;
	UPROPERTY(Replicated) EGSWeather PreviousWeather = EGSWeather::Clear;
	UPROPERTY(Replicated) float WeatherChangedAt = -1000.f;   // server world time
	UPROPERTY(Replicated) float StartHour = 12.f;
	UPROPERTY(Replicated) float ClockStartedAt = 0.f;        // server world time
	UPROPERTY(Replicated) TArray<FGSKillFeedEntry> KillFeed;

	/** Match flow: Lobby (warm-up, no damage, press Enter when ready) -> Countdown -> Playing -> Ended -> Lobby. */
	UPROPERTY(Replicated) EGSMatchPhase Phase = EGSMatchPhase::Lobby;
	UPROPERTY(Replicated) float PhaseEndsAt = 0.f;     // server world time (countdown / match / results)
	UPROPERTY(Replicated) FString Winner;
	static constexpr float MatchMinutes = 15.f;
	static constexpr int32 KillLimit = 25;
	static constexpr float CountdownSeconds = 10.f;
	static constexpr float ResultsSeconds = 15.f;
	/** Seconds left of the current phase (countdown, match or results). */
	float PhaseSecondsLeft() const { return FMath::Max(0.f, PhaseEndsAt - GetServerWorldTimeSeconds()); }

	/** Real minutes for one full in-game day. */
	static constexpr float DayLengthMinutes = 48.f;
	/** Weather changes roughly this often (seconds). */
	static constexpr float WeatherInterval = 420.f;
	static constexpr float WeatherBlendSeconds = 40.f;

protected:
	float NextWeatherChange = 0.f;
	EGSWeather RollWeather(EGSWeather Avoid) const;
};
