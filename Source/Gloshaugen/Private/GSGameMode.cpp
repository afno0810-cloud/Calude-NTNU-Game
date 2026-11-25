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
	SpawnTerminals();
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
