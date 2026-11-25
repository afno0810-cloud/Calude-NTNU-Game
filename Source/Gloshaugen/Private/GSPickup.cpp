#include "GSPickup.h"
#include "GSCharacter.h"
#include "GSUtils.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGSPickup::AGSPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetNetUpdateFrequency(5.f);

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(110.f);
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = Trigger;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Trigger);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Glow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Trigger);
	Glow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, -45.f));
	Glow->SetRelativeScale3D(FVector(0.9f, 0.9f, 0.02f));
	Glow->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylF(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Mesh->SetStaticMesh(CubeF.Object);
	Glow->SetStaticMesh(CylF.Object);
}

void AGSPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSPickup, WeaponId);
	DOREPLIFETIME(AGSPickup, bAvailable);
}

void AGSPickup::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGSPickup::OnOverlap);
	}
	OnRep_State();
}

void AGSPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() != NM_DedicatedServer)
	{
		Mesh->AddLocalRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));
		Mesh->SetRelativeLocation(FVector(0.f, 0.f, FMath::Sin(GetWorld()->GetTimeSeconds() * 2.f) * 8.f));
	}
}

void AGSPickup::OnRep_State()
{
	const FGSWeaponStats& S = GSWeapons::Get(WeaponId);
	Mesh->SetRelativeScale3D(S.MeshScale);
	GSUtils::SetColor(Mesh, S.Color);
	// Color of the ring shows how valuable it is (rarity system comes later).
	GSUtils::SetColor(Glow, WeaponId == EGSWeaponId::AG3 ? FLinearColor(0.1f, 0.5f, 1.f) : FLinearColor(0.6f, 0.6f, 0.6f));
	Mesh->SetVisibility(bAvailable);
	Glow->SetVisibility(bAvailable);
}

void AGSPickup::OnOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	TryGiveTo(Other);
}

void AGSPickup::TryGiveTo(AActor* Other)
{
	if (!HasAuthority() || !bAvailable) return;
	AGSCharacter* C = Cast<AGSCharacter>(Other);
	if (!C || C->bDead) return;
	if (C->GiveWeapon(WeaponId))
	{
		bAvailable = false;
		OnRep_State();
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &AGSPickup::Respawn, RespawnSeconds, false);
	}
}

void AGSPickup::Respawn()
{
	bAvailable = true;
	OnRep_State();
	// Someone may already be standing on it.
	TArray<AActor*> Overlapping;
	Trigger->GetOverlappingActors(Overlapping, AGSCharacter::StaticClass());
	for (AActor* A : Overlapping)
	{
		TryGiveTo(A);
		if (!bAvailable) break;
	}
}
