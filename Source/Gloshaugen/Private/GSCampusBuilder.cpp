#include "GSCampusBuilder.h"
#include "GSUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"

namespace
{
	constexpr float CmPerM = 100.f;          // meters -> cm
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

UStaticMesh* AGSCampusBuilder::MainBuildingMesh()
{
	static TWeakObjectPtr<UStaticMesh> Cached;
	static bool bTried = false;
	if (!bTried)
	{
		bTried = true;
		Cached = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Models/SM_Hovedbygningen.SM_Hovedbygningen"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	return Cached.Get();
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

UMaterialInterface* AGSCampusBuilder::GSMaterial(const TCHAR* Name)
{
	static TMap<FString, TWeakObjectPtr<UMaterialInterface>> Cache;
	static TSet<FString> Missing;
	const FString Key(Name);
	if (TWeakObjectPtr<UMaterialInterface>* Found = Cache.Find(Key))
	{
		if (Found->IsValid()) return Found->Get();
	}
	if (Missing.Contains(Key)) return nullptr;
	const FString Path = FString::Printf(TEXT("/Game/GS/Materials/MI_GS_%s.MI_GS_%s"), Name, Name);
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Mat)
	{
		Missing.Add(Key);
		return nullptr;
	}
	Cache.Add(Key, Mat);
	return Mat;
}

void AGSCampusBuilder::AddBoxMat(const FVector& CenterCm, const FRotator& Rot, const FVector& SizeM, const TCHAR* MatName,
	const FLinearColor& Fallback, bool bCollide)
{
	if (SizeM.X <= 0.01f || SizeM.Y <= 0.01f || SizeM.Z <= 0.01f) return;
	UMaterialInterface* Mat = GSMaterial(MatName);
	if (!Mat)
	{
		AddBox(CenterCm, Rot, SizeM, Fallback, bCollide);
		return;
	}
	const FString Key = FString::Printf(TEXT("Cube_%s_%d"), MatName, bCollide ? 1 : 0);
	UInstancedStaticMeshComponent* ISM = nullptr;
	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = ISMs.Find(Key))
	{
		ISM = *Found;
	}
	else
	{
		ISM = NewObject<UInstancedStaticMeshComponent>(this);
		ISM->SetMobility(EComponentMobility::Static);
		ISM->SetStaticMesh(GSUtils::Cube());
		ISM->SetupAttachment(RootComponent);
		if (bCollide) ISM->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		else ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ISM->RegisterComponent();
		ISM->SetMaterial(0, Mat);
		ISMs.Add(Key, ISM);
	}
	ISM->AddInstance(FTransform(Rot, CenterCm, SizeM), /*bWorldSpace*/ true);
}

FString AGSCampusBuilder::SafeName(const FString& Name)
{
	FString Out = Name;
	for (TCHAR& C : Out.GetCharArray())
	{
		if (C != 0 && !FChar::IsAlnum(C)) C = TEXT('_');
	}
	return Out;
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
	BuildSidewalks();
	BuildTreesAndProps();
	BuildCampusProps();
	for (const GSCampus::FBuilding& B : GSCampus::Buildings())
	{
		FurnishBuilding(B);
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		BuildStreetLights();
	}

	int32 Count = 0;
	for (const auto& Pair : ISMs) Count += Pair.Value->GetInstanceCount();
	for (const auto& Pair : PropISMs) Count += Pair.Value->GetInstanceCount();
	UE_LOG(LogTemp, Log, TEXT("Gloshaugen: campus built, %d pieces in %d groups, %d props (%.2f s)"),
		Count, ISMs.Num() + PropISMs.Num(), NumProps, FPlatformTime::Seconds() - StartTime);
}

void AGSCampusBuilder::BuildGround()
{
	// Big lawn under everything. Top surface at Z = 0.
	AddBoxMat(FVector(70.f * CmPerM, 220.f * CmPerM, -0.5f * CmPerM), FRotator::ZeroRotator, FVector(2400.f, 2400.f, 1.f), TEXT("W_Grass"), FLinearColor(0.16f, 0.27f, 0.11f), true);

	// Horizon: town, forest hills and the fjord 1.3-4 km out, so the world does not end at a green edge.
	// Never culled (it is meant to be seen from far away), no shadows, no collision.
	if (UStaticMesh* Backdrop = ModelMesh(TEXT("SM_Backdrop")))
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetMobility(EComponentMobility::Static);
		C->SetStaticMesh(Backdrop);
		C->SetupAttachment(RootComponent);
		C->SetRelativeLocation(FVector(70.f * CmPerM, 220.f * CmPerM, 0.f));
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->bAffectDistanceFieldLighting = false;
		C->RegisterComponent();
		AddInstanceComponent(C);
	}
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
		AddBoxMat(FVector(C.X * CmPerM, C.Y * CmPerM, Z * CmPerM), FRotator(0.f, Yaw, 0.f), FVector(Len + R.Width, R.Width, 0.1f),
			R.bPath ? TEXT("W_Paving") : TEXT("W_Asphalt"), Col, true);
	}
}

void AGSCampusBuilder::BuildSidewalks()
{
	// Car roads get painted lines, kerbs and paved sidewalks on both sides. Done in short pieces,
	// so they stop at crossings and at building walls.
	const TArray<GSCampus::FRoad>& Roads = GSCampus::Roads();
	auto OnOtherRoad = [&](const FVector2D& P, const GSCampus::FRoad* Self, float Margin)
	{
		for (const GSCampus::FRoad& R : Roads)
		{
			if (&R == Self || R.bPath) continue;
			if (DistToSegment(P, R.A, R.B) < R.Width * 0.5f + Margin) return true;
		}
		return false;
	};
	auto InBuilding = [&](const FVector2D& P, float Margin)
	{
		for (const FBox2D& Box : Footprints) if (Box.ExpandBy(Margin).IsInside(P)) return true;
		return false;
	};
	const FLinearColor Paint(0.85f, 0.85f, 0.8f);
	const FLinearColor KerbCol(0.55f, 0.55f, 0.53f);
	const FLinearColor WalkCol(0.45f, 0.44f, 0.42f);
	constexpr float Piece = 6.f;
	constexpr float WalkW = 2.2f;
	constexpr float KerbW = 0.2f;
	for (const GSCampus::FRoad& R : Roads)
	{
		if (R.bPath) continue;
		const FVector2D D = (R.B - R.A).GetSafeNormal();
		const FVector2D Side(-D.Y, D.X);
		const float Len = FVector2D::Distance(R.A, R.B);
		const FRotator Rot(0.f, FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)), 0.f);
		int32 PieceNo = 0;
		for (float S = 0.f; S < Len; S += Piece, ++PieceNo)
		{
			const float L = FMath::Min(Piece, Len - S);
			const FVector2D Mid = R.A + D * (S + L * 0.5f);
			const bool bCrossing = OnOtherRoad(Mid, &R, 1.f);
			// manholes in the lanes now and then
			if (!bCrossing && PieceNo % 7 == 3)
			{
				const FVector2D H = Mid + Side * ((PieceNo & 8) ? 1.f : -1.f) * R.Width * 0.25f;
				AddModel(TEXT("SM_Manhole"), FTransform(Rot, FVector(H.X * CmPerM, H.Y * CmPerM, 0.025f * CmPerM)), nullptr, false);
			}
			// centre line: 3 m dashes, 3 m gaps; solid edge lines
			if (!bCrossing)
			{
				AddBox(FVector(Mid.X * CmPerM, Mid.Y * CmPerM, 0.035f * CmPerM), Rot, FVector(L * 0.5f, 0.12f, 0.01f), Paint, false);
				for (float Sgn : { -1.f, 1.f })
				{
					const FVector2D E = Mid + Side * Sgn * (R.Width * 0.5f - 0.35f);
					AddBox(FVector(E.X * CmPerM, E.Y * CmPerM, 0.035f * CmPerM), Rot, FVector(L, 0.1f, 0.01f), Paint, false);
				}
			}
			for (float Sgn : { -1.f, 1.f })
			{
				const FVector2D K = Mid + Side * Sgn * (R.Width * 0.5f + KerbW * 0.5f);
				const FVector2D W = Mid + Side * Sgn * (R.Width * 0.5f + KerbW + WalkW * 0.5f);
				if (OnOtherRoad(K, &R, 0.3f) || InBuilding(W, 0.3f)) continue;
				AddBoxMat(FVector(K.X * CmPerM, K.Y * CmPerM, 0.07f * CmPerM), Rot, FVector(L, KerbW, 0.16f), TEXT("W_Concrete"), KerbCol, true);
				AddBoxMat(FVector(W.X * CmPerM, W.Y * CmPerM, 0.06f * CmPerM), Rot, FVector(L, WalkW, 0.12f), TEXT("W_Paving"), WalkCol, true);
				// drain grates in the gutter, hydrants and utility boxes at the back of the sidewalk
				if (L < Piece * 0.9f) continue;
				const FRotator Facing(0.f, Rot.Yaw + (Sgn > 0.f ? 90.f : -90.f), 0.f);
				if (PieceNo % 5 == 2)
				{
					const FVector2D G = Mid + Side * Sgn * (R.Width * 0.5f - 0.3f);
					AddModel(TEXT("SM_Drain"), FTransform(Rot, FVector(G.X * CmPerM, G.Y * CmPerM, 0.035f * CmPerM)), nullptr, false);
				}
				const FVector2D Back = Mid + Side * Sgn * (R.Width * 0.5f + KerbW + WalkW - 0.4f);
				if (PieceNo % 16 == 9)
				{
					AddModel(TEXT("SM_Hydrant"), FTransform(Facing, FVector(Back.X * CmPerM, Back.Y * CmPerM, 0.12f * CmPerM)), nullptr, true);
				}
				else if (PieceNo % 23 == 14)
				{
					AddModel(TEXT("SM_PowerBox"), FTransform(Facing, FVector(Back.X * CmPerM, Back.Y * CmPerM, 0.12f * CmPerM)), nullptr, true);
				}
			}
		}
	}
}

void AGSCampusBuilder::BuildBuilding(const GSCampus::FBuilding& B)
{
	const FTransform BT(FRotator(0.f, B.Yaw, 0.f), FVector(B.X * CmPerM, B.Y * CmPerM, 0.f));
	auto Local = [&BT](float X, float Y, float Z) { return BT.TransformPosition(FVector(X, Y, Z) * CmPerM); };
	const FRotator BRot = BT.Rotator();

	Footprints.Add(FBox2D(FVector2D(B.X - B.SizeX * 0.5f, B.Y - B.SizeY * 0.5f), FVector2D(B.X + B.SizeX * 0.5f, B.Y + B.SizeY * 0.5f)));

	// The hand-made Blender model replaces the block version of the main building and its tower.
	const FString Name(B.Name);
	if (Name == TEXT("Hovedbygningen") || Name == TEXT("Hovedbygningen taarn"))
	{
		if (UStaticMesh* Custom = MainBuildingMesh())
		{
			if (Name == TEXT("Hovedbygningen"))
			{
				UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
				C->SetMobility(EComponentMobility::Static);
				C->SetStaticMesh(Custom);
				C->SetupAttachment(RootComponent);
				C->SetWorldTransform(BT);
				C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
				C->RegisterComponent();
				if (UStaticMesh* Glass = ModelMesh(TEXT("SM_Glass_Hovedbygningen")))
				{
					UStaticMeshComponent* G = NewObject<UStaticMeshComponent>(this);
					G->SetMobility(EComponentMobility::Static);
					G->SetStaticMesh(Glass);
					G->SetupAttachment(RootComponent);
					G->SetWorldTransform(BT);
					G->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					G->SetCastShadow(false);
					G->RegisterComponent();
				}
				// front bay with the main door and the turrets
				Footprints.Add(FBox2D(FVector2D(B.X - 12.f, B.Y - 21.f), FVector2D(B.X + 12.f, B.Y - 14.f)));
			}
			return;
		}
	}

	// Detailed Blender model of this building (Tools/Blender/buildings.py), plus its window glass.
	if (B.bEnterable)
	{
		const FString Safe = SafeName(Name);
		const FString MeshPath = FString::Printf(TEXT("/Game/Models/SM_Bygg_%s.SM_Bygg_%s"), *Safe, *Safe);
		if (UStaticMesh* Model = LoadObject<UStaticMesh>(nullptr, *MeshPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
			C->SetMobility(EComponentMobility::Static);
			C->SetStaticMesh(Model);
			C->SetupAttachment(RootComponent);
			C->SetWorldTransform(BT);
			C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			C->RegisterComponent();
			const FString GlassPath = FString::Printf(TEXT("/Game/Models/SM_Glass_%s.SM_Glass_%s"), *Safe, *Safe);
			if (UStaticMesh* Glass = LoadObject<UStaticMesh>(nullptr, *GlassPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				UStaticMeshComponent* G = NewObject<UStaticMeshComponent>(this);
				G->SetMobility(EComponentMobility::Static);
				G->SetStaticMesh(Glass);
				G->SetupAttachment(RootComponent);
				G->SetWorldTransform(BT);
				G->SetCollisionEnabled(ECollisionEnabled::NoCollision); // shoot and jump through windows
				G->SetCastShadow(false);
				G->RegisterComponent();
			}
			return;
		}
	}

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
		AddBox(BT.TransformPosition(FVector(LX, LY, (Z0 + Z1) * 0.5f) * CmPerM), BRot, Size, Color);
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
	const bool bRealTrees = ModelMesh(TEXT("SM_Tree_Maple")) != nullptr;

	// Trees: maples and birches on the lawns, spruces in groups (Trondheim). Leaves have no collision;
	// an invisible trunk collider stops players and bullets.
	int32 Trees = 0;
	for (int32 Try = 0; Try < 3000 && Trees < 220; ++Try)
	{
		const FVector2D P(Rand.FRandRange(-320.f, 460.f), Rand.FRandRange(-100.f, 550.f));
		if (!IsFree(P, 6.f)) continue;
		if (bRealTrees)
		{
			const float Pick = Rand.FRand();
			const TCHAR* Kind = Pick < 0.38f ? TEXT("SM_Tree_Maple") : Pick < 0.68f ? TEXT("SM_Tree_Birch")
				: Pick < 0.88f ? TEXT("SM_Tree_Spruce") : TEXT("SM_Tree_SpruceSmall");
			const float Scale = Rand.FRandRange(0.8f, 1.3f);
			AddModel(Kind, FTransform(FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f), FVector(P.X * CmPerM, P.Y * CmPerM, 0.f), FVector(Scale)), nullptr, false);
			AddTrunkCollider(P, 0.35f * Scale, 6.f * Scale);
			// a few bushes around some trees
			if (Rand.FRand() < 0.35f)
			{
				const FVector2D Q = P + FVector2D(Rand.FRandRange(-3.f, 3.f), Rand.FRandRange(-3.f, 3.f));
				if (IsFree(Q, 2.f)) AddModel(TEXT("SM_Bush"), FTransform(FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f), FVector(Q.X * CmPerM, Q.Y * CmPerM, 0.f), FVector(Rand.FRandRange(0.8f, 1.4f))), nullptr, false);
			}
		}
		else
		{
			const float H = Rand.FRandRange(6.f, 12.f);
			AddMesh(GSUtils::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(P.X * CmPerM, P.Y * CmPerM, H * 0.25f * CmPerM), FVector(0.45f, 0.45f, H * 0.5f)), Trunk, true);
			const float R = Rand.FRandRange(3.f, 5.5f);
			AddMesh(GSUtils::Sphere(), FTransform(FRotator::ZeroRotator, FVector(P.X * CmPerM, P.Y * CmPerM, (H * 0.5f + R * 0.6f) * CmPerM), FVector(R, R, R * 1.3f)),
				Leaves[Rand.RandRange(0, 2)], false);
		}
		++Trees;
	}

	// Moraine boulders on the lawns (they block bullets, good cover) and round flower beds.
	if (bRealTrees)
	{
		int32 Rocks = 0;
		for (int32 Try = 0; Try < 800 && Rocks < 70; ++Try)
		{
			const FVector2D P(Rand.FRandRange(-320.f, 460.f), Rand.FRandRange(-100.f, 550.f));
			if (!IsFree(P, 4.f)) continue;
			const float Pick = Rand.FRand();
			const TCHAR* Kind = Pick < 0.45f ? TEXT("SM_Rock_A") : Pick < 0.8f ? TEXT("SM_Rock_B") : TEXT("SM_Rock_C");
			AddModel(Kind, FTransform(FRotator(Rand.FRandRange(-6.f, 6.f), Rand.FRandRange(0.f, 360.f), 0.f), FVector(P.X * CmPerM, P.Y * CmPerM, -0.15f * CmPerM),
				FVector(Rand.FRandRange(0.7f, 1.3f))), nullptr, true);
			++Rocks;
		}
		int32 Beds = 0;
		for (int32 Try = 0; Try < 600 && Beds < 30; ++Try)
		{
			const FVector2D P(Rand.FRandRange(-320.f, 460.f), Rand.FRandRange(-100.f, 550.f));
			if (!IsFree(P, 4.f)) continue;
			AddModel(TEXT("SM_FlowerBed"), FTransform(FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f), FVector(P.X * CmPerM, P.Y * CmPerM, 0.f),
				FVector(Rand.FRandRange(0.8f, 1.3f))), nullptr, false);
			++Beds;
		}
	}

	// Bushes and trimmed hedges along the building walls (not in front of the doors: hedges go on the short sides).
	if (bRealTrees)
	{
		for (const GSCampus::FBuilding& B : GSCampus::Buildings())
		{
			if (!B.bEnterable) continue;
			const FTransform BT(FRotator(0.f, B.Yaw, 0.f), FVector(B.X * CmPerM, B.Y * CmPerM, 0.f));
			const float HX = B.SizeX * 0.5f, HY = B.SizeY * 0.5f;
			for (float Sgn : { -1.f, 1.f })
			{
				for (float Y = -HY + 3.f; Y < HY - 3.f; Y += 4.2f)
				{
					const FVector W = BT.TransformPosition(FVector((HX + 1.2f) * Sgn, Y, 0.f) * CmPerM);
					const FVector2D P(W.X / CmPerM, W.Y / CmPerM);
					if (!IsFree(P, 0.6f)) continue;
					AddModel(TEXT("SM_Hedge"), FTransform(FRotator(0.f, B.Yaw + 90.f, 0.f), W), nullptr, true);
				}
			}
			for (int32 I = 0; I < 6; ++I)
			{
				const FVector W = BT.TransformPosition(FVector(Rand.FRandRange(-HX, HX), (HY + 1.5f) * (Rand.FRand() < 0.5f ? -1.f : 1.f), 0.f) * CmPerM);
				const FVector2D P(W.X / CmPerM, W.Y / CmPerM);
				if (IsFree(P, 0.8f)) AddModel(TEXT("SM_Bush"), FTransform(FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f), W, FVector(Rand.FRandRange(0.7f, 1.2f))), nullptr, false);
			}
		}
	}

	// Parked cars along the kerb of the car roads: good cover. Five body types in seven colours.
	const TCHAR* CarModels[] = { TEXT("SM_Car_Sedan"), TEXT("SM_Car_Hatch"), TEXT("SM_Car_SUV"), TEXT("SM_Car_Estate"), TEXT("SM_Car_Van") };
	const TCHAR* CarPaints[] = { TEXT("CarPaint_Silver"), TEXT("CarPaint_Black"), TEXT("CarPaint_White"), TEXT("CarPaint_Red"),
		TEXT("CarPaint_Blue"), TEXT("CarPaint_Grey"), TEXT("CarPaint_Green") };
	const bool bRealCars = ModelMesh(CarModels[0]) != nullptr;
	const FLinearColor CarColors[] = { FLinearColor(0.6f, 0.6f, 0.62f), FLinearColor(0.05f, 0.05f, 0.06f),
		FLinearColor(0.5f, 0.06f, 0.05f), FLinearColor(0.1f, 0.18f, 0.4f), FLinearColor(0.85f, 0.85f, 0.85f) };
	int32 Cars = 0;
	TArray<FVector2D> CarSpots;
	for (int32 Try = 0; Try < 600 && Cars < 60; ++Try)
	{
		const GSCampus::FRoad& R = Roads[Rand.RandRange(0, Roads.Num() - 1)];
		if (R.bPath) continue;
		const FVector2D D = (R.B - R.A).GetSafeNormal();
		const FVector2D Side(-D.Y, D.X);
		const float T = Rand.FRandRange(0.05f, 0.95f);
		const float SideSign = Rand.FRand() > 0.5f ? 1.f : -1.f;
		const FVector2D P = R.A + (R.B - R.A) * T + Side * (R.Width * 0.5f - 1.15f) * SideSign;
		bool bBlocked = false;
		for (const FBox2D& Box : Footprints) if (Box.ExpandBy(3.f).IsInside(P)) bBlocked = true;
		for (const FVector2D& Q : CarSpots) if (FVector2D::Distance(P, Q) < 6.f) bBlocked = true;
		for (const GSCampus::FRoad& O : Roads) if (&O != &R && !O.bPath && DistToSegment(P, O.A, O.B) < O.Width * 0.5f + 4.f) bBlocked = true;
		if (bBlocked) continue;
		CarSpots.Add(P);
		const FRotator Rot(0.f, FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)) + (SideSign > 0.f ? 0.f : 180.f) + Rand.FRandRange(-3.f, 3.f), 0.f);
		if (bRealCars)
		{
			AddModel(CarModels[Rand.RandRange(0, 4)], FTransform(Rot, FVector(P.X * CmPerM, P.Y * CmPerM, 0.f)), CarPaints[Rand.RandRange(0, 6)], true);
		}
		else
		{
			const FLinearColor C = CarColors[Rand.RandRange(0, 4)];
			AddBox(FVector(P.X * CmPerM, P.Y * CmPerM, 0.75f * CmPerM), Rot, FVector(4.4f, 1.8f, 1.0f), C);
			AddBox(FVector(P.X * CmPerM, P.Y * CmPerM, 1.55f * CmPerM), Rot, FVector(2.3f, 1.6f, 0.6f), C * 0.8f);
		}
		++Cars;
	}
}

UStaticMesh* AGSCampusBuilder::ModelMesh(const TCHAR* Name)
{
	static TMap<FString, TWeakObjectPtr<UStaticMesh>> Cache;
	static TSet<FString> Missing;
	const FString Key(Name);
	if (TWeakObjectPtr<UStaticMesh>* Found = Cache.Find(Key))
	{
		if (Found->IsValid()) return Found->Get();
	}
	if (Missing.Contains(Key)) return nullptr;
	// Scanned Poly Haven models (imported on the PC into /Game/PolyHaven) win over ours where the shape
	// does not care about facing direction.
	static const TMap<FString, FString> Scanned = {
		{ TEXT("SM_Rock_A"), TEXT("boulder_01") },
		{ TEXT("SM_Rock_B"), TEXT("rock_07") },
	};
	UStaticMesh* Mesh = nullptr;
	if (const FString* Id = Scanned.Find(Key))
	{
		Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/PolyHaven/Models/%s/%s_1k/StaticMeshes/%s_1k.%s_1k"),
			**Id, **Id, **Id, **Id), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	if (!Mesh) Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Models/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Mesh)
	{
		Missing.Add(Key);
		return nullptr;
	}
	Cache.Add(Key, Mesh);
	return Mesh;
}

void AGSCampusBuilder::AddModel(const TCHAR* Name, const FTransform& WorldT, const TCHAR* PaintMat, bool bCollide)
{
	UStaticMesh* Mesh = ModelMesh(Name);
	if (!Mesh) return;
	const FString Key = FString::Printf(TEXT("%s_%s_%d"), Name, PaintMat ? PaintMat : TEXT("-"), bCollide ? 1 : 0);
	UInstancedStaticMeshComponent* ISM = nullptr;
	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = PropISMs.Find(Key))
	{
		ISM = *Found;
	}
	else
	{
		ISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
		ISM->SetMobility(EComponentMobility::Static);
		ISM->SetStaticMesh(Mesh);
		ISM->SetupAttachment(RootComponent);
		if (bCollide) ISM->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		else ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ISM->SetCullDistances(0, 40000);
		ISM->RegisterComponent();
		if (PaintMat)
		{
			const int32 Slot = Mesh->GetMaterialIndex(FName(TEXT("CarPaint")));
			if (UMaterialInterface* Paint = GSMaterial(PaintMat))
			{
				if (Slot != INDEX_NONE) ISM->SetMaterial(Slot, Paint);
			}
		}
		PropISMs.Add(Key, ISM);
	}
	ISM->AddInstance(WorldT, /*bWorldSpace*/ true);
	++NumProps;
}

void AGSCampusBuilder::AddTrunkCollider(const FVector2D& P, float Radius, float Height)
{
	if (!TrunkColliders)
	{
		TrunkColliders = NewObject<UInstancedStaticMeshComponent>(this);
		TrunkColliders->SetMobility(EComponentMobility::Static);
		TrunkColliders->SetStaticMesh(GSUtils::Cylinder());
		TrunkColliders->SetupAttachment(RootComponent);
		TrunkColliders->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		TrunkColliders->SetVisibility(false);
		TrunkColliders->SetHiddenInGame(true);
		TrunkColliders->SetCastShadow(false);
		TrunkColliders->RegisterComponent();
	}
	// engine cylinder: 100 cm tall, 100 cm across, pivot in the middle
	TrunkColliders->AddInstance(FTransform(FRotator::ZeroRotator, FVector(P.X * CmPerM, P.Y * CmPerM, Height * 0.5f * CmPerM),
		FVector(Radius * 2.f, Radius * 2.f, Height)), /*bWorldSpace*/ true);
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

			FVector LightPos(P.X * CmPerM, P.Y * CmPerM, 5.7f * CmPerM);
			if (PropMesh(TEXT("StreetLamp")))
			{
				// lamp head reaches 1.45 m over the road
				const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(-Side.Y, -Side.X));
				AddProp(TEXT("StreetLamp"), FTransform::Identity, P.X, P.Y, 0.f, Yaw, true);
				const FVector2D Head = P - Side * 1.45f;
				LightPos = FVector(Head.X * CmPerM, Head.Y * CmPerM, 5.85f * CmPerM);
			}
			else
			{
				AddMesh(GSUtils::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(P.X * CmPerM, P.Y * CmPerM, 3.f * CmPerM), FVector(0.15f, 0.15f, 6.f)), Pole, true);
				AddBox(FVector(P.X * CmPerM, P.Y * CmPerM, 6.1f * CmPerM), FRotator::ZeroRotator, FVector(0.6f, 0.6f, 0.2f), Lamp, false);
			}

			UPointLightComponent* L = NewObject<UPointLightComponent>(this);
			L->SetupAttachment(RootComponent);
			L->SetMobility(EComponentMobility::Movable);
			L->SetWorldLocation(LightPos);
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

// ---------------------------------------------------------------------------------------------
// Props and furniture (meshes from Tools/Blender/props.py, imported to /Game/Models/SM_Prop_*).
// Everything here uses fixed random seeds, because every machine builds the campus itself and
// all players must see the same chairs in the same places.
// ---------------------------------------------------------------------------------------------

UStaticMesh* AGSCampusBuilder::PropMesh(const TCHAR* Name)
{
	static TMap<FString, TWeakObjectPtr<UStaticMesh>> Cache;
	static TSet<FString> Missing;
	const FString Key(Name);
	if (TWeakObjectPtr<UStaticMesh>* Found = Cache.Find(Key))
	{
		if (Found->IsValid()) return Found->Get();
	}
	if (Missing.Contains(Key)) return nullptr;
	const FString Path = FString::Printf(TEXT("/Game/Models/SM_Prop_%s.SM_Prop_%s"), Name, Name);
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Mesh)
	{
		Missing.Add(Key);
		return nullptr;
	}
	Cache.Add(Key, Mesh);
	return Mesh;
}

void AGSCampusBuilder::AddProp(const TCHAR* Name, const FTransform& BT, float X, float Y, float Z, float Yaw, bool bCollide, float Scale)
{
	UStaticMesh* Mesh = PropMesh(Name);
	if (!Mesh) return;
	const FString Key = FString::Printf(TEXT("%s_%d"), Name, bCollide ? 1 : 0);
	UInstancedStaticMeshComponent* ISM = nullptr;
	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = PropISMs.Find(Key))
	{
		ISM = *Found;
	}
	else
	{
		ISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this); // culls per cluster: much cheaper with 30 000 props
		ISM->SetMobility(EComponentMobility::Static);
		ISM->SetStaticMesh(Mesh);
		ISM->SetupAttachment(RootComponent);
		if (bCollide) ISM->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		else ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// small things stop casting far shadows; saves a lot on a 6 GB card
		ISM->SetCullDistances(0, 9000);
		ISM->RegisterComponent();
		PropISMs.Add(Key, ISM);
	}
	const FVector Loc = BT.TransformPosition(FVector(X, Y, Z) * CmPerM);
	const FRotator Rot = (FQuat(BT.GetRotation()) * FQuat(FRotator(0.f, Yaw, 0.f))).Rotator();
	ISM->AddInstance(FTransform(Rot, Loc, FVector(Scale)), /*bWorldSpace*/ true);
	++NumProps;
}

void AGSCampusBuilder::FurnishRoom(const FTransform& BT, float X0, float X1, float Y0, float Y1, float Z, FRandomStream& Rand, const TArray<FVector>& Keep)
{
	const float W = X1 - X0, D = Y1 - Y0;
	if (W < 3.f || D < 3.f) return;
	auto Free = [&](float X, float Y, float R)
	{
		for (const FVector& K : Keep)
		{
			if (FMath::Abs(K.Z - Z) < 1.f && FVector2D::Distance(FVector2D(K.X, K.Y), FVector2D(X, Y)) < R + 1.6f) return false;
		}
		return true;
	};
	auto Put = [&](const TCHAR* Name, float X, float Y, float Yaw, float R = 0.8f)
	{
		if (X < X0 || X > X1 || Y < Y0 || Y > Y1) return;
		if (!Free(X, Y, R)) return;
		AddProp(Name, BT, X, Y, Z, Yaw);
	};
	const float CX = (X0 + X1) * 0.5f, CY = (Y0 + Y1) * 0.5f;

	// Room type. Small rooms become offices, big ones classrooms, labs, cafeterias or libraries.
	const float Area = W * D;
	int32 Type = Rand.RandRange(0, 7);
	if (Area < 30.f) Type = (Rand.FRand() < 0.5f) ? 1 : 6;

	switch (Type)
	{
	case 0: // classroom: rows of desks facing -X, lectern and whiteboard at the front
	case 7:
	{
		Put(TEXT("Whiteboard"), X0 + 0.05f, CY, 0.f, 0.3f);
		Put(TEXT("Lectern"), X0 + 1.4f, CY + FMath::Min(2.5f, D * 0.3f), 180.f, 0.5f);
		for (float X = X0 + 3.2f; X < X1 - 0.6f; X += 1.9f)
		{
			for (float Y = Y0 + 1.1f; Y < Y1 - 0.9f; Y += 1.6f)
			{
				if (FMath::Abs(Y - CY) < 0.7f) continue; // aisle
				Put(TEXT("Desk"), X, Y, 0.f, 0.7f);
				Put(TEXT("Chair"), X + 0.55f, Y, 180.f, 0.4f);
			}
		}
		Put(TEXT("TrashBin"), X1 - 0.4f, Y0 + 0.4f, 0.f, 0.3f);
		break;
	}
	case 1: // offices: desk with chair, shelves along the wall, plant
	{
		for (float Y = Y0 + 1.2f; Y < Y1 - 1.0f; Y += 3.2f)
		{
			Put(TEXT("Desk"), X1 - 1.0f, Y + 0.6f, 180.f);
			Put(TEXT("OfficeChair"), X1 - 1.8f, Y + 0.6f, 0.f, 0.4f);
		}
		Put(TEXT("Bookshelf"), X0 + 0.25f, Y0 + 0.8f, 0.f);
		Put(TEXT("FilingCabinet"), X0 + 0.35f, Y1 - 0.6f, 0.f, 0.4f);
		Put(TEXT("Plant"), X0 + 0.5f, CY, 0.f, 0.4f);
		Put(TEXT("TrashBin"), X1 - 0.4f, Y1 - 0.4f, 0.f, 0.3f);
		break;
	}
	case 2: // chemistry lab: benches with stools, fire extinguisher by the door
	{
		for (float X = X0 + 1.6f; X < X1 - 1.4f; X += 3.0f)
		{
			for (float Y = Y0 + 1.8f; Y < Y1 - 1.6f; Y += 3.0f)
			{
				Put(TEXT("LabBench"), X, Y, 0.f, 1.3f);
				Put(TEXT("Stool"), X + 0.9f, Y - 0.6f, 0.f, 0.3f);
				Put(TEXT("Stool"), X + 0.9f, Y + 0.6f, 0.f, 0.3f);
			}
		}
		Put(TEXT("FireExtinguisher"), X0 + 0.05f, Y0 + 0.6f, 0.f, 0.2f);
		break;
	}
	case 3: // cafeteria / study hall: tables with chairs, vending machine, coffee
	{
		for (float X = X0 + 1.6f; X < X1 - 1.4f; X += 2.8f)
		{
			for (float Y = Y0 + 1.6f; Y < Y1 - 1.4f; Y += 3.0f)
			{
				Put(TEXT("CafeTable"), X, Y, 0.f, 1.0f);
				Put(TEXT("Chair"), X - 0.75f, Y - 0.45f, 0.f, 0.3f);
				Put(TEXT("Chair"), X - 0.75f, Y + 0.45f, 0.f, 0.3f);
				Put(TEXT("Chair"), X + 0.75f, Y - 0.45f, 180.f, 0.3f);
				Put(TEXT("Chair"), X + 0.75f, Y + 0.45f, 180.f, 0.3f);
			}
		}
		Put(TEXT("VendingMachine"), X0 + 0.45f, Y1 - 0.7f, 0.f);
		Put(TEXT("CoffeeMachine"), X0 + 0.35f, Y1 - 2.4f, 0.f);
		Put(TEXT("TrashBin"), X0 + 0.4f, Y1 - 3.6f, 0.f, 0.3f);
		break;
	}
	case 4: // library: rows of shelves, reading table
	{
		for (float X = X0 + 1.0f; X < X1 - 3.0f; X += 2.2f)
		{
			for (float Y = Y0 + 1.0f; Y < Y1 - 1.0f; Y += 1.05f)
			{
				if (FMath::Abs(Y - CY) < 0.9f) continue;
				Put(TEXT("Bookshelf"), X, Y, (FMath::RoundToInt(X) % 2) ? 0.f : 180.f, 0.5f);
			}
		}
		Put(TEXT("CafeTable"), X1 - 1.6f, CY, 0.f);
		Put(TEXT("Chair"), X1 - 2.4f, CY, 0.f, 0.3f);
		Put(TEXT("Chair"), X1 - 0.8f, CY, 180.f, 0.3f);
		Put(TEXT("Plant"), X1 - 0.5f, Y0 + 0.5f, 0.f, 0.4f);
		break;
	}
	case 5: // lounge: sofas around a table, coffee machine, plants
	{
		Put(TEXT("Sofa"), CX - 1.6f, CY, 0.f, 1.0f);
		Put(TEXT("Sofa"), CX + 1.6f, CY, 180.f, 1.0f);
		Put(TEXT("CafeTable"), CX, CY, 90.f, 0.8f);
		Put(TEXT("CoffeeMachine"), X0 + 0.35f, Y0 + 1.0f, 0.f);
		Put(TEXT("VendingMachine"), X0 + 0.45f, Y1 - 0.8f, 0.f);
		Put(TEXT("Plant"), X1 - 0.5f, Y0 + 0.5f, 0.f, 0.4f);
		Put(TEXT("Plant"), X1 - 0.5f, Y1 - 0.5f, 0.f, 0.4f);
		break;
	}
	default: // 6: server room / storage
	{
		if ((Rand.FRand() < 0.5f))
		{
			for (float X = X0 + 1.2f; X < X1 - 1.0f; X += 2.4f)
			{
				for (float Y = Y0 + 0.8f; Y < Y1 - 0.6f; Y += 0.65f)
				{
					if (FMath::Abs(Y - CY) < 0.7f) continue;
					Put(TEXT("ServerRack"), X, Y, 0.f, 0.4f);
				}
			}
		}
		else
		{
			for (float X = X0 + 0.9f; X < X1 - 0.6f; X += 1.6f)
			{
				Put(TEXT("Pallet"), X, Y0 + 0.9f, 0.f, 0.7f);
				Put(TEXT("Crates"), X, Y1 - 1.0f, Rand.FRandRange(-20.f, 20.f), 0.8f);
			}
			Put(TEXT("FilingCabinet"), X1 - 0.4f, CY, 180.f, 0.4f);
		}
		Put(TEXT("FireExtinguisher"), X0 + 0.05f, CY, 0.f, 0.2f);
		break;
	}
	}
}

void AGSCampusBuilder::FurnishBuilding(const GSCampus::FBuilding& B)
{
	const FString Name(B.Name);
	if (!B.bEnterable) return;
	if (MainBuildingMesh() && Name == TEXT("Hovedbygningen"))
	{
		FurnishMainBuilding(B);
		return;
	}
	if (MainBuildingMesh() && Name == TEXT("Hovedbygningen taarn")) return;

	const FTransform BT(FRotator(0.f, B.Yaw, 0.f), FVector(B.X * CmPerM, B.Y * CmPerM, 0.f));
	FRandomStream Rand(static_cast<int32>(GetTypeHash(Name)));
	const float HX = B.SizeX * 0.5f, HY = B.SizeY * 0.5f;
	const int32 Floors = FMath::Max(1, FMath::FloorToInt(B.Height / B.FloorHeight + 0.01f));
	const float FH = B.FloorHeight;
	const float InX0 = -HX + WallT, InX1 = HX - WallT, InY0 = -HY + WallT, InY1 = HY - WallT;
	const float RampLen = FH / FMath::Tan(FMath::DegreesToRadians(RampAngle));
	const float OpenX0 = InX1 - RampLen, OpenY0 = InY1 - RampW;

	// Spots that must stay clear: the terminals AGSGameMode::SpawnTerminals puts in every building (same formulas).
	TArray<FVector> Keep;
	Keep.Add(FVector(-HX + 1.0f, -HY * 0.4f, 0.f));
	Keep.Add(FVector(-HX + 0.9f, HY * 0.4f, FH));

	// Rooms: split each floor into cells of about 9 x 8 m, with a 3 m cross corridor through the middle
	// (it lines up with the doors in the outer walls) and 1.2 m along the walls.
	const float Corr = 1.5f, Edge = 1.2f;
	for (int32 F = 0; F < Floors; ++F)
	{
		const float Z = F * FH;
		for (int32 SX = 0; SX < 2; ++SX)
		{
			const float AX0 = SX == 0 ? InX0 + Edge : Corr, AX1 = SX == 0 ? -Corr : InX1 - Edge;
			const int32 NX = FMath::Max(1, FMath::RoundToInt((AX1 - AX0) / 9.f));
			for (int32 SY = 0; SY < 2; ++SY)
			{
				const float AY0 = SY == 0 ? InY0 + Edge : Corr, AY1 = SY == 0 ? -Corr : InY1 - Edge;
				const int32 NY = FMath::Max(1, FMath::RoundToInt((AY1 - AY0) / 8.f));
				for (int32 IX = 0; IX < NX; ++IX)
				{
					for (int32 IY = 0; IY < NY; ++IY)
					{
						float X0 = AX0 + (AX1 - AX0) * IX / NX, X1 = AX0 + (AX1 - AX0) * (IX + 1) / NX;
						float Y0 = AY0 + (AY1 - AY0) * IY / NY, Y1 = AY0 + (AY1 - AY0) * (IY + 1) / NY;
						// 1 m gaps between rooms so you can walk between them
						X0 += 0.5f; X1 -= 0.5f; Y0 += 0.5f; Y1 -= 0.5f;
						// stairwell and ramp corner: shrink the room away from it
						if (X1 > OpenX0 - 1.f && Y1 > OpenY0 - 1.f)
						{
							if (OpenY0 - 1.f - Y0 > 3.f) Y1 = OpenY0 - 1.f;
							else if (OpenX0 - 1.f - X0 > 3.f) X1 = OpenX0 - 1.f;
							else continue;
						}
						FurnishRoom(BT, X0, X1, Y0, Y1, Z, Rand, Keep);
					}
				}
			}
		}
		// fire extinguisher by the stairs on every floor
		AddProp(TEXT("FireExtinguisher"), BT, OpenX0 - 0.6f, InY1 - 0.05f, Z, -90.f, false);
	}
}

void AGSCampusBuilder::FurnishMainBuilding(const GSCampus::FBuilding& B)
{
	// Matches Tools/Blender/building_hovedbygningen.py: 4 floors of 5.5 m, corridor y in [-5, 5],
	// cross walls at x = +-26, +-36, +-46, Lysgaarden |x| < 14, ramp rooms south of the corridor at |x| > 36.
	const FTransform BT(FRotator(0.f, B.Yaw, 0.f), FVector(B.X * CmPerM, B.Y * CmPerM, 0.f));
	FRandomStream Rand(4242);
	const float FH = 5.5f;
	TArray<FVector> Keep = {
		FVector(-30.f, -4.5f, 0.f), FVector(30.f, 4.5f, 11.f), FVector(30.f, -4.5f, 5.5f), FVector(-30.f, 4.5f, 16.5f),
	};
	const float Segs[][2] = { { 15.f, 26.f }, { 26.f, 36.f }, { 36.f, 46.f }, { 46.f, 54.2f } };
	for (int32 F = 0; F < 4; ++F)
	{
		const float Z = F * FH;
		for (float Side : { -1.f, 1.f })
		{
			for (const auto& Seg : Segs)
			{
				const float A = Side < 0.f ? -Seg[1] : Seg[0];
				const float Bx = Side < 0.f ? -Seg[0] : Seg[1];
				// north rooms
				FurnishRoom(BT, A + 0.9f, Bx - 0.9f, -13.f, -6.8f, Z, Rand, Keep);
				// south rooms, except the ones with the ramps
				if (Seg[0] < 36.f) FurnishRoom(BT, A + 0.9f, Bx - 0.9f, 6.8f, 13.f, Z, Rand, Keep);
			}
		}
		// Lockers in the corridors, between the doors
		for (float X : { -46.8f, -38.8f, -30.8f, -22.8f, 22.f, 30.f, 38.f, 46.f })
		{
			for (float Y : { -4.45f, 4.45f })
			{
				bool bFree = true;
				for (const FVector& K : Keep) if (FMath::Abs(K.Z - Z) < 1.f && FVector2D::Distance(FVector2D(K.X, K.Y), FVector2D(X, Y)) < 2.6f) bFree = false;
				if (bFree) AddProp(TEXT("Locker"), BT, X, Y, Z, Y < 0.f ? 90.f : -90.f);
			}
		}
		AddProp(TEXT("FireExtinguisher"), BT, -18.f, -4.95f, Z, 90.f, false);
		AddProp(TEXT("FireExtinguisher"), BT, 18.f, 4.95f, Z, -90.f, false);
		AddProp(TEXT("Plant"), BT, -15.5f, 4.2f, Z, 0.f);
		AddProp(TEXT("Plant"), BT, 15.5f, -4.2f, Z, 0.f);
	}

	// Lysgaarden: rows of chairs facing the stage (stage at y -9..-6), aisle in the middle, lectern on the stage
	for (float Y = -4.8f; Y <= -1.0f; Y += 1.0f)
	{
		for (float X = -8.f; X <= 8.01f; X += 0.6f)
		{
			if (FMath::Abs(X) < 1.0f) continue;
			AddProp(TEXT("Chair"), BT, X, Y, 0.f, -90.f);
		}
	}
	AddProp(TEXT("Lectern"), BT, 3.f, -8.f, 0.85f, 90.f);
	AddProp(TEXT("Plant"), BT, -6.f, -8.5f, 0.85f, 0.f);
	AddProp(TEXT("Plant"), BT, 6.f, -8.5f, 0.85f, 0.f);
	// cafe corner on the west side and a sofa lounge on the east side of Lysgaarden
	for (float Y : { -6.f, -2.5f, 1.f })
	{
		AddProp(TEXT("CafeTable"), BT, -11.5f, Y, 0.f, 0.f);
		AddProp(TEXT("Chair"), BT, -12.3f, Y - 0.45f, 0.f, 0.f);
		AddProp(TEXT("Chair"), BT, -12.3f, Y + 0.45f, 0.f, 0.f);
		AddProp(TEXT("Chair"), BT, -10.7f, Y - 0.45f, 0.f, 180.f);
		AddProp(TEXT("Chair"), BT, -10.7f, Y + 0.45f, 0.f, 180.f);
	}
	AddProp(TEXT("CoffeeMachine"), BT, -13.2f, 5.f, 0.f, 0.f);
	AddProp(TEXT("VendingMachine"), BT, -13.1f, 7.f, 0.f, 0.f);
	AddProp(TEXT("Sofa"), BT, 11.f, -3.f, 0.f, 180.f);
	AddProp(TEXT("Sofa"), BT, 11.f, 1.f, 0.f, 180.f);
	AddProp(TEXT("CafeTable"), BT, 9.6f, -1.f, 0.f, 90.f);
	AddProp(TEXT("VendingMachine"), BT, 13.1f, 7.f, 0.f, 180.f);
	// security robot patrolling spot by the main door, robot dog in Lysgaarden
	AddProp(TEXT("RobotGuard"), BT, 2.5f, -13.f, 0.f, -90.f);
	AddProp(TEXT("RobotDog"), BT, -5.f, 4.f, 0.f, 30.f);
}

void AGSCampusBuilder::BuildCampusProps()
{
	FRandomStream Rand(777);
	const FTransform WorldT = FTransform::Identity;
	const TArray<GSCampus::FRoad>& Roads = GSCampus::Roads();
	auto Inside = [&](const FVector2D& P, float Margin)
	{
		for (const FBox2D& Box : Footprints) if (Box.ExpandBy(Margin).IsInside(P)) return true;
		return false;
	};
	auto OnRoad = [&](const FVector2D& P, float Margin)
	{
		for (const GSCampus::FRoad& R : Roads) if (DistToSegment(P, R.A, R.B) < R.Width * 0.5f + Margin) return true;
		return false;
	};
	auto Yaw = [](const FVector2D& D) { return FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)); };

	// Along every road and path: benches with a bin, scooters, bollards, signs; bus stops on car roads.
	for (const GSCampus::FRoad& R : Roads)
	{
		const FVector2D D = (R.B - R.A).GetSafeNormal();
		const FVector2D Side(-D.Y, D.X);
		const float Len = FVector2D::Distance(R.A, R.B);
		const float Spacing = R.bPath ? 28.f : 45.f;
		for (float S = 12.f + Rand.FRandRange(0.f, 8.f); S < Len - 6.f; S += Spacing + Rand.FRandRange(-6.f, 6.f))
		{
			const float SideSign = (Rand.FRand() < 0.5f) ? 1.f : -1.f;
			const FVector2D Edge = R.A + D * S + Side * SideSign * (R.Width * 0.5f + 1.4f);
			if (Inside(Edge, 2.f) || OnRoad(Edge, 0.6f)) continue;
			// benches face the road
			const float FaceRoad = Yaw(-Side * SideSign);
			const int32 Kind = Rand.RandRange(0, 9);
			if (Kind <= 3)
			{
				AddProp(TEXT("ParkBench"), WorldT, Edge.X, Edge.Y, 0.f, FaceRoad);
				const FVector2D BinP = Edge + D * 1.4f;
				AddProp(TEXT("OutdoorBin"), WorldT, BinP.X, BinP.Y, 0.f, FaceRoad);
			}
			else if (Kind <= 5)
			{
				AddProp(TEXT("Scooter"), WorldT, Edge.X, Edge.Y, 0.f, Yaw(D) + Rand.FRandRange(-30.f, 30.f), false);
				if ((Rand.FRand() < 0.5f))
				{
					const FVector2D P2 = Edge + D * 0.9f;
					AddProp(TEXT("Scooter"), WorldT, P2.X, P2.Y, 0.f, Yaw(D) + Rand.FRandRange(-30.f, 30.f), false);
				}
			}
			else if (Kind == 6)
			{
				AddProp(TEXT("Bike"), WorldT, Edge.X, Edge.Y, 0.f, Yaw(D) + Rand.FRandRange(-15.f, 15.f));
			}
			else if (Kind == 7)
			{
				AddProp(TEXT("StreetSign"), WorldT, Edge.X, Edge.Y, 0.f, FaceRoad);
			}
			else if (Kind == 8 && !R.bPath)
			{
				const FVector2D P = R.A + D * S + Side * SideSign * (R.Width * 0.5f + 2.2f);
				if (!Inside(P, 3.f)) AddProp(TEXT("BusStop"), WorldT, P.X, P.Y, 0.f, FaceRoad + 180.f);
			}
			else
			{
				AddProp(TEXT("Planter"), WorldT, Edge.X, Edge.Y, 0.f, 0.f);
			}
		}
		// a delivery robot somewhere on every path
		if (R.bPath && Len > 30.f)
		{
			const FVector2D P = R.A + D * Rand.FRandRange(8.f, Len - 8.f) + Side * Rand.FRandRange(-0.8f, 0.8f);
			if (!Inside(P, 1.f)) AddProp(TEXT("RobotDelivery"), WorldT, P.X, P.Y, 0.f, Yaw(D) + ((Rand.FRand() < 0.5f) ? 0.f : 180.f));
		}
	}

	// Around the buildings: bike racks and bollards at the front, bins and dumpsters with pallets behind.
	for (const GSCampus::FBuilding& B : GSCampus::Buildings())
	{
		if (!B.bEnterable) continue;
		const FTransform BT(FRotator(0.f, B.Yaw, 0.f), FVector(B.X * CmPerM, B.Y * CmPerM, 0.f));
		const float HX = B.SizeX * 0.5f, HY = B.SizeY * 0.5f;
		const bool bMain = MainBuildingMesh() && FString(B.Name) == TEXT("Hovedbygningen");
		auto Ok = [&](float LX, float LY, float Margin)
		{
			const FVector W = BT.TransformPosition(FVector(LX, LY, 0.f));
			const FVector2D P(W.X, W.Y);
			return !Inside(P, Margin) && !OnRoad(P, 0.8f);
		};
		const float FrontY = bMain ? -HY - 9.f : -HY - 3.5f;
		if (Ok(HX * 0.55f, FrontY, 1.f)) AddProp(TEXT("BikeRack"), BT, HX * 0.55f, FrontY, 0.f, 90.f);
		if (Ok(-HX * 0.55f, FrontY, 1.f)) AddProp(TEXT("BikeRack"), BT, -HX * 0.55f, FrontY, 0.f, 90.f);
		for (float X = -6.f; X <= 6.f; X += 3.f)
		{
			if (Ok(X, FrontY - 2.5f, 0.5f)) AddProp(TEXT("Bollard"), BT, X, FrontY - 2.5f, 0.f, 0.f);
		}
		const float BackY = HY + 3.5f;
		if (Ok(-HX + 4.f, BackY, 1.f))
		{
			AddProp(TEXT("Dumpster"), BT, -HX + 4.f, BackY, 0.f, 90.f);
			if (Ok(-HX + 6.5f, BackY, 0.8f)) AddProp(TEXT("Dumpster"), BT, -HX + 6.5f, BackY, 0.f, 90.f);
			if (Ok(-HX + 9.f, BackY, 0.8f)) AddProp(TEXT("Pallet"), BT, -HX + 9.f, BackY, 0.f, Rand.FRandRange(0.f, 30.f));
			if (Ok(-HX + 10.8f, BackY + 0.4f, 0.8f)) AddProp(TEXT("Crates"), BT, -HX + 10.8f, BackY + 0.4f, 0.f, Rand.FRandRange(-20.f, 20.f));
		}
		if (Ok(HX + 2.f, 0.f, 0.5f)) AddProp(TEXT("OutdoorBin"), BT, HX + 2.f, 0.f, 0.f, 0.f);
		if (bMain)
		{
			AddProp(TEXT("NTNUSign"), BT, 16.f, -HY - 8.f, 0.f, -90.f);
			AddProp(TEXT("Planter"), BT, -9.f, -HY - 7.f, 0.f, 0.f);
			AddProp(TEXT("Planter"), BT, 9.f, -HY - 7.f, 0.f, 0.f);
		}
	}

	// Lawns: picnic tables, robot dogs, the odd construction barrier and fence
	int32 Lawn = 0;
	for (int32 Try = 0; Try < 600 && Lawn < 45; ++Try)
	{
		const FVector2D P(Rand.FRandRange(-300.f, 440.f), Rand.FRandRange(-95.f, 530.f));
		if (Inside(P, 5.f) || OnRoad(P, 3.f)) continue;
		const int32 Kind = Rand.RandRange(0, 9);
		const float Y = Rand.FRandRange(0.f, 360.f);
		if (Kind <= 4) AddProp(TEXT("PicnicTable"), WorldT, P.X, P.Y, 0.f, Y);
		else if (Kind <= 6)
		{
			// a short construction site: fence and barriers
			const FVector2D D(FMath::Cos(FMath::DegreesToRadians(Y)), FMath::Sin(FMath::DegreesToRadians(Y)));
			for (int32 I = 0; I < 3; ++I)
			{
				const FVector2D Q = P + D * (I * 3.f);
				if (!Inside(Q, 3.f) && !OnRoad(Q, 2.f)) AddProp(TEXT("Fence"), WorldT, Q.X, Q.Y, 0.f, Y + 90.f);
			}
			const FVector2D Q = P + FVector2D(-D.Y, D.X) * 2.f;
			if (!Inside(Q, 3.f) && !OnRoad(Q, 2.f)) AddProp(TEXT("Barrier"), WorldT, Q.X, Q.Y, 0.f, Y + 90.f);
		}
		else if (Kind == 7) AddProp(TEXT("RobotDog"), WorldT, P.X, P.Y, 0.f, Y);
		else AddProp(TEXT("ParkBench"), WorldT, P.X, P.Y, 0.f, Y);
		++Lawn;
	}
}
