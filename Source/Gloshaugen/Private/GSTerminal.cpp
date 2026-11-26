#include "GSTerminal.h"
#include "GSCharacter.h"
#include "GSItems.h"
#include "GSPickup.h"
#include "GSUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace
{
	UStaticMesh* LoadModel(const TCHAR* Name)
	{
		return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Models/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	float ServerNow(const UWorld* W)
	{
		if (!W) return 0.f;
		const AGameStateBase* GS = W->GetGameState();
		return GS ? GS->GetServerWorldTimeSeconds() : W->GetTimeSeconds();
	}
}

// ------------------------------------------------------------------------------------------------ AGSTerminal

AGSTerminal::AGSTerminal()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	bReplicates = true;
	SetNetUpdateFrequency(2.f);
	SetNetCullDistanceSquared(FMath::Square(15000.f));

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));

	Screen = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Mesh);
	Screen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Screen->SetCastShadow(false);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Mesh);
	Glow->SetIntensity(400.f);
	Glow->SetAttenuationRadius(220.f);
	Glow->SetCastShadows(false);
}

void AGSTerminal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSTerminal, Kind);
	DOREPLIFETIME(AGSTerminal, Label);
	DOREPLIFETIME(AGSTerminal, PrintingId);
	DOREPLIFETIME(AGSTerminal, PrintStart);
	DOREPLIFETIME(AGSTerminal, PrintEnd);
}

const TArray<FGSPrintRecipe>& AGSTerminal::Recipes()
{
	static const TArray<FGSPrintRecipe> List = {
		{ TEXT("PrintKnife"),   1,  8.f },
		{ TEXT("Lockpick"),     1, 10.f },
		{ TEXT("Compass"),      1,  6.f },
		{ TEXT("PrintPistol"),  3, 25.f },
		{ TEXT("Suppressor"),   2, 15.f },
		{ TEXT("RedDot"),       2, 15.f },
		{ TEXT("ExtMag"),       2, 15.f },
		{ TEXT("ArmorPlate"),   3, 20.f },
		{ TEXT("PrintHelmet"),  4, 30.f },
		{ TEXT("Grapple"),      3, 25.f },
		{ TEXT("TripMine"),     4, 30.f },
		{ TEXT("PrintDrone"),   5, 40.f },
		{ TEXT("Drone"),        8, 60.f },
	};
	return List;
}

void AGSTerminal::BeginPlay()
{
	Super::BeginPlay();
	ApplyLook();
}

void AGSTerminal::OnRep_Kind()
{
	ApplyLook();
}

void AGSTerminal::ApplyLook()
{
	// Real models from Blender if they are imported, simple boxes otherwise.
	const bool bPC = Kind == EGSTerminalKind::Computer;
	if (UStaticMesh* Model = LoadModel(bPC ? TEXT("SM_Prop_PC") : TEXT("SM_Prop_3DPrinter")))
	{
		Mesh->SetStaticMesh(Model);
		Mesh->SetRelativeScale3D(FVector(1.f));
		Mesh->SetRelativeLocation(FVector::ZeroVector);
		Screen->SetVisibility(false);
	}
	else
	{
		Mesh->SetStaticMesh(GSUtils::Cube());
		const FVector Size = bPC ? FVector(0.7f, 1.2f, 0.75f) : FVector(0.5f, 0.5f, 0.6f);
		Mesh->SetRelativeScale3D(Size);
		Mesh->SetRelativeLocation(FVector(0.f, 0.f, Size.Z * 50.f));
		GSUtils::SetColor(Mesh, bPC ? FLinearColor(0.08f, 0.08f, 0.09f) : FLinearColor(0.85f, 0.85f, 0.86f));
		Screen->SetStaticMesh(GSUtils::Cube());
		Screen->SetRelativeLocation(FVector(51.f, 0.f, 10.f));
		Screen->SetRelativeScale3D(FVector(0.02f, 0.8f, 0.6f));
		GSUtils::SetColor(Screen, bPC ? FLinearColor(0.1f, 0.4f, 1.f) : FLinearColor(1.f, 0.5f, 0.1f));
		Screen->SetVisibility(true);
	}
	Glow->SetRelativeLocation(FVector(60.f, 0.f, 50.f));
	Glow->SetLightColor(bPC ? FLinearColor(0.35f, 0.55f, 1.f) : FLinearColor(1.f, 0.55f, 0.2f));
}

void AGSTerminal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() == NM_DedicatedServer) return;
	// Screens flicker a little; a busy printer glows and pulses.
	const float T = GetWorld()->GetTimeSeconds();
	if (Kind == EGSTerminalKind::Computer)
	{
		Glow->SetIntensity(320.f + 60.f * FMath::Sin(T * 7.f + GetUniqueID()));
	}
	else
	{
		Glow->SetIntensity(IsBusy() ? 700.f + 300.f * FMath::Sin(T * 5.f) : 120.f);
	}
}

FVector AGSTerminal::FrontPoint(float Distance) const
{
	return GetActorLocation() + GetActorForwardVector() * Distance;
}

FString AGSTerminal::StartPrint(AGSCharacter* Who, int32 Index)
{
	check(HasAuthority());
	if (Kind != EGSTerminalKind::Printer) return TEXT("Dette er ikke en 3D-printer");
	if (IsBusy()) return TEXT("Printeren er opptatt");
	if (!Recipes().IsValidIndex(Index) || !Who) return TEXT("Ukjent oppskrift");
	const FGSPrintRecipe& R = Recipes()[Index];
	const FGSItemDef* Def = GSItems::Find(R.ItemId);
	if (!Def) return TEXT("Ukjent gjenstand");
	static const FName Filament(TEXT("Filament"));
	if (!Who->ConsumeItem(Filament, R.Filament))
	{
		return FString::Printf(TEXT("Du trenger %d filament-spoler (du har %d)"), R.Filament, Who->CountItem(Filament));
	}
	PrintingId = R.ItemId;
	PrintStart = ServerNow(GetWorld());
	PrintEnd = PrintStart + R.Seconds;
	GetWorldTimerManager().SetTimer(PrintTimer, this, &AGSTerminal::FinishPrint, R.Seconds, false);
	ForceNetUpdate();
	return FString();
}

void AGSTerminal::FinishPrint()
{
	if (PrintingId.IsNone()) return;
	AGSPickup::SpawnDropped(GetWorld(), FrontPoint(80.f) + FVector(0.f, 0.f, 40.f), PrintingId, 1);
	PrintingId = NAME_None;
	ForceNetUpdate();
}

void AGSTerminal::CancelPrint()
{
	GetWorldTimerManager().ClearTimer(PrintTimer);
	PrintingId = NAME_None;
	PrintStart = PrintEnd = 0.f;
	ForceNetUpdate();
}

// ------------------------------------------------------------------------------------------------ AGSSecurityCamera

AGSSecurityCamera::AGSSecurityCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(0.5f);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetFieldOfView(95.f);
	Camera->SetRelativeLocation(FVector(25.f, 0.f, -10.f));
	RecLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Rec"));
	RecLight->SetupAttachment(Mesh);
	RecLight->SetRelativeLocation(FVector(20.f, 0.f, 5.f));
	RecLight->SetLightColor(FLinearColor(1.f, 0.05f, 0.05f));
	RecLight->SetIntensity(60.f);
	RecLight->SetAttenuationRadius(40.f);
	RecLight->SetCastShadows(false);

	if (UStaticMesh* Model = LoadModel(TEXT("SM_Prop_SecurityCamera")))
	{
		Mesh->SetStaticMesh(Model);
	}
}

void AGSSecurityCamera::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSSecurityCamera, Label);
	DOREPLIFETIME(AGSSecurityCamera, SweepDeg);
}

void AGSSecurityCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Mesh->GetStaticMesh())
	{
		if (UStaticMesh* Model = LoadModel(TEXT("SM_Prop_SecurityCamera"))) Mesh->SetStaticMesh(Model);
		else { Mesh->SetStaticMesh(GSUtils::Cube()); Mesh->SetRelativeScale3D(FVector(0.35f, 0.15f, 0.15f)); }
	}
	if (Phase == 0.f) Phase = FMath::FRandRange(0.f, 6.28f);
	// Slow left-right sweep, same on server and clients (driven by server time).
	const float Yaw = FMath::Sin(ServerNow(GetWorld()) * 0.25f + (GetActorLocation().X + GetActorLocation().Y) * 0.001f) * SweepDeg;
	const FRotator R(-18.f, Yaw, 0.f);
	Mesh->SetRelativeRotation(R);
	Camera->SetRelativeRotation(R);
	Camera->SetRelativeLocation(R.RotateVector(FVector(30.f, 0.f, -5.f)));
	RecLight->SetVisibility(FMath::Fmod(GetWorld()->GetTimeSeconds(), 1.2f) < 0.6f);
}
