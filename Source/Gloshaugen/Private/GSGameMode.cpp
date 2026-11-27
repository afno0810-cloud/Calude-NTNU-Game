#include "GSGameMode.h"
#include "GSCampusBuilder.h"
#include "GSCharacter.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"
#include "GSGameState.h"
#include "GSHUD.h"
#include "GSPickup.h"
#include "GSTerminal.h"
#include "GSCampus.h"
#include "GSFlag.h"
#include "GSProjectile.h"
#include "GSExplore.h"
#include "GSItems.h"
#include "GSMapState.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AGSGameMode::AGSGameMode()
{
	DefaultPawnClass = AGSCharacter::StaticClass();
	PlayerControllerClass = AGSPlayerController::StaticClass();
	PlayerStateClass = AGSPlayerState::StaticClass();
	GameStateClass = AGSGameState::StaticClass();
	HUDClass = AGSHUD::StaticClass();
	bUseSeamlessTravel = false;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
}

bool AGSGameMode::DamageEnabled() const
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	return !GS || GS->Phase == EGSMatchPhase::Playing;
}

void AGSGameMode::SetReady(AController* Who, bool bReady)
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	AGSPlayerState* PS = Who ? Who->GetPlayerState<AGSPlayerState>() : nullptr;
	if (!GS || !PS || (GS->Phase != EGSMatchPhase::Lobby && GS->Phase != EGSMatchPhase::Countdown)) return;
	PS->bReady = bReady;
	// Someone changed their mind. A countdown started because the lobby time ran out cannot be stopped by one player.
	if (!bReady && GS->Phase == EGSMatchPhase::Countdown && !bCountdownForced)
	{
		GS->Phase = EGSMatchPhase::Lobby;
	}
}

void AGSGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// The engine has already started listening for a listen server here (UEngine::LoadMap: Listen before
	// InitializeActorsForPlay), so the net mode is the real one. Everything is clamped: the URL can come from anyone.
	const FString Mode = UGameplayStatics::ParseOption(Options, TEXT("Mode"));
	uint8 Kind = GSGameKind::Online;
	if (Mode.Equals(TEXT("Bots"), ESearchCase::IgnoreCase)) Kind = GSGameKind::Bots;
	else if (Mode.Equals(TEXT("Explore"), ESearchCase::IgnoreCase)) Kind = GSGameKind::Explore;
	if (Kind != GSGameKind::Online && GetNetMode() != NM_Standalone)
	{
		UE_LOG(LogTemp, Warning, TEXT("GSGameMode: Mode=%s ignoreres: bots og test kartet bare når du spiller alene (nettmodus %d)"), *Mode, int32(GetNetMode()));
		Kind = GSGameKind::Online;
	}
	PendingKind = Kind;
	const bool bBots = Kind == GSGameKind::Bots;
	PendingCaps = uint8(FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Caps"), 3), 1, 10));
	PendingMinutes = uint8(FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Min"), 15), 3, 60));
	bPendingFF = UGameplayStatics::GetIntOption(Options, TEXT("FF"), 0) != 0;
	PendingSkill = uint8(FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Skill"), 1), 0, 3));
	PendingAllies = bBots ? uint8(FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Allies"), 4), 0, 9)) : 0;
	PendingEnemies = bBots ? uint8(FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Enemies"), 5), 1, 10)) : 0;
	PendingTeam = uint8(FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("Team"), 0), 0, 2));
	bPendingKit = Kind != GSGameKind::Explore && UGameplayStatics::GetIntOption(Options, TEXT("Kit"), bBots ? 1 : 0) != 0;
	UE_LOG(LogTemp, Log, TEXT("GSGameMode: spill %s, %d min, %d erobringer, venneild %s, startutstyr %s%s"),
		Kind == GSGameKind::Bots ? TEXT("mot bots") : (Kind == GSGameKind::Explore ? TEXT("test kartet") : TEXT("online")),
		PendingMinutes, PendingCaps, bPendingFF ? TEXT("på") : TEXT("av"), bPendingKit ? TEXT("på") : TEXT("av"),
		bBots ? *FString::Printf(TEXT(", bots %d + %d (nivå %d)%s"), PendingAllies, PendingEnemies, PendingSkill,
			AGSGameState::bBotsAvailable ? TEXT("") : TEXT(" - botene finnes ikke ennå, du spiller alene")) : TEXT(""));
}

void AGSGameMode::InitGameState()
{
	Super::InitGameState();
	AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS) return;
	GS->GameKind = PendingKind;
	GS->CaptureGoal = PendingCaps;
	GS->MatchMinutesSet = PendingMinutes;
	GS->bFriendlyFire = bPendingFF;
	GS->BotSkill = PendingSkill;
	GS->BotAllies = PendingAllies;
	GS->BotEnemies = PendingEnemies;
	GS->bStartKit = bPendingKit;
	if (PendingKind == GSGameKind::Explore) GS->Phase = EGSMatchPhase::Explore;
}

void AGSGameMode::PostLogin(APlayerController* NewPlayer)
{
	// Before Super: Super spawns the body (HandleStartingNewPlayer -> RestartPlayer), and the team decides the base.
	AGSGameState* GS = GetGameState<AGSGameState>();
	if (GS && GS->GameKind == GSGameKind::Bots && !bBotHumanPlaced && NewPlayer)
	{
		if (AGSPlayerState* PS = NewPlayer->GetPlayerState<AGSPlayerState>())
		{
			bBotHumanPlaced = true;
			PS->Team = PendingTeam == 1 ? GSTeam::Red : (PendingTeam == 2 ? GSTeam::Blue : (FMath::RandBool() ? GSTeam::Red : GSTeam::Blue));
			PS->bReady = true;   // the countdown starts at once (Tick)
			PS->ForceNetUpdate();
			UE_LOG(LogTemp, Log, TEXT("GSGameMode: kamp mot bots, du er på %s"), GSTeam::Name(PS->Team));
		}
	}
	Super::PostLogin(NewPlayer);
}

void AGSGameMode::GiveStartKit(AController* C)
{
	AGSCharacter* Char = C ? Cast<AGSCharacter>(C->GetPawn()) : nullptr;
	if (!Char) return;
	if (!bKitLooked)
	{
		// Looked up in the catalog, not by fixed names: the first plain rifle (Common/Uncommon), else the least rare rifle.
		bKitLooked = true;
		const FGSItemDef* Best = nullptr;
		for (const FGSItemDef& D : GSItems::All())
		{
			if (D.IsWeapon() && D.W.Class == EGSWeaponClass::Rifle && (!Best || D.Rarity < Best->Rarity)) Best = &D;
			if (Best && Best->Rarity <= EGSRarity::Uncommon) break;
		}
		if (Best) KitRifle = Best->Id;
		for (const FGSItemDef& D : GSItems::All())
		{
			if (D.Cat == EGSItemCat::Medicine && (D.Id.ToString().Contains(TEXT("Bandage")) || D.Name.Contains(TEXT("Bandasje"))))
			{
				KitBandage = D.Id;
				break;
			}
		}
		UE_LOG(LogTemp, Log, TEXT("GSGameMode: startutstyr = %s, %s, 2 x %s"), *KitRifle.ToString(), *GSItems::Pistol.ToString(), *KitBandage.ToString());
	}
	if (!KitRifle.IsNone()) Char->GiveWeapon(KitRifle);
	Char->GiveWeapon(GSItems::Pistol);
	if (!KitBandage.IsNone()) Char->AddToBackpack(KitBandage, 2);
}

void AGSGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS) return;
	if (GS->Phase == EGSMatchPhase::Explore) return;   // Test kartet: no lobby, no countdown, no match
	const float Now = GS->GetServerWorldTimeSeconds();
	switch (GS->Phase)
	{
	case EGSMatchPhase::Lobby:
	{
		// Start when everyone is ready, or automatically 90 s after the second player came in.
		int32 Total = 0, Ready = 0;
		for (APlayerState* P : GS->PlayerArray)
		{
			if (const AGSPlayerState* PS = Cast<AGSPlayerState>(P)) { ++Total; Ready += PS->bReady ? 1 : 0; }
		}
		if (GS->GameKind == GSGameKind::Bots)
		{
			// Bot match: no waiting for players. Ready (or BotLobbySeconds after the last match) starts a short countdown.
			if (Total == 0) break;
			GS->PhaseEndsAt = LobbyStartedAt + BotLobbySeconds;
			if (Ready == Total || Now >= GS->PhaseEndsAt) StartCountdown(true, BotCountdownSeconds);
			break;
		}
		if (Total >= 2 && LobbyPlayersLast < 2) LobbyStartedAt = Now;   // the 90 s start when player two arrives
		LobbyPlayersLast = Total;
		GS->PhaseEndsAt = Total >= 2 ? LobbyStartedAt + AGSGameState::LobbyAutoStartSeconds : 0.f;
		if (Total > 0 && Ready == Total) StartCountdown(false);
		else if (Total >= 2 && Now - LobbyStartedAt > AGSGameState::LobbyAutoStartSeconds) StartCountdown(true);
		break;
	}
	case EGSMatchPhase::Countdown:
		if (Now >= GS->PhaseEndsAt) StartMatch();
		break;
	case EGSMatchPhase::Playing:
		if (GS->RedScore >= GS->CaptureGoal || GS->BlueScore >= GS->CaptureGoal)
		{
			EndMatch();
		}
		else if (!GS->bOvertime && Now >= GS->PhaseEndsAt)
		{
			if (AnyDecisiveFlagAway())
			{
				// A flag run that can change the result is on: play it out (at most OvertimeMaxSeconds).
				GS->bOvertime = true;
				GS->PhaseEndsAt = Now + AGSGameState::OvertimeMaxSeconds;
				GS->AddKill(TEXT("Tiden er ute"), TEXT("til flagget er hjemme"), TEXT("overtid"), false);
			}
			else
			{
				EndMatch();
			}
		}
		else if (GS->bOvertime && (Now >= GS->PhaseEndsAt || !AnyDecisiveFlagAway()))
		{
			// Captured, brought home, or out of time: the score as it stands decides.
			EndMatch();
		}
		else if (bSoloMatch && GS->PlayerArray.Num() >= 2)
		{
			// A friend arrived during a match started alone: back to the lobby so you can start together.
			BackToLobby();
		}
		break;
	case EGSMatchPhase::Ended:
		if (Now >= GS->PhaseEndsAt) BackToLobby();
		break;
	default:
		break;
	}
}

void AGSGameMode::StartCountdown(bool bForced, float Seconds)
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	GS->Phase = EGSMatchPhase::Countdown;
	GS->PhaseEndsAt = GS->GetServerWorldTimeSeconds() + (Seconds < 0.f ? AGSGameState::CountdownSeconds : Seconds);
	bCountdownForced = bForced;
}

void AGSGameMode::StartMatch()
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	// 1. Clock and scores: everyone starts fresh.
	GS->Phase = EGSMatchPhase::Playing;
	GS->PhaseEndsAt = GS->GetServerWorldTimeSeconds() + float(GS->MatchMinutesSet) * 60.f;
	GS->bOvertime = false;
	GS->Winner.Empty();
	GS->KillFeed.Reset();
	GS->RedScore = 0;
	GS->BlueScore = 0;
	TArray<AGSPlayerState*> States;
	for (APlayerState* P : GS->PlayerArray)
	{
		if (AGSPlayerState* PS = Cast<AGSPlayerState>(P))
		{
			PS->Kills = 0;
			PS->Deaths = 0;
			PS->Captures = 0;
			PS->Returns = 0;
			PS->CarriedFlagTeam = 0;
			PS->SetScore(0.f);
			States.Add(PS);
		}
	}

	// 2. Even teams: nobody without a team, and never more than one player difference.
	//    Not in a bot match: there the menu chose the team (and the bots fill the rest, plan-06).
	const bool bBotMatch = GS->GameKind == GSGameKind::Bots;
	for (AGSPlayerState* PS : States)
	{
		if (PS->Team == GSTeam::None) AssignTeam(PS);
	}
	for (int32 Guard = 0; Guard < 40 && !bBotMatch; ++Guard)
	{
		const int32 Red = TeamCount(GSTeam::Red), Blue = TeamCount(GSTeam::Blue);
		if (FMath::Abs(Red - Blue) <= 1) break;
		const uint8 Big = Red > Blue ? GSTeam::Red : GSTeam::Blue;
		TArray<AGSPlayerState*> Candidates = States.FilterByPredicate([Big](const AGSPlayerState* PS) { return PS->Team == Big; });
		if (Candidates.Num() == 0) break;
		Candidates[FMath::RandRange(0, Candidates.Num() - 1)]->Team = GSTeam::Other(Big);
	}

	// 3. A match started alone is stopped when a friend comes in.
	bSoloMatch = !bBotMatch && States.Num() == 1;

	// 4. Remove all old bodies first (no new bodies yet) ...
	TArray<AController*> Players;
	for (APlayerState* P : GS->PlayerArray)
	{
		if (AController* C = P ? P->GetOwningController() : nullptr) Players.Add(C);
	}
	for (AController* C : Players)
	{
		if (APawn* Old = C->GetPawn())
		{
			C->UnPossess();
			Old->Destroy();
		}
	}
	// 5. ... clean campus ...
	CleanWorldForMatch();
	// 6. ... and only then a new body at the own base, with empty pockets.
	for (AController* C : Players)
	{
		RestartPlayer(C);
	}
}

void AGSGameMode::EndMatch()
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	const int32 Red = GS->RedScore, Blue = GS->BlueScore;
	// Empty = draw.
	GS->Winner = Red > Blue ? FString(GSTeam::Name(GSTeam::Red)) : (Blue > Red ? FString(GSTeam::Name(GSTeam::Blue)) : FString());
	GS->Phase = EGSMatchPhase::Ended;
	GS->bOvertime = false;
	GS->PhaseEndsAt = GS->GetServerWorldTimeSeconds() + AGSGameState::ResultsSeconds;
	for (TActorIterator<AGSFlag> It(GetWorld()); It; ++It) It->ResetHome();
}

void AGSGameMode::BackToLobby()
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	GS->Phase = EGSMatchPhase::Lobby;
	GS->bOvertime = false;
	LobbyStartedAt = GS->GetServerWorldTimeSeconds();
	LobbyPlayersLast = 0;   // the 90 s start again now
	bSoloMatch = false;
	bCountdownForced = false;
	for (APlayerState* P : GS->PlayerArray)
	{
		if (AGSPlayerState* PS = Cast<AGSPlayerState>(P)) PS->bReady = false;
	}
	for (TActorIterator<AGSFlag> It(GetWorld()); It; ++It) It->ResetHome();
}

bool AGSGameMode::AnyDecisiveFlagAway() const
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS) return false;
	for (TActorIterator<AGSFlag> It(GetWorld()); It; ++It)
	{
		const AGSFlag* F = *It;
		if (F->State == EGSFlagState::Home) continue;
		// The attackers are the other team. D = flag owner's score minus the attackers' score: one capture
		// turns a draw (0) into a win, or one behind (1) into a draw. Ahead already, or two behind: it changes nothing.
		const int32 D = GS->TeamScore(F->Team) - GS->TeamScore(GSTeam::Other(F->Team));
		if (D == 0 || D == 1) return true;
	}
	return false;
}

void AGSGameMode::Logout(AController* Exiting)
{
	// The engine has already removed the pawn (APlayerController::Destroyed -> PawnLeavingGame); the player state lives.
	AGSGameState* GS = GetGameState<AGSGameState>();
	APlayerState* PS = Exiting ? Exiting->PlayerState.Get() : nullptr;
	if (GS && PS)
	{
		for (TActorIterator<AGSFlag> It(GetWorld()); It; ++It) It->DropIfCarriedBy(PS);   // the flag stays in the match
		if (GS->Phase == EGSMatchPhase::Playing) GS->AddKill(PS->GetPlayerName(), TEXT("kampen"), TEXT("forlot"), false);
	}
	Super::Logout(Exiting);
}

void AGSGameMode::OnFlagCaptured(uint8 ScoringTeam)
{
	AGSGameState* GS = GetGameState<AGSGameState>();
	if (!GS || GS->Phase != EGSMatchPhase::Playing) return;
	if (ScoringTeam == GSTeam::Red) GS->RedScore++;
	else if (ScoringTeam == GSTeam::Blue) GS->BlueScore++;
	// The end of the match is checked in Tick.
}

int32 AGSGameMode::TeamCount(uint8 Team) const
{
	int32 N = 0;
	if (const AGSGameState* GS = GetGameState<AGSGameState>())
	{
		for (APlayerState* P : GS->PlayerArray)
		{
			const AGSPlayerState* PS = Cast<AGSPlayerState>(P);
			if (PS && PS->Team == Team) ++N;
		}
	}
	return N;
}

void AGSGameMode::AssignTeam(AGSPlayerState* PS)
{
	if (!PS) return;
	const int32 Red = TeamCount(GSTeam::Red), Blue = TeamCount(GSTeam::Blue);
	PS->Team = Red < Blue ? GSTeam::Red : (Blue < Red ? GSTeam::Blue : (FMath::RandBool() ? GSTeam::Red : GSTeam::Blue));
	PS->ForceNetUpdate();
}

void AGSGameMode::RequestTeam(AGSPlayerState* PS, uint8 NewTeam)
{
	const AGSGameState* GS = GetGameState<AGSGameState>();
	if (!PS || !GS || GS->Phase != EGSMatchPhase::Lobby) return;
	if (NewTeam != GSTeam::Red && NewTeam != GSTeam::Blue) return;
	const uint8 Old = PS->Team;
	if (NewTeam == Old) return;
	// Never make the teams more uneven.
	const int32 NewCount = TeamCount(NewTeam);
	const int32 OldCount = Old == GSTeam::None ? TeamCount(GSTeam::Other(NewTeam)) + 1 : TeamCount(Old);
	if (NewCount >= OldCount || NewCount >= MaxPerTeam) return;
	// Every switch destroys and respawns the pawn: at most one accepted switch per player every TeamSwitchCooldown.
	const float Now = GetWorld()->GetTimeSeconds();
	if (const float* Last = LastTeamSwitch.Find(PS))
	{
		if (Now - *Last < TeamSwitchCooldown) return;
	}
	if (LastTeamSwitch.Num() > 64)
	{
		for (auto It = LastTeamSwitch.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid()) It.RemoveCurrent();
		}
	}
	LastTeamSwitch.Add(PS, Now);

	PS->Team = NewTeam;
	PS->bReady = false;
	PS->ForceNetUpdate();
	// Move to the new base
	if (AController* C = PS->GetOwningController())
	{
		if (APawn* P = C->GetPawn())
		{
			C->UnPossess();
			P->Destroy();
		}
		RestartPlayer(C);
	}
}

FVector AGSGameMode::BasePointM(uint8 Team)
{
	// Bases from the map data: index 0 = west (Gamle fysikk) is red, index 1 = east (Gamle kjemi) is blue.
	const TArray<GSCampus::FBase>& Bases = GSCampus::Bases();
	const int32 Index = Team == GSTeam::Blue ? 1 : 0;
	if (Bases.IsValidIndex(Index)) return Bases[Index].Flag;
	return Team == GSTeam::Blue ? FVector(60.f, -80.f, 0.f) : FVector(-60.f, -80.f, 0.f);
}

void AGSGameMode::SpawnFlags()
{
	UWorld* World = GetWorld();
	for (uint8 Team = GSTeam::Red; Team <= GSTeam::Blue; ++Team)
	{
		const FVector Base = BasePointM(Team) * 100.f;
		FVector Loc(Base.X, Base.Y, Base.Z);
		FHitResult Hit;
		// From 2 m above the flag point, so an indoor base never starts the trace inside the ceiling (see PickSpawnTransform).
		if (World->LineTraceSingleByObjectType(Hit, Base + FVector(0.f, 0.f, 200.f), Base - FVector(0.f, 0.f, 300.f), FCollisionObjectQueryParams(ECC_WorldStatic)))
		{
			Loc = Hit.ImpactPoint;
		}
		const TArray<GSCampus::FBase>& Bases = GSCampus::Bases();
		const int32 Index = Team == GSTeam::Blue ? 1 : 0;
		const float Yaw = Bases.IsValidIndex(Index) ? Bases[Index].SpawnYaw : 0.f;
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AGSFlag* Flag = World->SpawnActor<AGSFlag>(Loc, FRotator(0.f, Yaw, 0.f), P))
		{
			Flag->Team = Team;
			Flag->ResetHome();
		}
	}
}

void AGSGameMode::CleanWorldForMatch()
{
	UWorld* World = GetWorld();
	// Collect first, destroy afterwards (never destroy while iterating).
	TArray<AActor*> Doomed;
	for (TActorIterator<AGSProjectile> It(World); It; ++It) Doomed.Add(*It);   // mines, fire, smoke, grenades, bullets
	AGSMapState::ResetForMatch(World);   // doors, locks and broken glass as new (Interaktiv resets too; twice is harmless)
	for (TActorIterator<AGSPickup> It(World); It; ++It)
	{
		if (!It->bRespawns) Doomed.Add(*It);   // dropped items and loot from the dead
		else It->ResetForMatch();
	}
	for (TActorIterator<AGSTerminal> It(World); It; ++It) It->CancelPrint();
	for (TActorIterator<AGSFlag> It(World); It; ++It) It->ResetHome();
	for (TActorIterator<AGSCharacter> It(World); It; ++It) Doomed.Add(*It);   // bodies still lying around
	for (AActor* A : Doomed)
	{
		if (IsValid(A)) A->Destroy();
	}
}

void AGSGameMode::BeginPlay()
{
	Super::BeginPlay();
	AGSGameState* GS = GetGameState<AGSGameState>();
	// Second check (the editor's Play may start listening later than a packaged game): bots and test only alone.
	if (GS && GS->GameKind != GSGameKind::Online && GetNetMode() != NM_Standalone)
	{
		UE_LOG(LogTemp, Warning, TEXT("GSGameMode: bots/test kartet slått av: nettmodus %d er ikke alene"), int32(GetNetMode()));
		GS->GameKind = GSGameKind::Online;
		GS->BotAllies = GS->BotEnemies = 0;
		GS->bStartKit = false;
		if (GS->Phase == EGSMatchPhase::Explore) GS->Phase = EGSMatchPhase::Lobby;
	}
	SpawnPickups();
	SpawnTerminals();
	SpawnFlags();   // campus is built before this (UGSWorldSubsystem::OnWorldBeginPlay); in the test they only mark the bases
	if (GS && GS->GameKind == GSGameKind::Explore)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AGSExplore>(FVector::ZeroVector, FRotator::ZeroRotator, P);
		UE_LOG(LogTemp, Log, TEXT("GSGameMode: Test kartet - ingen kamp, ingen skade"));
	}
}

namespace GSGameModeImpl
{
	/** Smallest room-acoustics box (Helgasetr) that holds a floor point (m), or nullptr. The point is lifted 0.3 m so a
	 *  spot on a floor belongs to the room above that floor, not to the one below whose ceiling it is. */
	const TCHAR* HelgasetrRoomAt(const FVector& PM)
	{
		const TCHAR* Best = nullptr;
		float BestVolume = TNumericLimits<float>::Max();
		const float Z = PM.Z + 0.3f;
		for (const GSHelgasetr::FSoundZoneM& S : GSHelgasetr::SoundZones())
		{
			if (PM.X < S.X0 || PM.X > S.X1 || PM.Y < S.Y0 || PM.Y > S.Y1 || Z < S.Z0 || Z > S.Z1) continue;
			const float Volume = (S.X1 - S.X0) * (S.Y1 - S.Y0) * (S.Z1 - S.Z0);
			if (Volume < BestVolume) { BestVolume = Volume; Best = S.Name; }
		}
		return Best;
	}

	/**
	 * Helgasetr loot from the map data (GSHelgasetr::LootSpots()): what a spot holds follows the room it is in, how far it
	 * is from both bases (further = rarer) and its tier. Both teams get the same worth within reach. Server only.
	 */
	void SpawnPickupsHelgasetr(UWorld* World, FRandomStream& Rng)
	{
		struct FSpot { FVector PM; int32 Tier; EGSLootPlace Place; float Depth; float DistRed; float DistBlue; int32 Level; };
		const TConstArrayView<GSHelgasetr::FBaseM> Bases = GSHelgasetr::Bases();
		if (Bases.Num() < 2) { UE_LOG(LogTemp, Warning, TEXT("Helgasetr-loot: mangler baser i kartdataene")); return; }
		const FVector FlagRed(Bases[0].X, Bases[0].Y, Bases[0].Z);    // Sørbasen = rødt lag (index 0, like GSCampus::Bases())
		const FVector FlagBlue(Bases[1].X, Bases[1].Y, Bases[1].Z);   // Nordbasen = blått lag

		TArray<FSpot> Spots;
		int32 NearDoor = 0, Upgraded = 0;
		for (const GSHelgasetr::FLootM& L : GSHelgasetr::LootSpots())
		{
			FSpot S;
			S.PM = FVector(L.X, L.Y, L.Z);
			S.Tier = FMath::Clamp<int32>(L.Tier, 0, 2);
			S.Level = GSHelgasetr::LevelAt(L.Z);
			S.DistRed = FVector::Dist(S.PM, FlagRed);
			S.DistBlue = FVector::Dist(S.PM, FlagBlue);
			S.Depth = FMath::Clamp((FMath::Min(S.DistRed, S.DistBlue) - 12.f) / 16.f, 0.f, 1.f);

			const TCHAR* SpotRoom = HelgasetrRoomAt(S.PM);
			for (const GSHelgasetr::FDoorM& D : GSHelgasetr::DoorSlots())
			{
				if (D.Level != S.Level) continue;
				const float Dist2D = FVector2D::Distance(FVector2D(S.PM), FVector2D(D.X, D.Y));
				// Rooms behind a locked code door (glass_kode) hold one tier better (a crate stays a crate). Only on the
				// room side: a door faces out of its room (Yaw = the front, helgasetr_plan.py), so the room lies behind
				// it. The spot must also be in the same room box (HelgasetrRoomAt) as a point 1 m inside the door, so a
				// spot past the room in the atrium does not count. The data flag bLocked decides here, not
				// AGSDoor::MakeDefault (which locks every GlassCode door; with the 4b data the two agree).
				if (S.Tier < 1 && D.Kind == 1 && D.bLocked && Dist2D <= 4.f)
				{
					const FVector2D Front(FMath::Cos(FMath::DegreesToRadians(D.Yaw)), FMath::Sin(FMath::DegreesToRadians(D.Yaw)));
					const float Behind = -FVector2D::DotProduct(FVector2D(S.PM) - FVector2D(D.X, D.Y), Front);
					const TCHAR* Inside = HelgasetrRoomAt(FVector(D.X - Front.X, D.Y - Front.Y, D.Z));
					const bool bSameRoom = (SpotRoom == nullptr && Inside == nullptr)
						|| (SpotRoom && Inside && FCString::Strcmp(SpotRoom, Inside) == 0);
					if (Behind >= 0.3f && bSameRoom) { S.Tier = 1; ++Upgraded; }
				}
				if (Dist2D < 0.8f)
				{
					++NearDoor;
					UE_LOG(LogTemp, Warning, TEXT("Helgasetr-loot: plassen (%.1f, %.1f, %.1f) ligger %.2f m fra døra %d (%s)"),
						L.X, L.Y, L.Z, Dist2D, D.Id, D.Room ? D.Room : TEXT(""));
				}
			}

			const FString RoomName = SpotRoom ? SpotRoom : TEXT("");
			if (S.Tier >= 2) S.Place = EGSLootPlace::Crate;
			else if (RoomName == TEXT("Laberadene") || RoomName == TEXT("Østfløyens rom")) S.Place = EGSLootPlace::Medical;
			else if (RoomName == TEXT("Kantina") || RoomName == TEXT("Studentarbeidsplassene")) S.Place = EGSLootPlace::Canteen;
			else S.Place = EGSLootPlace::Corridor;
			Spots.Add(S);
		}

		// Balance: a spot belongs to a side when it is at least 3 m closer to that side's flag. Worth = (1 + 2 tier)(1 + depth).
		auto Worth = [](const FSpot& S) { return (1.f + 2.f * S.Tier) * (1.f + S.Depth); };
		float Red = 0.f, Blue = 0.f;
		auto Measure = [&]()
		{
			Red = Blue = 0.f;
			for (const FSpot& S : Spots)
			{
				if (S.DistRed <= S.DistBlue - 3.f) Red += Worth(S);
				else if (S.DistBlue <= S.DistRed - 3.f) Blue += Worth(S);
			}
		};
		Measure();
		int32 Adjusted = 0;
		for (int32 Round = 0; Round < 4 && FMath::Abs(Red - Blue) > 0.1f * FMath::Max(Red, Blue); ++Round)
		{
			// The richer side gives up its better spot nearest its own base (least felt by the team that loses it)
			const bool bRedRicher = Red > Blue;
			int32 Best = INDEX_NONE;
			float BestDist = TNumericLimits<float>::Max();
			for (int32 I = 0; I < Spots.Num(); ++I)
			{
				const FSpot& S = Spots[I];
				if (S.Tier != 1) continue;
				const bool bSide = bRedRicher ? S.DistRed <= S.DistBlue - 3.f : S.DistBlue <= S.DistRed - 3.f;
				const float Own = bRedRicher ? S.DistRed : S.DistBlue;
				if (bSide && Own < BestDist) { BestDist = Own; Best = I; }
			}
			if (Best == INDEX_NONE) break;
			Spots[Best].Tier = 0;
			++Adjusted;
			Measure();
		}

		// Put each thing on the floor (or the table) under its spot. Object trace: only the world blocks, not players.
		int32 Total = 0, NoFloor = 0, Counts[3] = {}, Places[5] = {}, PerLevel[GSHelgasetr::NumLevels] = {};
		const FCollisionObjectQueryParams WorldOnly(ECC_WorldStatic);
		FCollisionQueryParams Query(SCENE_QUERY_STAT(GSHelgasetrLoot), false);
		for (const FSpot& S : Spots)
		{
			const FVector P = S.PM * 100.f;
			FHitResult Hit;
			if (!World->LineTraceSingleByObjectType(Hit, P + FVector(0.f, 0.f, 150.f), P - FVector(0.f, 0.f, 150.f), WorldOnly, Query)
				|| Hit.bStartPenetrating)   // a trace that starts inside a block (a stair, a wall) "hits" where it starts
			{
				++NoFloor;
				UE_LOG(LogTemp, Warning, TEXT("Helgasetr-loot: ingen gulv under (%.1f, %.1f, %.1f)%s"), S.PM.X, S.PM.Y, S.PM.Z,
					Hit.bStartPenetrating ? TEXT(" (sporet starter inne i noe)") : TEXT(""));
				continue;
			}
			const FVector Loc(P.X, P.Y, Hit.ImpactPoint.Z + 60.f);
			// The final spot must be free: nothing a player bumps into within 15 cm (the floor is 60 cm below)
			TArray<FOverlapResult> Overlaps;
			World->OverlapMultiByObjectType(Overlaps, Loc, FQuat::Identity, WorldOnly, FCollisionShape::MakeSphere(15.f), Query);
			const FOverlapResult* Blocking = Overlaps.FindByPredicate([](const FOverlapResult& O)
			{
				const UPrimitiveComponent* C = O.GetComponent();
				return C && C->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
			});
			if (Blocking)
			{
				++NoFloor;
				UE_LOG(LogTemp, Warning, TEXT("Helgasetr-loot: plassen (%.1f, %.1f, %.1f) ligger inne i noe (%s)"), S.PM.X, S.PM.Y, S.PM.Z,
					Blocking->GetComponent() ? *Blocking->GetComponent()->GetName() : TEXT("?"));
				continue;
			}
			FActorSpawnParameters SP;
			SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AGSPickup* Pickup = World->SpawnActor<AGSPickup>(Loc, FRotator::ZeroRotator, SP);
			if (!Pickup) continue;
			Pickup->Rng.Initialize(Rng.RandHelper(1 << 30));
			Pickup->bIsCrate = S.Tier >= 2;
			Pickup->MinRarity = S.Tier >= 2 ? (S.Depth >= 0.8f ? EGSRarity::Epic : EGSRarity::Rare)
				: S.Tier == 1 ? EGSRarity::Uncommon : EGSRarity::Common;
			Pickup->MaxRarity = EGSRarity::Legendary;
			Pickup->Place = S.Place;
			Pickup->Depth = S.Depth;
			Pickup->Roll();
			++Total;
			++Counts[S.Tier];
			++Places[(int32)S.Place];
			++PerLevel[FMath::Clamp(S.Level, 0, GSHelgasetr::NumLevels - 1)];
		}
		UE_LOG(LogTemp, Log, TEXT("Helgasetr-loot: %d plasser (%d vanlige, %d bedre, %d kasser), %d uten gulv eller inne i noe; korridor %d, medisinsk %d, kantine %d; %d bedre bak låst kodelås; balanse rød %.1f / blå %.1f (%d justert)"),
			Total, Counts[0], Counts[1], Counts[2], NoFloor, Places[(int32)EGSLootPlace::Corridor], Places[(int32)EGSLootPlace::Medical],
			Places[(int32)EGSLootPlace::Canteen], Upgraded, Red, Blue, Adjusted);
		UE_LOG(LogTemp, Log, TEXT("Helgasetr-loot per etasje: 1. %d, 2. %d, 3. %d, 4. %d; %d plasser nærmere enn 0,8 m en dør"),
			PerLevel[0], PerLevel[1], PerLevel[2], PerLevel[3], NearDoor);
	}
}

void AGSGameMode::SpawnPickups()
{
	FRandomStream Rng(FMath::Rand());
	if constexpr (GSCampus::bHelgasetr)
	{
		GSGameModeImpl::SpawnPickupsHelgasetr(GetWorld(), Rng);   // no mythic crate and no outdoor loot on Helgasetr
	}
	else   // the old campus
	{
		int32 Total = 0;
		auto Spawn = [&](const FVector& LocM, bool bCrate, EGSRarity MinRarity)
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const FVector Loc = LocM * 100.f + FVector(0.f, 0.f, 60.f);
			if (AGSPickup* Pickup = GetWorld()->SpawnActor<AGSPickup>(Loc, FRotator::ZeroRotator, P))
			{
				Pickup->Rng.Initialize(Rng.RandHelper(1 << 30));
				Pickup->bIsCrate = bCrate;
				Pickup->MinRarity = MinRarity;
				Pickup->Roll();
				++Total;
			}
		};

		// Loot inside every building, on every floor. Stay away from walls and the stairwell corner (+X +Y).
		const TArray<GSCampus::FBuilding>& Buildings = GSCampus::Buildings();
		const bool bCustomMain = AGSCampusBuilder::MainBuildingMesh() != nullptr;
		for (const GSCampus::FBuilding& B : Buildings)
		{
			if (!B.bEnterable) continue;
			const bool bMain = bCustomMain && FString(B.Name) == TEXT("Hovedbygningen");
			const int32 Floors = FMath::Max(1, FMath::FloorToInt(B.Height / B.FloorHeight + 0.01f));
			const FTransform BT(FRotator(0.f, B.Yaw, 0.f), FVector(B.X, B.Y, 0.f));
			const float HX = B.SizeX * 0.5f - 2.f, HY = B.SizeY * 0.5f - 2.f;
			const int32 PerFloor = FMath::Clamp(FMath::RoundToInt(B.SizeX * B.SizeY / 450.f), 2, 9);
			for (int32 F = 0; F < Floors; ++F)
			{
				for (int32 I = 0; I < PerFloor; ++I)
				{
					float X = 0.f, Y = 0.f;
					for (int32 Try = 0; Try < 10; ++Try)
					{
						X = Rng.FRandRange(-HX, HX);
						Y = Rng.FRandRange(-HY, HY);
						if (bMain)
						{
							// Hovedbygningen model: no loot in the air over Lysgaarden, on the grand stair or on the ramps
							const bool bAtrium = FMath::Abs(X) < 15.f && FMath::Abs(Y) < 10.f && (F > 0 || (FMath::Abs(X) < 4.5f && Y > -0.5f));
							const bool bRamp = FMath::Abs(X) > 38.f && Y > 9.5f;
							if (!bAtrium && !bRamp) break;
						}
						else if (!(X > HX - 16.f && Y > HY - 5.f)) break;
					}
					Spawn(BT.TransformPosition(FVector(X, Y, F * B.FloorHeight + 0.35f)), false, EGSRarity::Common);
				}
			}
			// A locked crate with good stuff on the top floor of the bigger buildings
			if (Floors >= 3 || B.SizeX * B.SizeY > 4000.f)
			{
				Spawn(BT.TransformPosition(FVector(Rng.FRandRange(-HX, 0.f), Rng.FRandRange(-HY, 0.f), (Floors - 1) * B.FloorHeight + 0.35f)), true,
					Floors >= 8 ? EGSRarity::Legendary : EGSRarity::Epic);
			}
		}
		// The big one: a mythic crate in the middle of Lysgaarden
		Spawn(bCustomMain ? FVector(0.f, -7.5f, 0.85f) : FVector(0.f, 4.f, 0.35f), true, EGSRarity::Mythic); // on the stage

		// Outdoors: scattered over the play area (5 m in from the edge), not inside buildings
		const FBox2D Area = GSCampus::PlayArea().ExpandBy(-5.f);
		for (int32 I = 0; I < 20; ++I)
		{
			for (int32 Try = 0; Try < 20; ++Try)
			{
				const FVector2D P(Rng.FRandRange(Area.Min.X, Area.Max.X), Rng.FRandRange(Area.Min.Y, Area.Max.Y));
				bool bInside = false;
				for (const GSCampus::FBuilding& B : Buildings)
				{
					if (FMath::Abs(P.X - B.X) < B.SizeX * 0.5f + 3.f && FMath::Abs(P.Y - B.Y) < B.SizeY * 0.5f + 3.f) { bInside = true; break; }
				}
				if (!bInside) { Spawn(FVector(P.X, P.Y, 0.f), false, EGSRarity::Common); break; }
			}
		}
		UE_LOG(LogTemp, Log, TEXT("Gloshaugen: spawned %d loot spots"), Total);
	}
}

FTransform AGSGameMode::PickSpawnTransform(const AController* For) const
{
	UWorld* World = GetWorld();
	const AGSPlayerState* PS = For ? For->GetPlayerState<AGSPlayerState>() : nullptr;
	const uint8 Team = PS && PS->Team != GSTeam::None ? PS->Team : GSTeam::Red;
	const FVector Base = BasePointM(Team) * 100.f;
	const FVector EnemyBase = BasePointM(GSTeam::Other(Team)) * 100.f;
	const FVector Away = (Base - EnemyBase).GetSafeNormal2D();

	// Candidates: the base's own spawn points from the map data, or a fan behind the base if there are none.
	TArray<FVector> Candidates;
	float Yaw = (EnemyBase - Base).Rotation().Yaw;   // look towards the enemy
	const TArray<GSCampus::FBase>& Bases = GSCampus::Bases();
	const int32 BaseIndex = Team == GSTeam::Blue ? 1 : 0;
	if (Bases.IsValidIndex(BaseIndex) && Bases[BaseIndex].Spawns.Num() > 0)
	{
		for (const FVector2D& S : Bases[BaseIndex].Spawns) Candidates.Add(FVector(S.X * 100.f, S.Y * 100.f, Base.Z));
		Yaw = Bases[BaseIndex].SpawnYaw;
	}
	else
	{
		for (int32 I = 0; I < 12; ++I)
		{
			const float A = -100.f + 200.f * I / 11.f;
			const float R = (I % 2 == 0) ? 700.f : 1300.f;
			Candidates.Add(Base + Away.RotateAngleAxis(A, FVector::UpVector) * R);
		}
	}

	TArray<FVector> Living;
	for (TActorIterator<AGSCharacter> It(World); It; ++It)
	{
		if (!It->bDead) Living.Add(It->GetActorLocation());
	}

	// On the ground, room for a body, and nobody standing there. The trace starts only 2 m above the point: indoors
	// (Helgasetr: floor 13.9 m, the deck above from 17.85 m) a higher start begins inside the ceiling and hits its
	// underside. Object trace: only the world counts, not a body standing on the spot.
	auto TryPlace = [&](FVector Loc, FVector& Out) -> bool
	{
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Loc + FVector(0.f, 0.f, 200.f), Loc - FVector(0.f, 0.f, 300.f), FCollisionObjectQueryParams(ECC_WorldStatic)))
		{
			Loc.Z = Hit.ImpactPoint.Z + 100.f;
		}
		else
		{
			Loc.Z = Base.Z + 120.f;
		}
		if (World->OverlapBlockingTestByChannel(Loc, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(42.f, 92.f))) return false;
		for (const FVector& L : Living)
		{
			if (FVector::Dist(L, Loc) < 150.f) return false;
		}
		Out = Loc;
		return true;
	};

	TArray<FVector> Valid;
	for (const FVector& C : Candidates)
	{
		FVector Loc;
		if (TryPlace(C, Loc)) Valid.Add(Loc);
	}
	if (Valid.Num() == 0)
	{
		// All taken: try a little to the side of each one.
		for (const FVector& C : Candidates)
		{
			const FVector Side = FVector(1.f, 0.f, 0.f).RotateAngleAxis(FMath::FRandRange(0.f, 360.f), FVector::UpVector) * FMath::FRandRange(200.f, 350.f);
			FVector Loc;
			if (TryPlace(C + Side, Loc)) { Valid.Add(Loc); break; }
		}
	}
	const FVector Loc = Valid.Num() > 0 ? Valid[FMath::RandRange(0, Valid.Num() - 1)] : Base + FVector(0.f, 0.f, 120.f);
	return FTransform(FRotator(0.f, Yaw, 0.f), Loc);
}

void AGSGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer || NewPlayer->IsPendingKillPending()) return;
	// The team is set before the body is made, so it spawns at the right base.
	AGSPlayerState* PS = NewPlayer->GetPlayerState<AGSPlayerState>();
	const AGSGameState* KindGS = GetGameState<AGSGameState>();
	const bool bExplore = KindGS && KindGS->GameKind == GSGameKind::Explore;
	if (bExplore)
	{
		// Test kartet: no teams; PickSpawnTransform puts a player without a team at the red base.
		RestartPlayerAtTransform(NewPlayer, PickSpawnTransform(NewPlayer));
		if (!bExploreStartLogged)
		{
			bExploreStartLogged = true;
			UE_LOG(LogTemp, Log, TEXT("Test kartet: startet ved rød base"));
		}
		return;
	}
	if (PS && PS->Team == GSTeam::None)
	{
		AssignTeam(PS);
		// Came in during a match (StartMatch sets every team before it respawns anyone, so this is a real late
		// arrival): everyone sees "Kari [ble med på] rødt lag". 2 s later, so the name from the client is there.
		const AGSGameState* GS = GetGameState<AGSGameState>();
		if (GS && GS->Phase == EGSMatchPhase::Playing)
		{
			TWeakObjectPtr<AGSPlayerState> WeakPS = PS;
			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, WeakPS]()
			{
				AGSPlayerState* P = WeakPS.Get();
				AGSGameState* G = GetGameState<AGSGameState>();
				if (!P || !G || G->Phase != EGSMatchPhase::Playing || P->Team == GSTeam::None) return;
				G->AddKill(P->GetPlayerName(), FString(GSTeam::Name(P->Team)).ToLower(), TEXT("ble med på"), false);
			}), 2.f, false);
		}
	}
	RestartPlayerAtTransform(NewPlayer, PickSpawnTransform(NewPlayer));
	// Start kit (bot match by default): only for a match, not for warming up in the lobby.
	if (KindGS && KindGS->bStartKit && (KindGS->Phase == EGSMatchPhase::Playing || KindGS->Phase == EGSMatchPhase::Countdown))
	{
		GiveStartKit(NewPlayer);
	}
}

void AGSGameMode::OnPlayerKilled(AController* Killer, AController* Victim, const FString& WeaponName, bool bHeadshot)
{
	AGSPlayerState* KillerPS = Killer ? Killer->GetPlayerState<AGSPlayerState>() : nullptr;
	AGSPlayerState* VictimPS = Victim ? Victim->GetPlayerState<AGSPlayerState>() : nullptr;

	if (VictimPS) VictimPS->Deaths++;
	// Killing a team mate gives no kill and no points.
	const bool bTeamKill = KillerPS && VictimPS && KillerPS != VictimPS && KillerPS->Team != GSTeam::None && KillerPS->Team == VictimPS->Team;
	if (KillerPS && KillerPS != VictimPS && !bTeamKill)
	{
		KillerPS->Kills++;
		KillerPS->SetScore(KillerPS->GetScore() + 1.f);
	}

	if (AGSGameState* GS = GetGameState<AGSGameState>())
	{
		GS->AddKill(KillerPS ? KillerPS->GetPlayerName() : TEXT("Verden"),
			VictimPS ? VictimPS->GetPlayerName() : TEXT("?"), WeaponName, bHeadshot);
	}

	if (Victim)
	{
		TWeakObjectPtr<AController> WeakVictim = Victim;
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, WeakVictim]()
		{
			if (AController* C = WeakVictim.Get())
			{
				if (!C->GetPawn()) RestartPlayer(C);
			}
		}), RespawnDelay, false);
	}
}

void AGSGameMode::SpawnTerminals()
{
	UWorld* World = GetWorld();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	int32 NumPC = 0, NumPrinters = 0, NumCams = 0;

	auto Terminal = [&](EGSTerminalKind Kind, const FVector& LocM, float Yaw, const FString& Label)
	{
		if (AGSTerminal* T = World->SpawnActor<AGSTerminal>(LocM * 100.f, FRotator(0.f, Yaw, 0.f), P))
		{
			T->Kind = Kind;
			T->Label = Label;
			Kind == EGSTerminalKind::Computer ? ++NumPC : ++NumPrinters;
		}
	};
	auto Camera = [&](const FVector& LocM, float Yaw, const FString& Label, float Sweep = 35.f)
	{
		if (AGSSecurityCamera* C = World->SpawnActor<AGSSecurityCamera>(LocM * 100.f, FRotator(0.f, Yaw, 0.f), P))
		{
			C->Label = Label;
			C->SweepDeg = Sweep;
			++NumCams;
		}
	};

	if constexpr (GSCampus::bHelgasetr)
	{
		// Helgasetr: the PCs, 3D printers and cameras stand in real rooms from the map data (GSHelgasetr.h)
		for (const GSHelgasetr::FTerminalM& T : GSHelgasetr::Terminals())
		{
			Terminal(T.Kind == 1 ? EGSTerminalKind::Printer : EGSTerminalKind::Computer, FVector(T.X, T.Y, T.Z), T.Yaw, T.Name);
		}
		for (const GSHelgasetr::FCameraM& C : GSHelgasetr::Cameras())
		{
			// Over the open atrium, the canteen and the mezzanine a camera sweeps wider than in a corridor
			const FString Name(C.Name);
			const bool bWide = Name.Contains(TEXT("Atriet")) || Name.Contains(TEXT("Kantina")) || Name.Contains(TEXT("Mesaninen"));
			Camera(FVector(C.X, C.Y, C.Z), C.Yaw, Name, bWide ? 50.f : 35.f);
		}
		UE_LOG(LogTemp, Log, TEXT("Helgasetr: %d PC-er, %d 3D-printere, %d kameraer"), NumPC, NumPrinters, NumCams);
	}
	else   // the old campus
	{
		const bool bCustomMain = AGSCampusBuilder::MainBuildingMesh() != nullptr;
		for (const GSCampus::FBuilding& B : GSCampus::Buildings())
		{
			const FString Name(B.Name);
			const FTransform BT(FRotator(0.f, B.Yaw, 0.f), FVector(B.X, B.Y, 0.f));
			const float HX = B.SizeX * 0.5f, HY = B.SizeY * 0.5f;

			if (bCustomMain && Name == TEXT("Hovedbygningen"))
			{
				// Hand-made model: corridors at y = +-5, Lysgaarden in the middle, tower behind.
				Terminal(EGSTerminalKind::Computer, FVector(-30.f, -4.5f, 0.f), 90.f, TEXT("Hovedbygningen, vaktrommet"));
				Terminal(EGSTerminalKind::Computer, FVector(30.f, 4.5f, 11.f), -90.f, TEXT("Hovedbygningen, 3. etasje"));
				Terminal(EGSTerminalKind::Printer, FVector(30.f, -4.5f, 5.5f), 90.f, TEXT("Hovedbygningen, verkstedet"));
				Terminal(EGSTerminalKind::Printer, FVector(-30.f, 4.5f, 16.5f), -90.f, TEXT("Hovedbygningen, 4. etasje"));
				Camera(FVector(-13.3f, -8.3f, 4.6f), 35.f, TEXT("Lysg\u00E5rden, scenen"));
				Camera(FVector(13.3f, 8.3f, 4.6f), -145.f, TEXT("Lysg\u00E5rden, trappa"));
				Camera(FVector(0.f, -19.f, 6.5f), -90.f, TEXT("Hovedinngangen"), 50.f);
				Camera(FVector(0.f, 13.f, 3.6f), -90.f, TEXT("Korridoren mot t\u00E5rnet"));
				Camera(FVector(-50.f, -4.f, 4.6f), 0.f, TEXT("Hovedbygningen vest, korridor"));
				Camera(FVector(50.f, 4.f, 4.6f), 180.f, TEXT("Hovedbygningen \u00F8st, korridor"));
				Camera(FVector(4.f, 24.f, 35.f), 90.f, TEXT("T\u00E5rnet, utsikt s\u00F8r"), 70.f);
				Camera(FVector(-4.f, 16.f, 35.f), -90.f, TEXT("T\u00E5rnet, utsikt nord"), 70.f);
				continue;
			}
			if (bCustomMain && Name == TEXT("Hovedbygningen taarn")) continue;
			if (!B.bEnterable) continue;

			const int32 Floors = FMath::Max(1, FMath::FloorToInt(B.Height / B.FloorHeight + 0.01f));
			// PC against the -X wall on the ground floor, facing into the room.
			Terminal(EGSTerminalKind::Computer, BT.TransformPosition(FVector(-HX + 1.0f, -HY * 0.4f, 0.f)), B.Yaw, Name);
			// 3D printer on the first floor of the bigger buildings
			if (Floors >= 2 && B.SizeX * B.SizeY > 1200.f)
			{
				Terminal(EGSTerminalKind::Printer, BT.TransformPosition(FVector(-HX + 0.9f, HY * 0.4f, B.FloorHeight)), B.Yaw,
					FString::Printf(TEXT("%s, 2. etasje"), *Name));
			}
			// One camera inside (high in the -X -Y corner looking across the room) and one outside the front
			Camera(BT.TransformPosition(FVector(-HX + 0.7f, -HY + 0.7f, FMath::Min(B.FloorHeight - 0.6f, 3.6f))), B.Yaw + 40.f,
				FString::Printf(TEXT("%s, inne"), *Name));
			Camera(BT.TransformPosition(FVector(0.f, -HY - 0.4f, FMath::Min(B.Height - 1.f, 6.f))), B.Yaw - 90.f,
				FString::Printf(TEXT("%s, utenfor"), *Name), 45.f);
		}
		UE_LOG(LogTemp, Log, TEXT("Gloshaugen: %d PC-er, %d 3D-printere, %d kameraer"), NumPC, NumPrinters, NumCams);
	}
}

void AGSGameMode::ChangeName(AController* Controller, const FString& NewName, bool bNameChange)
{
	if (!Controller || !Controller->PlayerState) return;
	// Letters (also æøå), digits, space and - _ . only: no control, URL or invisible characters (U+200B, U+00A0),
	// one space at a time. Same rule as the menu's name field (GSMenu.cpp CleanName).
	FString Clean;
	for (int32 I = 0; I < NewName.Len(); ++I)
	{
		const TCHAR Ch = NewName[I];
		const bool bOk = Ch == TEXT(' ') || Ch == TEXT('-') || Ch == TEXT('_') || Ch == TEXT('.') || (Ch > 32 && FChar::IsAlnum(Ch));
		if (!bOk || (Ch == TEXT(' ') && Clean.EndsWith(TEXT(" ")))) continue;
		Clean.AppendChar(Ch);
	}
	Clean.TrimStartAndEndInline();
	Clean.LeftInline(20);
	Clean.TrimEndInline();
	if (Clean.IsEmpty()) return;   // keep the old name
	if (Clean.Equals(Controller->PlayerState->GetPlayerName(), ESearchCase::CaseSensitive)) return;   // nothing to change

	// Changes asked for by the player (not the name from the join URL): one every NameChangeCooldown.
	if (bNameChange)
	{
		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		if (const float* Last = LastNameChange.Find(Controller))
		{
			if (Now - *Last < NameChangeCooldown) return;
		}
		if (LastNameChange.Num() > 64)
		{
			for (auto It = LastNameChange.CreateIterator(); It; ++It)
			{
				if (!It.Key().IsValid()) It.RemoveCurrent();
			}
		}
		LastNameChange.Add(Controller, Now);
	}

	// Unique among the players: "Kari", "Kari 2", "Kari 3" ...
	const AGameStateBase* GS = GameState;
	auto Taken = [&](const FString& Name)
	{
		if (!GS) return false;
		for (const APlayerState* P : GS->PlayerArray)
		{
			if (P && P != Controller->PlayerState && P->GetPlayerName().Equals(Name, ESearchCase::IgnoreCase)) return true;
		}
		return false;
	};
	FString Final = Clean;
	for (int32 N = 2; N < 100 && Taken(Final); ++N)
	{
		const FString Suffix = FString::Printf(TEXT(" %d"), N);
		Final = Clean.Left(20 - Suffix.Len()).TrimEnd() + Suffix;
	}
	Super::ChangeName(Controller, Final, bNameChange);
}
