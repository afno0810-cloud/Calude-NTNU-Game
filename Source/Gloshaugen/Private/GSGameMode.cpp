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
	FRandomStream Rng(FMath::Rand());
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
	for (const GSCampus::FBuilding& B : Buildings)
	{
		if (!B.bEnterable) continue;
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
					if (!(X > HX - 16.f && Y > HY - 5.f)) break;
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
	Spawn(FVector(0.f, 4.f, 0.35f), true, EGSRarity::Mythic);

	// Outdoors: scattered around campus, not inside buildings
	for (int32 I = 0; I < 60; ++I)
	{
		for (int32 Try = 0; Try < 20; ++Try)
		{
			const FVector2D P(Rng.FRandRange(-280.f, 430.f), Rng.FRandRange(-110.f, 520.f));
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
