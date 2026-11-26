#include "GSFlag.h"
#include "GSCharacter.h"
#include "GSGameMode.h"
#include "GSGameState.h"
#include "GSPlayerState.h"
#include "GSUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AGSFlag::AGSFlag()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(10.f);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto Visual = [this](const TCHAR* Name, USceneComponent* Parent)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Parent);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		C->CanCharacterStepUpOn = ECB_No;
		return C;
	};

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylF(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// Base plate on the ground: stays where the flag belongs, also while the flag is away.
	BasePlate = Visual(TEXT("BasePlate"), Root);
	BasePlate->SetStaticMesh(CylF.Object);
	BasePlate->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.04f));
	BasePlate->SetRelativeLocation(FVector(0.f, 0.f, 2.f));

	// Pole and cloth move in world space (carrier's back, on the ground where he fell).
	FlagRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FlagRoot"));
	FlagRoot->SetupAttachment(Root);
	FlagRoot->SetUsingAbsoluteLocation(true);
	FlagRoot->SetUsingAbsoluteRotation(true);

	Pole = Visual(TEXT("Pole"), FlagRoot);
	Pole->SetStaticMesh(CylF.Object);
	Pole->SetRelativeScale3D(FVector(0.035f, 0.035f, 2.3f));
	Pole->SetRelativeLocation(FVector(0.f, 0.f, 115.f));

	Cloth = Visual(TEXT("Cloth"), FlagRoot);
	Cloth->SetStaticMesh(CubeF.Object);
	Cloth->SetRelativeScale3D(FVector(0.85f, 0.015f, 0.5f));
	Cloth->SetRelativeLocation(FVector(45.f, 0.f, 200.f));
}

void AGSFlag::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSFlag, Team);
	DOREPLIFETIME(AGSFlag, State);
	DOREPLIFETIME(AGSFlag, Carrier);
	DOREPLIFETIME(AGSFlag, DropLocation);
}

void AGSFlag::BeginPlay()
{
	Super::BeginPlay();
	LastCarrierLocation = GetActorLocation();
	OnRep_Look();
}

void AGSFlag::OnRep_Look()
{
	if (AppliedTeam != Team)
	{
		AppliedTeam = Team;
		GSUtils::SetColor(Cloth, GSTeam::Color(Team));
		GSUtils::SetColor(BasePlate, GSTeam::Color(Team) * 0.6f);
		GSUtils::SetColor(Pole, FLinearColor(0.12f, 0.12f, 0.12f));
	}
	UpdatePose();
}

FVector AGSFlag::GetFlagLocation() const
{
	switch (State)
	{
	case EGSFlagState::Dropped:
		return DropLocation;
	case EGSFlagState::Carried:
		if (const APawn* P = Carrier ? Carrier->GetPawn() : nullptr)
		{
			// Pole along the back, cloth above the head.
			LastCarrierLocation = P->GetActorLocation() - P->GetActorForwardVector() * 28.f + FVector(0.f, 0.f, -50.f);
			LastCarrierYaw = P->GetActorRotation().Yaw;
		}
		return LastCarrierLocation;   // pawn not known on this machine: last place we saw it
	default:
		return GetActorLocation();
	}
}

void AGSFlag::UpdatePose()
{
	if (!FlagRoot) return;
	const FVector Loc = GetFlagLocation();
	FlagRoot->SetWorldLocation(Loc);
	switch (State)
	{
	case EGSFlagState::Carried:
		// Cloth trails behind the carrier.
		FlagRoot->SetWorldRotation(FRotator(0.f, LastCarrierYaw + 180.f, 0.f));
		break;
	case EGSFlagState::Dropped:
		FlagRoot->SetWorldRotation(FRotator(0.f, GetActorRotation().Yaw, 80.f));   // lying on the ground
		break;
	default:
		FlagRoot->SetWorldRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
		break;
	}

	// In first person the carrier would have the pole in his face: hide it for him only.
	bool bMine = false;
	if (State == EGSFlagState::Carried && Carrier)
	{
		const APlayerController* LPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		bMine = LPC && LPC->IsLocalController() && LPC->PlayerState == Carrier;
	}
	FlagRoot->SetHiddenInGame(bMine, true);
}

void AGSFlag::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdatePose();
	if (HasAuthority())
	{
		ServerAccum += DeltaSeconds;
		if (ServerAccum >= 0.1f)
		{
			ServerAccum = 0.f;
			ServerStep();
		}
	}
}

void AGSFlag::ServerStep()
{
	if (!HasAuthority()) return;
	UWorld* World = GetWorld();
	const AGSGameState* GS = World ? World->GetGameState<AGSGameState>() : nullptr;
	if (!GS || GS->Phase != EGSMatchPhase::Playing) return;

	if (State == EGSFlagState::Carried)
	{
		// A carrier who left the game: his player state is being destroyed.
		AGSPlayerState* PS = IsValid(Carrier) ? Cast<AGSPlayerState>(Carrier) : nullptr;
		AGSCharacter* C = PS ? Cast<AGSCharacter>(PS->GetPawn()) : nullptr;
		if (!PS || !IsValid(C) || C->bDead || PS->Team == Team)
		{
			Drop();
			return;
		}
		// Capture: at the own base, and only while the own flag is at home.
		for (TActorIterator<AGSFlag> It(World); It; ++It)
		{
			AGSFlag* Own = *It;
			if (Own == this || Own->Team != PS->Team) continue;
			if (Own->State == EGSFlagState::Home
				&& FVector::Dist2D(C->GetActorLocation(), Own->GetActorLocation()) < CaptureRadius
				&& FMath::Abs(C->GetActorLocation().Z - Own->GetActorLocation().Z) < 250.f)
			{
				Capture(PS);
			}
			break;
		}
		return;
	}

	// Home or dropped: someone walks up to it.
	const FVector FlagLoc = GetFlagLocation();
	for (TActorIterator<AGSCharacter> It(World); It; ++It)
	{
		AGSCharacter* C = *It;
		if (C->bDead) continue;
		AGSPlayerState* PS = C->GetPlayerState<AGSPlayerState>();
		if (!PS || PS->Team == GSTeam::None) continue;
		const FVector Loc = C->GetActorLocation();
		if (FVector::Dist2D(Loc, FlagLoc) > TouchRadius || FMath::Abs(Loc.Z - FlagLoc.Z) > 200.f) continue;
		// Not through a wall or a floor
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSFlagTouch), false, C);
		Params.AddIgnoredActor(this);
		// Object trace against world geometry only: a channel trace also hits player capsules, so a defender
		// standing on his own flag would make it impossible to take.
		if (World->LineTraceTestByObjectType(Loc, FlagLoc + FVector(0.f, 0.f, 60.f), FCollisionObjectQueryParams(ECC_WorldStatic), Params)) continue;

		if (PS->Team != Team)
		{
			if (PS->CarriedFlagTeam == 0)
			{
				Take(C);
				return;
			}
		}
		else if (State == EGSFlagState::Dropped)
		{
			ReturnHome(PS);
			return;
		}
	}

	if (State == EGSFlagState::Dropped
		&& (World->GetTimeSeconds() - DroppedAt > SafetyReturnSeconds || DropLocation.Z < -5000.f))
	{
		ReturnHome(nullptr);
	}
}

void AGSFlag::Take(AGSCharacter* By)
{
	AGSPlayerState* PS = By ? By->GetPlayerState<AGSPlayerState>() : nullptr;
	if (!PS) return;
	State = EGSFlagState::Carried;
	Carrier = PS;
	PS->CarriedFlagTeam = Team;
	PS->ForceNetUpdate();
	Announce(GSFlagEvent::Taken, By->GetActorLocation(), PS, TEXT("tok"));
}

void AGSFlag::Drop()
{
	AGSPlayerState* Old = IsValid(Carrier) ? Cast<AGSPlayerState>(Carrier) : nullptr;
	APawn* P = Old ? Old->GetPawn() : nullptr;
	if (!IsValid(P)) P = nullptr;
	const FVector From = P ? P->GetActorLocation() : GetFlagLocation();

	FVector Floor = From;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSFlagDrop), false, P);
	Params.AddIgnoredActor(this);
	// World geometry only (not players), and far down: a carrier who dies in the air drops the flag to the ground.
	if (GetWorld()->LineTraceSingleByObjectType(Hit, From + FVector(0.f, 0.f, 50.f), From - FVector(0.f, 0.f, 5000.f), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		Floor = Hit.ImpactPoint;
	}
	DropLocation = Floor;
	State = EGSFlagState::Dropped;
	DroppedAt = GetWorld()->GetTimeSeconds();
	if (Old)
	{
		Old->CarriedFlagTeam = 0;
		Old->ForceNetUpdate();
	}
	Carrier = nullptr;
	// Carrier left the game: nobody to name, so the line reads "Det blå flagget [ligger ute]".
	Announce(GSFlagEvent::Dropped, Floor, Old, Old ? TEXT("mistet") : TEXT("ligger ute"));
}

void AGSFlag::ReturnHome(AGSPlayerState* By)
{
	State = EGSFlagState::Home;
	Carrier = nullptr;
	if (By)
	{
		By->Returns++;
		By->ForceNetUpdate();
		Announce(GSFlagEvent::Returned, GetActorLocation(), By, TEXT("hentet hjem"));
	}
	else
	{
		Announce(GSFlagEvent::Returned, GetActorLocation(), nullptr, TEXT("hentet hjem"));
	}
}

void AGSFlag::Capture(AGSPlayerState* By)
{
	if (!By) return;
	const FVector Where = By->GetPawn() ? By->GetPawn()->GetActorLocation() : GetFlagLocation();
	By->CarriedFlagTeam = 0;
	By->Captures++;
	By->SetScore(By->GetScore() + 5.f);
	By->ForceNetUpdate();
	State = EGSFlagState::Home;
	Carrier = nullptr;
	Announce(GSFlagEvent::Captured, Where, By, TEXT("erobret"));
	if (AGSGameMode* GM = GetWorld()->GetAuthGameMode<AGSGameMode>())
	{
		GM->OnFlagCaptured(By->Team);
	}
}

void AGSFlag::Announce(uint8 Event, const FVector& Where, APlayerState* Who, const TCHAR* Verb)
{
	if (AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>())
	{
		GS->MulticastFlagEvent(Event, Team, FVector_NetQuantize(Where), Who);
		// Also a line in the event list on the left, so it shows in today's HUD: "Kari  [tok]  det blaa flagget"
		if (Who) GS->AddKill(Who->GetPlayerName(), GSTeam::FlagName(Team), Verb, false);
		else GS->AddKill(Team == GSTeam::Red ? TEXT("Det røde flagget") : TEXT("Det blå flagget"), FString(), Verb, false);
	}
	OnRep_Look();
	ForceNetUpdate();
}

void AGSFlag::ResetHome()
{
	if (AGSPlayerState* Old = Cast<AGSPlayerState>(Carrier))
	{
		Old->CarriedFlagTeam = 0;
		Old->ForceNetUpdate();
	}
	State = EGSFlagState::Home;
	Carrier = nullptr;
	DroppedAt = 0.f;
	OnRep_Look();
	ForceNetUpdate();
}
