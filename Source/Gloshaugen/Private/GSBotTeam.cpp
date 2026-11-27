#include "GSBotTeam.h"
#include "GSBotController.h"
#include "GSBots.h"
#include "GSNavSetup.h"
#include "GSCharacter.h"
#include "GSFlag.h"
#include "GSGameState.h"
#include "GSPlayerState.h"
#include "EngineUtils.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogGSBotTeam, Log, All);

AGSBotTeam::AGSBotTeam()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
	bReplicates = false;
}

void AGSBotTeam::Register(AGSBotController* Bot)
{
	if (Bot) Bots.AddUnique(Bot);
	bAssignNow = true;
}

void AGSBotTeam::Unregister(AGSBotController* Bot)
{
	Bots.Remove(Bot);
	ViaOf.Remove(Bot);
	DefOf.Remove(Bot);
	bAssignNow = true;
}

AGSFlag* AGSBotTeam::OwnFlag() const
{
	if (!CachedOwn.IsValid() || !CachedEnemy.IsValid())
	{
		for (TActorIterator<AGSFlag> It(GetWorld()); It; ++It)
		{
			if (It->Team == Team) CachedOwn = *It;
			else if (It->Team == GSTeam::Other(Team)) CachedEnemy = *It;
		}
	}
	return CachedOwn.Get();
}

AGSFlag* AGSBotTeam::EnemyFlag() const
{
	OwnFlag();
	return CachedEnemy.Get();
}

bool AGSBotTeam::OwnFlagBelief(FVector& Out) const
{
	const AGSFlag* F = OwnFlag();
	if (!F) return false;
	switch (F->State)
	{
	case EGSFlagState::Dropped: Out = F->DropLocation; return true;   // the HUD shows where it lies
	case EGSFlagState::Carried:
		if (F->bRevealed) { Out = F->RevealLocation; return true; }   // the radio report, as for a human
		break;
	default: break;
	}
	// Not reported yet: the base itself (the carrier has to leave it).
	Out = F->GetActorLocation();
	return true;
}

FVector AGSBotTeam::EnemyFlagGoal() const
{
	const AGSFlag* F = EnemyFlag();
	if (!F) return FVector::ZeroVector;
	if (F->State == EGSFlagState::Dropped) return F->DropLocation;
	return F->GetActorLocation();
}

bool AGSBotTeam::TeamCarriesEnemyFlag() const
{
	const AGSFlag* F = EnemyFlag();
	const AGSPlayerState* C = F && F->State == EGSFlagState::Carried ? Cast<AGSPlayerState>(F->Carrier) : nullptr;
	return C && C->Team == Team;
}

void AGSBotTeam::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority()) return;
	WatchFlags();
	AssignAccum += DeltaSeconds;
	if (bAssignNow || AssignAccum >= 2.f)
	{
		AssignAccum = 0.f;
		bAssignNow = false;
		Assign();
	}
}

void AGSBotTeam::WatchFlags()
{
	for (TActorIterator<AGSFlag> It(GetWorld()); It; ++It)
	{
		const uint8 T = FMath::Min<uint8>(It->Team, 2);
		const uint8 S = uint8(It->State);
		if (S != LastFlagState[T] || LastCarrier[T].Get() != It->Carrier)
		{
			LastFlagState[T] = S;
			LastCarrier[T] = It->Carrier;
			bAssignNow = true;
		}
	}
}

void AGSBotTeam::Assign()
{
	Bots.RemoveAll([](const TWeakObjectPtr<AGSBotController>& B) { return !B.IsValid(); });
	const AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>();
	if (!GS || Bots.Num() == 0) return;
	int32 Humans = 0;
	for (const APlayerState* P : GS->PlayerArray)
	{
		const AGSPlayerState* PS = Cast<AGSPlayerState>(P);
		if (PS && !PS->IsABot() && PS->Team == Team) ++Humans;
	}
	const int32 N = Bots.Num();
	const int32 Size = N + Humans;
	const int32 WantDef = Size <= 2 ? 0 : (Size <= 5 ? 1 : (Size <= 8 ? 2 : 3));

	const AGSFlag* Own = OwnFlag();
	const FVector OwnBase = Own ? Own->GetActorLocation() : FVector::ZeroVector;
	const bool bOwnAway = Own && Own->State != EGSFlagState::Home;

	TArray<AGSBotController*> Free;
	for (const TWeakObjectPtr<AGSBotController>& W : Bots)
	{
		AGSBotController* B = W.Get();
		const AGSPlayerState* PS = B->GetPlayerState<AGSPlayerState>();
		if (PS && PS->CarriedFlagTeam != 0) { B->SetRole(EGSBotRole::CarryHome); continue; }
		Free.Add(B);
	}

	// Defenders: keep the ones there are, then the bots nearest the own flag.
	TArray<AGSBotController*> Def;
	for (AGSBotController* B : Free) if (B->Role == EGSBotRole::Defend && Def.Num() < WantDef) Def.Add(B);
	if (Def.Num() < WantDef)
	{
		TArray<AGSBotController*> ByDist = Free.FilterByPredicate([&Def](AGSBotController* B) { return !Def.Contains(B); });
		ByDist.Sort([&OwnBase](const AGSBotController& A, const AGSBotController& B)
		{
			const APawn* PA = A.GetPawn();
			const APawn* PB = B.GetPawn();
			const float DA = PA ? FVector::DistSquared(PA->GetActorLocation(), OwnBase) : BIG_NUMBER;
			const float DB = PB ? FVector::DistSquared(PB->GetActorLocation(), OwnBase) : BIG_NUMBER;
			return DA < DB;
		});
		for (AGSBotController* B : ByDist) { if (Def.Num() >= WantDef) break; Def.Add(B); }
	}
	for (AGSBotController* B : Def) B->SetRole(EGSBotRole::Defend);

	TArray<AGSBotController*> Rest = Free.FilterByPredicate([&Def](AGSBotController* B) { return !Def.Contains(B); });
	// Own flag away: half the attackers (at least one) go after it; keep those already on it.
	const int32 WantRet = bOwnAway && Rest.Num() > 0 ? FMath::Max(1, Rest.Num() / 2) : 0;
	TArray<AGSBotController*> Ret;
	for (AGSBotController* B : Rest) if (B->Role == EGSBotRole::Retrieve && Ret.Num() < WantRet) Ret.Add(B);
	for (AGSBotController* B : Rest) if (!Ret.Contains(B) && Ret.Num() < WantRet) Ret.Add(B);
	for (AGSBotController* B : Rest) B->SetRole(Ret.Contains(B) ? EGSBotRole::Retrieve : EGSBotRole::Attack);
}

int32 AGSBotTeam::ClaimVia(AGSBotController* Bot, int32 Avoid, bool bPreferOther)
{
	const TArray<FGSViaPoint>& Via = GSNavSetup::ViaPoints(GetWorld());
	if (Via.Num() == 0) return -1;
	ViaOf.Remove(Bot);
	TArray<int32> Use;
	Use.Init(0, Via.Num());
	for (auto It = ViaOf.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) { It.RemoveCurrent(); continue; }
		if (Use.IsValidIndex(It.Value())) Use[It.Value()]++;
	}
	const FGSViaPoint* Old = Via.IsValidIndex(Avoid) ? &Via[Avoid] : nullptr;
	TArray<int32> Best;
	int32 BestScore = MIN_int32;
	for (int32 I = 0; I < Via.Num(); ++I)
	{
		if (I == Avoid && Via.Num() > 1) continue;
		if (Use[I] >= 2) continue;
		int32 Score = -Use[I] * 4;
		if (bPreferOther && Old)
		{
			if (Via[I].Side != Old->Side) Score += 3;
			if (Via[I].Level != Old->Level) Score += 2;
		}
		if (Score > BestScore) { BestScore = Score; Best.Reset(); }
		if (Score == BestScore) Best.Add(I);
	}
	if (Best.Num() == 0)
	{
		for (int32 I = 0; I < Via.Num(); ++I) if (I != Avoid || Via.Num() == 1) Best.Add(I);
	}
	const int32 Pick = Best[FMath::RandRange(0, Best.Num() - 1)];
	ViaOf.Add(Bot, Pick);
	return Pick;
}

int32 AGSBotTeam::ClaimDefense(AGSBotController* Bot, int32 Avoid)
{
	const TArray<FGSDefSpot>& Spots = GSNavSetup::DefenseSpots(GetWorld(), Team);
	if (Spots.Num() == 0) return -1;
	DefOf.Remove(Bot);
	TSet<int32> Taken;
	for (auto It = DefOf.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) { It.RemoveCurrent(); continue; }
		Taken.Add(It.Value());
	}
	TArray<int32> Free;
	for (int32 I = 0; I < Spots.Num(); ++I) if (!Taken.Contains(I) && I != Avoid) Free.Add(I);
	if (Free.Num() == 0) for (int32 I = 0; I < Spots.Num(); ++I) if (I != Avoid || Spots.Num() == 1) Free.Add(I);
	// The better spots (more entrances seen) come first in the list: favour them.
	const int32 Pick = Free[FMath::Min(FMath::RandRange(0, Free.Num() - 1), FMath::RandRange(0, Free.Num() - 1))];
	DefOf.Add(Bot, Pick);
	return Pick;
}
