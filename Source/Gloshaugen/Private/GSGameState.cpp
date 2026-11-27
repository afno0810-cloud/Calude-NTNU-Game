#include "GSGameState.h"
#include "GSHelp.h"
#include "GSMenu.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

AGSGameState::AGSGameState()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSGameState, Weather);
	DOREPLIFETIME(AGSGameState, PreviousWeather);
	DOREPLIFETIME(AGSGameState, WeatherChangedAt);
	DOREPLIFETIME(AGSGameState, StartHour);
	DOREPLIFETIME(AGSGameState, ClockStartedAt);
	DOREPLIFETIME(AGSGameState, KillFeed);
	DOREPLIFETIME(AGSGameState, Phase);
	DOREPLIFETIME(AGSGameState, PhaseEndsAt);
	DOREPLIFETIME(AGSGameState, Winner);
	DOREPLIFETIME(AGSGameState, RedScore);
	DOREPLIFETIME(AGSGameState, BlueScore);
	DOREPLIFETIME(AGSGameState, bFriendlyFire);
	DOREPLIFETIME(AGSGameState, bOvertime);
}

void AGSGameState::MulticastFlagEvent_Implementation(uint8 Event, uint8 FlagTeam, FVector_NetQuantize Where, APlayerState* Who)
{
	LastFlagEvent.Event = Event;
	LastFlagEvent.FlagTeam = FlagTeam;
	LastFlagEvent.Where = Where;
	LastFlagEvent.Who = Who ? Who->GetPlayerName() : FString();
	LastFlagEvent.LocalTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	OnFlagEvent.Broadcast(Event, FlagTeam, LastFlagEvent.Where, Who);
}

EGSWeather AGSGameState::RollWeather(EGSWeather Avoid) const
{
	// Trondheim weights: plenty of grey, some rain, snow in winter.
	struct FW { EGSWeather W; float Weight; };
	const FW Table[] = {
		{ EGSWeather::Clear, 3.f }, { EGSWeather::Cloudy, 3.f }, { EGSWeather::Fog, 1.5f },
		{ EGSWeather::Rain, 2.5f }, { EGSWeather::Snow, 2.f } };
	float Total = 0.f;
	for (const FW& E : Table) if (E.W != Avoid) Total += E.Weight;
	float R = FMath::FRandRange(0.f, Total);
	for (const FW& E : Table)
	{
		if (E.W == Avoid) continue;
		R -= E.Weight;
		if (R <= 0.f) return E.W;
	}
	return EGSWeather::Cloudy;
}

void AGSGameState::BeginPlay()
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		GSHelp::EnsureHooked();
		GSMenu::EnsureHooked();   // after GSHelp: the menu is drawn on top
	}
	Super::BeginPlay();
	if (HasAuthority())
	{
		const float Now = GetServerWorldTimeSeconds();
		StartHour = FMath::FRandRange(0.f, 24.f);
		ClockStartedAt = Now;
		Weather = RollWeather(EGSWeather::MAX);
		PreviousWeather = Weather;
		WeatherChangedAt = Now - WeatherBlendSeconds;
		NextWeatherChange = Now + WeatherInterval * FMath::FRandRange(0.7f, 1.3f);
		UE_LOG(LogTemp, Log, TEXT("Gloshaugen: start %.1f h, weather %s"), StartHour, GSNames::Weather(Weather));
	}
}

void AGSGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority()) return;

	const float Now = GetServerWorldTimeSeconds();
	if (Now >= NextWeatherChange)
	{
		PreviousWeather = Weather;
		Weather = RollWeather(Weather);
		WeatherChangedAt = Now;
		NextWeatherChange = Now + WeatherInterval * FMath::FRandRange(0.7f, 1.3f);
	}

	// Keep only recent kill feed entries.
	KillFeed.RemoveAll([Now](const FGSKillFeedEntry& E) { return Now - E.ServerTime > 12.f; });
}

float AGSGameState::GetTimeOfDayHours() const
{
	const float Elapsed = GetServerWorldTimeSeconds() - ClockStartedAt;
	const float Hours = StartHour + Elapsed / (DayLengthMinutes * 60.f) * 24.f;
	return FMath::Fmod(Hours, 24.f);
}

float AGSGameState::GetWeatherBlend() const
{
	return FMath::Clamp((GetServerWorldTimeSeconds() - WeatherChangedAt) / WeatherBlendSeconds, 0.f, 1.f);
}

void AGSGameState::AddKill(const FString& Killer, const FString& Victim, const FString& Weapon, bool bHeadshot)
{
	FGSKillFeedEntry E;
	E.Killer = Killer;
	E.Victim = Victim;
	E.Weapon = Weapon;
	E.bHeadshot = bHeadshot;
	E.ServerTime = GetServerWorldTimeSeconds();
	KillFeed.Add(E);
	if (KillFeed.Num() > 6) KillFeed.RemoveAt(0);
}
