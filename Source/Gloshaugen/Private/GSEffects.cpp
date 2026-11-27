#include "GSEffects.h"
#include "GSUtils.h"
#include "GSAudio.h"
#include "GSTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Pawn.h"
#include "CollisionQueryParams.h"

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

	/** Wet blood on a surface (built by Content/Python/gs_materials.py). Null until the editor has made it. */
	UMaterialInterface* GSBloodMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Blood;
		static bool bMissing = false;
		if (!Blood.IsValid() && !bMissing)
		{
			Blood = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/GS/Materials/M_GS_Blood.M_GS_Blood"), nullptr, LOAD_NoWarn | LOAD_Quiet);
			bMissing = !Blood.IsValid();
		}
		return Blood.Get();
	}

	// Every blood stain on this machine, oldest first. The oldest erode away when there are too many.
	TArray<TWeakObjectPtr<AGSShortFX>> GSStains;
	constexpr int32 MaxStains = 220;
	// Drops from people who bleed: their own list, so a long trail never wipes spatter or pools.
	TArray<TWeakObjectPtr<AGSShortFX>> GSBloodTrailStains;
	constexpr int32 GSMaxBloodTrail = 90;
	const FLinearColor BloodFresh(0.16f, 0.005f, 0.005f);
	const FLinearColor BloodDry(0.045f, 0.008f, 0.006f);
	const FLinearColor BloodMist(0.125f, 0.004f, 0.004f);   // in the air (unlit sprites): a little darker
	const FLinearColor BloodDrop(0.11f, 0.0035f, 0.0035f);

	/**
	 * How much of a flat patch really lies on the surface. Checks the four ends (HalfX along X, HalfY along Y) and
	 * shrinks the patch until all of them have something under them, so blood does not hang out over ledges,
	 * stair steps and wall corners. Returns the scale to use (1 = fits as it is).
	 */
	float GSFitStain(UWorld* World, const FVector& Location, const FVector& N, const FVector& X, const FVector& Y,
		float HalfX, float HalfY, const AActor* Ignore)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBloodFit), true);
		if (Ignore) Params.AddIgnoredActor(Ignore);
		float K = 1.f;
		for (int32 Try = 0; Try < 4; ++Try)
		{
			bool bAll = true;
			for (int32 I = 0; I < 4 && bAll; ++I)
			{
				const FVector P = Location + (I < 2 ? X * HalfX : Y * HalfY) * ((I % 2) ? -0.8f : 0.8f) * K;
				FHitResult Hit;
				bAll = World->LineTraceSingleByChannel(Hit, P + N * 8.f, P - N * 8.f, ECC_Visibility, Params)
					&& (Hit.ImpactNormal | N) > 0.7f;
			}
			if (bAll) break;
			K *= 0.7f;
		}
		return K;
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

AGSShortFX* AGSShortFX::SpawnBulletStreak(UWorld* World)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer) return nullptr;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGSShortFX* FX = World->SpawnActor<AGSShortFX>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	if (!FX) return nullptr;
	// The same glow as the old muzzle streak. No life span: the bullet that owns it destroys it (GSBallistics).
	// It does not tick (neither sprite nor stain), so nothing here moves or scales it behind the owner's back.
	FX->Mesh->SetStaticMesh(GSUtils::Cube());
	FX->Mesh->SetWorldScale3D(FVector(0.01f, 0.012f, 0.012f));
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
	FX->SetActorHiddenInGame(true);   // shown at the first SetStreak
	return FX;
}

void AGSShortFX::SetStreak(const FVector& Tail, const FVector& Head)
{
	const FVector Delta = Head - Tail;
	const float Len = Delta.Size();
	if (Len < 1.f) return;
	SetActorLocationAndRotation((Tail + Head) * 0.5f, Delta.Rotation());
	Mesh->SetWorldScale3D(FVector(Len / 100.f, 0.012f, 0.012f));
	if (IsHidden()) SetActorHiddenInGame(false);
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
	if (bStain)
	{
		Age += DeltaSeconds;
		if (Age < 0.f) return;                       // the drop has not landed yet
		if (IsHidden()) SetActorHiddenInGame(false);
		const float T = FMath::Clamp(Age / GrowTime, 0.f, 1.f);
		const float Left = Life - Age;
		const float Fade = FMath::Clamp(Left / ErodeTime, 0.f, 1.f);
		float SizeNow = FMath::Lerp(Size0, Size1, 1.f - FMath::Square(1.f - T));   // spreads fast, then creeps
		if (!MID) SizeNow = FMath::Max(SizeNow * Fade, 0.05f);                     // plain disc: shrinks away at the end
		Mesh->SetWorldScale3D(FVector(SizeNow * StretchK / 100.f, SizeNow / 100.f, MID ? 1.f : 0.002f));
		if (MID)
		{
			const float Dry = FMath::Clamp((Age - GrowTime) / 60.f, 0.f, 1.f);   // darker and duller as it dries
			MID->SetVectorParameterValue(TEXT("Color"), BloodFresh + (BloodDry - BloodFresh) * Dry);
			MID->SetScalarParameterValue(TEXT("Roughness"), FMath::Lerp(0.12f, 0.55f, Dry));
			MID->SetScalarParameterValue(TEXT("Opacity"), Fade);                 // erodes from the edge inwards
		}
		// once it has stopped spreading it only needs to dry: tick twice a second (every frame again while eroding)
		const float Interval = (T >= 1.f && Left > ErodeTime + 0.5f) ? 0.5f : 0.f;
		if (PrimaryActorTick.TickInterval != Interval) SetActorTickInterval(Interval);
		return;
	}
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

// ---------------------------------------------------------------- Blood

void AGSShortFX::RetireStain()
{
	if (!bStain) return;
	if (Age < 0.f) { Destroy(); return; }   // never landed: nobody has seen it
	constexpr float Quick = 1.f;
	if (Life - Age <= Quick) return;        // already on its way out
	ErodeTime = Quick;
	Life = Age + Quick;
	SetLifeSpan(Quick + 0.1f);
	SetActorTickInterval(0.f);
}

AGSShortFX* AGSShortFX::SpawnBloodStain(UWorld* World, const FVector& Location, const FVector& Normal, float Size,
	float GrowSeconds, float LifeSeconds, const FVector& Along, float Stretch, float Delay, bool bTrail)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer || Size < 0.5f) return nullptr;

	// Never more than the limit: the oldest make room (trail drops and other blood each in their own list)
	TArray<TWeakObjectPtr<AGSShortFX>>& List = bTrail ? GSBloodTrailStains : GSStains;
	const int32 Limit = bTrail ? GSMaxBloodTrail : MaxStains;
	List.RemoveAll([](const TWeakObjectPtr<AGSShortFX>& S) { return !S.IsValid(); });
	while (List.Num() >= Limit)
	{
		List[0]->RetireStain();
		List.RemoveAt(0);
	}

	// Lies flat on the surface, long side along the way the blood flew
	FVector N = Normal.GetSafeNormal();
	if (N.IsNearlyZero()) N = FVector::UpVector;
	FVector X = FVector::VectorPlaneProject(Along, N).GetSafeNormal();
	if (X.IsNearlyZero()) X = FVector::VectorPlaneProject(FMath::VRand(), N).GetSafeNormal();
	if (X.IsNearlyZero()) X = N.Rotation().RotateVector(FVector::RightVector);

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// A hair above the surface, each stain at its own height, so overlapping stains and the wall do not flicker
	const float Lift = FMath::FRandRange(0.25f, 0.45f) + Size * 0.004f;
	AGSShortFX* FX = World->SpawnActor<AGSShortFX>(Location + N * Lift, FRotationMatrix::MakeFromZX(N, X).Rotator(), P);
	if (!FX) return nullptr;

	// Too small and too many to matter for bounce light
	FX->Mesh->bAffectDynamicIndirectLighting = false;
	FX->Mesh->bAffectDistanceFieldLighting = false;
	UMaterialInterface* Mat = GSBloodMaterial();
	UStaticMesh* Plane = Mat ? GSFXPlane() : nullptr;
	if (Mat && Plane)
	{
		FX->Mesh->SetStaticMesh(Plane);
		FX->MID = UMaterialInstanceDynamic::Create(Mat, FX);
		FX->MID->SetVectorParameterValue(TEXT("Color"), BloodFresh);
		FX->MID->SetScalarParameterValue(TEXT("Roughness"), 0.12f);
		FX->MID->SetScalarParameterValue(TEXT("Opacity"), 1.f);
		FX->MID->SetScalarParameterValue(TEXT("Soft"), FMath::FRandRange(0.45f, 0.8f));
		FX->Mesh->SetMaterial(0, FX->MID);
	}
	else
	{
		// Blood material not built yet: a flat dark red disc
		FX->Mesh->SetStaticMesh(GSUtils::Cylinder());
		GSUtils::SetColor(FX->Mesh, FLinearColor(0.10f, 0.004f, 0.004f));
	}
	FX->bStain = true;
	FX->Size0 = Size * 0.12f;
	FX->Size1 = Size;
	FX->StretchK = FMath::Clamp(Stretch, 1.f, 4.f);
	FX->GrowTime = FMath::Max(GrowSeconds, 0.02f);
	FX->Life = FMath::Max(LifeSeconds, 6.f);
	const float Wait = FMath::Max(Delay, 0.f);
	FX->Age = -Wait;
	FX->Mesh->SetCullDistance(Size < 12.f ? 2500.f : 7000.f);   // small stains are not drawn far away
	if (Wait > 0.f) FX->SetActorHiddenInGame(true);
	FX->SetLifeSpan(FX->Life + Wait + 0.1f);
	FX->SetActorTickEnabled(true);
	FX->Tick(0.f);
	List.Add(FX);
	return FX;
}

void AGSShortFX::SpawnBloodDrip(UWorld* World, const FVector& From, float Size, const AActor* Victim)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer) return;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBloodDrip), true);
	if (Victim) Params.AddIgnoredActor(Victim);
	if (!World->LineTraceSingleByChannel(Hit, From, From - FVector(0.f, 0.f, 250.f), ECC_Visibility, Params)) return;
	if (Cast<APawn>(Hit.GetActor()) || Hit.ImpactNormal.Z <= 0.5f) return;   // on another player, or a wall: nothing
	// The drop lands after the time it takes to fall that far, small and round, and dries like other blood.
	SpawnBloodStain(World, Hit.ImpactPoint, Hit.ImpactNormal, Size, 0.12f, FMath::FRandRange(70.f, 100.f), FVector::ZeroVector, 1.f,
		FMath::Sqrt(2.f * Hit.Distance / 980.f), true);
}

UStaticMeshComponent* AGSShortFX::MakeBodyWound(AActor* Owner, float Size)
{
	if (!Owner) return nullptr;
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
	if (!C) return nullptr;
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetGenerateOverlapEvents(false);
	C->CanCharacterStepUpOn = ECB_No;
	C->SetCastShadow(false);
	C->bAffectDynamicIndirectLighting = false;
	C->bAffectDistanceFieldLighting = false;
	C->PrimaryComponentTick.bCanEverTick = false;
	UMaterialInterface* Mat = GSBloodMaterial();
	UStaticMesh* Plane = Mat ? GSFXPlane() : nullptr;
	if (Mat && Plane)
	{
		C->SetStaticMesh(Plane);
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Mat, C);
		M->SetVectorParameterValue(TEXT("Color"), BloodFresh * 0.7f);   // darker than a fresh stain: soaked into cloth
		M->SetScalarParameterValue(TEXT("Roughness"), 0.25f);
		M->SetScalarParameterValue(TEXT("Opacity"), 1.f);
		M->SetScalarParameterValue(TEXT("Soft"), 0.6f);
		C->SetMaterial(0, M);
		C->SetWorldScale3D(FVector(Size / 100.f));
	}
	else
	{
		// Blood material not built yet: a flat dark red disc, as in SpawnBloodStain
		C->SetStaticMesh(GSUtils::Cylinder());
		GSUtils::SetColor(C, FLinearColor(0.08f, 0.003f, 0.003f));
		C->SetWorldScale3D(FVector(Size / 100.f, Size / 100.f, 0.002f));
	}
	C->SetCullDistance(3000.f);
	C->RegisterComponent();
	return C;
}

void AGSShortFX::SpawnBloodHit(UWorld* World, const FVector& Point, const FVector& Dir, uint8 Kind, float Strength,
	const AActor* Victim, bool bOwnView)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer) return;
	const float S = FMath::Clamp(Strength, 0.f, 1.f);
	FVector D = Dir.GetSafeNormal();
	if (D.IsNearlyZero()) D = FVector::DownVector;

	// Where is the local player looking from? Mist and flying drops are only made near him, fine detail nearer still.
	float CamDist = 0.f;
	if (const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(World, 0))
	{
		CamDist = FVector::Dist(Cam->GetCameraLocation(), Point);
	}
	const bool bNear = CamDist < 8000.f;
	const bool bDetail = CamDist < 4000.f;   // small stains are not drawn beyond 25 m anyway

	// Many hits at the same moment (shotgun, several fights in view): less of everything per hit, so 20 players
	// shooting at once do not cost more than a few.
	static double WindowStart = 0.0;
	static int32 WindowHits = 0;
	const double RealNow = FPlatformTime::Seconds();
	if (RealNow - WindowStart > 0.3) { WindowStart = RealNow; WindowHits = 0; }
	++WindowHits;
	const float Q = (WindowHits > 12 ? 0.25f : WindowHits > 5 ? 0.5f : 1.f) * (bDetail ? 1.f : 0.5f);
	const bool bMinimal = WindowHits > 24;
	auto Count = [Q](int32 N, int32 Min) { return FMath::Max(Min, FMath::RoundToInt(N * Q)); };

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSBlood), true);
	if (Victim) Params.AddIgnoredActor(Victim);
	// Only surfaces that stay where they are: no stains hanging in the air where another player stood
	auto Trace = [World, &Params](const FVector& From, const FVector& To, FHitResult& OutHit)
	{
		return World->LineTraceSingleByChannel(OutHit, From, To, ECC_Visibility, Params)
			&& !(OutHit.GetActor() && OutHit.GetActor()->IsA(APawn::StaticClass()));
	};

	const bool bShowAir = bNear && !bOwnView;
	if (Kind == GSWound::Armor)
	{
		// The plate or helmet stopped it: dust and fibre, no blood
		if (bShowAir)
		{
			for (int32 I = 0; I < 2; ++I)
			{
				SpawnSprite(World, Point - D * 4.f, 5.f, FMath::FRandRange(22.f, 34.f), FLinearColor(0.34f, 0.32f, 0.29f), 0.5f,
					FMath::FRandRange(0.5f, 0.8f), -D * 70.f + FMath::VRand() * 25.f, false, 1.f, -10.f, 3.f);
			}
		}
		return;
	}

	const bool bHead = Kind == GSWound::Head;
	const bool bBlast = Kind == GSWound::Blast;
	const bool bExit = Kind == GSWound::Through || bHead || bBlast;

	// --- In the air: a puff at the entry wound, a longer spray out of the exit wound, and drops flying off
	if (bShowAir)
	{
		SpawnSprite(World, Point - D * 3.f, 4.f, 16.f + 22.f * S, BloodMist, 0.7f, 0.3f, -D * 60.f, false, 1.f, -200.f, 3.f);
		if (!bMinimal)
		{
			if (bExit)
			{
				const int32 Puffs = Count(bHead ? 4 : 2, 1);
				for (int32 I = 0; I < Puffs; ++I)
				{
					SpawnSprite(World, Point + D * 22.f, 6.f, FMath::FRandRange(30.f, 45.f) + 35.f * S, BloodMist, 0.65f, FMath::FRandRange(0.35f, 0.6f),
						(D + FMath::VRand() * 0.25f) * FMath::FRandRange(180.f, 320.f), false, 1.f, -250.f, 3.f);
				}
			}
			const int32 Drops = Count(3 + FMath::RoundToInt(5.f * S) + (bHead ? 4 : 0), 1);
			const FVector Base = bExit ? D : -D * 0.3f;   // no exit wound: the little there is comes back out of the entry
			for (int32 I = 0; I < Drops; ++I)
			{
				SpawnSprite(World, Point + D * 10.f, FMath::FRandRange(1.2f, 2.4f), 0.8f, BloodDrop, 0.95f, FMath::FRandRange(0.35f, 0.6f),
					(Base + FMath::VRand() * 0.55f).GetSafeNormal() * FMath::FRandRange(220.f, 560.f), false, 1.f, -980.f, 0.6f);
			}
		}
	}

	// --- Behind the body: what came out of the exit wound lands on the wall, or falls to the ground further on
	if (bExit && !bMinimal)
	{
		const float Reach = bHead ? 380.f : bBlast ? 260.f : 320.f;
		const float Cone = bBlast ? 35.f : 13.f;
		const int32 Big = Count(bHead ? 5 : 2 + FMath::RoundToInt(3.f * S), 1);
		const int32 Fine = bDetail ? Count(3 + FMath::RoundToInt(4.f * S), 0) : 0;
		for (int32 I = 0; I < Big + Fine; ++I)
		{
			const bool bBig = I < Big;
			const FVector Dd = I == 0 ? D : FMath::VRandCone(D, FMath::DegreesToRadians(bBig ? Cone : Cone + 16.f));
			FHitResult Hit;
			if (Trace(Point + D * 5.f, Point + Dd * Reach, Hit))
			{
				const FVector HN = Hit.ImpactNormal;
				float Sz = FMath::FRandRange(1.5f, 4.5f);
				if (bBig)
				{
					// big close to the body, small far away
					Sz = FMath::Lerp(24.f, 7.f, Hit.Distance / Reach) * FMath::FRandRange(0.7f, 1.3f) * (0.6f + 0.8f * S) * (bHead ? 1.5f : 1.f);
					Sz = FMath::Min(Sz, 40.f);
				}
				// hits at an angle = a long stain
				float Stretch = FMath::Clamp(1.f / FMath::Max(0.28f, FMath::Abs(Dd | HN)), 1.f, 3.5f);
				if (Sz * Stretch > 90.f) Stretch = FMath::Max(1.f, 90.f / Sz);
				if (bBig && Sz >= 10.f)
				{
					// keep it on the wall: not out past a corner or a door opening
					const FVector X = FVector::VectorPlaneProject(Dd, HN).GetSafeNormal();
					if (!X.IsNearlyZero())
					{
						Sz *= GSFitStain(World, Hit.ImpactPoint, HN, X, HN ^ X, Sz * Stretch * 0.5f, Sz * 0.5f, Victim);
					}
				}
				SpawnBloodStain(World, Hit.ImpactPoint, HN, Sz, 0.08f, FMath::FRandRange(100.f, 140.f), Dd, Stretch, Hit.Distance / 2500.f);
			}
			else
			{
				// nothing behind within reach: it comes down on the ground a few metres on
				const FVector Fall = Point + Dd * Reach * FMath::FRandRange(0.45f, 1.f);
				if (Trace(Fall, Fall - FVector(0.f, 0.f, 320.f), Hit) && Hit.ImpactNormal.Z > 0.5f)
				{
					const float Sz = bBig ? FMath::FRandRange(5.f, 11.f) * (0.6f + 0.8f * S) * (bHead ? 1.4f : 1.f) : FMath::FRandRange(1.5f, 4.f);
					SpawnBloodStain(World, Hit.ImpactPoint, Hit.ImpactNormal, Sz, 0.1f, FMath::FRandRange(100.f, 140.f), Dd,
						FMath::FRandRange(1.2f, 2.2f), 0.12f + FMath::Sqrt(2.f * Hit.Distance / 980.f));
				}
			}
		}
	}

	// --- On the floor under the body: drops that fall from the wound and land a moment later
	{
		const int32 Drops = bMinimal ? 1 : Count(1 + FMath::RoundToInt(3.f * S) + (bBlast ? 4 : bHead ? 2 : 0), 1);
		for (int32 I = 0; I < Drops; ++I)
		{
			const FVector Off = FVector(FMath::FRandRange(-35.f, 35.f), FMath::FRandRange(-35.f, 35.f), 0.f) + FVector(D.X, D.Y, 0.f) * FMath::FRandRange(0.f, 70.f);
			FHitResult Hit;
			if (Trace(Point, Point + Off, Hit)) continue;   // a wall between the wound and there
			if (Trace(Point + Off, Point + Off - FVector(0.f, 0.f, 260.f), Hit) && Hit.ImpactNormal.Z > 0.5f)
			{
				SpawnBloodStain(World, Hit.ImpactPoint, Hit.ImpactNormal, FMath::FRandRange(3.f, 9.f) * (0.7f + 0.6f * S), 0.15f, FMath::FRandRange(90.f, 130.f),
					FVector::ZeroVector, 1.f, FMath::Sqrt(2.f * Hit.Distance / 980.f));
			}
		}
	}
}

void AGSShortFX::SpawnBloodPool(UWorld* World, const FVector& Location, const FVector& Normal, float Size, const AActor* Body)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer) return;
	FVector N = Normal.GetSafeNormal();
	if (N.IsNearlyZero()) N = FVector::UpVector;
	FVector X, Y;
	N.FindBestAxisVectors(X, Y);
	// Fit the floor it lies on: a pool does not hang out over a ledge or a stair step (never under 18 cm across)
	const float R = FMath::Max(Size * 0.5f * GSFitStain(World, Location, N, X, Y, Size * 0.5f, Size * 0.5f, Body), 9.f);
	SpawnBloodStain(World, Location, N, R * 2.f, 16.f, 85.f);
	// Two lobes that spread at their own pace make the outline uneven, like blood finding its way over a floor
	for (int32 I = 0; I < 2; ++I)
	{
		const float A = FMath::FRandRange(0.f, 2.f * PI);
		const FVector Out = X * FMath::Cos(A) + Y * FMath::Sin(A);
		SpawnBloodStain(World, Location + Out * R * FMath::FRandRange(0.25f, 0.45f), N, R * FMath::FRandRange(0.9f, 1.3f),
			FMath::FRandRange(10.f, 20.f), 85.f, Out, FMath::FRandRange(1.f, 1.6f));
	}
}
