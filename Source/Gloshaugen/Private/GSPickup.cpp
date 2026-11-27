#include "GSPickup.h"
#include "GSCharacter.h"
#include "GSUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGSPickup::AGSPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetNetUpdateFrequency(2.f);
	SetNetCullDistanceSquared(FMath::Square(25000.f));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto Visual = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Root);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		C->CanCharacterStepUpOn = ECB_No;
		return C;
	};
	Mesh = Visual(TEXT("Mesh"));
	Mesh->SetCastShadow(false);   // it floats and turns: a moving shadow on hundreds of them costs more than it gives
	Glow = Visual(TEXT("Glow"));
	Beam = Visual(TEXT("Beam"));
	Crate = Visual(TEXT("Crate"));

	Glow->SetRelativeLocation(FVector(0.f, 0.f, -45.f));
	Glow->SetRelativeScale3D(FVector(0.9f, 0.9f, 0.02f));
	Glow->SetCastShadow(false);
	Beam->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	Beam->SetRelativeScale3D(FVector(0.04f, 0.04f, 4.f));
	Beam->SetCastShadow(false);
	Crate->SetRelativeLocation(FVector(0.f, 0.f, -20.f));
	Crate->SetRelativeScale3D(FVector(0.9f, 0.6f, 0.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylF(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Mesh->SetStaticMesh(CubeF.Object);
	Crate->SetStaticMesh(CubeF.Object);
	Glow->SetStaticMesh(CylF.Object);
	Beam->SetStaticMesh(CylF.Object);
}

void AGSPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSPickup, ItemId);
	DOREPLIFETIME(AGSPickup, Count);
	DOREPLIFETIME(AGSPickup, bAvailable);
	DOREPLIFETIME(AGSPickup, bLocked);
}

AGSPickup* AGSPickup::SpawnDropped(UWorld* World, const FVector& Location, FName ItemId, int32 Count, int32 Ammo, int32 Reserve, uint16 Attachments)
{
	if (!World || !GSItems::Find(ItemId) || Count <= 0) return nullptr;
	// Put it on the floor
	FVector Loc = Location;
	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Location + FVector(0.f, 0.f, 50.f), Location - FVector(0.f, 0.f, 400.f), ECC_WorldStatic))
	{
		Loc = Hit.ImpactPoint + FVector(0.f, 0.f, 50.f);
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGSPickup* Pickup = World->SpawnActor<AGSPickup>(Loc, FRotator::ZeroRotator, P);
	if (!Pickup) return nullptr;
	Pickup->bRespawns = false;
	Pickup->ItemId = ItemId;
	Pickup->Count = Count;
	Pickup->Ammo = Ammo;
	Pickup->Reserve = Reserve;
	Pickup->Attachments = Attachments;
	Pickup->SetLifeSpan(180.f);
	Pickup->OnRep_State();
	return Pickup;
}

void AGSPickup::BeginPlay()
{
	Super::BeginPlay();
	OnRep_State();
	// Tick only turns the item for someone looking at it: nobody looks on a dedicated server.
	if (GetNetMode() == NM_DedicatedServer) SetActorTickEnabled(false);
}

void AGSPickup::Roll()
{
	ItemId = GSItems::RandomLoot(Rng, MinRarity);
	const FGSItemDef* Def = GSItems::Find(ItemId);
	Count = Def ? GSItems::PickupCount(*Def, Rng) : 1;
	Ammo = Reserve = -1;
	Attachments = 0;
	bLocked = bIsCrate;
	bAvailable = true;
	OnRep_State();
}

void AGSPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Only loot near what the local player sees turns and bobs (further through a scope); the rest checks twice a
	// second whether someone came closer. The tick interval is the only state.
	bool bNear = false;
	if (bAvailable && !bLocked)
	{
		const APlayerController* LPC = GetWorld()->GetFirstPlayerController();
		if (LPC && LPC->IsLocalController())
		{
			FVector Eye;
			FRotator Rot;
			LPC->GetPlayerViewPoint(Eye, Rot);
			const float FOV = LPC->PlayerCameraManager ? FMath::Max(LPC->PlayerCameraManager->GetFOVAngle(), 5.f) : 90.f;
			const float Range = 3000.f * FMath::Max(1.f, 90.f / FOV);
			bNear = FVector::DistSquared(Eye, GetActorLocation()) < FMath::Square(Range);
		}
	}
	SetActorTickInterval(bNear ? 0.f : 0.5f);
	if (!bNear) return;
	// Angle from the clock (not added up), so an item that wakes up does not jump.
	const float T = GetWorld()->GetTimeSeconds();
	Mesh->SetRelativeRotation(FRotator(0.f, FMath::Fmod(T * 60.f + float(GetUniqueID() % 360), 360.f), 0.f));
	Mesh->SetRelativeLocation(FVector(Mesh->GetRelativeLocation().X, Mesh->GetRelativeLocation().Y,
		BaseMeshZ + FMath::Sin(T * 2.f) * 6.f));
}

void AGSPickup::OnRep_State()
{
	const FGSItemDef* Def = GSItems::Find(ItemId);
	const FLinearColor RarityCol = Def ? GSItems::RarityColor(Def->Rarity) : FLinearColor::Gray;

	if (UStaticMesh* Model = GSItems::GetMesh(ItemId))
	{
		Mesh->SetStaticMesh(Model);
		Mesh->EmptyOverrideMaterials();
		// Small things (pills, cards) are scaled up so you can see them.
		const float Size = Model->GetBounds().BoxExtent.GetMax() * 2.f;
		const float Scale = FMath::Clamp(45.f / FMath::Max(Size, 1.f), 1.f, 4.f);
		Mesh->SetRelativeScale3D(FVector(Scale));
		// Center it over the ring
		const FVector Center = Model->GetBounds().Origin * Scale;
		Mesh->SetRelativeLocation(FVector(-Center.X, -Center.Y, -Center.Z));
		BaseMeshZ = -Center.Z;
	}
	else
	{
		Mesh->SetStaticMesh(GSUtils::Cube());
		Mesh->SetRelativeScale3D(Def && Def->IsWeapon() ? FVector(0.7f, 0.06f, 0.15f) : FVector(0.25f));
		Mesh->SetRelativeLocation(FVector::ZeroVector);
		BaseMeshZ = 0.f;
		GSUtils::SetColor(Mesh, RarityCol);
	}

	GSUtils::SetColor(Glow, RarityCol * 2.f);
	const bool bBeam = Def && Def->Rarity >= EGSRarity::Epic;
	if (bBeam)
	{
		GSUtils::SetColor(Beam, RarityCol * 6.f);
		Beam->SetRelativeScale3D(FVector(0.04f, 0.04f, Def->Rarity >= EGSRarity::Legendary ? 6.f : 3.f));
		Beam->SetRelativeLocation(FVector(0.f, 0.f, Def->Rarity >= EGSRarity::Legendary ? 250.f : 100.f));
	}
	GSUtils::SetColor(Crate, bLocked ? FLinearColor(0.12f, 0.16f, 0.1f) : FLinearColor(0.3f, 0.3f, 0.3f));

	Mesh->SetVisibility(bAvailable && !bLocked);
	Glow->SetVisibility(bAvailable);
	Beam->SetVisibility(bAvailable && bBeam);
	Crate->SetVisibility(bLocked);
}

void AGSPickup::Unlock()
{
	bLocked = false;
	OnRep_State();
}

void AGSPickup::OnTaken(int32 Taken)
{
	Count -= Taken;
	if (Count > 0)
	{
		OnRep_State();
		return;
	}
	bAvailable = false;
	OnRep_State();
	if (!bRespawns)
	{
		Destroy();
		return;
	}
	const float Delay = bIsCrate ? Rng.FRandRange(150.f, 240.f) : Rng.FRandRange(75.f, 150.f);
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &AGSPickup::Respawn, Delay, false);
}

void AGSPickup::Respawn()
{
	Roll();
}

void AGSPickup::ResetForMatch()
{
	GetWorldTimerManager().ClearTimer(RespawnTimer);
	Roll();
	ForceNetUpdate();
}
