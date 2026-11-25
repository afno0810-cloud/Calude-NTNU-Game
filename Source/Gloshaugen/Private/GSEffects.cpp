#include "GSEffects.h"
#include "GSUtils.h"
#include "GSAudio.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	UMaterialInterface* GSFXMaterial(bool bAdditive)
	{
		static TWeakObjectPtr<UMaterialInterface> Smoke, Add;
		static bool bTriedSmoke = false, bTriedAdd = false;
		TWeakObjectPtr<UMaterialInterface>& Slot = bAdditive ? Add : Smoke;
		bool& bTried = bAdditive ? bTriedAdd : bTriedSmoke;
		if (!Slot.IsValid() && !bTried)
		{
			bTried = true;
			Slot = LoadObject<UMaterialInterface>(nullptr, bAdditive ? TEXT("/Game/GS/Materials/M_GS_FXAdd.M_GS_FXAdd") : TEXT("/Game/GS/Materials/M_GS_FX.M_GS_FX"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
		return Slot.Get();
	}

	UStaticMesh* GSFXPlane()
	{
		static TWeakObjectPtr<UStaticMesh> Plane;
		if (!Plane.IsValid()) Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		return Plane.Get();
	}
}

AGSShortFX::AGSShortFX()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
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
	FX->Mesh->SetWorldScale3D(FVector(Shown / 100.f, 0.008f, 0.008f));
	if (UMaterialInterface* AddMat = GSFXMaterial(true))
	{
		UMaterialInstanceDynamic* TM = UMaterialInstanceDynamic::Create(AddMat, FX);
		TM->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.7f, 0.3f));
		TM->SetScalarParameterValue(TEXT("Glow"), 25.f);
		TM->SetScalarParameterValue(TEXT("Soft"), 0.01f);
		FX->Mesh->SetMaterial(0, TM);
	}
	else
	{
		GSUtils::SetColor(FX->Mesh, FLinearColor(1.f, 0.85f, 0.4f));
	}
	FX->SetLifeSpan(0.05f);
}

void AGSShortFX::SpawnImpact(UWorld* World, const FVector& Location, const FVector& Normal)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer) return;
	GSAudio::PlayAt(World, GSAudio::Variant(TEXT("Impact"), 3), Location, 0.7f, 25.f, FMath::FRandRange(0.85f, 1.15f));
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Bullet hole: a thin dark chip flush with the surface, turned randomly, stays for a while.
	FRotator HoleRot = Normal.Rotation();
	HoleRot.Roll = FMath::FRandRange(0.f, 360.f);
	AGSShortFX* FX = World->SpawnActor<AGSShortFX>(Location + Normal * 0.15f, HoleRot, P);
	if (!FX) return;
	FX->Mesh->SetStaticMesh(GSUtils::Cube());
	const float Size = FMath::FRandRange(0.03f, 0.045f);
	FX->Mesh->SetWorldScale3D(FVector(0.002f, Size, Size * FMath::FRandRange(0.8f, 1.2f)));
	FX->Mesh->SetCastShadow(false);
	GSUtils::SetColor(FX->Mesh, FLinearColor(0.015f, 0.014f, 0.013f));
	FX->SetLifeSpan(40.f);
	// Spark flash, a few sparks flying off, and a dust puff that blooms out of the hole and settles
	SpawnSprite(World, Location + Normal * 2.f, 10.f, 4.f, FLinearColor(1.f, 0.6f, 0.25f), 1.f, 0.06f, FVector::ZeroVector, true, 30.f);
	for (int32 I = 0; I < 4; ++I)
	{
		const FVector D = (Normal + FMath::VRand() * 0.8f).GetSafeNormal();
		SpawnSprite(World, Location + Normal * 2.f, 1.6f, 0.6f, FLinearColor(1.f, 0.55f, 0.15f), 1.f, FMath::FRandRange(0.12f, 0.25f), D * FMath::FRandRange(300.f, 700.f), true, 40.f, -900.f, 2.f);
	}
	for (int32 I = 0; I < 3; ++I)
	{
		const FVector D = (Normal * 1.2f + FMath::VRand() * 0.5f).GetSafeNormal();
		SpawnSprite(World, Location + Normal * 4.f, FMath::FRandRange(8.f, 14.f), FMath::FRandRange(45.f, 70.f), FLinearColor(0.42f, 0.39f, 0.34f), 0.55f,
			FMath::FRandRange(0.8f, 1.4f), D * FMath::FRandRange(60.f, 140.f), false, 1.f, -15.f, 3.5f);
	}
}

AGSShortFX* AGSShortFX::SpawnSprite(UWorld* World, const FVector& Location, float Size, float EndSize, const FLinearColor& Color,
	float Opacity, float LifeSeconds, const FVector& Velocity, bool bAdditive, float Glow, float Rise, float Drag)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer) return nullptr;
	UMaterialInterface* Mat = GSFXMaterial(bAdditive);
	UStaticMesh* Plane = GSFXPlane();
	if (!Mat || !Plane)
	{
		// FX materials not built yet: fall back to the old solid spheres
		return SpawnBlob(World, Location, FVector(Size / 100.f), bAdditive ? Color * Glow : Color, LifeSeconds);
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGSShortFX* FX = World->SpawnActor<AGSShortFX>(Location, FRotator::ZeroRotator, P);
	if (!FX) return nullptr;
	FX->Mesh->SetStaticMesh(Plane);
	FX->Mesh->SetTranslucentSortPriority(bAdditive ? 2 : 1);
	FX->MID = UMaterialInstanceDynamic::Create(Mat, FX);
	FX->MID->SetVectorParameterValue(TEXT("Color"), Color);
	FX->MID->SetScalarParameterValue(TEXT("Glow"), Glow);
	FX->MID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	FX->MID->SetScalarParameterValue(TEXT("Soft"), bAdditive ? 2.2f : 1.4f);
	FX->Mesh->SetMaterial(0, FX->MID);
	FX->bSprite = true;
	FX->bAdd = bAdditive;
	FX->Life = FMath::Max(LifeSeconds, 0.02f);
	FX->Size0 = Size;
	FX->Size1 = EndSize;
	FX->Opacity0 = Opacity;
	FX->Glow0 = Glow;
	FX->Vel = Velocity;
	FX->RiseAccel = Rise;
	FX->DragK = Drag;
	FX->SpriteSpin = FMath::FRandRange(0.f, 360.f);
	FX->SpinRate = FMath::FRandRange(-40.f, 40.f);
	FX->SetLifeSpan(FX->Life + 0.05f);
	FX->SetActorTickEnabled(true);
	FX->Tick(0.f);
	return FX;
}

void AGSShortFX::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bSprite) return;
	Age += DeltaSeconds;
	const float T = FMath::Clamp(Age / Life, 0.f, 1.f);
	// motion: drag, rising (smoke) or falling (sparks, negative rise)
	Vel *= FMath::Exp(-DragK * DeltaSeconds);
	Vel.Z += RiseAccel * DeltaSeconds;
	AddActorWorldOffset(Vel * DeltaSeconds);
	// grow (fast at first, like real smoke) and fade in quickly, out slowly
	const float G = 1.f - FMath::Square(1.f - T);
	const float SizeNow = FMath::Lerp(Size0, Size1, G);
	const float FadeIn = bAdd ? 1.f : FMath::Clamp(Age / FMath::Min(0.08f, Life * 0.2f), 0.f, 1.f);
	const float FadeOut = bAdd ? (1.f - T) : 1.f - FMath::Square(T);
	if (MID)
	{
		MID->SetScalarParameterValue(TEXT("Opacity"), Opacity0 * FadeIn * FadeOut);
		if (bAdd) MID->SetScalarParameterValue(TEXT("Glow"), Glow0 * (1.f - T * 0.7f));
	}
	SpriteSpin += SpinRate * DeltaSeconds;
	// face the local camera
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCam = (Cam->GetCameraLocation() - GetActorLocation()).GetSafeNormal();
		if (!ToCam.IsNearlyZero())
		{
			const FQuat Face = FRotationMatrix::MakeFromZ(ToCam).ToQuat();
			SetActorRotation(FQuat(ToCam, FMath::DegreesToRadians(SpriteSpin)) * Face);
		}
	}
	Mesh->SetWorldScale3D(FVector(SizeNow / 100.f, SizeNow / 100.f, 1.f));
}

void AGSShortFX::SpawnSmokeCloud(UWorld* World, const FVector& Location, float Radius, const FLinearColor& Color, float LifeSeconds, int32 Count)
{
	for (int32 I = 0; I < Count; ++I)
	{
		const FVector Off = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(0.f, 0.8f)) * Radius * 0.6f;
		const float Sz = Radius * FMath::FRandRange(0.9f, 1.4f);
		SpawnSprite(World, Location + Off, Sz * 0.4f, Sz * 1.6f, Color * FMath::FRandRange(0.85f, 1.1f), FMath::FRandRange(0.55f, 0.8f),
			LifeSeconds * FMath::FRandRange(0.7f, 1.1f), Off.GetSafeNormal() * FMath::FRandRange(20.f, 60.f) + FVector(0.f, 0.f, 8.f), false, 1.f, 0.5f, 0.6f);
	}
}

void AGSShortFX::SpawnBarrelSmoke(UWorld* World, const FVector& Location, const FVector& Dir)
{
	SpawnSprite(World, Location + Dir * 2.f, 2.f, FMath::FRandRange(10.f, 16.f), FLinearColor(0.8f, 0.8f, 0.82f), 0.18f, FMath::FRandRange(0.9f, 1.4f),
		Dir * 15.f + FVector(FMath::FRandRange(-4.f, 4.f), FMath::FRandRange(-4.f, 4.f), 25.f), false, 1.f, 20.f, 1.2f);
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
		SpawnSprite(World, Location + Dir * 4.f, 4.f, 14.f, FLinearColor(0.7f, 0.7f, 0.7f), 0.25f, 0.4f, Dir * 60.f, false, 1.f, 10.f, 4.f);
		return;
	}
	// hot core, petals along the barrel, a bit of side flare, then a smoke puff
	SpawnSprite(World, Location + Dir * 3.f, 14.f, 9.f, FLinearColor(1.f, 0.72f, 0.32f), 1.f, 0.045f, FVector::ZeroVector, true, 60.f);
	SpawnSprite(World, Location + Dir * 10.f, 11.f, 6.f, FLinearColor(1.f, 0.55f, 0.18f), 1.f, 0.04f, FVector::ZeroVector, true, 40.f);
	SpawnSprite(World, Location + Dir * 17.f, 7.f, 3.f, FLinearColor(1.f, 0.45f, 0.12f), 1.f, 0.035f, FVector::ZeroVector, true, 30.f);
	SpawnSprite(World, Location + Dir * 6.f, 6.f, 26.f, FLinearColor(0.75f, 0.74f, 0.72f), 0.22f, 0.7f, Dir * 120.f, false, 1.f, 20.f, 4.f);
}

void AGSShortFX::SpawnExplosion(UWorld* World, const FVector& Location, float Radius)
{
	// flash, expanding fireball lobes, debris sparks, then a dark smoke cloud that rises and spreads
	SpawnSprite(World, Location, Radius * 1.2f, Radius * 2.f, FLinearColor(1.f, 0.85f, 0.55f), 1.f, 0.1f, FVector::ZeroVector, true, 50.f);
	for (int32 I = 0; I < 7; ++I)
	{
		const FVector D = FMath::VRand() * FVector(1.f, 1.f, 0.6f) + FVector(0.f, 0.f, 0.4f);
		SpawnSprite(World, Location + D * Radius * 0.15f, Radius * 0.4f, Radius * FMath::FRandRange(0.9f, 1.3f), FLinearColor(1.f, 0.45f, 0.1f), 1.f,
			FMath::FRandRange(0.3f, 0.55f), D * Radius * 2.5f, true, FMath::FRandRange(12.f, 20.f), 0.f, 5.f);
	}
	for (int32 I = 0; I < 10; ++I)
	{
		const FVector D = (FMath::VRand() + FVector(0.f, 0.f, 0.7f)).GetSafeNormal();
		SpawnSprite(World, Location, 3.f, 1.f, FLinearColor(1.f, 0.6f, 0.2f), 1.f, FMath::FRandRange(0.4f, 0.9f), D * FMath::FRandRange(900.f, 1600.f), true, 40.f, -980.f, 0.8f);
	}
	for (int32 I = 0; I < 9; ++I)
	{
		const FVector Off = FMath::VRand() * Radius * 0.35f + FVector(0.f, 0.f, Radius * 0.25f);
		SpawnSprite(World, Location + Off, Radius * 0.5f, Radius * FMath::FRandRange(1.4f, 2.2f), FLinearColor(0.06f, 0.055f, 0.05f), FMath::FRandRange(0.6f, 0.85f),
			FMath::FRandRange(3.f, 6.f), Off.GetSafeNormal() * Radius * 0.8f + FVector(0.f, 0.f, 60.f), false, 1.f, 25.f, 1.2f);
	}
}
