#include "GSEffects.h"
#include "GSUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AGSShortFX::AGSShortFX()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetMobility(EComponentMobility::Movable);
}

void AGSShortFX::SpawnTracer(UWorld* World, const FVector& Start, const FVector& End)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer) return;
	const FVector Delta = End - Start;
	const float Len = Delta.Size();
	if (Len < 50.f) return;

	// Draw only the last part of the path so the tracer does not cover the screen.
	const float Shown = FMath::Min(Len - 60.f, 1500.f);
	const FVector From = End - Delta.GetSafeNormal() * Shown;
	const FVector Mid = (From + End) * 0.5f;

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGSShortFX* FX = World->SpawnActor<AGSShortFX>(Mid, Delta.Rotation(), P);
	if (!FX) return;
	FX->Mesh->SetStaticMesh(GSUtils::Cube());
	FX->Mesh->SetWorldScale3D(FVector(Shown / 100.f, 0.012f, 0.012f));
	GSUtils::SetColor(FX->Mesh, FLinearColor(1.f, 0.85f, 0.4f));
	FX->SetLifeSpan(0.05f);
}

void AGSShortFX::SpawnImpact(UWorld* World, const FVector& Location, const FVector& Normal)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer) return;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGSShortFX* FX = World->SpawnActor<AGSShortFX>(Location + Normal * 2.f, Normal.Rotation(), P);
	if (!FX) return;
	FX->Mesh->SetStaticMesh(GSUtils::Cube());
	FX->Mesh->SetWorldScale3D(FVector(0.02f, 0.08f, 0.08f));
	GSUtils::SetColor(FX->Mesh, FLinearColor(0.05f, 0.05f, 0.05f));
	FX->SetLifeSpan(6.f);
}
