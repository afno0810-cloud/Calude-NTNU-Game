#include "GSCampusBuilder.h"
#include "GSUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"

namespace
{
	constexpr float M = 100.f;          // meters -> cm
	constexpr float WallT = 0.4f;       // wall thickness (m)
	constexpr float SlabT = 0.3f;       // floor slab thickness (m)
	constexpr float WindowW = 1.8f;
	constexpr float MinPier = 1.4f;
	constexpr float RampW = 2.6f;
	constexpr float RampAngle = 25.f;   // degrees

	FString ColorKey(const UStaticMesh* Mesh, const FLinearColor& C, bool bCollide)
	{
		return FString::Printf(TEXT("%s_%d_%d_%d_%d"), *GetNameSafe(Mesh),
			FMath::RoundToInt(C.R * 255.f), FMath::RoundToInt(C.G * 255.f), FMath::RoundToInt(C.B * 255.f), bCollide ? 1 : 0);
	}

	float DistToSegment(const FVector2D& P, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D AB = B - A;
		const float T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 1.f), 0.f, 1.f);
		return FVector2D::Distance(P, A + AB * T);
	}
}

AGSCampusBuilder::AGSCampusBuilder()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
}

UInstancedStaticMeshComponent* AGSCampusBuilder::GetISM(UStaticMesh* Mesh, const FLinearColor& Color, bool bCollide)
{
	const FString Key = ColorKey(Mesh, Color, bCollide);
	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = ISMs.Find(Key))
	{
		return *Found;
	}
	UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this);
	ISM->SetMobility(EComponentMobility::Static);
	ISM->SetStaticMesh(Mesh);
	ISM->SetupAttachment(RootComponent);
	if (bCollide)
	{
		ISM->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
	else
	{
		ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	ISM->RegisterComponent();
	GSUtils::SetColor(ISM, Color);
	ISMs.Add(Key, ISM);
	return ISM;
}

void AGSCampusBuilder::AddMesh(UStaticMesh* Mesh, const FTransform& T, const FLinearColor& Color, bool bCollide)
{
	if (!Mesh) return;
	GetISM(Mesh, Color, bCollide)->AddInstance(T, /*bWorldSpace*/ true);
}

void AGSCampusBuilder::AddBox(const FVector& CenterCm, const FRotator& Rot, const FVector& SizeM, const FLinearColor& Color, bool bCollide)
{
	if (SizeM.X <= 0.01f || SizeM.Y <= 0.01f || SizeM.Z <= 0.01f) return;
	AddMesh(GSUtils::Cube(), FTransform(Rot, CenterCm, SizeM), Color, bCollide);
}

void AGSCampusBuilder::Build()
{
	const double StartTime = FPlatformTime::Seconds();
	BuildGround();
	BuildRoads();
	for (const GSCampus::FBuilding& B : GSCampus::Buildings())
	{
		BuildBuilding(B);
	}
	BuildTreesAndProps();
	if (GetNetMode() != NM_DedicatedServer)
	{
		BuildStreetLights();
	}

	int32 Count = 0;
	for (const auto& Pair : ISMs) Count += Pair.Value->GetInstanceCount();
	UE_LOG(LogTemp, Log, TEXT("Gloshaugen: campus built, %d pieces in %d groups (%.2f s)"),
		Count, ISMs.Num(), FPlatformTime::Seconds() - StartTime);
}

void AGSCampusBuilder::BuildGround()
{
	// Big lawn under everything. Top surface at Z = 0.
	AddBox(FVector(70.f * M, 220.f * M, -0.5f * M), FRotator::ZeroRotator, FVector(2400.f, 2400.f, 1.f), FLinearColor(0.16f, 0.27f, 0.11f));
}

void AGSCampusBuilder::BuildRoads()
{
	for (const GSCampus::FRoad& R : GSCampus::Roads())
	{
		const FVector2D D = R.B - R.A;
		const float Len = D.Size();
		const FVector2D C = (R.A + R.B) * 0.5f;
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
		const FLinearColor Col = R.bPath ? FLinearColor(0.42f, 0.41f, 0.39f) : FLinearColor(0.10f, 0.10f, 0.11f);
		const float Z = R.bPath ? -0.01f : -0.02f; // path on top of road where they cross
		AddBox(FVector(C.X * M, C.Y * M, Z * M), FRotator(0.f, Yaw, 0.f), FVector(Len + R.Width, R.Width, 0.1f), Col);
	}
}

void AGSCampusBuilder::BuildBuilding(const GSCampus::FBuilding& B)
{
	const FTransform BT(FRotator(0.f, B.Yaw, 0.f), FVector(B.X * M, B.Y * M, 0.f));
	auto Local = [&BT](float X, float Y, float Z) { return BT.TransformPosition(FVector(X, Y, Z) * M); };
	const FRotator BRot = BT.Rotator();

	Footprints.Add(FBox2D(FVector2D(B.X - B.SizeX * 0.5f, B.Y - B.SizeY * 0.5f), FVector2D(B.X + B.SizeX * 0.5f, B.Y + B.SizeY * 0.5f)));

	if (!B.bEnterable)
	{
		AddBox(Local(0.f, 0.f, B.Height * 0.5f), BRot, FVector(B.SizeX, B.SizeY, B.Height), B.Color);
		// small pointed roof for towers
		AddMesh(GSUtils::Cone(), FTransform(BRot, Local(0.f, 0.f, B.Height + 3.f), FVector(B.SizeX * 0.012f, B.SizeY * 0.012f, 0.06f)), FLinearColor(0.2f, 0.32f, 0.28f), true);
		return;
	}

	const float HX = B.SizeX * 0.5f;
	const float HY = B.SizeY * 0.5f;
	const int32 Floors = FMath::Max(1, FMath::FloorToInt(B.Height / B.FloorHeight + 0.01f));
	const float FH = B.FloorHeight;

	// Outer walls with windows (and doors on the ground floor).
	BuildWallSide(BT, 0.f, -HY + WallT * 0.5f, B.SizeX, true, B.Height, FH, Floors, true, B.Color);
	BuildWallSide(BT, 0.f, HY - WallT * 0.5f, B.SizeX, true, B.Height, FH, Floors, true, B.Color);
	BuildWallSide(BT, -HX + WallT * 0.5f, 0.f, B.SizeY - 2.f * WallT, false, B.Height, FH, Floors, true, B.Color);
	BuildWallSide(BT, HX - WallT * 0.5f, 0.f, B.SizeY - 2.f * WallT, false, B.Height, FH, Floors, true, B.Color);

	// Floors with a stairwell opening in the (+X, +Y) corner, and ramps between floors.
	const FLinearColor FloorCol(0.33f, 0.31f, 0.29f);
	const float RampLen = FH / FMath::Tan(FMath::DegreesToRadians(RampAngle));
	const float InX0 = -HX + WallT, InX1 = HX - WallT;
	const float InY0 = -HY + WallT, InY1 = HY - WallT;
	const float OpenX0 = InX1 - RampLen;
	const float OpenY0 = InY1 - RampW;
	const bool bHasStairs = (InX1 - InX0) > RampLen + 2.f && (InY1 - InY0) > RampW + 2.f;

	for (int32 K = 1; K < Floors; ++K)
	{
		const float ZTop = K * FH;
		const float ZC = ZTop - SlabT * 0.5f;
		if (bHasStairs)
		{
			// Slab A: everything west of the opening
			AddBox(Local((InX0 + OpenX0) * 0.5f, 0.f, ZC), BRot, FVector(OpenX0 - InX0, InY1 - InY0, SlabT), FloorCol);
			// Slab B: next to the opening
			AddBox(Local((OpenX0 + InX1) * 0.5f, (InY0 + OpenY0) * 0.5f, ZC), BRot, FVector(InX1 - OpenX0, OpenY0 - InY0, SlabT), FloorCol);

			// Ramp from floor K-1 up to floor K
			const float Z0 = (K - 1) * FH;
			const float Len = FMath::Sqrt(RampLen * RampLen + FH * FH);
			const FRotator RampRot = (FQuat(BRot) * FQuat(FRotator(RampAngle, 0.f, 0.f))).Rotator();
			AddBox(Local((OpenX0 + InX1) * 0.5f, (OpenY0 + InY1) * 0.5f, (Z0 + ZTop) * 0.5f - SlabT * 0.5f),
				RampRot, FVector(Len, RampW - 0.1f, SlabT), FLinearColor(0.25f, 0.24f, 0.23f));
		}
		else
		{
			AddBox(Local(0.f, 0.f, ZC), BRot, FVector(InX1 - InX0, InY1 - InY0, SlabT), FloorCol);
		}
	}

	// Roof
	AddBox(Local(0.f, 0.f, B.Height + SlabT * 0.5f), BRot, FVector(B.SizeX + 0.4f, B.SizeY + 0.4f, SlabT), B.Color * 0.7f);
}

void AGSCampusBuilder::BuildWallSide(const FTransform& BT, float CX, float CY, float Length, bool bAlongX,
	float Height, float FH, int32 Floors, bool bDoors, const FLinearColor& Color)
{
	const FRotator BRot = BT.Rotator();
	// Place one wall piece: S = position along the wall, Z0..Z1 height, W = width along the wall.
	auto Piece = [&](float S, float W, float Z0, float Z1)
	{
		if (W <= 0.01f || Z1 - Z0 <= 0.01f) return;
		const float LX = bAlongX ? CX + S : CX;
		const float LY = bAlongX ? CY : CY + S;
		const FVector Size = bAlongX ? FVector(W, WallT, Z1 - Z0) : FVector(WallT, W, Z1 - Z0);
		AddBox(BT.TransformPosition(FVector(LX, LY, (Z0 + Z1) * 0.5f) * M), BRot, Size, Color);
	};

	const int32 NumWindows = FMath::FloorToInt((Length - MinPier) / (WindowW + MinPier));
	if (NumWindows < 1)
	{
		Piece(0.f, Length, 0.f, Height);
		return;
	}
	const float Pier = (Length - NumWindows * WindowW) / (NumWindows + 1);

	TSet<int32> DoorIdx;
	if (bDoors)
	{
		if (Length < 60.f) DoorIdx.Add(NumWindows / 2);
		else { DoorIdx.Add(NumWindows / 4); DoorIdx.Add((NumWindows * 3) / 4); }
	}

	for (int32 F = 0; F < Floors; ++F)
	{
		const float ZB = F * FH;
		const float ZT = (F == Floors - 1) ? Height : (F + 1) * FH;
		const float ZSill = ZB + 1.0f;
		const float ZLintel = ZB + FMath::Min(2.7f, FH - 0.6f);

		for (int32 I = 0; I <= NumWindows; ++I)
		{
			const float S = -Length * 0.5f + Pier * 0.5f + I * (Pier + WindowW);
			Piece(S, Pier, ZB, ZLintel);
		}
		for (int32 I = 0; I < NumWindows; ++I)
		{
			const bool bDoor = (F == 0) && DoorIdx.Contains(I);
			if (bDoor) continue;
			const float S = -Length * 0.5f + Pier + WindowW * 0.5f + I * (Pier + WindowW);
			Piece(S, WindowW, ZB, ZSill);
		}
		Piece(0.f, Length, ZLintel, ZT);
	}
}

void AGSCampusBuilder::BuildTreesAndProps()
{
	FRandomStream Rand(1337);
	const TArray<GSCampus::FRoad>& Roads = GSCampus::Roads();

	auto IsFree = [&](const FVector2D& P, float Margin)
	{
		for (const FBox2D& Box : Footprints)
		{
			if (Box.ExpandBy(Margin).IsInside(P)) return false;
		}
		for (const GSCampus::FRoad& R : Roads)
		{
			if (DistToSegment(P, R.A, R.B) < R.Width * 0.5f + Margin * 0.5f) return false;
		}
		return true;
	};

	const FLinearColor Trunk(0.22f, 0.16f, 0.11f);
	const FLinearColor Leaves[] = { FLinearColor(0.12f, 0.25f, 0.08f), FLinearColor(0.16f, 0.30f, 0.10f), FLinearColor(0.20f, 0.28f, 0.09f) };

	int32 Trees = 0;
	for (int32 Try = 0; Try < 2000 && Trees < 160; ++Try)
	{
		const FVector2D P(Rand.FRandRange(-320.f, 460.f), Rand.FRandRange(-100.f, 550.f));
		if (!IsFree(P, 6.f)) continue;
		const float H = Rand.FRandRange(6.f, 12.f);
		AddMesh(GSUtils::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(P.X * M, P.Y * M, H * 0.25f * M), FVector(0.45f, 0.45f, H * 0.5f)), Trunk, true);
		const float R = Rand.FRandRange(3.f, 5.5f);
		AddMesh(GSUtils::Sphere(), FTransform(FRotator::ZeroRotator, FVector(P.X * M, P.Y * M, (H * 0.5f + R * 0.6f) * M), FVector(R, R, R * 1.3f)),
			Leaves[Rand.RandRange(0, 2)], false);
		++Trees;
	}

	// Parked cars along the car roads: good cover.
	const FLinearColor CarColors[] = { FLinearColor(0.6f, 0.6f, 0.62f), FLinearColor(0.05f, 0.05f, 0.06f),
		FLinearColor(0.5f, 0.06f, 0.05f), FLinearColor(0.1f, 0.18f, 0.4f), FLinearColor(0.85f, 0.85f, 0.85f) };
	int32 Cars = 0;
	for (int32 Try = 0; Try < 300 && Cars < 36; ++Try)
	{
		const GSCampus::FRoad& R = Roads[Rand.RandRange(0, Roads.Num() - 1)];
		if (R.bPath) continue;
		const FVector2D D = (R.B - R.A).GetSafeNormal();
		const FVector2D Side(-D.Y, D.X);
		const float T = Rand.FRandRange(0.05f, 0.95f);
		const FVector2D P = R.A + (R.B - R.A) * T + Side * (R.Width * 0.5f + 1.6f) * (Rand.FRand() > 0.5f ? 1.f : -1.f);
		bool bInBuilding = false;
		for (const FBox2D& Box : Footprints) if (Box.ExpandBy(3.f).IsInside(P)) bInBuilding = true;
		if (bInBuilding) continue;
		const FRotator Rot(0.f, FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)), 0.f);
		const FLinearColor C = CarColors[Rand.RandRange(0, 4)];
		AddBox(FVector(P.X * M, P.Y * M, 0.75f * M), Rot, FVector(4.4f, 1.8f, 1.0f), C);
		AddBox(FVector(P.X * M, P.Y * M, 1.55f * M), Rot, FVector(2.3f, 1.6f, 0.6f), C * 0.8f);
		++Cars;
	}
}

void AGSCampusBuilder::BuildStreetLights()
{
	const FLinearColor Pole(0.15f, 0.15f, 0.16f);
	const FLinearColor Lamp(1.f, 0.9f, 0.7f);
	for (const GSCampus::FRoad& R : GSCampus::Roads())
	{
		const FVector2D D = (R.B - R.A).GetSafeNormal();
		const FVector2D Side(-D.Y, D.X);
		const float Len = FVector2D::Distance(R.A, R.B);
		for (float S = 20.f; S < Len; S += 55.f)
		{
			const FVector2D P = R.A + D * S + Side * (R.Width * 0.5f + 0.8f);
			bool bBlocked = false;
			for (const FBox2D& Box : Footprints) if (Box.ExpandBy(1.f).IsInside(P)) bBlocked = true;
			if (bBlocked) continue;

			AddMesh(GSUtils::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(P.X * M, P.Y * M, 3.f * M), FVector(0.15f, 0.15f, 6.f)), Pole, true);
			AddBox(FVector(P.X * M, P.Y * M, 6.1f * M), FRotator::ZeroRotator, FVector(0.6f, 0.6f, 0.2f), Lamp, false);

			UPointLightComponent* L = NewObject<UPointLightComponent>(this);
			L->SetupAttachment(RootComponent);
			L->SetMobility(EComponentMobility::Movable);
			L->SetWorldLocation(FVector(P.X * M, P.Y * M, 5.7f * M));
			L->SetIntensityUnits(ELightUnits::Lumens);
			L->SetIntensity(6000.f);
			L->SetAttenuationRadius(2200.f);
			L->SetLightColor(FLinearColor(1.f, 0.78f, 0.5f));
			L->SetCastShadows(false);
			L->RegisterComponent();
			StreetLights.Add(L);
		}
	}
}

void AGSCampusBuilder::SetStreetLightsOn(bool bOn)
{
	if (bOn == bLightsOn) return;
	bLightsOn = bOn;
	for (UPointLightComponent* L : StreetLights)
	{
		if (L) L->SetVisibility(bOn);
	}
}
