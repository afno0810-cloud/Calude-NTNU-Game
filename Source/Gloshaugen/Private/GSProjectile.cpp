#include "GSProjectile.h"
#include "GSCharacter.h"
#include "GSEffects.h"
#include "GSAudio.h"
#include "GSItems.h"
#include "GSUtils.h"
#include "GSMapState.h"
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
	// Launch() hands the velocity over in world space. With the engine default (true) InitializeComponent would turn
	// it once more by the actor's rotation, so a throw to the east flew south, one to the south flew west, and so on.
	Movement->bInitialVelocityInLocalSpace = false;
	Movement->Bounciness = 0.3f;   // same on server and clients
	Movement->Friction = 0.4f;
}

void AGSProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSProjectile, Kind);
	DOREPLIFETIME(AGSProjectile, ItemId);
	DOREPLIFETIME(AGSProjectile, bExploded);
	DOREPLIFETIME(AGSProjectile, NetGravityScale);
	DOREPLIFETIME(AGSProjectile, bNetBounce);
}

AGSProjectile* AGSProjectile::Launch(UWorld* World, AGSCharacter* Owner, FName Kind, FName ItemId, const FVector& Location,
	const FVector& Velocity, float GravityScale, float Damage, float Radius, float Fuse)
{
	if (!World) return nullptr;
	AController* const ByController = Owner ? Owner->GetController() : nullptr;
	// Trip mines: at most MaxMinesPerPlayer per player. Placing one more removes his oldest (it goes quietly, no blast).
	if (Kind == FName(TEXT("mine")) && ByController)
	{
		TArray<AGSProjectile*> Mines;
		for (TActorIterator<AGSProjectile> It(World); It; ++It)
		{
			if (!It->bExploded && It->IsKind(TEXT("mine")) && It->OwnerController.Get() == ByController && !It->IsActorBeingDestroyed()) Mines.Add(*It);
		}
		Mines.Sort([](const AGSProjectile& A, const AGSProjectile& B) { return A.SpawnTime < B.SpawnTime; });
		for (int32 I = 0; I <= Mines.Num() - MaxMinesPerPlayer; ++I) Mines[I]->Destroy();
	}
	AGSProjectile* Proj = World->SpawnActorDeferred<AGSProjectile>(AGSProjectile::StaticClass(), FTransform(Velocity.Rotation(), Location), Owner, Owner,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Proj) return nullptr;
	Proj->Kind = Kind;
	Proj->ItemId = ItemId;
	Proj->Damage = Damage;
	Proj->Radius = Radius;
	Proj->Fuse = Fuse;
	Proj->OwnerController = ByController;
	Proj->LaunchLocation = Location;
	Proj->Movement->Velocity = Velocity;
	Proj->NetGravityScale = GravityScale;
	Proj->bNetBounce = Kind == FName(TEXT("frag")) || Kind == FName(TEXT("smoke")) || Kind == FName(TEXT("flash"));
	Proj->OnRep_Flight();
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
	else
	{
		OnRep_Flight(); // gravity and bounce arrive with the spawn, before BeginPlay
	}
	// Bolts, rockets and 40 mm hit people too: let the sphere also block pawns' meshes via a sweep in Tick.
	OnRep_Visual();
}

AController* AGSProjectile::KillerController() const
{
	return OwnerController.IsValid() ? OwnerController.Get() : GetInstigatorController();
}

AGSCharacter* AGSProjectile::GetThrowerNow(bool& bSameBody) const
{
	AController* C = KillerController();
	AGSCharacter* Now = C ? Cast<AGSCharacter>(C->GetPawn()) : nullptr;
	bSameBody = Now && Now == GetOwner();
	return Now;
}

void AGSProjectile::OnRep_Flight()
{
	Movement->bShouldBounce = bNetBounce;
	// Lying still or gone off: no gravity, or a client would pull it down between network updates.
	// A client that gets it while it already lies still on the server (comes into range, joins late) sees speed 0 in
	// the replicated movement: it must not fall here, the server sends no more updates for something lying still.
	const bool bResting = bExploded || (!HasAuthority() && ((HasActorBegunPlay() && !Movement->UpdatedComponent)
		|| GetReplicatedMovement().LinearVelocity.IsNearlyZero(1.f)));
	Movement->ProjectileGravityScale = bResting ? 0.f : NetGravityScale;
}

void AGSProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	Super::PostNetReceiveVelocity(NewVelocity);
	// The engine moves the actor to the server's position but does not hand the speed to the movement
	// component. Without it the client lets the projectile drop straight down between updates.
	if (bExploded) return;
	if (NewVelocity.IsNearlyZero(1.f))
	{
		// Stuck or at rest on the server.
		Movement->Velocity = FVector::ZeroVector;
		Movement->ProjectileGravityScale = 0.f;
		return;
	}
	// Still flying on the server: wake our own simulation if it stopped too early (hit something first here).
	if (!Movement->UpdatedComponent) Movement->SetUpdatedComponent(Collision);
	Movement->Velocity = NewVelocity;
	Movement->ProjectileGravityScale = NetGravityScale;
	Movement->UpdateComponentVelocity();
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
		AGSShortFX::SpawnSprite(GetWorld(), GetActorLocation(), 12.f, 70.f, FLinearColor(0.7f, 0.7f, 0.7f), 0.45f, 1.6f, FMath::VRand() * 20.f, false, 1.f, 8.f, 1.f);
		AGSShortFX::SpawnSprite(GetWorld(), GetActorLocation(), 18.f, 6.f, FLinearColor(1.f, 0.6f, 0.2f), 1.f, 0.06f, FVector::ZeroVector, true, 30.f);
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
					Victim->TakeDamage(Damage, Event, KillerController(), this);
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

	// Trip mine: goes off when someone else walks past. The one who placed it never sets it off himself, also not in a
	// new body after he has died (one rule: compare players, not bodies). He still gets hurt if someone else does.
	if (bStuck && !bExploded && IsKind(TEXT("mine")) && Age > 2.f)
	{
		const AController* Placer = OwnerController.Get();
		for (TActorIterator<AGSCharacter> It(GetWorld()); It; ++It)
		{
			if (It->bDead || *It == GetOwner() || (Placer && It->GetController() == Placer)) continue;
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
	// A placed trip mine lies there until someone walks into it (the 60 s from BeginPlay only covers the throw).
	if (IsKind(TEXT("mine"))) SetLifeSpan(0.f);
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
		C->TakeDamage(Amount * Scale, FDamageEvent(), KillerController(), this);
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
		for (TActorIterator<AGSCharacter> It(GetWorld()); It; ++It) It->ApplyFlash(Loc, Radius, KillerController());
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
		// Grenade, mine, C4, rocket: doors fly open, locks break and glass shatters (Interaktiv). Flash, smoke and fire
		// do not get here.
		if (Radius > 0.f) GSMapState::ApplyBlast(GetWorld(), Loc, Radius, Damage);
		if (Damage > 0.f && Radius > 0.f) DamageAround(Loc, Damage, Radius, true);
		SetLifeSpan(0.5f);
	}
	OnRep_Exploded();
}

void AGSProjectile::OnRep_Exploded()
{
	if (!bExploded) return;
	if (!HasAuthority())
	{
		Movement->StopMovementImmediately();
		Movement->ProjectileGravityScale = 0.f;
	}
	UWorld* World = GetWorld();
	const FVector Loc = GetActorLocation();
	Mesh->SetVisibility(false);
	if (IsKind(TEXT("smoke")))
	{
		// A thick grey cloud you cannot see through: billowing soft sprites
		AGSShortFX::SpawnSmokeCloud(World, Loc + FVector(0.f, 0.f, 120.f), FMath::Max(Radius, 300.f), FLinearColor(0.55f, 0.56f, 0.58f), 20.f, 34);
	}
	else if (IsKind(TEXT("flash")))
	{
		AGSShortFX::SpawnSprite(World, Loc, 300.f, 500.f, FLinearColor(1.f, 1.f, 1.f), 1.f, 0.15f, FVector::ZeroVector, true, 80.f);
		GSAudio::PlayExplosion(World, Loc, Radius, true);   // sharp bang; long ringing for those inside the radius
		AGSShortFX::SpawnSmokeCloud(World, Loc, 80.f, FLinearColor(0.7f, 0.7f, 0.7f), 3.f, 4);
	}
	else if (IsKind(TEXT("fire")))
	{
		for (int32 I = 0; I < 14; ++I)
		{
			const FVector Off(FMath::FRandRange(-1.f, 1.f) * Radius * 0.7f, FMath::FRandRange(-1.f, 1.f) * Radius * 0.7f, 20.f);
			AGSShortFX::SpawnSprite(World, Loc + Off + FVector(0.f, 0.f, 30.f), 60.f, FMath::FRandRange(90.f, 140.f), FLinearColor(1.f, 0.4f, 0.08f), 1.f, 8.f, FVector::ZeroVector, true, 12.f, 0.f, 1.f);
			AGSShortFX::SpawnSprite(World, Loc + Off + FVector(0.f, 0.f, 120.f), 80.f, 260.f, FLinearColor(0.08f, 0.07f, 0.06f), 0.6f, 8.f, FVector(0.f, 0.f, 20.f), false, 1.f, 3.f, 0.5f);
		}
	}
	else
	{
		AGSShortFX::SpawnExplosion(World, Loc, FMath::Max(Radius, 150.f));
		// close bang / indoor boom / far rumble, the room's tail and debris coming down; a pressure wave close by
		GSAudio::PlayExplosion(World, Loc, FMath::Max(Radius, 150.f));
	}
}
