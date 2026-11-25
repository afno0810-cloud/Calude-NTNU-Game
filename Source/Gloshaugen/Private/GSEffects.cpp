#include "GSEffects.h"
#include "GSUtils.h"
#include "GSAudio.h"
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
	GSAudio::PlayAt(World, GSAudio::Variant(TEXT("Impact"), 3), Location, 0.7f, 25.f, FMath::FRandRange(0.85f, 1.15f));
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGSShortFX* FX = World->SpawnActor<AGSShortFX>(Location + Normal * 2.f, Normal.Rotation(), P);
	if (!FX) return;
	FX->Mesh->SetStaticMesh(GSUtils::Cube());
	FX->Mesh->SetWorldScale3D(FVector(0.02f, 0.08f, 0.08f));
	GSUtils::SetColor(FX->Mesh, FLinearColor(0.05f, 0.05f, 0.05f));
	FX->SetLifeSpan(6.f);
}

AGSShortFX* AGSShortFX::SpawnBlob(UWorld* World, const FVector& Location, const FVector& Scale, const FLinearColor& Color, float LifeSeconds)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer) return nullptr;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGSShortFX* FX = World->SpawnActor<AGSShortFX>(Location, FRotator::ZeroRotator, P);
	if (!FX) return nullptr;
	FX->Mesh->SetStaticMesh(GSUtils::Sphere());
	FX->Mesh->SetWorldScale3D(Scale);
	GSUtils::SetColor(FX->Mesh, Color);
	FX->SetLifeSpan(LifeSeconds);
	return FX;
}

void AGSShortFX::SpawnMuzzleFlash(UWorld* World, const FVector& Location, const FVector& Dir, bool bSilent)
{
	if (bSilent)
	{
		SpawnBlob(World, Location + Dir * 5.f, FVector(0.05f), FLinearColor(0.4f, 0.4f, 0.4f), 0.05f);
		return;
	}
	SpawnBlob(World, Location + Dir * 6.f, FVector(0.16f, 0.1f, 0.1f), FLinearColor(1.f, 0.75f, 0.25f) * 6.f, 0.04f);
}

void AGSShortFX::SpawnExplosion(UWorld* World, const FVector& Location, float Radius)
{
	const float S = Radius / 50.f;
	SpawnBlob(World, Location, FVector(S * 0.7f), FLinearColor(1.f, 0.5f, 0.1f) * 8.f, 0.18f);
	SpawnBlob(World, Location, FVector(S * 0.45f), FLinearColor(1.f, 0.9f, 0.5f) * 12.f, 0.08f);
	for (int32 I = 0; I < 5; ++I)
	{
		const FVector Off = FMath::VRand() * Radius * 0.3f + FVector(0.f, 0.f, Radius * 0.2f);
		SpawnBlob(World, Location + Off, FVector(S * FMath::FRandRange(0.25f, 0.4f)), FLinearColor(0.08f, 0.08f, 0.08f), FMath::FRandRange(1.5f, 3.f));
	}
}
