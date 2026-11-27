#include "GSBots.h"
#include "GSBotController.h"
#include "GSBotTeam.h"
#include "GSNavSetup.h"
#include "GSGameMode.h"
#include "GSGameState.h"
#include "GSPlayerState.h"
#include "GSCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "HAL/IConsoleManager.h"

DEFINE_STAT(STAT_GSBotThink);
DEFINE_LOG_CATEGORY_STATIC(LogGSBots, Log, All);

namespace GSBotsImpl
{
	// Lett, Middels, Vanskelig, Ekspert. No level gets more health, damage or knowledge; no night factor indoors.
	static const FGSBotSkill Table[4] = {
		//  Name                 React        Turn   Err   Half  Lead  Recoil Head   Fov    Sight Burst Hear Cover Retreat
		{ TEXT("Lett"),          0.9f, 1.3f,  120.f, 4.0f, 0.9f, 0.6f, 0.3f,  0.03f, 100.f, 35.f, 8.f,  25.f, 0.2f, 0.f },
		{ TEXT("Middels"),       0.55f, 0.8f, 200.f, 2.5f, 0.6f, 0.4f, 0.5f,  0.08f, 110.f, 45.f, 12.f, 35.f, 0.5f, 30.f },
		{ TEXT("Vanskelig"),     0.35f, 0.5f, 320.f, 1.4f, 0.4f, 0.2f, 0.7f,  0.15f, 120.f, 55.f, 15.f, 45.f, 0.8f, 40.f },
		{ TEXT("Ekspert"),       0.22f, 0.32f, 450.f, 0.8f, 0.3f, 0.1f, 0.85f, 0.25f, 120.f, 60.f, 18.f, 50.f, 0.95f, 45.f },
	};

	static const TCHAR* Names[] = {
		TEXT("Ola"), TEXT("Kari"), TEXT("Ingrid"), TEXT("Magnus"), TEXT("Sigrid"), TEXT("Jonas"), TEXT("Nora"), TEXT("Emil"),
		TEXT("Thea"), TEXT("Henrik"), TEXT("Ida"), TEXT("Sander"), TEXT("Maja"), TEXT("Lars"), TEXT("Hanna"), TEXT("Even"),
		TEXT("Silje"), TEXT("Kristian"), TEXT("Marte"), TEXT("Eirik"), TEXT("Synne"), TEXT("Håkon"), TEXT("Tuva"), TEXT("Anders"),
		TEXT("Live"), TEXT("Sindre"), TEXT("Vilde"), TEXT("Torbjørn"), TEXT("Astrid"), TEXT("Ole"),
	};

	static bool bDebug = false;
	static bool bThink = true;
	constexpr int32 SightBudget = 48;

	static FAutoConsoleCommandWithWorldAndArgs CmdFill(TEXT("gs.bots.fill"),
		TEXT("gs.bots.fill <venner> <fiender> [grad 0-3]: bots på ditt lag og motstanderlaget (bot-kamp eller test kartet, alene)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UGSBotsSubsystem* S = UGSBotsSubsystem::Get(World);
			if (!S) return;
			const int32 A = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 4;
			const int32 E = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 5;
			const int32 L = Args.Num() > 2 ? FCString::Atoi(*Args[2]) : -1;
			S->ForceFill(A, E, L);
		}));

	static FAutoConsoleCommandWithWorldAndArgs CmdDebug(TEXT("gs.bots.debug"),
		TEXT("gs.bots.debug 0/1: tegn botenes rolle, mål, sti og via-punktene"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
		{
			bDebug = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !bDebug;
			UE_LOG(LogGSBots, Log, TEXT("Bots: tegning %s"), bDebug ? TEXT("på") : TEXT("av"));
		}));

	static FAutoConsoleCommandWithWorldAndArgs CmdThink(TEXT("gs.bots.think"),
		TEXT("gs.bots.think 0/1: 0 = botene står stille uten å tenke (ytelsesmåling)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
		{
			bThink = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !bThink;
			UE_LOG(LogGSBots, Log, TEXT("Bots: tenking %s"), bThink ? TEXT("på") : TEXT("av"));
		}));
}

namespace GSBots
{
	const FGSBotSkill& Skill(uint8 Level) { return GSBotsImpl::Table[FMath::Min<int32>(Level, 3)]; }
	bool IsDebug() { return GSBotsImpl::bDebug; }
	bool IsThinking() { return GSBotsImpl::bThink; }
}

bool UGSBotsSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UGSBotsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Whether this is a bot match is checked in Step: GameKind is set by the game mode, and the order of the world
	// subsystems is not fixed.
	InWorld.GetTimerManager().SetTimer(StepTimer, FTimerDelegate::CreateUObject(this, &UGSBotsSubsystem::Step), 0.25f, true, 0.25f);
	WindowStart = FPlatformTime::Seconds();
	WindowFrame = GFrameCounter;
}

void UGSBotsSubsystem::Deinitialize()
{
	if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(StepTimer);
	Super::Deinitialize();
}

UGSBotsSubsystem* UGSBotsSubsystem::Get(const UObject* Ctx)
{
	const UWorld* W = Ctx ? Ctx->GetWorld() : nullptr;
	return W ? W->GetSubsystem<UGSBotsSubsystem>() : nullptr;
}

bool UGSBotsSubsystem::IsActive() const
{
	const UWorld* W = GetWorld();
	if (!W || W->GetNetMode() != NM_Standalone) return false;
	const AGSGameState* GS = W->GetGameState<AGSGameState>();
	if (!GS) return false;
	return GS->GameKind == GSGameKind::Bots || bForced;
}

void UGSBotsSubsystem::ForceFill(int32 Allies, int32 Enemies, int32 Level)
{
	UWorld* W = GetWorld();
	AGSGameState* GS = W ? W->GetGameState<AGSGameState>() : nullptr;
	if (!GS || W->GetNetMode() != NM_Standalone)
	{
		UE_LOG(LogGSBots, Warning, TEXT("gs.bots.fill: bare når du spiller alene (bot-kamp eller test kartet)"));
		return;
	}
	GS->BotAllies = uint8(FMath::Clamp(Allies, 0, 9));
	GS->BotEnemies = uint8(FMath::Clamp(Enemies, 0, 10));
	if (Level >= 0) GS->BotSkill = uint8(FMath::Clamp(Level, 0, 3));
	bForced = true;
	bLoggedTable = false;
	UE_LOG(LogGSBots, Log, TEXT("gs.bots.fill: %d venner, %d fiender, nivå %s"), GS->BotAllies, GS->BotEnemies, GSBots::Skill(GS->BotSkill).Name);
}

bool UGSBotsSubsystem::TakeSightTraces(int32 N)
{
	if (BudgetFrame != GFrameCounter) { BudgetFrame = GFrameCounter; BudgetUsed = 0; }
	if (BudgetUsed + N > GSBotsImpl::SightBudget) return false;
	BudgetUsed += N;
	return true;
}

AGSBotTeam* UGSBotsSubsystem::GetTeam(uint8 Team)
{
	if (Team < 1 || Team > 2) return nullptr;
	if (!Teams[Team])
	{
		FActorSpawnParameters P;
		P.ObjectFlags |= RF_Transient;
		Teams[Team] = GetWorld()->SpawnActor<AGSBotTeam>(P);
		if (Teams[Team]) Teams[Team]->Team = Team;
	}
	return Teams[Team];
}

FString UGSBotsSubsystem::PickName() const
{
	TSet<FString> Used;
	if (const AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>())
	{
		for (const APlayerState* P : GS->PlayerArray) if (P) Used.Add(P->GetPlayerName());
	}
	TArray<int32> Order;
	for (int32 I = 0; I < UE_ARRAY_COUNT(GSBotsImpl::Names); ++I) Order.Add(I);
	for (int32 I = Order.Num() - 1; I > 0; --I) Order.Swap(I, FMath::RandRange(0, I));
	for (int32 I : Order)
	{
		const FString N = FString(GSBotsImpl::Names[I]) + TEXT(" (bot)");
		if (!Used.Contains(N)) return N;
	}
	return FString::Printf(TEXT("Bot %d (bot)"), NextIndex + 1);
}

AGSBotController* UGSBotsSubsystem::SpawnBot(uint8 Team)
{
	UWorld* W = GetWorld();
	AGSGameMode* GM = W->GetAuthGameMode<AGSGameMode>();
	AGSGameState* GS = W->GetGameState<AGSGameState>();
	if (!GM || !GS) return nullptr;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	P.ObjectFlags |= RF_Transient;
	AGSBotController* Bot = W->SpawnActor<AGSBotController>(AGSBotController::StaticClass(), FTransform::Identity, P);
	if (!Bot) return nullptr;
	// bWantsPlayerState: the player state came with the controller (GameMode's class) and is already in PlayerArray.
	// Ready and team before the game mode's next Tick counts the lobby.
	AGSPlayerState* PS = Bot->GetPlayerState<AGSPlayerState>();
	if (PS)
	{
		PS->SetIsABot(true);
		PS->SetPlayerName(PickName());
		PS->Team = Team;
		PS->bReady = true;
	}
	Bot->Skill = GS->BotSkill;
	Bot->BotIndex = NextIndex++;
	Bots.Add(Bot);
	if (AGSBotTeam* T = GetTeam(Team)) T->Register(Bot);
	GM->RestartPlayer(Bot);
	UE_LOG(LogGSBots, Log, TEXT("Bots: %s med på %s (nivå %s)%s"), PS ? *PS->GetPlayerName() : TEXT("?"), GSTeam::Name(Team),
		GSBots::Skill(Bot->Skill).Name, Bot->GetPawn() ? TEXT("") : TEXT(" - ingen kropp ennå"));
	return Bot;
}

void UGSBotsSubsystem::RemoveBot(uint8 Team)
{
	for (int32 I = Bots.Num() - 1; I >= 0; --I)
	{
		AGSBotController* B = Bots[I].Get();
		if (!B || B->GetTeamId() != Team) continue;
		if (AGSBotTeam* T = GetTeam(Team)) T->Unregister(B);
		if (APawn* Pw = B->GetPawn()) { B->UnPossess(); Pw->Destroy(); }
		B->Destroy();
		Bots.RemoveAt(I);
		return;
	}
}

void UGSBotsSubsystem::Step()
{
	if (!IsActive()) return;
	UWorld* W = GetWorld();
	AGSGameState* GS = W->GetGameState<AGSGameState>();

	if (!bNavAsked)
	{
		bNavAsked = true;
		GSNavSetup::EnsureNavMesh(W);
	}

	// Think time, every 30 s.
	if (FPlatformTime::Seconds() - WindowStart >= 30.0) LogThinkTime();

	// The human (the only one who is not a bot).
	const AGSPlayerState* Human = nullptr;
	for (APlayerState* P : GS->PlayerArray)
	{
		const AGSPlayerState* PS = Cast<AGSPlayerState>(P);
		if (PS && !PS->IsABot()) { Human = PS; break; }
	}
	if (!Human) return;
	const bool bExplore = GS->Phase == EGSMatchPhase::Explore;
	if (!bExplore && Human->Team == GSTeam::None) return;
	const uint8 HumanTeam = Human->Team != GSTeam::None ? Human->Team : GSTeam::Red;

	if (!bLoggedTable)
	{
		bLoggedTable = true;
		LogSkillTable();
	}
	if (!bFlagLogBound)
	{
		// Flag events in the log (a headless run can show takes, returns and captures).
		bFlagLogBound = true;
		GS->OnFlagEvent.AddWeakLambda(this, [](uint8 Event, uint8 FlagTeam, const FVector& Where, APlayerState* Who)
		{
			static const TCHAR* Ev[] = { TEXT("?"), TEXT("tatt"), TEXT("mistet"), TEXT("levert tilbake"), TEXT("erobret") };
			UE_LOG(LogGSBots, Log, TEXT("Flagg: %s %s av %s ved (%.1f, %.1f, %.1f) m"), GSTeam::FlagName(FlagTeam), Ev[FMath::Min<int32>(Event, 4)],
				Who ? *Who->GetPlayerName() : TEXT("ingen"), Where.X / 100.f, Where.Y / 100.f, Where.Z / 100.f);
		});
	}

	Bots.RemoveAll([](const TWeakObjectPtr<AGSBotController>& B) { return !B.IsValid(); });
	int32 Have[3] = { 0, 0, 0 };
	for (const TWeakObjectPtr<AGSBotController>& B : Bots) Have[FMath::Min<int32>(B->GetTeamId(), 2)]++;
	const int32 Want[3] = { 0, HumanTeam == GSTeam::Red ? GS->BotAllies : GS->BotEnemies, HumanTeam == GSTeam::Blue ? GS->BotAllies : GS->BotEnemies };

	// Adding is fine in every phase but the results (a bot that comes in during a match spawns with the start kit);
	// removing only before a match.
	const bool bMayAdd = GS->Phase != EGSMatchPhase::Ended;
	const bool bMayRemove = GS->Phase == EGSMatchPhase::Lobby || GS->Phase == EGSMatchPhase::Countdown || bExplore;
	for (uint8 T = GSTeam::Red; T <= GSTeam::Blue; ++T)
	{
		if (Have[T] < Want[T] && bMayAdd)
		{
			SpawnBot(T);
			return;   // one per step: no hitch
		}
		if (Have[T] > Want[T] && bMayRemove)
		{
			RemoveBot(T);
			return;
		}
	}

	if (uint8(GS->Phase) != LastPhase)
	{
		if (GS->Phase == EGSMatchPhase::Playing)
		{
			UE_LOG(LogGSBots, Log, TEXT("Bots: kampen starter, rødt lag %d bots, blått lag %d bots, du er på %s"), Have[1], Have[2], GSTeam::Name(HumanTeam));
			LogSkillTable();
		}
		LastPhase = uint8(GS->Phase);
	}
}

void UGSBotsSubsystem::LogSkillTable() const
{
	const AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>();
	const uint8 L = GS ? GS->BotSkill : 1;
	const FGSBotSkill& S = GSBots::Skill(L);
	UE_LOG(LogGSBots, Log, TEXT("Bots: nivå %s - reaksjon %.2f-%.2f s, sving %.0f grader/s, siktefeil %.1f grader (halveres hver %.2f s), forsprangfeil %.2f, rekylkontroll %.2f, hode %.0f %%, synsfelt %.0f grader / %.0f m, holder avtrekkeren under %.0f m"),
		S.Name, S.ReactMin, S.ReactMax, S.TurnDegPerSec, S.AimErrorDeg, S.AimSettleHalfLife, S.LeadError, S.RecoilControl, S.HeadChance * 100.f,
		S.ViewFovDeg, S.SightM, S.BurstShortM);
}

void UGSBotsSubsystem::LogThinkTime()
{
	const double Now = FPlatformTime::Seconds();
	const uint64 Frames = FMath::Max<uint64>(GFrameCounter - WindowFrame, 1);
	int32 Alive = 0;
	for (const TWeakObjectPtr<AGSBotController>& B : Bots) if (B.IsValid() && B->GetPawn()) ++Alive;
	if (Bots.Num() > 0)
	{
		UE_LOG(LogGSBots, Log, TEXT("Bots: tenketid snitt %.3f ms per bilde for %d bots (%d i live), lengste enkelttenking %.3f ms, %llu bilder på %.0f s%s"),
			ThinkThisWindow * 1000.0 / double(Frames), Bots.Num(), Alive, ThinkMaxBot * 1000.0, Frames, Now - WindowStart,
			GSBots::IsThinking() ? TEXT("") : TEXT(" (tenking av)"));
	}
	// What they did: how many moved more than 5 m, the roles, shots, kills and captures (proof in a headless run).
	int32 Moved = 0, Roles[4] = { 0, 0, 0, 0 }, Kills = 0, Deaths = 0, Caps = 0;
	float Far = 0.f;
	for (const TWeakObjectPtr<AGSBotController>& B : Bots)
	{
		if (!B.IsValid()) continue;
		Roles[FMath::Clamp(int32(B->Role), 0, 3)]++;
		if (const AGSPlayerState* PS = B->GetPlayerState<AGSPlayerState>()) { Kills += PS->Kills; Deaths += PS->Deaths; Caps += PS->Captures; }
		const APawn* Pw = B->GetPawn();
		if (!Pw) continue;
		if (const FVector* Old = LastPos.Find(B))
		{
			const float D = FVector::Dist(*Old, Pw->GetActorLocation());
			Far += D;
			Moved += D > 500.f ? 1 : 0;
		}
		LastPos.Add(B, Pw->GetActorLocation());
	}
	if (Bots.Num() > 0)
	{
		UE_LOG(LogGSBots, Log, TEXT("Bots: %d av %d flyttet seg over 5 m (til sammen %.0f m), roller: %d angriper, %d forsvarer, %d henter, %d bærer; %d skudd, drap %d, døde %d, erobringer %d"),
			Moved, Bots.Num(), Far / 100.f, Roles[0], Roles[1], Roles[2], Roles[3], ShotsWindow, Kills, Deaths, Caps);
	}
	ShotsWindow = 0;
	ThinkThisWindow = 0.0;
	ThinkMaxBot = 0.0;
	WindowStart = Now;
	WindowFrame = GFrameCounter;
}
