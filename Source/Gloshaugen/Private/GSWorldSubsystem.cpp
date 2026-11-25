#include "GSWorldSubsystem.h"
#include "GSCampusBuilder.h"
#include "GSEnvironment.h"
#include "Engine/World.h"

bool UGSWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UGSWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	P.ObjectFlags |= RF_Transient;

	AGSCampusBuilder* Campus = InWorld.SpawnActor<AGSCampusBuilder>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	if (Campus)
	{
		Campus->Build();
	}

	if (InWorld.GetNetMode() != NM_DedicatedServer)
	{
		if (AGSEnvironment* Env = InWorld.SpawnActor<AGSEnvironment>(FVector::ZeroVector, FRotator::ZeroRotator, P))
		{
			Env->Campus = Campus;
		}
	}
}
