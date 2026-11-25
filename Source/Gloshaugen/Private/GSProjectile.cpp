#include "GSProjectile.h"
#include "GSCharacter.h"
#include "GSEffects.h"
#include "GSItems.h"
#include "GSUtils.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AGSProjectile::AGSProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(6.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);     // fly through capsules ...
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphF(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Mesh->SetStaticMesh(SphF.Object);
	Mesh->SetRelativeScale3D(FVector(0.1f));

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->SetUpdatedComponent(Collision);
	Movement->bRotationFollowsVelocity = true;
	Movement->MaxSpeed = 20000.f;
	Movement->InitialSpeed = 0.f;
}

void AGSProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSProjectile, Kind);
	DOREPLIFETIME(AGSProjectile, ItemId);
	DOREPLIFETIME(AGSProjectile, bExploded);
}

AGSProjectile* AGSProjectile::Launch(UWorld* World, AGSCharacter* Owner, FName Kind, FName ItemId, const FVector& Location,
	const FVector& Velocity, float GravityScale, float Damage, float Radius, float Fuse)
{
	if (!World) return nullptr;
	AGSProjectile* Proj = World->SpawnActorDeferred<AGSProjectile>(AGSProjectile::StaticClass(), FTransform(Velocity.Rotation(), Location), Owner, Owner,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Proj) return nullptr;
	Proj->Kind = Kind;
	Proj->ItemId = ItemId;
	Proj->Damage = Damage;
	Proj->Radius = Radius;
	Proj->Fuse = Fuse;
	Proj->Movement->Velocity = Velocity;
	Proj->Movement->ProjectileGravityScale = GravityScale;
	const bool bBouncy = Kind == FName(TEXT("frag")) || Kind == FName(TEXT("smoke")) || Kind == FName(TEXT("flash"));
	Proj->Movement->bShouldBounce = bBouncy;
	Proj->Movement->Bounciness = 0.3f;
	Proj->Movement->Friction = 0.4f;
	Proj->FinishSpawning(FTransform(Velocity.Rotation(), Location));
	return Proj;
}

void AGSProjectile::BeginPlay()
{
	Super::BeginPlay();
	SpawnTime = GetWorld()->GetTimeSeconds();
	if (AActor* O = GetOwner()) Collision->IgnoreActorWhenMoving(O, true);
	if (HasAuthority())
	{
		Movement->OnProjectileStop.AddDynamic(this, &AGSProjectile::OnStop);
		SetLifeSpan(60.f);
	}
	// Bolts, rockets and 40 mm hit people too: let the sphere also block pawns' meshes via a sweep in Tick.
	OnRep_Visual();
}

void AGSProjectile::OnRep_Visual()
{
	if (UStaticMesh* Model = GSItems::GetMesh(ItemId))
	{
		const FGSItemDef* Def = GSItems::Find(ItemId);
		if (Def && !Def->IsWeapon()) // thrown item: show the item itself
		{
			Mesh->SetStaticMesh(Model);
			Mesh->SetRelativeScale3D(FVector(1.f));
			Mesh->EmptyOverrideMaterials();
			return;
		}
	}
	FLinearColor Col(0.15f, 0.17f, 0.1f);
	FVector Scale(0.1f);
	if (IsKind(TEXT("rocket"))) { Col = FLinearColor(0.2f, 0.25f, 0.15f); Scale = FVector(0.5f, 0.09f, 0.09f); }
	else if (IsKind(TEXT("arrow"))) { Col = FLinearColor(0.3f, 0.3f, 0.3f); Scale = FVector(0.4f, 0.02f, 0.02f); }
	else if (IsKind(TEXT("grenade40"))) { Col = FLinearColor(0.6f, 0.55f, 0.2f); Scale = FVector(0.1f, 0.05f, 0.05f); }
	Mesh->SetRelativeScale3D(Scale);
	GSUtils::SetColor(Mesh, Col);
}

void AGSProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Now = GetWorld()->GetTimeSeconds();
	const float Age = Now - SpawnTime;

	// Rockets leave a smoke trail
	if (!bExploded && IsKind(TEXT("rocket")) && GetNetMode() != NM_DedicatedServer)
	{
		AGSShortFX::SpawnBlob(GetWorld(), GetActorLocation(), FVector(0.25f), FLinearColor(0.6f, 0.6f, 0.6f), 0.8f);
	}

	if (!HasAuthority()) return;

	if (!bExploded && !bStuck && (IsKind(TEXT("arrow")) || IsKind(TEXT("rocket")) || IsKind(TEXT("grenade40"))))
	{
		// Direct hits on players (the collision sphere ignores pawns so it does not get stuck on capsules).
		const FVector Vel = Movement->Velocity;
		const FVector From = GetActorLocation();
		const FVector To = From + Vel * DeltaSeconds * 1.2f;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSProjHit), false, this);
		if (AActor* O = GetOwner()) Params.AddIgnoredActor(O);
		if (GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params))
		{
			if (AGSCharacter* Victim = Cast<AGSCharacter>(Hit.GetActor()))
			{
				if (IsKind(TEXT("arrow")))
				{
					FPointDamageEvent Event(Damage, Hit, Vel.GetSafeNormal(), nullptr);
					Victim->TakeDamage(Damage, Event, GetInstigatorController(), this);
					Destroy();
					return;
				}
				SetActorLocation(Hit.ImpactPoint);
				Explode();
				return;
			}
		}
	}

	if (!bExploded && Fuse > 0.f && Age >= Fuse) Explode();

	// Trip mine: goes off when someone else walks past
	if (bStuck && !bExploded && IsKind(TEXT("mine")) && Age > 2.f)
	{
		for (TActorIterator<AGSCharacter> It(GetWorld()); It; ++It)
		{
			if (*It == GetOwner() || It->bDead) continue;
			if (FVector::Dist(It->GetActorLocation(), GetActorLocation()) < 250.f) { Explode(); break; }
		}
	}

	// Molotov fire: damage people standing in it
	if (bExploded && IsKind(TEXT("fire")) && Now - LastAreaTick >= 0.5f)
	{
		LastAreaTick = Now;
		DamageAround(GetActorLocation(), Damage * 0.5f, Radius, false);
	}
}

void AGSProjectile::OnStop(const FHitResult& Hit)
{
	if (bExploded) return;
	bStuck = true;
	if (IsKind(TEXT("arrow")))
	{
		SetLifeSpan(10.f); // stays stuck in the wall for a while
		return;
	}
	if (IsKind(TEXT("rocket")) || IsKind(TEXT("grenade40")) || IsKind(TEXT("fire")))
	{
		Explode();
	}
	// frag/smoke/flash: wait for the fuse. c4/mine: stick where they land.
}

void AGSProjectile::DamageAround(const FVector& Center, float Amount, float R, bool bFalloff)
{
	for (TActorIterator<AGSCharacter> It(GetWorld()); It; ++It)
	{
		AGSCharacter* C = *It;
		if (C->bDead) continue;
		const float Dist = FVector::Dist(C->GetActorLocation(), Center);
		if (Dist > R) continue;
		FHitResult Block;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBlast), false, this);
		Params.AddIgnoredActor(C);
		if (GetWorld()->LineTraceSingleByChannel(Block, Center + FVector(0.f, 0.f, 20.f), C->GetActorLocation(), ECC_WorldStatic, Params)) continue;
		const float Scale = bFalloff ? FMath::Clamp(1.f - Dist / R, 0.15f, 1.f) : 1.f;
		C->TakeDamage(Amount * Scale, FDamageEvent(), GetInstigatorController(), this);
	}
}

void AGSProjectile::Explode()
{
	if (bExploded || !HasAuthority()) return;
	bExploded = true;
	Movement->StopMovementImmediately();
	Movement->ProjectileGravityScale = 0.f;
	const FVector Loc = GetActorLocation();

	if (IsKind(TEXT("flash")))
	{
		for (TActorIterator<AGSCharacter> It(GetWorld()); It; ++It) It->ApplyFlash(Loc, Radius);
		SetLifeSpan(1.f);
	}
	else if (IsKind(TEXT("smoke")))
	{
		SetLifeSpan(22.f);
	}
	else if (IsKind(TEXT("fire")))
	{
		SetLifeSpan(8.f);
	}
	else
	{
		if (Damage > 0.f && Radius > 0.f) DamageAround(Loc, Damage, Radius, true);
		SetLifeSpan(0.5f);
	}
	OnRep_Exploded();
}

void AGSProjectile::OnRep_Exploded()
{
	if (!bExploded) return;
	UWorld* World = GetWorld();
	const FVector Loc = GetActorLocation();
	Mesh->SetVisibility(false);
	if (IsKind(TEXT("smoke")))
	{
		// A thick grey cloud you cannot see through
		for (int32 I = 0; I < 26; ++I)
		{
			const FVector Off(FMath::FRandRange(-1.f, 1.f) * Radius * 0.6f, FMath::FRandRange(-1.f, 1.f) * Radius * 0.6f, FMath::FRandRange(0.f, 1.f) * 300.f);
			AGSShortFX::SpawnBlob(World, Loc + Off, FVector(FMath::FRandRange(2.f, 3.5f)), FLinearColor(0.55f, 0.56f, 0.58f), 20.f);
		}
	}
	else if (IsKind(TEXT("flash")))
	{
		AGSShortFX::SpawnBlob(World, Loc, FVector(3.f), FLinearColor(1.f, 1.f, 1.f) * 40.f, 0.12f);
	}
	else if (IsKind(TEXT("fire")))
	{
		for (int32 I = 0; I < 14; ++I)
		{
			const FVector Off(FMath::FRandRange(-1.f, 1.f) * Radius * 0.7f, FMath::FRandRange(-1.f, 1.f) * Radius * 0.7f, 20.f);
			AGSShortFX::SpawnBlob(World, Loc + Off, FVector(0.8f, 0.8f, FMath::FRandRange(0.8f, 1.8f)), FLinearColor(1.f, 0.35f, 0.05f) * 10.f, 8.f);
		}
	}
	else
	{
		AGSShortFX::SpawnExplosion(World, Loc, FMath::Max(Radius, 150.f));
	}
}
