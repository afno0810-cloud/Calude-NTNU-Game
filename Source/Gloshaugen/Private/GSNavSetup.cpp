#include "GSNavSetup.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavAreas/NavArea_Null.h"
#include "Components/BrushComponent.h"
#include "Components/BoxComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GSCampus.h"
#include "GSHelgasetr.h"
#include "GSGameMode.h"
#include "GSPlayerState.h"
#include "GSFlag.h"
#include "GSMapState.h"
#include "GSDoor.h"

DEFINE_LOG_CATEGORY_STATIC(LogGSNav, Log, All);

namespace GSNavImpl
{
	const FVector ProjectExtent(100.f, 100.f, 250.f);
	constexpr float ReadyTimeout = 20.f;
	constexpr int32 ViaSamplesPerOrigin = 40;
	constexpr int32 ViaMaxChecks = 40;
	constexpr int32 ViaChecksPerTick = 4;
	constexpr int32 ViaMax = 12;
	constexpr float ViaMinSpacing = 600.f;
	constexpr float BaseRadius = 1200.f;   // entrances: where the routes enter this circle round the flag

	UNavigationSystemV1* Nav(const UWorld* World)
	{
		return World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(const_cast<UWorld*>(World)) : nullptr;
	}

	ARecastNavMesh* Recast(const UWorld* World)
	{
		UNavigationSystemV1* N = Nav(World);
		return N ? Cast<ARecastNavMesh>(N->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)) : nullptr;
	}

	UGSNavSubsystem* Sub(const UWorld* World)
	{
		return World ? World->GetSubsystem<UGSNavSubsystem>() : nullptr;
	}

	int32 LevelOf(float ZCm) { return GSHelgasetr::LevelAt(ZCm / 100.f); }

	FString M(float Cm) { return FString::Printf(TEXT("%.1f"), Cm / 100.f); }
}

// ---------------------------------------------------------------------------------------------------------------------
// The namespace API

namespace GSNavSetup
{
	void EnsureNavMesh(UWorld* World)
	{
		if (UGSNavSubsystem* S = GSNavImpl::Sub(World)) S->Ensure();
	}

	bool IsNavReady(const UWorld* World)
	{
		const UGSNavSubsystem* S = GSNavImpl::Sub(World);
		return S && S->IsReady();
	}

	bool AreRoutesReady(const UWorld* World)
	{
		const UGSNavSubsystem* S = GSNavImpl::Sub(World);
		return S && S->RoutesReady();
	}

	void NotifyMapChanged(UWorld* World, const FBox& BoxCm)
	{
		if (UNavigationSystemV1* N = GSNavImpl::Nav(World))
		{
			if (GSNavImpl::Recast(World)) N->AddDirtyArea(BoxCm, ENavigationDirtyFlag::All, TEXT("GSNavSetup"));
		}
	}

	bool ToNav(UWorld* World, const FVector& In, FVector& Out)
	{
		UNavigationSystemV1* N = GSNavImpl::Nav(World);
		ARecastNavMesh* R = GSNavImpl::Recast(World);
		if (!N || !R) return false;
		FNavLocation L;
		if (!N->ProjectPointToNavigation(In, L, GSNavImpl::ProjectExtent, R)) return false;
		Out = L.Location;
		return true;
	}

	bool FindPath(UWorld* World, const FVector& From, const FVector& To, float& OutLenCm, TArray<FVector>* OutPoints)
	{
		UNavigationSystemV1* N = GSNavImpl::Nav(World);
		ARecastNavMesh* R = GSNavImpl::Recast(World);
		if (!N || !R) return false;
		FPathFindingQuery Q(nullptr, *R, From, To);
		const FPathFindingResult Res = N->FindPathSync(Q);
		if (!Res.IsSuccessful() || !Res.Path.IsValid() || Res.IsPartial()) return false;
		OutLenCm = Res.Path->GetLength();
		if (OutPoints)
		{
			OutPoints->Reset();
			for (const FNavPathPoint& P : Res.Path->GetPathPoints()) OutPoints->Add(P.Location);
		}
		return true;
	}

	const TArray<FGSViaPoint>& ViaPoints(const UWorld* World)
	{
		static const TArray<FGSViaPoint> None;
		const UGSNavSubsystem* S = GSNavImpl::Sub(World);
		return S && S->RoutesReady() ? S->Via : None;
	}

	const TArray<FGSDefSpot>& DefenseSpots(const UWorld* World, uint8 Team)
	{
		static const TArray<FGSDefSpot> None;
		const UGSNavSubsystem* S = GSNavImpl::Sub(World);
		return S && S->RoutesReady() && Team <= 2 ? S->Defense[Team] : None;
	}

	bool FlagNav(const UWorld* World, uint8 Team, FVector& Out)
	{
		const UGSNavSubsystem* S = GSNavImpl::Sub(World);
		if (!S || Team > 2 || !S->bFlagNav[Team]) return false;
		Out = S->FlagNavLoc[Team];
		return true;
	}

	bool ClearLine(const UWorld* World, const FVector& From, const FVector& To, const FCollisionQueryParams& Params,
		bool bObjects, ECollisionChannel Channel, bool bSkipGlass, FHitResult* OutHit)
	{
		if (!World) return false;
		FVector Start = From;
		const FVector Dir = (To - From).GetSafeNormal();
		for (int32 Try = 0; Try < 4; ++Try)
		{
			FHitResult Hit;
			const bool bHit = bObjects
				? World->LineTraceSingleByObjectType(Hit, Start, To, FCollisionObjectQueryParams(ECC_WorldStatic), Params)
				: World->LineTraceSingleByChannel(Hit, Start, To, Channel, Params);
			if (!bHit) return true;
			const UPrimitiveComponent* C = Hit.GetComponent();
			if (bSkipGlass && Try < 3 && C && C->ComponentHasTag(TEXT("Mat_Glass")))
			{
				Start = Hit.ImpactPoint + Dir * 3.f;   // through the pane, on from the other side
				continue;
			}
			if (OutHit) *OutHit = Hit;
			return false;
		}
		return false;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// The subsystem

bool UGSNavSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UGSNavSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	for (int32 T = 0; T < 3; ++T) FlagNavLoc[T] = FVector::ZeroVector;
}

void UGSNavSubsystem::Deinitialize()
{
	if (WalkableHandle.IsValid()) GSMapState::OnWalkableChanged().Remove(WalkableHandle);
	WalkableHandle.Reset();
	Super::Deinitialize();
}

TStatId UGSNavSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGSNavSubsystem, STATGROUP_Tickables);
}

void UGSNavSubsystem::Ensure()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client || Volume.IsValid() || Stage != EStage::Idle) return;

	UNavigationSystemV1* Nav = GSNavImpl::Nav(World);
	if (!Nav)
	{
		UE_LOG(LogGSNav, Log, TEXT("Navnett: verden har ikke noe navigasjonssystem, lager et"));
		FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::GameMode);
		Nav = GSNavImpl::Nav(World);
		UE_LOG(LogGSNav, Log, TEXT("Navnett: navigasjonssystem %s"), Nav ? TEXT("laget") : TEXT("MANGLER"));
		if (!Nav) { Stage = EStage::Failed; return; }
	}

	// Bounds from the map data (meters): Plan 1 to just under the Plan 5 deck, inside the play area.
	FBox BoxM = GSHelgasetr::NavBoundsM();
	if (!BoxM.IsValid)
	{
		const FBox2D A = GSCampus::PlayArea();
		BoxM = FBox(FVector(A.Min.X, A.Min.Y, -1.f), FVector(A.Max.X, A.Max.Y, 17.5f));
	}
	const FBox Box(BoxM.Min * 100.f, BoxM.Max * 100.f);
	const FVector Center = Box.GetCenter();
	const FVector Size = Box.GetSize();

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	P.bDeferConstruction = true;
	P.ObjectFlags |= RF_Transient;
	ANavMeshBoundsVolume* V = World->SpawnActor<ANavMeshBoundsVolume>(ANavMeshBoundsVolume::StaticClass(), FTransform(Center), P);
	if (!V)
	{
		UE_LOG(LogGSNav, Warning, TEXT("Navnett: kunne ikke lage grenseboksen"));
		Stage = EStage::Failed;
		return;
	}
	UBodySetup* BS = NewObject<UBodySetup>(V);
	BS->AggGeom.BoxElems.Add(FKBoxElem(Size.X, Size.Y, Size.Z));
	BS->CollisionTraceFlag = CTF_UseSimpleAsComplex;
	BS->CreatePhysicsMeshes();
	V->GetBrushComponent()->BrushBodySetup = BS;
	V->FinishSpawning(FTransform(Center));
	V->GetBrushComponent()->UpdateBounds();
	Nav->OnNavigationBoundsUpdated(V);
	Volume = V;
	EnsureTime = FPlatformTime::Seconds();
	Stage = EStage::Waiting;
	UE_LOG(LogGSNav, Log, TEXT("Navnett: grenser x %.1f..%.1f, y %.1f..%.1f, z %.1f..%.1f m (volumets boks %s)"),
		BoxM.Min.X, BoxM.Max.X, BoxM.Min.Y, BoxM.Max.Y, BoxM.Min.Z, BoxM.Max.Z, *V->GetComponentsBoundingBox(true).ToString());

	if (!WalkableHandle.IsValid())
	{
		WalkableHandle = GSMapState::OnWalkableChanged().AddUObject(this, &UGSNavSubsystem::OnWalkableChanged);
	}
}

void UGSNavSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	switch (Stage)
	{
	case EStage::Waiting:
		CheckAccum += DeltaTime;
		if (CheckAccum >= 0.5f) { CheckAccum = 0.f; CheckReady(); }
		break;
	case EStage::SelfTest: RunSelfTest(); break;
	case EStage::ViaSample: SampleVia(); break;
	case EStage::ViaPaths: StepViaPaths(); break;
	case EStage::ViaPick: PickVia(); break;
	case EStage::Defense: BuildDefense(); break;
	case EStage::Done:
		DoorAccum += DeltaTime;
		if (DoorAccum >= 1.f) { DoorAccum = 0.f; SyncLockedDoors(); }
		break;
	default: break;
	}
}

void UGSNavSubsystem::CheckReady()
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* Nav = GSNavImpl::Nav(World);
	ARecastNavMesh* R = GSNavImpl::Recast(World);
	const double T = FPlatformTime::Seconds() - EnsureTime;
	const int32 Tiles = R ? R->GetNumActiveTiles() : 0;
	const bool bBuilding = Nav && (Nav->IsNavigationBuildInProgress() || Nav->GetNumRemainingBuildTasks() > 0 || Nav->GetNumRunningBuildTasks() > 0);
	// Dynamic generation builds the tiles over many frames and can look idle between two batches: ready only when the
	// number of tiles has stood still for three checks in a row (1.5 s) with nothing building.
	StableChecks = (Tiles > 0 && !bBuilding && Tiles == LastTiles) ? StableChecks + 1 : 0;
	LastTiles = Tiles;
	if (StableChecks >= 3)
	{
		bNavReady = true;
		Stage = EStage::SelfTest;
		UE_LOG(LogGSNav, Log, TEXT("Navnett klart: %d fliser, %.1f s (celle %.0f x %.0f cm, agentradius %.0f, høyde %.0f, trinn %.0f cm)"), Tiles, T,
			R->GetCellSize(ENavigationDataResolution::Default), R->GetCellHeight(ENavigationDataResolution::Default), R->AgentRadius, R->AgentHeight,
			R->GetAgentMaxStepHeight(ENavigationDataResolution::Default));
		return;
	}
	// The nav data can come after the bounds (it is made when the first bounds arrive): ask once for a full build.
	if (!bRebuildKicked && T > 3.0 && R && !bBuilding && Tiles == 0)
	{
		bRebuildKicked = true;
		UE_LOG(LogGSNav, Log, TEXT("Navnett: ingen fliser etter %.1f s, ber om full bygging"), T);
		R->RebuildAll();
	}
	if (T > GSNavImpl::ReadyTimeout)
	{
		Stage = EStage::Failed;
		UE_LOG(LogGSNav, Warning, TEXT("Navnett: INGEN FLISER etter %.0f s (navdata %s, bygger %d) - botene står og forsvarer"),
			T, R ? TEXT("finnes") : TEXT("mangler"), bBuilding ? 1 : 0);
	}
}

bool UGSNavSubsystem::GetFlagFloor(uint8 Team, FVector& Out) const
{
	for (TActorIterator<AGSFlag> It(GetWorld()); It; ++It)
	{
		if (It->Team == Team) { Out = It->GetActorLocation(); return true; }
	}
	Out = AGSGameMode::BasePointM(Team) * 100.f;
	return true;
}

void UGSNavSubsystem::GetSpawnFloors(uint8 Team, TArray<FVector>& Out) const
{
	Out.Reset();
	const TArray<GSCampus::FBase>& Bases = GSCampus::Bases();
	const int32 Index = Team == GSTeam::Blue ? 1 : 0;
	if (!Bases.IsValidIndex(Index)) return;
	const float Z = Bases[Index].Flag.Z * 100.f;
	for (const FVector2D& S : Bases[Index].Spawns) Out.Add(FVector(S.X * 100.f, S.Y * 100.f, Z + 50.f));
}

void UGSNavSubsystem::RunSelfTest()
{
	UWorld* World = GetWorld();
	const double T0 = FPlatformTime::Seconds();

	for (uint8 Team = GSTeam::Red; Team <= GSTeam::Blue; ++Team)
	{
		FVector Floor;
		GetFlagFloor(Team, Floor);
		bFlagNav[Team] = GSNavSetup::ToNav(World, Floor + FVector(0.f, 0.f, 50.f), FlagNavLoc[Team]);
		if (!bFlagNav[Team]) UE_LOG(LogGSNav, Warning, TEXT("Selvtest: %s sitt flagg (%s) ligger ikke på navnettet"), GSTeam::Name(Team), *Floor.ToString());
	}

	// 1. Every spawn point to the enemy flag.
	float Sum[3] = { 0.f, 0.f, 0.f };
	int32 Count[3] = { 0, 0, 0 };
	int32 NoPath = 0;
	for (uint8 Team = GSTeam::Red; Team <= GSTeam::Blue; ++Team)
	{
		const uint8 Enemy = GSTeam::Other(Team);
		TArray<FVector> Spawns;
		GetSpawnFloors(Team, Spawns);
		for (int32 I = 0; I < Spawns.Num(); ++I)
		{
			FVector From;
			float Len = 0.f;
			TArray<FVector> Pts;
			if (!GSNavSetup::ToNav(World, Spawns[I], From))
			{
				++NoPath;
				UE_LOG(LogGSNav, Warning, TEXT("Selvtest: INGEN STI fra %s spawn %d (%s): punktet er ikke på navnettet"), GSTeam::Name(Team), I, *Spawns[I].ToString());
				continue;
			}
			if (!bFlagNav[Enemy] || !GSNavSetup::FindPath(World, From, FlagNavLoc[Enemy], Len, &Pts))
			{
				++NoPath;
				UE_LOG(LogGSNav, Warning, TEXT("Selvtest: INGEN STI fra %s spawn %d til %s"), GSTeam::Name(Team), I, GSTeam::FlagName(Enemy));
				continue;
			}
			Sum[Team] += Len;
			Count[Team]++;
			UE_LOG(LogGSNav, Log, TEXT("Selvtest: %s spawn %d -> %s: %s m, %d punkter"), GSTeam::Name(Team), I, GSTeam::FlagName(Enemy), *GSNavImpl::M(Len), Pts.Num());
		}
	}
	const float AvgR = Count[1] ? Sum[1] / Count[1] : 0.f, AvgB = Count[2] ? Sum[2] / Count[2] : 0.f;
	UE_LOG(LogGSNav, Log, TEXT("Selvtest: snitt rød->blå %s m (%d stier), blå->rød %s m (%d stier), forskjell %s m, %d uten sti"),
		*GSNavImpl::M(AvgR), Count[1], *GSNavImpl::M(AvgB), Count[2], *GSNavImpl::M(FMath::Abs(AvgR - AvgB)), NoPath);

	// What can be reached from each flag (the island it stands on): 300 random reachable points.
	if (UNavigationSystemV1* Nav = GSNavImpl::Nav(World))
	{
		for (uint8 Team = GSTeam::Red; Team <= GSTeam::Blue; ++Team)
		{
			if (!bFlagNav[Team]) continue;
			FBox Box(ForceInit);
			int32 PerLevel[6] = { 0 };
			int32 N = 0;
			for (int32 I = 0; I < 300; ++I)
			{
				FNavLocation L;
				if (!Nav->GetRandomReachablePointInRadius(FlagNavLoc[Team], 10000.f, L, GSNavImpl::Recast(World))) continue;
				Box += L.Location;
				PerLevel[FMath::Clamp(GSNavImpl::LevelOf(L.Location.Z), 0, 5)]++;
				++N;
			}
			UE_LOG(LogGSNav, Log, TEXT("Selvtest: nåbart fra %s: %d punkter, x %s..%s, y %s..%s m, per etasje P1 %d, P2 %d, P3 %d, P4 %d"),
				GSTeam::FlagName(Team), N, *GSNavImpl::M(Box.Min.X), *GSNavImpl::M(Box.Max.X), *GSNavImpl::M(Box.Min.Y), *GSNavImpl::M(Box.Max.Y),
				PerLevel[0], PerLevel[1], PerLevel[2], PerLevel[3]);
		}
	}

	// 2. Through every door opening: 0.8 m in front to 0.8 m behind, along the door's normal.
	TMap<int32, bool> LockedNow;
	GSMapState::ForEachDoor(World, [&LockedNow](const FGSDoorInfo& D)
	{
		LockedNow.Add(GSPart::IdOf(D.Key), D.Lock == GSLock::Locked);
	});
	int32 Ok = 0, Total = 0;
	for (const GSHelgasetr::FDoorM& D : GSHelgasetr::DoorSlots())
	{
		++Total;
		const FVector Base(D.X * 100.f, D.Y * 100.f, D.Z * 100.f + 50.f);
		const FVector N = FRotator(0.f, D.Yaw, 0.f).Vector();
		FVector A, B;
		float Len = 0.f;
		const bool bA = GSNavSetup::ToNav(World, Base + N * 80.f, A);
		const bool bB = GSNavSetup::ToNav(World, Base - N * 80.f, B);
		const bool bPath = bA && bB && GSNavSetup::FindPath(World, A, B, Len) && Len <= 400.f;
		const bool* bLocked = LockedNow.Find(D.Id);
		Ok += bPath ? 1 : 0;
		if (bPath)
		{
			UE_LOG(LogGSNav, Log, TEXT("dør %d (%s, %s, %.1f m): sti ja (%s m)%s"), D.Id, D.Room, *GSHelgasetr::LevelName(D.Level), D.Width, *GSNavImpl::M(Len),
				bLocked && *bLocked ? TEXT(", låst nå") : TEXT(""));
		}
		else
		{
			// What stands in the opening (pawn channel, across the door at three heights): for the map and door teams.
			FString Block;
			FCollisionQueryParams BP(SCENE_QUERY_STAT(GSNavDoorProbe), false);
			for (const float H : { 15.f, 60.f, 150.f })
			{
				FHitResult Hit;
				const FVector From = Base + N * 80.f + FVector(0.f, 0.f, H - 50.f), To = Base - N * 80.f + FVector(0.f, 0.f, H - 50.f);
				if (World->LineTraceSingleByChannel(Hit, From, To, ECC_Pawn, BP))
				{
					Block += FString::Printf(TEXT(" [%.0f cm: %s/%s]"), H, Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("?"),
						Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("?"));
				}
			}
			UE_LOG(LogGSNav, Warning, TEXT("dør %d (%s, %s, %.1f m): sti NEI (%s)%s%s"), D.Id, D.Room, *GSHelgasetr::LevelName(D.Level), D.Width,
				!bA || !bB ? TEXT("en side er ikke på navnettet") : (Len > 400.f ? *FString::Printf(TEXT("går rundt, %s m"), *GSNavImpl::M(Len)) : TEXT("ingen sti")),
				bLocked && *bLocked ? TEXT(", låst nå") : TEXT(""), Block.IsEmpty() ? TEXT(" [åpningen er fri]") : *Block);
		}
	}
	UE_LOG(LogGSNav, Log, TEXT("dører: %d av %d gangbare (selvtest %.0f ms)"), Ok, Total, (FPlatformTime::Seconds() - T0) * 1000.0);

	// The door test went through the openings as built: now cut out the doors that are locked.
	SyncLockedDoors();
	Stage = EStage::ViaSample;
}

void UGSNavSubsystem::SampleVia()
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* Nav = GSNavImpl::Nav(World);
	ARecastNavMesh* R = GSNavImpl::Recast(World);
	ViaCands.Reset();
	ViaQueue.Reset();
	ViaQueuePos = 0;
	if (!Nav || !R || !bFlagNav[1] || !bFlagNav[2])
	{
		UE_LOG(LogGSNav, Warning, TEXT("via-punkter: 0 (flaggene er ikke på navnettet)"));
		Stage = EStage::Defense;
		return;
	}
	const FVector A = FlagNavLoc[1], B = FlagNavLoc[2];
	const FVector2D A2(A), AB2 = FVector2D(B) - FVector2D(A);
	const float Len2 = AB2.SizeSquared();
	const float Radius = 0.8f * FVector::Dist2D(A, B);

	TArray<FVector> Origins;
	for (uint8 Team = GSTeam::Red; Team <= GSTeam::Blue; ++Team)
	{
		Origins.Add(FlagNavLoc[Team]);
		TArray<FVector> Spawns;
		GetSpawnFloors(Team, Spawns);
		const int32 Pick[2] = { 0, Spawns.Num() / 2 };
		for (int32 I : Pick)
		{
			FVector S;
			if (Spawns.IsValidIndex(I) && GSNavSetup::ToNav(World, Spawns[I], S)) Origins.Add(S);
		}
	}
	for (const FVector& O : Origins)
	{
		for (int32 I = 0; I < GSNavImpl::ViaSamplesPerOrigin; ++I)
		{
			FNavLocation L;
			if (!Nav->GetRandomReachablePointInRadius(O, Radius, L, R)) continue;
			const FVector P = L.Location;
			const float T = FVector2D::DotProduct(FVector2D(P) - A2, AB2) / FMath::Max(Len2, 1.f);
			if (T < 0.25f || T > 0.75f) continue;                                         // (a) the middle half
			if (FVector::Dist(P, A) < 800.f || FVector::Dist(P, B) < 800.f) continue;      // (b) 8 m from both flags
			if (P.Z > 1700.f) continue;                                                     // (c) not Plan 5+
			const float Cross = AB2.X * (P.Y - A.Y) - AB2.Y * (P.X - A.X);
			ViaCands.Add({ P, GSNavImpl::LevelOf(P.Z), Cross >= 0.f ? 1 : -1 });
		}
	}

	// (d) Path from both flags, checked for at most 40 candidates: round robin over (level, side) so every floor and both
	// sides get their turn.
	TMap<int32, TArray<int32>> Buckets;
	for (int32 I = 0; I < ViaCands.Num(); ++I) Buckets.FindOrAdd(ViaCands[I].Level * 10 + (ViaCands[I].Side > 0 ? 1 : 0)).Add(I);
	TArray<int32> Keys;
	Buckets.GetKeys(Keys);
	Keys.Sort();
	for (int32 Round = 0; ViaQueue.Num() < GSNavImpl::ViaMaxChecks; ++Round)
	{
		bool bAny = false;
		for (int32 K : Keys)
		{
			const TArray<int32>& L = Buckets[K];
			if (L.IsValidIndex(Round) && ViaQueue.Num() < GSNavImpl::ViaMaxChecks) { ViaQueue.Add(L[Round]); bAny = true; }
		}
		if (!bAny) break;
	}
	UE_LOG(LogGSNav, Log, TEXT("via-punkter: %d kandidater fra %d startpunkter (radius %s m), sjekker sti for %d"),
		ViaCands.Num(), Origins.Num(), *GSNavImpl::M(Radius), ViaQueue.Num());
	Stage = EStage::ViaPaths;
}

void UGSNavSubsystem::StepViaPaths()
{
	UWorld* World = GetWorld();
	for (int32 N = 0; N < GSNavImpl::ViaChecksPerTick && ViaQueuePos < ViaQueue.Num(); ++N, ++ViaQueuePos)
	{
		FViaCand& C = ViaCands[ViaQueue[ViaQueuePos]];
		float L1 = 0.f, L2 = 0.f;
		C.bChecked = true;
		C.bOk = GSNavSetup::FindPath(World, FlagNavLoc[1], C.Loc, L1) && GSNavSetup::FindPath(World, FlagNavLoc[2], C.Loc, L2);
	}
	if (ViaQueuePos >= ViaQueue.Num()) Stage = EStage::ViaPick;
}

void UGSNavSubsystem::PickVia()
{
	Via.Reset();
	TArray<const FViaCand*> Ok;
	for (const FViaCand& C : ViaCands) if (C.bOk) Ok.Add(&C);
	auto FarEnough = [this](const FVector& P)
	{
		for (const FGSViaPoint& V : Via) if (FVector::Dist(V.Loc, P) < GSNavImpl::ViaMinSpacing) return false;
		return true;
	};
	auto Take = [this](const FViaCand* C) { Via.Add({ C->Loc, C->Level, C->Side }); };

	// First one per floor that has candidates (both sides where possible), then fill up alternating sides.
	TSet<int32> Levels;
	for (const FViaCand* C : Ok) Levels.Add(C->Level);
	for (int32 Level : Levels)
	{
		for (int32 Side : { 1, -1 })
		{
			for (const FViaCand* C : Ok)
			{
				if (C->Level == Level && C->Side == Side && FarEnough(C->Loc) && Via.Num() < GSNavImpl::ViaMax) { Take(C); break; }
			}
		}
	}
	for (int32 Pass = 0; Pass < 2 && Via.Num() < GSNavImpl::ViaMax; ++Pass)
	{
		for (const FViaCand* C : Ok)
		{
			if (Via.Num() >= GSNavImpl::ViaMax) break;
			int32 Left = 0;
			for (const FGSViaPoint& V : Via) Left += V.Side > 0 ? 1 : -1;
			const int32 Want = Left > 0 ? -1 : 1;
			if ((Pass == 1 || C->Side == Want) && FarEnough(C->Loc)) Take(C);
		}
	}

	int32 PerLevel[8] = { 0 };
	int32 SideCount[2] = { 0, 0 };
	float SideX[2] = { 0.f, 0.f };
	for (const FGSViaPoint& V : Via)
	{
		PerLevel[FMath::Clamp(V.Level, 0, 7)]++;
		const int32 S = V.Side > 0 ? 0 : 1;
		SideCount[S]++;
		SideX[S] += V.Loc.X;
	}
	// Name the sides by where they lie (X = east in Helgasetr).
	auto SideName = [&](int32 S) { return SideCount[S] && SideX[S] / SideCount[S] < 0.f ? TEXT("vest") : TEXT("øst"); };
	UE_LOG(LogGSNav, Log, TEXT("via-punkter: %d (P1: %d, P2: %d, P3: %d, P4: %d; %s %d, %s %d), %d av %d kandidater hadde sti"),
		Via.Num(), PerLevel[0], PerLevel[1], PerLevel[2], PerLevel[3], SideName(0), SideCount[0], SideName(1), SideCount[1], Ok.Num(), ViaQueue.Num());
	Stage = EStage::Defense;
}

void UGSNavSubsystem::BuildDefense()
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* Nav = GSNavImpl::Nav(World);
	ARecastNavMesh* R = GSNavImpl::Recast(World);
	const double T0 = FPlatformTime::Seconds();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBotDefense), false);
	for (uint8 Team = GSTeam::Red; Team <= GSTeam::Blue; ++Team)
	{
		Defense[Team].Reset();
		if (!Nav || !R || !bFlagNav[Team]) continue;
		const FVector F = FlagNavLoc[Team];

		// Entrances: where the routes from the via points enter the 12 m circle round the flag.
		TArray<FVector> Entrances;
		for (const FGSViaPoint& V : Via)
		{
			TArray<FVector> Pts;
			float Len = 0.f;
			if (!GSNavSetup::FindPath(World, V.Loc, F, Len, &Pts)) continue;
			for (int32 I = 1; I < Pts.Num(); ++I)
			{
				const float D1 = FVector::Dist2D(Pts[I], F);
				if (D1 > GSNavImpl::BaseRadius || FMath::Abs(Pts[I].Z - F.Z) > 300.f) continue;
				const float D0 = FVector::Dist2D(Pts[I - 1], F);
				const float A = D0 > D1 ? FMath::Clamp((D0 - GSNavImpl::BaseRadius) / (D0 - D1), 0.f, 1.f) : 1.f;
				const FVector E = FMath::Lerp(Pts[I - 1], Pts[I], A);
				bool bNew = true;
				for (const FVector& Old : Entrances) if (FVector::Dist(Old, E) < 300.f) { bNew = false; break; }
				if (bNew) Entrances.Add(E);
				break;
			}
		}

		struct FCand { FGSDefSpot Spot; bool bFlag; bool bWall; };
		TArray<FCand> Cands;
		for (int32 I = 0; I < 24 * 3 && Cands.Num() < 24; ++I)
		{
			FNavLocation L;
			if (!Nav->GetRandomReachablePointInRadius(F, 1400.f, L, R)) continue;
			const FVector P = L.Location;
			const float D = FVector::Dist2D(P, F);
			if (D < 400.f || D > 1400.f || FMath::Abs(P.Z - F.Z) > 200.f) continue;
			FCand C;
			C.Spot.Loc = P;
			const FVector Eye = P + FVector(0.f, 0.f, 150.f);
			C.bFlag = GSNavSetup::ClearLine(World, Eye, F + FVector(0.f, 0.f, 100.f), Params, true, ECC_WorldStatic, true);
			const FVector Back = (P - F).GetSafeNormal2D();
			const FVector Side = FVector::CrossProduct(Back, FVector::UpVector);
			const FVector Dirs[4] = { Back, Side, -Side, (Back + Side).GetSafeNormal() };
			C.bWall = false;
			for (const FVector& Dir : Dirs)
			{
				const FVector W0 = P + FVector(0.f, 0.f, 100.f);
				if (!GSNavSetup::ClearLine(World, W0, W0 + Dir * 120.f, Params, true, ECC_WorldStatic, false)) { C.bWall = true; break; }
			}
			float Best = BIG_NUMBER;
			C.Spot.LookAt = F + FVector(0.f, 0.f, 100.f);
			for (const FVector& E : Entrances)
			{
				if (!GSNavSetup::ClearLine(World, Eye, E + FVector(0.f, 0.f, 120.f), Params, true, ECC_WorldStatic, true)) continue;
				C.Spot.Entrances++;
				const float ED = FVector::Dist(P, E);
				if (ED < Best) { Best = ED; C.Spot.LookAt = E + FVector(0.f, 0.f, 120.f); }
			}
			Cands.Add(C);
		}
		// Best first: sees the flag and has a wall, then the most entrances.
		Cands.Sort([](const FCand& X, const FCand& Y)
		{
			const int32 QX = (X.bFlag ? 2 : 0) + (X.bWall ? 1 : 0), QY = (Y.bFlag ? 2 : 0) + (Y.bWall ? 1 : 0);
			return QX != QY ? QX > QY : X.Spot.Entrances > Y.Spot.Entrances;
		});
		int32 Full = 0;
		for (const FCand& C : Cands)
		{
			if (Defense[Team].Num() >= 6) break;
			bool bFar = true;
			for (const FGSDefSpot& S : Defense[Team]) if (FVector::Dist(S.Loc, C.Spot.Loc) < 250.f) { bFar = false; break; }
			if (!bFar) continue;
			Defense[Team].Add(C.Spot);
			Full += (C.bFlag && C.bWall) ? 1 : 0;
		}
		UE_LOG(LogGSNav, Log, TEXT("forsvarspunkter %s: %d (%d ser flagget og har vegg bak), %d innganger, %d kandidater"),
			GSTeam::Name(Team), Defense[Team].Num(), Full, Entrances.Num(), Cands.Num());
	}
	UE_LOG(LogGSNav, Log, TEXT("Navnett: ruter ferdige (forsvar %.0f ms)"), (FPlatformTime::Seconds() - T0) * 1000.0);
	Stage = EStage::Done;
}

void UGSNavSubsystem::SyncLockedDoors()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client) return;
	TSet<int32> Want;
	TArray<FGSDoorInfo> Infos;
	GSMapState::ForEachDoor(World, [&](const FGSDoorInfo& D)
	{
		if (D.Lock == GSLock::Locked && !D.bOpen) { Want.Add(D.Key); Infos.Add(D); }
	});
	// Unlocked, broken or open: the opening is walkable again.
	for (auto It = DoorBlocks.CreateIterator(); It; ++It)
	{
		if (Want.Contains(It.Key())) continue;
		if (UBoxComponent* B = It.Value()) B->DestroyComponent();
		It.RemoveCurrent();
	}
	for (const FGSDoorInfo& D : Infos)
	{
		if (DoorBlocks.Contains(D.Key)) continue;
		if (!DoorBlockHolder)
		{
			FActorSpawnParameters P;
			P.ObjectFlags |= RF_Transient;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			DoorBlockHolder = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, P);
			if (!DoorBlockHolder) return;
			USceneComponent* Root = NewObject<USceneComponent>(DoorBlockHolder);
			DoorBlockHolder->SetRootComponent(Root);
			Root->RegisterComponent();
		}
		UBoxComponent* B = NewObject<UBoxComponent>(DoorBlockHolder);
		B->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		B->SetGenerateOverlapEvents(false);
		B->SetCanEverAffectNavigation(true);
		B->bDynamicObstacle = true;
		B->SetAreaClassOverride(UNavArea_Null::StaticClass());
		B->SetBoxExtent(FVector(20.f, D.WidthCm * 0.5f + 10.f, 100.f), false);
		// The holder stands at the origin, so the relative transform is the world one.
		B->SetupAttachment(DoorBlockHolder->GetRootComponent());
		B->SetRelativeLocationAndRotation(D.LocationCm + FVector(0.f, 0.f, 100.f), D.Normal.Rotation());
		B->RegisterComponent();
		DoorBlocks.Add(D.Key, B);
	}
	static int32 LastLogged = -1;
	if (DoorBlocks.Num() != LastLogged)
	{
		LastLogged = DoorBlocks.Num();
		UE_LOG(LogGSNav, Log, TEXT("Navnett: %d låste dører er skåret ut av nettet"), DoorBlocks.Num());
	}
}

void UGSNavSubsystem::OnWalkableChanged(UWorld* InWorld, const FBox& BoxCm)
{
	if (InWorld != GetWorld()) return;
	GSNavSetup::NotifyMapChanged(InWorld, BoxCm.ExpandBy(50.f));
	if (Stage == EStage::Done) SyncLockedDoors();
}
