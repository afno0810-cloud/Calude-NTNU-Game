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

	// --- Capture the Flag ---
	UPROPERTY(Replicated) uint8 RedScore = 0;
	UPROPERTY(Replicated) uint8 BlueScore = 0;
	/** Off: team mates cannot hurt each other (checked in AGSCharacter::TakeDamage by the combat team). */
	UPROPERTY(Replicated) bool bFriendlyFire = false;
	int32 TeamScore(uint8 Team) const { return Team == 1 ? RedScore : (Team == 2 ? BlueScore : 0); }
	static constexpr int32 CaptureLimit = 3;
	static constexpr float LobbyAutoStartSeconds = 90.f;
	/** Time is out but a flag run that can still decide the match is on. */
	UPROPERTY(Replicated) bool bOvertime = false;
	static constexpr float OvertimeMaxSeconds = 120.f;
	/** After this long with the flag, the flag's own team hears where the carrier was last seen ... */
	static constexpr float CarrierRevealSeconds = 20.f;
	/** ... and again this often while he keeps it. */
	static constexpr float CarrierRevealInterval = 5.f;

	/** Event: 1 taken, 2 dropped, 3 returned, 4 captured (GSFlagEvent in GSFlag.h). Runs on every machine. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFlagEvent(uint8 Event, uint8 FlagTeam, FVector_NetQuantize Where, APlayerState* Who);
	/** Last flag event on this machine (for the status line and for sounds). LocalTime = GetWorld()->GetTimeSeconds(). */
	struct FLastFlagEvent { uint8 Event = 0; uint8 FlagTeam = 0; FVector Where = FVector::ZeroVector; FString Who; float LocalTime = -100.f; };
	FLastFlagEvent LastFlagEvent;
	DECLARE_MULTICAST_DELEGATE_FourParams(FOnFlagEvent, uint8 /*Event*/, uint8 /*FlagTeam*/, const FVector& /*Where*/, APlayerState* /*Who*/);
	/** The combat team binds sounds / HUD cues here. */
	FOnFlagEvent OnFlagEvent;

	/** Real minutes for one full in-game day. */
	static constexpr float DayLengthMinutes = 48.f;
	/** Weather changes roughly this often (seconds). */
	static constexpr float WeatherInterval = 420.f;
	static constexpr float WeatherBlendSeconds = 40.f;

protected:
	float NextWeatherChange = 0.f;
	EGSWeather RollWeather(EGSWeather Avoid) const;
};
