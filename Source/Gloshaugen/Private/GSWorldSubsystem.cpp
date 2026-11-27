#include "GSWorldSubsystem.h"
#include "GSCampusBuilder.h"
#include "GSHelgasetrBuilder.h"
#include "GSEnvironment.h"
#include "Engine/World.h"

bool UGSWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UGSWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UWorld* World = GetWorld())
	{
		ActorsInitializedHandle = World->OnActorsInitialized.AddUObject(this, &UGSWorldSubsystem::HandleActorsInitialized);
	}
}

void UGSWorldSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->OnActorsInitialized.Remove(ActorsInitializedHandle);
	}
	Super::Deinitialize();
}

void UGSWorldSubsystem::HandleActorsInitialized(const FActorsInitializedParams& Params)
{
	if (Params.World && Params.World == GetWorld())
	{
		BuildMap(*Params.World);
	}
}

void UGSWorldSubsystem::BuildMap(UWorld& InWorld)
{
	if (bBuilt) return;
	bBuilt = true;

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	P.ObjectFlags |= RF_Transient;

	// The map: Helgasetr (indoor CTF) or the old Gloshaugen campus, chosen by GSCampus::bHelgasetr.
	UClass* BuilderClass = GSCampus::bHelgasetr ? AGSHelgasetrBuilder::StaticClass() : AGSCampusBuilder::StaticClass();
	AGSCampusBuilder* Builder = InWorld.SpawnActor<AGSCampusBuilder>(BuilderClass, FVector::ZeroVector, FRotator::ZeroRotator, P);
	if (Builder)
	{
		Builder->Build();
	}
	Campus = Builder;
}

void UGSWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	BuildMap(InWorld);

	if (InWorld.GetNetMode() != NM_DedicatedServer)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		P.ObjectFlags |= RF_Transient;
		if (AGSEnvironment* Env = InWorld.SpawnActor<AGSEnvironment>(FVector::ZeroVector, FRotator::ZeroRotator, P))
		{
			Env->Campus = Campus.Get();
		}
	}
}
