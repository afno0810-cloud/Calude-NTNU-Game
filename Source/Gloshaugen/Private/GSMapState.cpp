#include "GSMapState.h"
#include "GSDoor.h"
#include "GSInteract.h"
#include "GSCharacter.h"
#include "GSPlayerController.h"
#include "GSGameState.h"
#include "GSPickup.h"
#include "GSAudio.h"
#include "GSEffects.h"
#include "GSUtils.h"
#include "GSCampusBuilder.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/PlayerState.h"

#if __has_include("GSHelgasetr.h")
#include "GSHelgasetr.h"
#include "GSCampus.h"
#define GS_INT_HAS_HELGASETR 1
#else
#define GS_INT_HAS_HELGASETR 0
#endif

namespace GSMapStateImpl
{
	/** Test room: floor centre, on every machine the same (60 m up, over the roof, inside the play area). */
	const FVector TestOrigin(0.f, 0.f, 6000.f);

	UMaterialInterface* MapMat(const TCHAR* Name)
	{
		static TMap<FString, TWeakObjectPtr<UMaterialInterface>> Cache;
		static TSet<FString> Missing;
		const FString Key(Name);
		if (TWeakObjectPtr<UMaterialInterface>* Found = Cache.Find(Key))
		{
			if (Found->IsValid()) return Found->Get();
		}
		if (Missing.Contains(Key)) return nullptr;
		const FString Path = FString::Printf(TEXT("/Game/GS/Materials/MI_GS_%s.MI_GS_%s"), Name, Name);
		UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!M)
		{
			Missing.Add(Key);
			UE_LOG(LogTemp, Warning, TEXT("Interaktiv: materialet MI_GS_%s mangler, bruker farge"), Name);
			return nullptr;
		}
		Cache.Add(Key, M);
		return M;
	}

	void CopyPayload(FGSPartState& Dst, const FGSPartState& Src)
	{
		Dst.Key = Src.Key;
		Dst.State = Src.State;
		Dst.Open = Src.Open;
		Dst.Health = Src.Health;
		Dst.Flags = Src.Flags;
		Dst.Cause = Src.Cause;
		Dst.HitDir = Src.HitDir;
		Dst.ChangedAt = Src.ChangedAt;
	}

	/** The map's doors (Helgasetr data). Ids from the data, so every machine gets the same keys. */
	void CollectMapDefs(TArray<FGSDoorDef>& Out)
	{
#if GS_INT_HAS_HELGASETR
		if constexpr (GSCampus::bHelgasetr)
		{
			for (const GSHelgasetr::FDoorM& D : GSHelgasetr::DoorSlots())
			{
				FGSDoorDef Def;
				Def.Id = D.Id;
				Def.BaseCm = FVector(D.X, D.Y, D.Z) * 100.f;
				Def.WidthCm = D.Width * 100.f;
				Def.HeightCm = D.Height * 100.f;
				Def.YawDeg = D.Yaw;
				Def.bHingeLeft = D.bHingeLeft;
				// DoorKindNames: tre / glass_kode / dobbel / brann / laast
				switch (static_cast<int32>(D.Kind))
				{
				case 1: Def.Art = EGSDoorArt::GlassCode; break;
				case 2: Def.Art = EGSDoorArt::Double; break;
				case 3: Def.Art = EGSDoorArt::Fire; break;
				case 4: Def.Art = EGSDoorArt::Fire; break;
				default: Def.Art = EGSDoorArt::Wood; break;
				}
				Def.bLocked = D.bLocked || D.Kind == 1 || D.Kind == 4;
				Def.bOpen = D.bOpen && !Def.bLocked;
				Def.Room = D.Room ? FString(D.Room) : FString();
				Out.Add(Def);
			}
		}
#endif
		if (Out.Num() == 0) UE_LOG(LogTemp, Log, TEXT("Interaktiv: ingen kartdata – bare testrommet"));
	}

	/** The test room's doors, in local cm around GSMapStateImpl::TestOrigin. */
	void CollectTestDefs(TArray<FGSDoorDef>& Out)
	{
		auto Add = [&Out](const FVector& Local, float W, float Yaw, EGSDoorArt Art, bool bLocked, const TCHAR* Room)
		{
			FGSDoorDef D;
			D.Id = GSPart::TestIdBase + Out.Num();
			D.BaseCm = GSMapStateImpl::TestOrigin + Local;
			D.WidthCm = W;
			D.HeightCm = 210.f;
			D.YawDeg = Yaw;
			D.Art = Art;
			D.bLocked = bLocked;
			D.Room = Room;
			Out.Add(D);
		};
		// The cross wall (X = 0), front towards the start (-X); behind it is "the room"
		Add(FVector(0.f, -450.f, 0.f), 100.f, 180.f, EGSDoorArt::Wood, false, TEXT("Testrom"));
		Add(FVector(0.f, -150.f, 0.f), 100.f, 180.f, EGSDoorArt::Wood, true, TEXT("Testrom"));
		Add(FVector(0.f, 150.f, 0.f), 100.f, 180.f, EGSDoorArt::GlassCode, true, TEXT("Grupperom (test)"));
		Add(FVector(0.f, 450.f, 0.f), 160.f, 180.f, EGSDoorArt::Double, false, TEXT("Testrom"));
		// Stairwell (X -800..-500, Y -600..-300): a fire door in (closes by itself) and the alarm door out
		// (front = escape side = the stairwell; from the main room it stays shut)
		Add(FVector(-500.f, -450.f, 0.f), 100.f, 0.f, EGSDoorArt::Fire, false, TEXT("Trapperom (test)"));
		Add(FVector(-650.f, -300.f, 0.f), 100.f, -90.f, EGSDoorArt::Fire, true, TEXT("Trapperom (test)"));
	}

	TArray<TWeakObjectPtr<AGSInteractor>>& Interactors()
	{
		static TArray<TWeakObjectPtr<AGSInteractor>> List;
		return List;
	}
}


// ---------------------------------------------------------------------------------------------------------------------
// FastArray callbacks (clients)

void FGSPartState::PreReplicatedRemove(const FGSPartArray& InArray)
{
	if (InArray.Owner) InArray.Owner->OnPartReplicated(*this, true);
}

void FGSPartState::PostReplicatedAdd(const FGSPartArray& InArray)
{
	if (InArray.Owner) InArray.Owner->OnPartReplicated(*this, false);
}

void FGSPartState::PostReplicatedChange(const FGSPartArray& InArray)
{
	if (InArray.Owner) InArray.Owner->OnPartReplicated(*this, false);
}

// ---------------------------------------------------------------------------------------------------------------------
// AGSMapState

AGSMapState::AGSMapState()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(20.f);
	NetPriority = 2.f;
	PrimaryActorTick.bCanEverTick = false;
	Parts.Owner = this;
}

void AGSMapState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSMapState, Parts);
	DOREPLIFETIME(AGSMapState, ResetCount);
	DOREPLIFETIME(AGSMapState, bTestRoom);
}

void AGSMapState::BeginPlay()
{
	Super::BeginPlay();
	Parts.Owner = this;
	// Never touch Parts here: on clients the list (and OnRep) can come before BeginPlay.
	FTimerManager& TM = GetWorldTimerManager();
	TM.SetTimer(RegistryTimer, this, &AGSMapState::TryBuildRegistry, 0.5f, true, 0.1f);
	if (HasAuthority())
	{
		TM.SetTimer(SlowTimer, this, &AGSMapState::ServerSlowTick, 0.5f, true, 0.5f);
		TM.SetTimer(InteractorTimer, this, &AGSMapState::ServerInteractors, 1.f, true, 0.2f);
		TM.SetTimer(BotTimer, this, &AGSMapState::ServerBots, 0.2f, true, 1.f);
	}
	GSInteract::EnsureHooked();
}

void AGSMapState::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	for (auto& Pair : Doors)
	{
		if (AGSDoor* D = Pair.Value.Get()) D->Destroy();
	}
	Doors.Reset();
	Usables.Reset();
	if (TestRoomGeo) TestRoomGeo->Destroy();
	Super::EndPlay(Reason);
}

void AGSMapState::PostNetReceive()
{
	Super::PostNetReceive();
	// Everything that came in the first replication is old news for this client: no sounds, no swings.
	bFirstRepDone = true;
}

AGSMapState* AGSMapState::Get(const UObject* Ctx)
{
	UWorld* W = Ctx && GEngine ? GEngine->GetWorldFromContextObject(Ctx, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!W) return nullptr;
	for (TActorIterator<AGSMapState> It(W); It; ++It)
	{
		if (IsValid(*It)) return *It;
	}
	return nullptr;
}

float AGSMapState::ServerNow() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

float AGSMapState::GetServerTime() const
{
	const UWorld* W = GetWorld();
	if (!W) return 0.f;
	if (W->GetNetMode() != NM_Client) return W->GetTimeSeconds();
	const AGameStateBase* GS = W->GetGameState();
	return GS ? GS->GetServerWorldTimeSeconds() : 0.f;
}

AGSDoor* AGSMapState::FindDoor(int32 Key) const
{
	const TWeakObjectPtr<AGSDoor>* D = Doors.Find(Key);
	return D ? D->Get() : nullptr;
}

AActor* AGSMapState::FindUsable(int32 Key) const
{
	// Doors are the only usable parts in 1a
	return FindDoor(Key);
}

bool AGSMapState::GetPart(int32 Key, FGSPartState& Out) const
{
	for (const FGSPartState& It : Parts.Items)
	{
		if (It.Key == Key)
		{
			Out = It;
			return true;
		}
	}
	if (const AGSDoor* D = FindDoor(Key))
	{
		Out = D->MakeDefault();
		return true;
	}
	return false;
}

void AGSMapState::SetPart(int32 Key, TFunctionRef<void(FGSPartState&)> Fn)
{
	if (!HasAuthority()) return;
	FGSPartState Cur;
	if (!GetPart(Key, Cur)) return;
	Fn(Cur);
	Cur.Key = Key;
	Cur.ChangedAt = ServerNow();
	for (FGSPartState& It : Parts.Items)
	{
		if (It.Key == Key)
		{
			GSMapStateImpl::CopyPayload(It, Cur);
			Parts.MarkItemDirty(It);
			ApplyLocal(It, true);   // FastArray callbacks do not run on the server
			return;
		}
	}
	FGSPartState& New = Parts.Items.AddDefaulted_GetRef();
	GSMapStateImpl::CopyPayload(New, Cur);
	Parts.MarkItemDirty(New);
	ApplyLocal(New, true);
}

void AGSMapState::OnPartReplicated(const FGSPartState& Item, bool bRemoved)
{
	if (bRemoved)
	{
		ResetPartLocal(Item.Key);
		return;
	}
	// Fresh = it just happened: not part of this client's first picture, the doors exist, and the server clock agrees.
	const float Dt = GetServerTime() - Item.ChangedAt;
	const bool bClockOk = GetWorld() && GetWorld()->GetGameState() != nullptr;
	const bool bFresh = bFirstRepDone && bRegistryBuilt && bClockOk && Dt >= 0.f && Dt < 2.f;
	ApplyLocal(Item, bFresh);
}

void AGSMapState::ApplyLocal(const FGSPartState& Item, bool bFresh)
{
	if (AGSDoor* D = FindDoor(Item.Key))
	{
		D->ApplyState(Item, bFresh);
		return;
	}
	PendingKeys.Add(Item.Key);
}

void AGSMapState::ApplyAllLocal()
{
	// Never fresh: this is catching up, not something happening now.
	for (const FGSPartState& It : Parts.Items)
	{
		if (AGSDoor* D = FindDoor(It.Key)) D->ApplyState(It, false);
	}
	PendingKeys.Reset();
}

void AGSMapState::ResetLocal()
{
	for (auto& Pair : Doors)
	{
		if (AGSDoor* D = Pair.Value.Get()) D->ResetLocal();
	}
}

void AGSMapState::ResetPartLocal(int32 Key)
{
	if (AGSDoor* D = FindDoor(Key)) D->ResetLocal();
}

void AGSMapState::OnRep_ResetCount()
{
	ResetLocal();
	ApplyAllLocal();
}

void AGSMapState::OnRep_TestRoom()
{
	if (bTestRoom && bRegistryBuilt && !bTestRoomBuilt)
	{
		BuildTestRoom();
		ApplyAllLocal();
	}
}

void AGSMapState::TryBuildRegistry()
{
	if (bRegistryBuilt)
	{
		GetWorldTimerManager().ClearTimer(RegistryTimer);
		return;
	}
	RegistryTries += 0.5f;
	bool bMapBuilt = false;
	for (TActorIterator<AGSCampusBuilder> It(GetWorld()); It; ++It)
	{
		bMapBuilt = true;
		break;
	}
	if (!bMapBuilt && RegistryTries < 20.f) return;
	if (!bMapBuilt) UE_LOG(LogTemp, Warning, TEXT("Interaktiv: fant ingen kartbygger etter 20 s, legger ut dørene likevel"));
	BuildRegistry();
}

AGSDoor* AGSMapState::SpawnDoor(const FGSDoorDef& Def, int32 Key)
{
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	P.ObjectFlags |= RF_Transient;
	AGSDoor* D = GetWorld()->SpawnActor<AGSDoor>(Def.BaseCm, FRotator(0.f, Def.YawDeg, 0.f), P);
	if (!D) return nullptr;
	D->Init(Def, Key);
	Doors.Add(Key, D);
	Usables.Add(D);
	return D;
}

void AGSMapState::BuildRegistry()
{
	GetWorldTimerManager().ClearTimer(RegistryTimer);
	TArray<FGSDoorDef> Defs;
	GSMapStateImpl::CollectMapDefs(Defs);
	for (const FGSDoorDef& Def : Defs)
	{
		SpawnDoor(Def, GSPart::MakeKey(GSPart::Door, Def.Id));
	}
	bRegistryBuilt = true;
	int32 Locked = 0, Glass = 0;
	for (const FGSDoorDef& Def : Defs)
	{
		Locked += (Def.bLocked || Def.Art == EGSDoorArt::GlassCode) ? 1 : 0;
		Glass += Def.Art == EGSDoorArt::GlassCode ? 1 : 0;
	}
	UE_LOG(LogTemp, Log, TEXT("Interaktiv: %d dører (%d låst, %d glassdører med kodelås)%s"), Defs.Num(), Locked, Glass,
		HasAuthority() ? TEXT(" [server]") : TEXT(" [klient]"));
	if (bTestRoom && !bTestRoomBuilt) BuildTestRoom();
	ApplyAllLocal();
}

void AGSMapState::BuildTestRoom()
{
	if (bTestRoomBuilt) return;
	bTestRoomBuilt = true;
	UWorld* W = GetWorld();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	P.ObjectFlags |= RF_Transient;
	AActor* Geo = W->SpawnActor<AActor>(AActor::StaticClass(), FTransform(GSMapStateImpl::TestOrigin), P);
	if (!Geo) return;
	TestRoomGeo = Geo;
	USceneComponent* GeoRoot = NewObject<USceneComponent>(Geo, TEXT("Root"));
	GeoRoot->SetMobility(EComponentMobility::Static);
	Geo->SetRootComponent(GeoRoot);
	GeoRoot->RegisterComponent();
	GeoRoot->SetWorldLocation(GSMapStateImpl::TestOrigin);

	TMap<FString, UInstancedStaticMeshComponent*> ISMs;
	auto Cube = [&](const FVector& Center, const FVector& Size, const TCHAR* MatName, const FLinearColor& Fallback)
	{
		UInstancedStaticMeshComponent*& ISM = ISMs.FindOrAdd(MatName);
		if (!ISM)
		{
			ISM = NewObject<UInstancedStaticMeshComponent>(Geo);
			ISM->SetStaticMesh(GSUtils::Cube());
			ISM->SetMobility(EComponentMobility::Static);
			ISM->SetCollisionProfileName(TEXT("BlockAll"));   // collision before register, so the bodies exist
			ISM->SetupAttachment(GeoRoot);
			if (UMaterialInterface* M = GSMapStateImpl::MapMat(MatName)) ISM->SetMaterial(0, M);
			else GSUtils::SetColor(ISM, Fallback);
			ISM->RegisterComponent();
			Geo->AddInstanceComponent(ISM);
		}
		ISM->AddInstance(FTransform(FRotator::ZeroRotator, Center, Size / 100.f), false);
	};
	const TCHAR* Wall = TEXT("PlasterWhite_In");
	const FLinearColor WallC(0.85f, 0.85f, 0.82f);
	const float H = 340.f, DoorH = 210.f, T = 14.f;
	// Floor, ceiling, outer walls (main room X -800..800, Y -600..600)
	Cube(FVector(0.f, 0.f, -10.f), FVector(1640.f, 1240.f, 20.f), TEXT("Terrazzo_In"), FLinearColor(0.6f, 0.6f, 0.58f));
	Cube(FVector(0.f, 0.f, H + 10.f), FVector(1640.f, 1240.f, 20.f), Wall, WallC);
	Cube(FVector(-810.f, 0.f, H * 0.5f), FVector(20.f, 1240.f, H), Wall, WallC);
	Cube(FVector(810.f, 0.f, H * 0.5f), FVector(20.f, 1240.f, H), Wall, WallC);
	Cube(FVector(0.f, -610.f, H * 0.5f), FVector(1640.f, 20.f, H), Wall, WallC);
	Cube(FVector(0.f, 610.f, H * 0.5f), FVector(1640.f, 20.f, H), Wall, WallC);
	// A straight wall with door holes: along Y (at X = Fixed) or along X (at Y = Fixed). Holes = (centre, width).
	auto WallWithHoles = [&](bool bAlongY, float Fixed, float From, float To, const TArray<FVector2D>& Holes)
	{
		float Pos = From;
		auto Piece = [&](float A, float B, float Z0, float Z1)
		{
			if (B - A < 0.5f) return;
			const float C = (A + B) * 0.5f, L = B - A;
			const FVector Center = bAlongY ? FVector(Fixed, C, (Z0 + Z1) * 0.5f) : FVector(C, Fixed, (Z0 + Z1) * 0.5f);
			const FVector Size = bAlongY ? FVector(T, L, Z1 - Z0) : FVector(L, T, Z1 - Z0);
			Cube(Center, Size, Wall, WallC);
		};
		for (const FVector2D& Hole : Holes)
		{
			const float A = Hole.X - Hole.Y * 0.5f, B = Hole.X + Hole.Y * 0.5f;
			Piece(Pos, A, 0.f, H);
			Piece(A, B, DoorH, H);   // lintel
			Pos = B;
		}
		Piece(Pos, To, 0.f, H);
	};
	TArray<FGSDoorDef> Defs;
	GSMapStateImpl::CollectTestDefs(Defs);
	WallWithHoles(true, 0.f, -600.f, 600.f, { FVector2D(-450.f, 100.f), FVector2D(-150.f, 100.f), FVector2D(150.f, 100.f), FVector2D(450.f, 160.f) });
	WallWithHoles(true, -500.f, -600.f, -300.f - T * 0.5f, { FVector2D(-450.f, 100.f) });
	WallWithHoles(false, -300.f, -800.f, -500.f + T * 0.5f, { FVector2D(-650.f, 100.f) });
	// Table with a card and a lock pick
	const TCHAR* Wood = TEXT("WoodLight_In");
	const FLinearColor WoodC(0.6f, 0.45f, 0.3f);
	Cube(FVector(-450.f, 350.f, 73.f), FVector(140.f, 70.f, 4.f), Wood, WoodC);
	for (const FVector2D L : { FVector2D(-515.f, 320.f), FVector2D(-385.f, 320.f), FVector2D(-515.f, 380.f), FVector2D(-385.f, 380.f) })
	{
		Cube(FVector(L.X, L.Y, 35.5f), FVector(5.f, 5.f, 71.f), Wood, WoodC);
	}
	// Two lights over the room and one in the stairwell, no shadows
	if (W->GetNetMode() != NM_DedicatedServer)
	{
		for (const FVector L : { FVector(-400.f, 0.f, 320.f), FVector(400.f, 0.f, 320.f), FVector(-650.f, -450.f, 320.f) })
		{
			UPointLightComponent* PL = NewObject<UPointLightComponent>(Geo);
			PL->SetupAttachment(GeoRoot);
			PL->SetMobility(EComponentMobility::Movable);
			PL->SetIntensityUnits(ELightUnits::Lumens);
			PL->SetIntensity(6000.f);
			PL->SetAttenuationRadius(1300.f);
			PL->SetCastShadows(false);
			PL->SetLightColor(FLinearColor(1.f, 0.96f, 0.9f));
			PL->RegisterComponent();
			PL->SetRelativeLocation(L);
			Geo->AddInstanceComponent(PL);
		}
	}
	for (const FGSDoorDef& Def : Defs)
	{
		SpawnDoor(Def, GSPart::MakeKey(GSPart::Door, Def.Id));
	}
	UE_LOG(LogTemp, Log, TEXT("Interaktiv: testrommet bygd (%d dører)%s"), Defs.Num(), HasAuthority() ? TEXT(" [server]") : TEXT(" [klient]"));
}

void AGSMapState::SpawnTestPickups()
{
	for (TWeakObjectPtr<AGSPickup>& P : TestPickups)
	{
		if (AGSPickup* Pk = P.Get()) Pk->Destroy();
	}
	TestPickups.Reset();
	if (AGSPickup* K = AGSPickup::SpawnDropped(GetWorld(), GSMapStateImpl::TestOrigin + FVector(-485.f, 350.f, 76.f), TEXT("Keycard"), 1)) TestPickups.Add(K);
	if (AGSPickup* L = AGSPickup::SpawnDropped(GetWorld(), GSMapStateImpl::TestOrigin + FVector(-415.f, 350.f, 76.f), TEXT("Lockpick"), 1)) TestPickups.Add(L);
}

void AGSMapState::DoEnterTestRoom(APlayerController* PC, bool bAll)
{
	if (!HasAuthority()) return;
	if (!bTestRoom)
	{
		bTestRoom = true;
		ForceNetUpdate();
	}
	if (bRegistryBuilt && !bTestRoomBuilt)
	{
		BuildTestRoom();
		ApplyAllLocal();
	}
	SpawnTestPickups();
	TArray<APawn*> Movers;
	if (bAll)
	{
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			if (AController* C = It->Get())
			{
				if (APawn* P = C->GetPawn()) Movers.Add(P);
			}
		}
	}
	else if (PC && PC->GetPawn())
	{
		Movers.Add(PC->GetPawn());
	}
	for (int32 I = 0; I < Movers.Num(); ++I)
	{
		const FVector Loc = GSMapStateImpl::TestOrigin + FVector(-600.f + (I / 6) * 90.f, -50.f + (I % 6) * 90.f, 100.f);
		Movers[I]->TeleportTo(Loc, FRotator::ZeroRotator);
		if (AController* C = Movers[I]->GetController())
		{
			C->SetControlRotation(FRotator::ZeroRotator);
			if (APlayerController* MPC = Cast<APlayerController>(C)) MPC->ClientSetRotation(FRotator::ZeroRotator);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Interaktiv: testrom – %d flyttet, kort og dirkesett ligger på bordet"), Movers.Num());
}

void AGSMapState::DoResetForMatch()
{
	if (!HasAuthority()) return;
	for (TWeakObjectPtr<AGSInteractor>& W : GSMapStateImpl::Interactors())
	{
		if (AGSInteractor* I = W.Get())
		{
			if (I->GetWorld() == GetWorld()) I->EndJob(false);
		}
	}
	Parts.Items.Reset();
	Parts.MarkArrayDirty();
	++ResetCount;
	ResetLocal();
	ForceNetUpdate();
	UE_LOG(LogTemp, Log, TEXT("Interaktiv: alt tilbakestilt (gang %d)"), ResetCount);
}

void AGSMapState::ResetForMatch(UWorld* World)
{
	if (AGSMapState* MS = Get(World)) MS->DoResetForMatch();
}

void AGSMapState::ServerSlowTick()
{
	// Without Spillbarhet's hook: a new match starts when the phase goes from Lobby/Ended to Countdown/Playing.
	const AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>();
	if (!GS) return;
	const uint8 Now = static_cast<uint8>(GS->Phase);
	if (LastPhase != 255 && Now != LastPhase)
	{
		const bool bWasIdle = LastPhase == static_cast<uint8>(EGSMatchPhase::Lobby) || LastPhase == static_cast<uint8>(EGSMatchPhase::Ended);
		const bool bMatch = Now == static_cast<uint8>(EGSMatchPhase::Countdown) || Now == static_cast<uint8>(EGSMatchPhase::Playing);
		if (bWasIdle && bMatch) DoResetForMatch();
	}
	LastPhase = Now;
}

void AGSMapState::ServerInteractors()
{
	UWorld* W = GetWorld();
	TArray<TWeakObjectPtr<AGSInteractor>>& List = GSMapStateImpl::Interactors();
	// Drop the dead ones (and the ones whose player left)
	for (int32 I = List.Num() - 1; I >= 0; --I)
	{
		AGSInteractor* It = List[I].Get();
		if (!It)
		{
			List.RemoveAtSwap(I);
			continue;
		}
		if (It->GetWorld() != W) continue;
		if (!IsValid(It->GetOwner()))
		{
			It->Destroy();
			List.RemoveAtSwap(I);
		}
	}
	for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
	{
		AGSPlayerController* PC = Cast<AGSPlayerController>(It->Get());
		if (!PC) continue;
		bool bHas = false;
		for (const TWeakObjectPtr<AGSInteractor>& I : List)
		{
			if (I.IsValid() && I->GetOwner() == PC)
			{
				bHas = true;
				break;
			}
		}
		if (bHas) continue;
		FActorSpawnParameters P;
		P.Owner = PC;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		P.ObjectFlags |= RF_Transient;
		if (AGSInteractor* New = W->SpawnActor<AGSInteractor>(FVector::ZeroVector, FRotator::ZeroRotator, P))
		{
			List.Add(New);
		}
	}
}

void AGSMapState::ServerBots()
{
	if (Doors.Num() == 0) return;
	for (TActorIterator<AGSCharacter> It(GetWorld()); It; ++It)
	{
		AGSCharacter* C = *It;
		if (C->bDead || !C->GetController() || C->IsPlayerControlled()) continue;
		const FVector P = C->GetActorLocation();
		for (auto& Pair : Doors)
		{
			AGSDoor* D = Pair.Value.Get();
			if (!D || D->IsLocked() || D->IsOpen()) continue;
			const FVector To = P - D->GetBase();
			if (FMath::Abs(To.Z - 90.f) > 150.f) continue;
			const float Dist = To.Size2D();
			if (Dist > 120.f || Dist < 1.f) continue;
			const float Dot = FMath::Abs(FVector::DotProduct(To.GetSafeNormal2D(), D->GetNormal()));
			if (Dot < 0.5f) continue;   // within 60 degrees of the door's normal
			D->ServerOpenAway(P, GSCause::Bot, false, false);
		}
	}
}

void AGSMapState::PlayPartSound(const UObject* Ctx, const TCHAR* Name, const FVector& Loc, float Volume, float RangeM, float Pitch)
{
	if (GSAudio::Get(Name))
	{
		GSAudio::PlayAt(Ctx, Name, Loc, Volume, RangeM, Pitch);
		return;
	}
	// Fallbacks until the real sounds exist (Kamp's K5)
	struct FFallback { const TCHAR* Name; const TCHAR* Use; float Pitch; float Vol; };
	static const FFallback Table[] = {
		{ TEXT("Door_Kick"), TEXT("Land"), 0.7f, 1.f },
		{ TEXT("Door_Slam"), TEXT("Land"), 0.7f, 1.f },
		{ TEXT("Door_Open"), TEXT("Reload_In"), 0.6f, 0.5f },
		{ TEXT("Door_Close"), TEXT("Reload_Out"), 0.6f, 0.5f },
		{ TEXT("Lock_Pick"), TEXT("Reload_In"), 2.2f, 0.3f },
		{ TEXT("Keypad_Beep"), TEXT("Pickup"), 1.6f, 1.f },
		{ TEXT("Keypad_Error"), TEXT("Dry_Fire"), 1.3f, 1.f },
		{ TEXT("Keypad_Break"), TEXT("Impact_2"), 1.3f, 1.f },
	};
	for (const FFallback& F : Table)
	{
		if (FCString::Strcmp(F.Name, Name) == 0)
		{
			GSAudio::PlayAt(Ctx, F.Use, Loc, Volume * F.Vol, RangeM, Pitch * F.Pitch);
			return;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// UGSMapStateSubsystem

bool UGSMapStateSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UGSMapStateSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (InWorld.GetNetMode() == NM_Client) return;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	P.ObjectFlags |= RF_Transient;
	InWorld.SpawnActor<AGSMapState>(FVector::ZeroVector, FRotator::ZeroRotator, P);
}

// ---------------------------------------------------------------------------------------------------------------------
// The calls other teams make

namespace GSMapState
{
	static AGSMapState* ServerState(UWorld* World)
	{
		if (!World || World->GetNetMode() == NM_Client) return nullptr;
		return AGSMapState::Get(World);
	}

	void ApplyBlast(UWorld* World, const FVector& OriginCm, float RadiusCm, float Strength)
	{
		AGSMapState* MS = ServerState(World);
		if (!MS || RadiusCm <= 0.f) return;
		int32 N = 0;
		for (const auto& Pair : MS->GetDoors())
		{
			AGSDoor* D = Pair.Value.Get();
			if (!D) continue;
			const FVector Mid = D->GetBase() + FVector(0.f, 0.f, 100.f);
			if (FVector::Dist(Mid, OriginCm) > RadiusCm + D->GetDef().WidthCm * 0.5f) continue;
			// A wall in between takes the blast
			FCollisionObjectQueryParams OP(ECC_WorldStatic);
			FCollisionQueryParams QP(SCENE_QUERY_STAT(GSDoorBlast), false, D);
			FHitResult H;
			if (World->LineTraceSingleByObjectType(H, OriginCm, Mid, OP, QP)) continue;
			D->ServerOpenAway(OriginCm, GSCause::Blast, true, false);
			++N;
		}
		OnNoise().Broadcast(World, OriginCm, 80.f);
		if (N > 0) UE_LOG(LogTemp, Log, TEXT("Interaktiv: smell (%.0f m) åpnet %d dører"), RadiusCm / 100.f, N);
	}

	void ApplyBulletHit(UWorld* World, const FHitResult& Hit, float Damage)
	{
		if (!ServerState(World)) return;
		if (AGSDoor* D = Cast<AGSDoor>(Hit.GetActor())) D->ServerBulletHit(Hit, Damage);
	}

	float ApplyMelee(AActor* User, const FVector& Start, const FVector& End, float Damage)
	{
		UWorld* World = User ? User->GetWorld() : nullptr;
		if (!ServerState(World)) return BIG_NUMBER;
		FCollisionObjectQueryParams OP;
		OP.AddObjectTypesToQuery(ECC_WorldDynamic);
		FCollisionQueryParams QP(SCENE_QUERY_STAT(GSDoorMelee), false, User);
		FHitResult H;
		if (World->SweepSingleByObjectType(H, Start, End, FQuat::Identity, OP, FCollisionShape::MakeSphere(22.f), QP))
		{
			if (AGSDoor* D = Cast<AGSDoor>(H.GetActor())) D->ServerMeleeHit(H, User);
		}
		return BIG_NUMBER;   // no breakable glass before 1b
	}

	float BulletSegment(UWorld* World, const FVector& From, const FVector& To, const FVector& Vel, float Damage, bool bReal)
	{
		return 1.f;   // 1b: glass along the path
	}

	void AddBulletMark(UWorld* World, const FHitResult& Hit)
	{
		// 1b: lasting bullet holes
	}

	void ResetForMatch(UWorld* World)
	{
		if (AGSMapState* MS = ServerState(World)) MS->DoResetForMatch();
	}

	void EnterTestRoom(APlayerController* PC, bool bAll)
	{
		if (AGSMapState* MS = ServerState(PC ? PC->GetWorld() : nullptr)) MS->DoEnterTestRoom(PC, bAll);
	}

	bool IsDoorPassable(UWorld* World, const FVector& PosCm)
	{
		const AGSMapState* MS = AGSMapState::Get(World);
		if (!MS) return true;
		for (const auto& Pair : MS->GetDoors())
		{
			const AGSDoor* D = Pair.Value.Get();
			if (!D || !D->IsLocked() || D->IsOpen()) continue;
			if (FVector::DistSquared(D->GetBase() + FVector(0.f, 0.f, 90.f), PosCm) < FMath::Square(100.f + D->GetDef().WidthCm * 0.5f)) return false;
		}
		return true;
	}

	void ForEachDoor(UWorld* World, TFunctionRef<void(const FGSDoorInfo&)> Fn)
	{
		const AGSMapState* MS = AGSMapState::Get(World);
		if (!MS) return;
		for (const auto& Pair : MS->GetDoors())
		{
			if (const AGSDoor* D = Pair.Value.Get()) Fn(D->GetInfo());
		}
	}

	FGSOnWalkableChanged& OnWalkableChanged()
	{
		static FGSOnWalkableChanged D;
		return D;
	}

	FGSOnNoise& OnNoise()
	{
		static FGSOnNoise D;
		return D;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Console (host or alone; in the editor's Play through the console key)

namespace GSMapStateImpl
{
	bool HostWorld(UWorld* World, const TCHAR* Cmd)
	{
		if (!World || !World->IsGameWorld())
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: bare i spillet (trykk Play først)"), Cmd);
			return false;
		}
		if (World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: bare verten kan gjøre dette"), Cmd);
			return false;
		}
		if (!AGSMapState::Get(World))
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: kartets tilstand finnes ikke ennå"), Cmd);
			return false;
		}
		return true;
	}

	bool LookHit(UWorld* World, FHitResult& Hit)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (!PC) return false;
		FVector Eye;
		FRotator Rot;
		PC->GetPlayerViewPoint(Eye, Rot);
		FCollisionQueryParams QP(SCENE_QUERY_STAT(GSIntLook), true, PC->GetPawn());
		return World->LineTraceSingleByChannel(Hit, Eye, Eye + Rot.Vector() * 3000.f, ECC_Visibility, QP);
	}

	void CmdTestrom(const TArray<FString>& Args, UWorld* World)
	{
		if (!HostWorld(World, TEXT("Interaktiv.Testrom"))) return;
		const bool bAll = Args.Num() > 0 && (Args[0].Equals(TEXT("alle"), ESearchCase::IgnoreCase) || Args[0].Equals(TEXT("all"), ESearchCase::IgnoreCase));
		GSMapState::EnterTestRoom(World->GetFirstPlayerController(), bAll);
	}

	void CmdReset(const TArray<FString>& Args, UWorld* World)
	{
		if (!HostWorld(World, TEXT("Interaktiv.Tilbakestill"))) return;
		GSMapState::ResetForMatch(World);
	}

	void CmdVis(const TArray<FString>& Args, UWorld* World)
	{
		if (!HostWorld(World, TEXT("Interaktiv.Vis"))) return;
		APlayerController* PC = World->GetFirstPlayerController();
		FVector Eye = FVector::ZeroVector;
		FRotator Rot;
		if (PC) PC->GetPlayerViewPoint(Eye, Rot);
		int32 N = 0;
		for (const auto& Pair : AGSMapState::Get(World)->GetDoors())
		{
			const AGSDoor* D = Pair.Value.Get();
			if (!D || FVector::Dist(D->GetBase(), Eye) > 1500.f) continue;
			const FColor Col = D->IsLocked() ? FColor(255, 210, 90) : (D->GetLock() == GSLock::Broken ? FColor(255, 120, 60) : FColor::White);
			DrawDebugString(World, D->GetBase() + FVector(0.f, 0.f, 235.f), D->DebugText(), nullptr, Col, 10.f, true, 1.1f);
			UE_LOG(LogTemp, Log, TEXT("Interaktiv.Vis: %s"), *D->DebugText());
			++N;
		}
		UE_LOG(LogTemp, Log, TEXT("Interaktiv.Vis: %d dører innen 15 m"), N);
	}

	void CmdSmell(const TArray<FString>& Args, UWorld* World)
	{
		if (!HostWorld(World, TEXT("Interaktiv.Smell"))) return;
		const float RadiusM = Args.Num() > 0 ? FMath::Clamp(FCString::Atof(*Args[0]), 0.5f, 20.f) : 3.f;
		FHitResult Hit;
		if (!LookHit(World, Hit))
		{
			UE_LOG(LogTemp, Warning, TEXT("Interaktiv.Smell: ser ikke noe innen 30 m"));
			return;
		}
		const FVector At = Hit.ImpactPoint + Hit.ImpactNormal * 20.f;
		AGSShortFX::SpawnExplosion(World, At, RadiusM * 100.f);
		GSAudio::PlayAt(World, TEXT("Shot_Launcher"), At, 1.f, 120.f, 0.8f);
		GSMapState::ApplyBlast(World, At, RadiusM * 100.f, 100.f);
	}

	void CmdKnus(const TArray<FString>& Args, UWorld* World)
	{
		if (!HostWorld(World, TEXT("Interaktiv.Knus"))) return;
		const float Damage = Args.Num() > 0 ? FMath::Max(1.f, FCString::Atof(*Args[0])) : 40.f;
		FHitResult Hit;
		if (!LookHit(World, Hit))
		{
			UE_LOG(LogTemp, Warning, TEXT("Interaktiv.Knus: ser ikke noe innen 30 m"));
			return;
		}
		AGSShortFX::SpawnImpact(World, Hit.ImpactPoint, Hit.ImpactNormal);
		GSMapState::ApplyBulletHit(World, Hit, Damage);
		UE_LOG(LogTemp, Log, TEXT("Interaktiv.Knus: traff %s (%s)"), Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("ingenting"),
			Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("-"));
	}

	/** A short check of every door type, run on the host with its own body (for headless runs: read the log). */
	void CmdSelvtest(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_Client)
		{
			HostWorld(World, TEXT("Interaktiv.Selvtest"));
			return;
		}
		APlayerController* PC = World->GetFirstPlayerController();
		AGSCharacter* C = PC ? Cast<AGSCharacter>(PC->GetPawn()) : nullptr;
		AGSMapState* MS = AGSMapState::Get(World);
		if (!C || !MS || !MS->IsRegistryBuilt())
		{
			// Started from the command line (-ExecCmds): wait for the body and the doors
			static int32 Tries = 0;
			if (++Tries > 40)
			{
				UE_LOG(LogTemp, Warning, TEXT("Interaktiv.Selvtest: ingen figur eller ingen dører etter 40 s"));
				return;
			}
			FTimerHandle H;
			TWeakObjectPtr<UWorld> WW(World);
			const TArray<FString> ArgsCopy = Args;
			World->GetTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([WW, ArgsCopy]()
			{
				if (UWorld* W2 = WW.Get()) CmdSelvtest(ArgsCopy, W2);
			}), 1.f, false);
			return;
		}
		MS->DoEnterTestRoom(PC, false);
		TArray<FGSDoorDef> Defs;
		GSMapStateImpl::CollectTestDefs(Defs);
		auto Door = [&](int32 I) { return MS->FindDoor(GSPart::MakeKey(GSPart::Door, Defs[I].Id)); };
		int32 Pass = 0, Fail = 0;
		auto Check = [&](bool bOk, const TCHAR* What)
		{
			bOk ? ++Pass : ++Fail;
			UE_LOG(LogTemp, Log, TEXT("Interaktiv.Selvtest: %s  %s"), bOk ? TEXT("OK  ") : TEXT("FEIL"), What);
		};
		auto StandAt = [&](AGSDoor* D, int32 Side)
		{
			const FVector Loc = D->GetBase() + D->GetNormal() * (Side * 110.f) + FVector(0.f, 0.f, 95.f);
			C->SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);
		};
		auto Use = [&](AGSDoor* D, EGSUseVerb Verb, FGSUseResult& R)
		{
			R = FGSUseResult();
			const bool bCan = GSInteract::CanUse(C, D);
			if (bCan) D->ServerUse(C, Verb, R);
			return bCan;
		};
		FGSUseResult R;
		C->ConsumeItem(TEXT("Keycard"), C->CountItem(TEXT("Keycard")));
		C->ConsumeItem(TEXT("Lockpick"), C->CountItem(TEXT("Lockpick")));
		// 1 wood, unlocked
		AGSDoor* D0 = Door(0);
		StandAt(D0, 1);
		Check(Use(D0, EGSUseVerb::Tap, R), TEXT("tredør: kan brukes fra 1,1 m (avstand + fri linje)"));
		Check(D0->IsOpen() && D0->SideOf(C->GetPawnViewLocation()) == 1, TEXT("tredør: E åpner den"));
		Use(D0, EGSUseVerb::Tap, R);
		Check(!D0->IsOpen(), TEXT("tredør: E igjen lukker den"));
		Use(D0, EGSUseVerb::HoldStart, R);
		Check(D0->IsOpen(), TEXT("tredør: hold E sparker den opp"));
		// 2 wood, locked
		AGSDoor* D1 = Door(1);
		StandAt(D1, 1);
		Use(D1, EGSUseVerb::Tap, R);
		Check(!D1->IsOpen() && !R.Feedback.IsEmpty(), TEXT("låst tredør: E utenfra gir «Låst.»"));
		Use(D1, EGSUseVerb::HoldStart, R);
		Check(R.Job.Kind == GSJob::Kick, TEXT("låst tredør: hold E starter spark"));
		StandAt(D1, -1);
		Use(D1, EGSUseVerb::Tap, R);
		Check(D1->IsOpen() && D1->IsLocked(), TEXT("låst tredør: åpnes innenfra, låsen blir låst"));
		Use(D1, EGSUseVerb::Tap, R);
		// 3 glass door with code lock
		AGSDoor* D2 = Door(2);
		StandAt(D2, 1);
		Use(D2, EGSUseVerb::Tap, R);
		Check(!D2->IsOpen() && D2->IsLocked(), TEXT("glassdør: uten kort forblir den låst"));
		Use(D2, EGSUseVerb::HoldStart, R);
		Check(R.Job.Kind == GSJob::Force, TEXT("glassdør: hold E uten dirkesett = bryte opp"));
		C->AddToBackpack(TEXT("Lockpick"), 1);
		Use(D2, EGSUseVerb::HoldStart, R);
		Check(R.Job.Kind == GSJob::Pick, TEXT("glassdør: hold E med dirkesett = dirke"));
		C->AddToBackpack(TEXT("Keycard"), 1);
		Use(D2, EGSUseVerb::Tap, R);
		Check(D2->IsOpen() && D2->GetLock() == GSLock::Open, TEXT("glassdør: kortet låser opp og åpner"));
		// 4 double
		AGSDoor* D3 = Door(3);
		StandAt(D3, -1);
		Use(D3, EGSUseVerb::Tap, R);
		Check(D3->IsOpen(), TEXT("dobbeldør: åpnes"));
		// 6 alarm door: shut from the main room, opens from the stairwell
		AGSDoor* D5 = Door(5);
		StandAt(D5, -1);
		Use(D5, EGSUseVerb::Tap, R);
		Check(!D5->IsOpen(), TEXT("alarmdør: kan ikke åpnes fra hovedrommet"));
		StandAt(D5, 1);
		Use(D5, EGSUseVerb::Tap, R);
		Check(D5->IsOpen() && D5->IsLocked(), TEXT("alarmdør: panikkstanga åpner fra trapperommet"));
		// Shot at the lock of a locked wooden door: reset first
		MS->DoResetForMatch();
		Check(!D0->IsOpen() && D1->IsLocked() && D2->IsLocked() && !D5->IsOpen(), TEXT("tilbakestilling: alle dører som ved start"));
		// the middle of the leaf; the lock is at the edge
		FHitResult Hit(D1, nullptr, D1->GetBase() + FVector(0.f, 0.f, 105.f), D1->GetNormal());
		GSMapState::ApplyBulletHit(World, Hit, 30.f);
		Check(D1->IsLocked(), TEXT("skudd midt på låst tredør: låsen holder"));
		GSMapState::ApplyBlast(World, D2->GetBase() + D2->GetNormal() * 150.f + FVector(0.f, 0.f, 100.f), 300.f, 100.f);
		Check(D2->IsOpen() && D2->GetLock() == GSLock::Broken, TEXT("smell foran glassdøra: slår opp, låsen ødelagt"));
		MS->DoResetForMatch();
		C->ConsumeItem(TEXT("Keycard"), C->CountItem(TEXT("Keycard")));
		C->ConsumeItem(TEXT("Lockpick"), C->CountItem(TEXT("Lockpick")));
		MS->DoEnterTestRoom(PC, false);
		UE_LOG(LogTemp, Log, TEXT("Interaktiv.Selvtest: %d OK, %d FEIL"), Pass, Fail);
	}

	FAutoConsoleCommandWithWorldAndArgs GCmdTestrom(TEXT("Interaktiv.Testrom"),
		TEXT("Bygger testrommet for dører og flytter deg dit ('Interaktiv.Testrom alle' flytter alle)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CmdTestrom));
	FAutoConsoleCommandWithWorldAndArgs GCmdReset(TEXT("Interaktiv.Tilbakestill"),
		TEXT("Setter alle dører og låser tilbake som ved kampstart."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CmdReset));
	FAutoConsoleCommandWithWorldAndArgs GCmdVis(TEXT("Interaktiv.Vis"),
		TEXT("Viser id og tilstand for dører innen 15 m i 10 sekunder."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CmdVis));
	FAutoConsoleCommandWithWorldAndArgs GCmdSmell(TEXT("Interaktiv.Smell"),
		TEXT("Et smell der du ser (radius i meter, standard 3): dører slår opp og låsene går i stykker."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CmdSmell));
	FAutoConsoleCommandWithWorldAndArgs GCmdKnus(TEXT("Interaktiv.Knus"),
		TEXT("Et kuletreff på det du ser på (skade, standard 40): kodelåser og låsen på tredører."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CmdKnus));
	FAutoConsoleCommandWithWorldAndArgs GCmdSelvtest(TEXT("Interaktiv.Selvtest"),
		TEXT("Prøver alle dørtypene i testrommet med figuren din og skriver resultatet i loggen."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CmdSelvtest));
}
