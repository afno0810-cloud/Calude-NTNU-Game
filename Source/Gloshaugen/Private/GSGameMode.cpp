#include "GSGameMode.h"
#include "GSCharacter.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"
#include "GSGameState.h"
#include "GSHUD.h"
#include "GSPickup.h"
#include "GSCampus.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "TimerManager.h"

AGSGameMode::AGSGameMode()
{
	DefaultPawnClass = AGSCharacter::StaticClass();
	PlayerControllerClass = AGSPlayerController::StaticClass();
	PlayerStateClass = AGSPlayerState::StaticClass();
	GameStateClass = AGSGameState::StaticClass();
	HUDClass = AGSHUD::StaticClass();
	bUseSeamlessTravel = false;
}

void AGSGameMode::BeginPlay()
{
	Super::BeginPlay();
	SpawnPickups();
}

void AGSGameMode::SpawnPickups()
{
	for (const GSCampus::FPickupSpot& Spot : GSCampus::PickupSpots())
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector Loc(Spot.X * 100.f, Spot.Y * 100.f, Spot.Z * 100.f + 60.f);
		if (AGSPickup* Pickup = GetWorld()->SpawnActor<AGSPickup>(Loc, FRotator::ZeroRotator, P))
		{
			Pickup->WeaponId = Spot.Weapon;
			Pickup->RespawnSeconds = Spot.Weapon == EGSWeaponId::AG3 ? 30.f : 20.f;
		}
	}
}

FTransform AGSGameMode::PickSpawnTransform() const
{
	const TArray<FVector2D>& Points = GSCampus::SpawnPoints();

	// Prefer the spawn point furthest from living players.
	TArray<FVector> Enemies;
	for (TActorIterator<AGSCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->bDead) Enemies.Add(It->GetActorLocation());
	}

	int32 Best = FMath::RandRange(0, Points.Num() - 1);
	if (Enemies.Num() > 0)
	{
		float BestScore = -1.f;
		for (int32 I = 0; I < Points.Num(); ++I)
		{
			const FVector P(Points[I].X * 100.f, Points[I].Y * 100.f, 0.f);
			float Closest = TNumericLimits<float>::Max();
			for (const FVector& E : Enemies) Closest = FMath::Min(Closest, FVector::Dist2D(P, E));
			const float Score = Closest + FMath::FRandRange(0.f, 3000.f); // a bit of randomness
			if (Score > BestScore) { BestScore = Score; Best = I; }
		}
	}

	const FVector Loc(Points[Best].X * 100.f, Points[Best].Y * 100.f, 120.f);
	const FRotator Rot(0.f, FMath::FRandRange(0.f, 360.f), 0.f);
	return FTransform(Rot, Loc);
}

void AGSGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!NewPlayer || NewPlayer->IsPendingKillPending()) return;
	RestartPlayerAtTransform(NewPlayer, PickSpawnTransform());
}

void AGSGameMode::OnPlayerKilled(AController* Killer, AController* Victim, const FString& WeaponName, bool bHeadshot)
{
	AGSPlayerState* KillerPS = Killer ? Killer->GetPlayerState<AGSPlayerState>() : nullptr;
	AGSPlayerState* VictimPS = Victim ? Victim->GetPlayerState<AGSPlayerState>() : nullptr;

	if (VictimPS) VictimPS->Deaths++;
	if (KillerPS && KillerPS != VictimPS)
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
