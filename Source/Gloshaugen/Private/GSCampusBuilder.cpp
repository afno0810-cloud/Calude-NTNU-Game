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

	// Paved squares (m): nothing grows or stands on them.
	struct FPlaza { float X0, Y0, X1, Y1; };
	const FPlaza Plazas[] = {
		{ -16.f, -34.f, 16.f, -20.5f },   // forecourt in front of the main entrance
		{ -41.f,  14.f, 41.f,  58.f },    // courtyard between the wings (the back yard)
	};
	bool InPlaza(const FVector2D& P, float Margin)
	{
		for (const FPlaza& Z : Plazas)
			if (P.X > Z.X0 - Margin && P.X < Z.X1 + Margin && P.Y > Z.Y0 - Margin && P.Y < Z.Y1 + Margin) return true;
		return false;
	}

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

	/** A box whose top is under 0.3 m and that lies flat (ground, roads, squares, painted lines, sidewalks, kerbs):
	 *  it cannot shade anything, so it goes in a group that casts no shadow. */
	bool IsLowBox(const FVector& CenterCm, const FRotator& Rot, const FVector& SizeM)
	{
		return CenterCm.Z / CmPerM + SizeM.Z * 0.5 < 0.3 && Rot.Pitch == 0. && Rot.Roll == 0.;
	}

	bool NameIn(const TCHAR* Name, std::initializer_list<const TCHAR*> List)
	{
		for (const TCHAR* N : List)
		{
			if (FCString::Strcmp(Name, N) == 0) return true;
		}
		return false;
	}
}

AGSCampusBuilder::AGSCampusBuilder()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
}

void AGSCampusBuilder::Queue(UInstancedStaticMeshComponent* ISM, const FTransform& WorldT)
{
	if (ISM) Pending.FindOrAdd(ISM).Add(WorldT);
}

void AGSCampusBuilder::FlushPending()
{
	for (TPair<UInstancedStaticMeshComponent*, TArray<FTransform>>& Pair : Pending)
	{
		if (Pair.Key && Pair.Value.Num() > 0)
		{
			Pair.Key->AddInstances(Pair.Value, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true);
		}
	}
	Pending.Empty();
}

UInstancedStaticMeshComponent* AGSCampusBuilder::GetISM(UStaticMesh* Mesh, const FLinearColor& Color, bool bCollide, bool bShadow)
{
	const FString Key = ColorKey(Mesh, Color, bCollide) + (bShadow ? TEXT("") : TEXT("_L"));
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
	if (!bShadow) ISM->SetCastShadow(false);
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

void AGSCampusBuilder::AddMesh(UStaticMesh* Mesh, const FTransform& T, const FLinearColor& Color, bool bCollide, bool bShadow)
{
	if (!Mesh) return;
	Queue(GetISM(Mesh, Color, bCollide, bShadow), T);
}

void AGSCampusBuilder::AddBox(const FVector& CenterCm, const FRotator& Rot, const FVector& SizeM, const FLinearColor& Color, bool bCollide)
{
	if (SizeM.X <= 0.01f || SizeM.Y <= 0.01f || SizeM.Z <= 0.01f) return;
	AddMesh(GSUtils::Cube(), FTransform(Rot, CenterCm, SizeM), Color, bCollide, !IsLowBox(CenterCm, Rot, SizeM));
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
	const bool bLow = IsLowBox(CenterCm, Rot, SizeM);
	const FString Key = FString::Printf(TEXT("Cube_%s_%d%s"), MatName, bCollide ? 1 : 0, bLow ? TEXT("_L") : TEXT(""));
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
		if (bLow) ISM->SetCastShadow(false);
		ISM->RegisterComponent();
		ISM->SetMaterial(0, Mat);
		ISMs.Add(Key, ISM);
	}
	Queue(ISM, FTransform(Rot, CenterCm, SizeM));
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
	BuildPlazas();
	for (const GSCampus::FBuilding& B : GSCampus::Buildings())
	{
		BuildBuilding(B);
	}
	BuildSidewalks();
	BuildPathEdges();
	BuildPerimeter();
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
	// Everything above only queued its instances: add them now, one call per group.
	FlushPending();

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

void AGSCampusBuilder::BuildPlazas()
{
	// Top 3 cm over the lawn and 1 cm under the paths, so nothing flickers where they meet.
	for (const FPlaza& Z : Plazas)
	{
		AddBoxMat(FVector((Z.X0 + Z.X1) * 0.5f, (Z.Y0 + Z.Y1) * 0.5f, -0.02f) * CmPerM, FRotator::ZeroRotator,
			FVector(Z.X1 - Z.X0, Z.Y1 - Z.Y0, 0.1f), TEXT("W_Paving"), FLinearColor(0.42f, 0.41f, 0.39f), true);
	}
}

void AGSCampusBuilder::BuildPathEdges()
{
	// Kerbs on both sides of every path, 3 cm higher than the paving, in pieces of at most 4 m. No collision, so nobody
	// gets stuck on them. Each side is walked in 25 cm steps, and a kerb is only laid along stretches that are clear of
	// other paths, squares and buildings from one end to the other: no piece lies across a path that joins or crosses.
	const TArray<GSCampus::FRoad>& Roads = GSCampus::Roads();
	constexpr float Piece = 4.f;
	constexpr float Step = 0.25f;
	for (const GSCampus::FRoad& R : Roads)
	{
		if (!R.bPath) continue;
		const FVector2D D = (R.B - R.A).GetSafeNormal();
		const FVector2D Side(-D.Y, D.X);
		const float Len = FVector2D::Distance(R.A, R.B);
		const FRotator Rot(0.f, FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)), 0.f);
		auto Blocked = [&](const FVector2D& K)
		{
			if (InPlaza(K, 0.f)) return true;
			for (const GSCampus::FRoad& O : Roads)
			{
				if (&O != &R && DistToSegment(K, O.A, O.B) < O.Width * 0.5f + 0.1f) return true;
			}
			for (const FBox2D& Box : Footprints)
			{
				if (Box.ExpandBy(0.2f).IsInside(K)) return true;
			}
			return false;
		};
		const int32 Steps = FMath::CeilToInt(Len / Step);
		for (float Sgn : { -1.f, 1.f })
		{
			const FVector2D Base = R.A + Side * Sgn * (R.Width * 0.5f + 0.06f);
			int32 RunStart = INDEX_NONE;
			// one sample more than the path is long, so the last free stretch is closed too
			for (int32 I = 0; I <= Steps + 1; ++I)
			{
				const bool bFree = I <= Steps && !Blocked(Base + D * FMath::Min(I * Step, Len));
				if (bFree)
				{
					if (RunStart == INDEX_NONE) RunStart = I;
					continue;
				}
				if (RunStart == INDEX_NONE) continue;
				// free from sample RunStart to sample I - 1
				const float S0 = RunStart * Step;
				const float S1 = FMath::Min((I - 1) * Step, Len);
				RunStart = INDEX_NONE;
				if (S1 - S0 < 0.5f) continue;
				const int32 N = FMath::CeilToInt((S1 - S0) / Piece);
				const float L = (S1 - S0) / N;
				for (int32 P = 0; P < N; ++P)
				{
					const FVector2D K = Base + D * (S0 + L * (P + 0.5f));
					AddBoxMat(FVector(K.X, K.Y, 0.f) * CmPerM, Rot, FVector(L, 0.12f, 0.14f), TEXT("W_Concrete"), FLinearColor(0.55f, 0.55f, 0.53f), false);
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------
// The edge of the map. The campus is "under rebuilding": site fences all the way round, the roads
// out are closed, and the same fences, containers and parked cars give cover on the long routes.
// ---------------------------------------------------------------------------------------------

int32 AGSCampusBuilder::FenceRun(const FVector2D& A, const FVector2D& B, bool bMixed)
{
	constexpr float PanelLen = 3.5f;   // SM_Prop_SiteFence, along its Y axis
	const float Len = FVector2D::Distance(A, B);
	if (Len < 0.5f) return 0;
	FenceSegs.Add(TPair<FVector2D, FVector2D>(A, B));
	const FVector2D D = (B - A).GetSafeNormal();
	const FVector2D Front(D.Y, -D.X);
	// model +Y along D, which turns model +X (the front) to Front
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(-D.X, D.Y));
	const int32 N = FMath::CeilToInt(Len / PanelLen);
	const float Spacing = Len / N;
	const FVector Scale(1.f, Spacing / PanelLen, 1.f);
	for (int32 I = 0; I < N; ++I)
	{
		const FVector2D Mid = A + D * (Spacing * (I + 0.5f));
		bool bOpen = false;
		if (bMixed)
		{
			// see through where a road or path leaves the map (you see the closed road behind), and now and then
			bOpen = I % 4 == 2;
			for (const GSCampus::FRoad& R : GSCampus::Roads())
			{
				if (DistToSegment(Mid, R.A, R.B) < R.Width * 0.5f + (R.bPath ? 0.5f : 2.8f)) bOpen = true;
			}
		}
		AddPropWorld(bOpen ? TEXT("SiteFence") : TEXT("SiteFenceTarp"),
			FTransform(FRotator(0.f, Yaw, 0.f), FVector(Mid.X, Mid.Y, 0.f) * CmPerM, Scale), true);
		if (bMixed && I % 8 == 4)
		{
			const FVector2D S = Mid + Front * 0.08f;
			AddPropWorld(TEXT("SiteSign"), FTransform(FRotator(0.f, Yaw, 0.f), FVector(S.X, S.Y, 0.f) * CmPerM), false);
		}
	}
	return N;
}

bool AGSCampusBuilder::NearPerimeter(const FVector2D& P, float Margin) const
{
	for (const TPair<FVector2D, FVector2D>& S : FenceSegs)
	{
		if (DistToSegment(P, S.Key, S.Value) < Margin) return true;
	}
	return false;
}

bool AGSCampusBuilder::IsReserved(const FVector2D& P, float Extra) const
{
	for (const FVector& R : Reserved)
	{
		if (FVector2D::Distance(P, FVector2D(R.X, R.Y)) < R.Z + Extra) return true;
	}
	return false;
}

void AGSCampusBuilder::BuildPerimeter()
{
	const FTransform WorldT = FTransform::Identity;
	const FBox2D Area = GSCampus::PlayArea();
	// the fence stands 0.5 m inside the edge, front (signs) towards the players
	const FBox2D Line(Area.Min + FVector2D(0.5f, 0.5f), Area.Max - FVector2D(0.5f, 0.5f));
	int32 Panels = 0, Closures = 0, Containers = 0;

	// 1. Fence all the way round. The corners are walked so that the front of every panel faces inwards.
	const FVector2D NW(Line.Min.X, Line.Min.Y), NE(Line.Max.X, Line.Min.Y), SE(Line.Max.X, Line.Max.Y), SW(Line.Min.X, Line.Max.Y);
	Panels += FenceRun(NE, NW, true);
	Panels += FenceRun(NW, SW, true);
	Panels += FenceRun(SW, SE, true);
	Panels += FenceRun(SE, NE, true);

	// 2. Every road and path that crosses the fence line is closed just inside it.
	for (const GSCampus::FRoad& R : GSCampus::Roads())
	{
		// the part of the road inside the fence rectangle is T0..T1 along A -> B (Liang-Barsky clipping)
		const FVector2D AB = R.B - R.A;
		double T0 = 0., T1 = 1.;
		bool bInside = true;
		const double Pv[4] = { -AB.X, AB.X, -AB.Y, AB.Y };
		const double Qv[4] = { R.A.X - Line.Min.X, Line.Max.X - R.A.X, R.A.Y - Line.Min.Y, Line.Max.Y - R.A.Y };
		for (int32 I = 0; I < 4; ++I)
		{
			if (FMath::IsNearlyZero(Pv[I]))
			{
				if (Qv[I] < 0.) bInside = false;
				continue;
			}
			const double T = Qv[I] / Pv[I];
			if (Pv[I] < 0.) T0 = FMath::Max(T0, T);
			else T1 = FMath::Min(T1, T);
		}
		if (!bInside || T0 >= T1) continue;   // the whole road lies outside
		const FVector2D Dir = AB.GetSafeNormal();
		for (int32 End = 0; End < 2; ++End)
		{
			if (End == 0 ? T0 <= 0. : T1 >= 1.) continue;   // this end of the road is inside the fence
			const FVector2D C = R.A + AB * (End == 0 ? T0 : T1);   // where the road meets the fence
			const FVector2D In = End == 0 ? Dir : -Dir;            // along the road, into the map
			const FVector2D Side(-In.Y, In.X);
			const float FaceIn = FMath::RadiansToDegrees(FMath::Atan2(In.Y, In.X));
			if (!R.bPath)
			{
				// car road: a row of concrete barriers across the lanes and both sidewalks, a no-entry trestle in
				// front of it, and a van left on the road outside the fence
				const FVector2D Row = C + In * 2.f;
				for (int32 K = -3; K <= 3; ++K)
				{
					const FVector2D P = Row + Side * (K * 2.1f);
					AddProp(TEXT("Barrier"), WorldT, P.X, P.Y, FMath::Abs(K) == 3 ? 0.12f : 0.f, FaceIn);
				}
				const FVector2D Trestle = C + In * 3.5f;
				AddProp(TEXT("RoadBlock"), WorldT, Trestle.X, Trestle.Y, 0.f, FaceIn);
				const FVector2D Van = C - In * 4.f;
				AddModel(TEXT("SM_Car_Van"), FTransform(FRotator(0.f, FaceIn + 8.f, 0.f), FVector(Van.X, Van.Y, 0.f) * CmPerM), TEXT("CarPaint_White"), true);
				Reserved.Add(FVector(Row.X, Row.Y, 9.f));
				Reserved.Add(FVector(Van.X, Van.Y, 7.f));
			}
			else
			{
				const FVector2D Trestle = C + In * 1.5f;
				AddProp(TEXT("RoadBlock"), WorldT, Trestle.X, Trestle.Y, 0.f, FaceIn);
				Reserved.Add(FVector(Trestle.X, Trestle.Y, 4.f));
			}
			++Closures;
		}
	}

	// 3. Invisible walls exactly on the edges of the play area, 0.35 m behind the fence: the fence is 2.0 m high and
	// players climb 2.2 m. They also stop grenades, so they must never stand inside the map.
	// 100 m high with a lid on top: the grapple hook (35 m, up to 26 m/s + 4.5 m/s lift) carries you about 47 m above
	// where you fire it, and hooking the wall itself twice from a 16 m base roof would reach about 110 m, so the box is
	// closed at the top. The hook stops on the wall (WorldStatic blocks), so nothing outside the map can be hooked.
	// Bullets, bullet holes and roof checks (Visibility) ignore it: a shot over the fence flies on and leaves the map.
	PerimeterBlock = NewObject<UInstancedStaticMeshComponent>(this);
	PerimeterBlock->SetMobility(EComponentMobility::Static);
	PerimeterBlock->SetStaticMesh(GSUtils::Cube());
	PerimeterBlock->SetupAttachment(RootComponent);
	PerimeterBlock->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	PerimeterBlock->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	PerimeterBlock->SetVisibility(false);
	PerimeterBlock->SetHiddenInGame(true);
	PerimeterBlock->SetCastShadow(false);
	PerimeterBlock->RegisterComponent();
	constexpr float BlockT = 0.3f, BlockH = 100.f;
	const FVector2D Mid = Area.GetCenter();
	const FVector2D Size = Area.GetSize();
	for (const double Y : { Area.Min.Y, Area.Max.Y })
	{
		Queue(PerimeterBlock, FTransform(FRotator::ZeroRotator, FVector(Mid.X, Y, BlockH * 0.5f) * CmPerM, FVector(Size.X + BlockT, BlockT, BlockH)));
	}
	for (const double X : { Area.Min.X, Area.Max.X })
	{
		Queue(PerimeterBlock, FTransform(FRotator::ZeroRotator, FVector(X, Mid.Y, BlockH * 0.5f) * CmPerM, FVector(BlockT, Size.Y + BlockT, BlockH)));
	}
	Queue(PerimeterBlock, FTransform(FRotator::ZeroRotator, FVector(Mid.X, Mid.Y, BlockH) * CmPerM, FVector(Size.X + BlockT, Size.Y + BlockT, BlockT)));   // the lid

	// 4. Shipping containers just inside the fence, long side along the edge: solid cover on the flanks.
	struct FContainerSpot { const TCHAR* Name; float X, Y, Yaw; };
	const FContainerSpot Spots[] = {
		{ TEXT("Container"), -159.f, -70.f, 0.f },        { TEXT("Container"), 159.f, 90.f, 180.f },
		{ TEXT("ContainerGreen"), -159.f, 75.f, 0.f },    { TEXT("ContainerGreen"), 159.f, -55.f, 180.f },
		{ TEXT("Container"), -70.f, 135.5f, 90.f },       { TEXT("ContainerGreen"), 70.f, 135.5f, -90.f },
	};
	for (const FContainerSpot& S : Spots)
	{
		AddProp(S.Name, WorldT, S.X, S.Y, 0.f, S.Yaw);
		// 6.06 m long (model Y) and 2.44 m wide; trees and props stay out of it
		const bool bAlongY = FMath::IsNearlyZero(FMath::Fmod(S.Yaw, 180.f));
		const FVector2D Half = bAlongY ? FVector2D(1.22f, 3.03f) : FVector2D(3.03f, 1.22f);
		Footprints.Add(FBox2D(FVector2D(S.X, S.Y) - Half, FVector2D(S.X, S.Y) + Half));
		if (PropMesh(S.Name)) ++Containers;
	}

	// 5. Rig area behind each base: tarpaulin fences from the edge in towards the outer corners of the base building,
	// with a 4 m opening next to the building that leads into a sluice (GSCampus::RigFences). Players spawn in here,
	// out of sight from all three routes.
	for (const GSCampus::FFence& F : GSCampus::RigFences())
	{
		Panels += FenceRun(F.A, F.B, false);                                        // west
		Panels += FenceRun(GSCampus::Mirror(F.A), GSCampus::Mirror(F.B), false);    // east
	}
	// nothing (trees, bushes, benches) in the sluices: they are only 1.9 m wide between the fences
	for (const FVector2D& S : { FVector2D(-151.5f, -3.25f), FVector2D(-151.5f, 23.25f) })
	{
		Reserved.Add(FVector(S.X, S.Y, 5.6f));
		const FVector2D E = GSCampus::Mirror(S);
		Reserved.Add(FVector(E.X, E.Y, 5.6f));
	}
	// a pallet and a stack of crates in each, against the fence and at least 3 m from every spawn point
	// (the east side is the west side turned 180 degrees about (0, 10))
	AddProp(TEXT("Pallet"), WorldT, -160.f, -1.4f, 0.f, 0.f);
	AddProp(TEXT("Crates"), WorldT, -158.f, 21.f, 0.f, 0.f);
	AddProp(TEXT("Pallet"), WorldT, 160.f, 21.4f, 0.f, 180.f);
	AddProp(TEXT("Crates"), WorldT, 158.f, -1.f, 0.f, 180.f);

	// 6. Dig sites fenced with tarpaulin right across the two long straight routes (north path 316 m, south
	// promenade 330 m): you walk round them on the lawn, and nobody can shoot from base to base along the path.
	auto DigSite = [&](float CX, float Y0, float Y1)
	{
		const FVector2D A(CX - 3.5f, Y0), B(CX + 3.5f, Y0), C(CX + 3.5f, Y1), D(CX - 3.5f, Y1);
		Panels += FenceRun(B, A, false);
		Panels += FenceRun(A, D, false);
		Panels += FenceRun(D, C, false);
		Panels += FenceRun(C, B, false);
		const float CY = (Y0 + Y1) * 0.5f;
		AddProp(TEXT("Pallet"), WorldT, CX - 1.3f, CY + 0.4f, 0.f, 15.f);
		AddProp(TEXT("Barrier"), WorldT, CX + 1.4f, CY - 0.2f, 0.f, 70.f);
		Reserved.Add(FVector(CX, CY, 7.f));   // at least 2 m of free lawn on both sides
	};
	for (const float Sgn : { -1.f, 1.f })
	{
		DigSite(Sgn * 96.f, -33.f, -28.f);     // north path (Y -32 .. -28)
		DigSite(Sgn * 100.f, 117.f, 123.f);    // south promenade (Y 117.5 .. 122.5)
	}

	// 7. Vehicles left on the paths (deliveries to the building site): cover half way along both routes.
	struct FParked { const TCHAR* Model; float X, Y, Yaw; const TCHAR* Paint; };
	const FParked Parked[] = {
		{ TEXT("SM_Car_Van"), -42.f, -30.7f, 4.f, TEXT("CarPaint_White") },
		{ TEXT("SM_Car_Estate"), 42.f, -29.3f, 184.f, TEXT("CarPaint_Blue") },
		{ TEXT("SM_Car_Van"), -30.f, 120.6f, 3.f, TEXT("CarPaint_Grey") },
		{ TEXT("SM_Car_Hatch"), 32.f, 119.4f, 183.f, TEXT("CarPaint_Red") },
	};
	for (const FParked& P : Parked)
	{
		AddModel(P.Model, FTransform(FRotator(0.f, P.Yaw, 0.f), FVector(P.X, P.Y, 0.f) * CmPerM), P.Paint, true);
		Reserved.Add(FVector(P.X, P.Y, 5.f));
	}
	// the forecourt in front of the main entrance: bike racks along the path and planters
	for (const float Sgn : { -1.f, 1.f })
	{
		AddProp(TEXT("BikeRack"), WorldT, Sgn * 11.f, -31.5f, 0.f, 90.f);
		AddProp(TEXT("Planter"), WorldT, Sgn * 5.f, -33.f, 0.f, 0.f);
	}

	UE_LOG(LogTemp, Log, TEXT("Gloshaugen: perimeter closed, %d fence panels, %d road closures, %d containers"), Panels, Closures, Containers);
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

	// The hand-made Blender model replaces the block version of the main building.
	const FString Name(B.Name);
	if (Name == TEXT("Hovedbygningen"))
	{
		if (UStaticMesh* Custom = MainBuildingMesh())
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
				G->ComponentTags.Add(TEXT("Mat_Glass"));
				G->RegisterComponent();
			}
			// front bay with the main door and the turrets
			Footprints.Add(FBox2D(FVector2D(B.X - 12.f, B.Y - 21.f), FVector2D(B.X + 12.f, B.Y - 14.f)));
			return;
		}
	}

	// Detailed Blender model of this building (Tools/Blender/buildings.py), plus its window glass. Scenery uses its model too.
	if (B.bEnterable || B.bBackdrop)
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
				G->ComponentTags.Add(TEXT("Mat_Glass"));
				G->RegisterComponent();
			}
			return;
		}
	}

	if (!B.bEnterable)
	{
		AddBox(Local(0.f, 0.f, B.Height * 0.5f), BRot, FVector(B.SizeX, B.SizeY, B.Height), B.Color);
		// small pointed roof for towers (not on scenery blocks)
		if (!B.bBackdrop) AddMesh(GSUtils::Cone(), FTransform(BRot, Local(0.f, 0.f, B.Height + 3.f), FVector(B.SizeX * 0.012f, B.SizeY * 0.012f, 0.06f)), FLinearColor(0.2f, 0.32f, 0.28f), true);
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

	TArray<FVector2D> Placed; // trees standing so far
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
		if (InPlaza(P, Margin * 0.5f)) return false;
		for (const FVector2D& S : GSCampus::SpawnPoints())
		{
			if (FVector2D::Distance(P, S) < 3.5f) return false;
		}
		for (const FVector2D& T : Placed)
		{
			if (FVector2D::Distance(P, T) < 4.f) return false;
		}
		// never in a site fence, a dig site, a road closure or on a parked car
		if (NearPerimeter(P, 1.5f) || IsReserved(P)) return false;
		return true;
	};

	const FLinearColor Trunk(0.22f, 0.16f, 0.11f);
	const FLinearColor Leaves[] = { FLinearColor(0.12f, 0.25f, 0.08f), FLinearColor(0.16f, 0.30f, 0.10f), FLinearColor(0.20f, 0.28f, 0.09f) };
	const bool bRealTrees = ModelMesh(TEXT("SM_Tree_Maple")) != nullptr;

	// One tree model with a random turn and size, and an invisible trunk collider as thick as its trunk.
	auto PlantTree = [&](const TCHAR* Kind, const FVector2D& P)
	{
		const float Scale = Rand.FRandRange(0.8f, 1.3f);
		AddModel(Kind, FTransform(FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f), FVector(P.X * CmPerM, P.Y * CmPerM, 0.f), FVector(Scale)), nullptr, false);
		const FString K(Kind);
		const float TrunkR = K == TEXT("SM_Tree_Maple") ? 0.35f : K == TEXT("SM_Tree_Birch") ? 0.2f : K == TEXT("SM_Tree_Spruce") ? 0.3f : 0.14f;
		AddTrunkCollider(P, TrunkR * Scale, 6.f * Scale);
		Placed.Add(P);
	};

	if (bRealTrees)
	{
		// Avenue of maples up to the main entrance, and street trees along Hoegskoleringen.
		for (int32 I = 0; I < 7; ++I)
		{
			for (float X : { -6.5f, 6.5f })
			{
				PlantTree(TEXT("SM_Tree_Maple"), FVector2D(X, -98.f + 9.5f * I));
			}
		}
		for (float X = -156.f; X <= 156.f; X += 13.f)
		{
			if (FMath::Abs(X) < 12.f) continue;
			PlantTree(TEXT("SM_Tree_Maple"), FVector2D(X, -100.f));
		}
	}

	// The rest: maples and birches on the lawns, spruces in groups (Trondheim). Leaves have no collision;
	// an invisible trunk collider stops players and bullets. Never on the axis in front of the main building.
	int32 Trees = 0;
	for (int32 Try = 0; Try < 1500 && Trees < 80; ++Try)
	{
		const FVector2D P(Rand.FRandRange(-230.f, 230.f), Rand.FRandRange(-125.f, 240.f));
		if (FMath::Abs(P.X) < 10.f && P.Y < -30.f) continue;
		if (!IsFree(P, 6.f)) continue;
		if (bRealTrees)
		{
			const float Pick = Rand.FRand();
			const TCHAR* Kind = Pick < 0.38f ? TEXT("SM_Tree_Maple") : Pick < 0.68f ? TEXT("SM_Tree_Birch")
				: Pick < 0.88f ? TEXT("SM_Tree_Spruce") : TEXT("SM_Tree_SpruceSmall");
			PlantTree(Kind, P);
			// a few bushes around some trees
			if (Rand.FRand() < 0.35f)
			{
				const FVector2D Q = P + FVector2D(Rand.FRandRange(-3.f, 3.f), Rand.FRandRange(-3.f, 3.f));
				// like IsFree(Q, 2), except that a bush may stand close to its own tree
				bool bOk = !InPlaza(Q, 1.f);
				for (const FBox2D& Box : Footprints) if (Box.ExpandBy(2.f).IsInside(Q)) bOk = false;
				for (const GSCampus::FRoad& R : Roads) if (DistToSegment(Q, R.A, R.B) < R.Width * 0.5f + 1.f) bOk = false;
				for (const FVector2D& S : GSCampus::SpawnPoints()) if (FVector2D::Distance(Q, S) < 3.5f) bOk = false;
				if (NearPerimeter(Q, 1.5f) || IsReserved(Q)) bOk = false;
				if (bOk) AddModel(TEXT("SM_Bush"), FTransform(FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f), FVector(Q.X * CmPerM, Q.Y * CmPerM, 0.f), FVector(Rand.FRandRange(0.8f, 1.4f))), nullptr, false);
			}
		}
		else
		{
			const float H = Rand.FRandRange(6.f, 12.f);
			AddMesh(GSUtils::Cylinder(), FTransform(FRotator::ZeroRotator, FVector(P.X * CmPerM, P.Y * CmPerM, H * 0.25f * CmPerM), FVector(0.45f, 0.45f, H * 0.5f)), Trunk, true);
			const float R = Rand.FRandRange(3.f, 5.5f);
			AddMesh(GSUtils::Sphere(), FTransform(FRotator::ZeroRotator, FVector(P.X * CmPerM, P.Y * CmPerM, (H * 0.5f + R * 0.6f) * CmPerM), FVector(R, R, R * 1.3f)),
				Leaves[Rand.RandRange(0, 2)], false);
			Placed.Add(P);
		}
		++Trees;
	}

	// A few moraine boulders on the lawns (they block bullets, good cover) and four flower beds by the forecourt.
	if (bRealTrees)
	{
		int32 Rocks = 0;
		for (int32 Try = 0; Try < 800 && Rocks < 10; ++Try)
		{
			const FVector2D P(Rand.FRandRange(-230.f, 230.f), Rand.FRandRange(-125.f, 240.f));
			if (!IsFree(P, 4.f)) continue;
			const float Pick = Rand.FRand();
			const TCHAR* Kind = Pick < 0.45f ? TEXT("SM_Rock_A") : Pick < 0.8f ? TEXT("SM_Rock_B") : TEXT("SM_Rock_C");
			AddModel(Kind, FTransform(FRotator(Rand.FRandRange(-6.f, 6.f), Rand.FRandRange(0.f, 360.f), 0.f), FVector(P.X * CmPerM, P.Y * CmPerM, -0.15f * CmPerM),
				FVector(Rand.FRandRange(0.7f, 1.3f))), nullptr, true);
			Placed.Add(P);
			++Rocks;
		}
		for (const FVector2D P : { FVector2D(-22.f, -38.f), FVector2D(22.f, -38.f), FVector2D(-32.f, -38.f), FVector2D(32.f, -38.f) })
		{
			if (!IsFree(P, 4.f)) continue;
			AddModel(TEXT("SM_FlowerBed"), FTransform(FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f), FVector(P.X * CmPerM, P.Y * CmPerM, 0.f),
				FVector(Rand.FRandRange(0.8f, 1.3f))), nullptr, false);
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
			// door position on the +-X walls (same formula as BuildWallSide); the Hovedbygningen model has its end doors at y = 0
			const float L = B.SizeY - 2.f * WallT;
			const int32 NW = FMath::FloorToInt((L - MinPier) / (WindowW + MinPier));
			const float Pier = NW >= 1 ? (L - NW * WindowW) / (NW + 1) : 0.f;
			const bool bMainModel = MainBuildingMesh() && FString(B.Name) == TEXT("Hovedbygningen");
			const float DoorY = bMainModel ? 0.f : -L * 0.5f + Pier + WindowW * 0.5f + (NW / 2) * (Pier + WindowW);
			for (float Sgn : { -1.f, 1.f })
			{
				for (float Y = -HY + 3.f; Y < HY - 3.f; Y += 4.2f)
				{
					if (NW >= 1 && FMath::Abs(Y - DoorY) < WindowW * 0.5f + 2.7f) continue;   // hedge piece is 4.2 m long (F6)
					const FVector W = BT.TransformPosition(FVector((HX + 1.2f) * Sgn, Y, 0.f) * CmPerM);
					const FVector2D P(W.X / CmPerM, W.Y / CmPerM);
					if (!IsFree(P, 0.6f) || InPlaza(P, 0.5f)) continue;
					AddModel(TEXT("SM_Hedge"), FTransform(FRotator(0.f, B.Yaw + 90.f, 0.f), W), nullptr, true);
				}
			}
			for (int32 I = 0; I < 6; ++I)
			{
				const FVector W = BT.TransformPosition(FVector(Rand.FRandRange(-HX, HX), (HY + 1.5f) * (Rand.FRand() < 0.5f ? -1.f : 1.f), 0.f) * CmPerM);
				const FVector2D P(W.X / CmPerM, W.Y / CmPerM);
				if (IsFree(P, 0.8f) && !InPlaza(P, 0.5f)) AddModel(TEXT("SM_Bush"), FTransform(FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f), W, FVector(Rand.FRandRange(0.7f, 1.2f))), nullptr, false);
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
	for (int32 Try = 0; Try < 600 && Cars < 22; ++Try)
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
		if (NearPerimeter(P, 4.f) || IsReserved(P)) bBlocked = true;   // not in the fence or in a road closure
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
		const FString ModelName(Name);
		if (ModelName.StartsWith(TEXT("SM_Tree_")) || ModelName.StartsWith(TEXT("SM_Bush")) || ModelName.StartsWith(TEXT("SM_Hedge")))
		{
			// Leaves sway with world position offset. Beyond 40 m the sway is switched off, and the sway never
			// throws away the cached shadow maps: otherwise every tree redraws its shadow every single frame.
			ISM->SetWorldPositionOffsetDisableDistance(4000);
			ISM->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Rigid;
		}
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
	Queue(ISM, WorldT);
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
	Queue(TrunkColliders, FTransform(FRotator::ZeroRotator, FVector(P.X * CmPerM, P.Y * CmPerM, Height * 0.5f * CmPerM),
		FVector(Radius * 2.f, Radius * 2.f, Height)));
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
			if (NearPerimeter(P, 1.f) || IsReserved(P)) bBlocked = true;
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
			// not drawn at all beyond 80 m, fading out over the last 15 m
			L->MaxDrawDistance = 8000.f;
			L->MaxDistanceFadeRange = 1500.f;
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
	const FVector Loc = BT.TransformPosition(FVector(X, Y, Z) * CmPerM);
	const FRotator Rot = (FQuat(BT.GetRotation()) * FQuat(FRotator(0.f, Yaw, 0.f))).Rotator();
	AddPropWorld(Name, FTransform(Rot, Loc, FVector(Scale)), bCollide);
}

void AGSCampusBuilder::AddPropWorld(const TCHAR* Name, const FTransform& WorldT, bool bCollide)
{
	UStaticMesh* Mesh = PropMesh(Name);
	if (!Mesh) return;
	// Small things nobody should get stuck on never collide, whatever the caller asks for.
	if (NameIn(Name, { TEXT("Plant"), TEXT("TrashBin"), TEXT("Stool"), TEXT("Projector") })) bCollide = false;
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
		// Small things disappear at 90 m (saves a lot on a 6 GB card); big things you steer by are seen to 400 m.
		const bool bSeenFar = NameIn(Name, { TEXT("StreetLamp"), TEXT("BusStop"), TEXT("Dumpster"), TEXT("NTNUSign"), TEXT("Barrier"),
			TEXT("SiteFence"), TEXT("SiteFenceTarp"), TEXT("SiteSign"), TEXT("RoadBlock"), TEXT("Container"), TEXT("ContainerGreen") });
		ISM->SetCullDistances(0, bSeenFar ? 40000 : 9000);
		ISM->RegisterComponent();
		PropISMs.Add(Key, ISM);
	}
	Queue(ISM, WorldT);
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
	// The bases have a screen wall inside the north and the south door (local x -3 .. 3 at y = +-6, see
	// Tools/Blender/buildings.py): no furniture in the walls or in the way round them.
	for (const GSCampus::FBase& Base : GSCampus::Bases())
	{
		if (Name != Base.Building) continue;
		for (const float X : { -4.5f, -2.5f, 0.f, 2.5f, 4.5f })
		{
			for (const float Y : { -6.f, 6.f }) Keep.Add(FVector(X, Y, 0.f));
		}
	}

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

	// Lysgaarden is the middle route between the bases, so the floor is kept open: free ways north-south at
	// x 1.6 .. 3.5 on both sides of the plinth (y -8.5 .. -6.5, where the loot crate stands) and east-west at y -3.3 .. 0.
	AddProp(TEXT("Plant"), BT, -6.5f, -8.3f, 0.f, 0.f);
	AddProp(TEXT("Plant"), BT, 6.5f, -8.3f, 0.f, 0.f);
	// cafe corner on the west side and a sofa lounge on the east side, against the walls
	for (float Y : { -7.f, -4.2f, 5.f })
	{
		AddProp(TEXT("CafeTable"), BT, -11.5f, Y, 0.f, 0.f);
		AddProp(TEXT("Chair"), BT, -12.3f, Y - 0.45f, 0.f, 0.f);
		AddProp(TEXT("Chair"), BT, -12.3f, Y + 0.45f, 0.f, 0.f);
		AddProp(TEXT("Chair"), BT, -10.7f, Y - 0.45f, 0.f, 180.f);
		AddProp(TEXT("Chair"), BT, -10.7f, Y + 0.45f, 0.f, 180.f);
	}
	AddProp(TEXT("CoffeeMachine"), BT, -13.2f, 5.f, 0.f, 0.f);
	AddProp(TEXT("VendingMachine"), BT, -13.1f, 7.f, 0.f, 0.f);
	AddProp(TEXT("Sofa"), BT, 11.f, -6.8f, 0.f, 180.f);
	AddProp(TEXT("Sofa"), BT, 11.f, -4.2f, 0.f, 180.f);
	AddProp(TEXT("CafeTable"), BT, 9.6f, -5.5f, 0.f, 90.f);
	AddProp(TEXT("VendingMachine"), BT, 13.1f, 7.f, 0.f, 180.f);
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
			// not in the site fences, and nothing within reach of the dig sites, the parked cars and the road closures
			if (NearPerimeter(Edge, 1.5f) || IsReserved(Edge)) continue;
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
			const FVector W = BT.TransformPosition(FVector(LX, LY, 0.f) * CmPerM);
			const FVector2D P(W.X / CmPerM, W.Y / CmPerM);   // world meters, like Footprints and the roads
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

	// Lawns inside the play area: picnic tables, benches, the odd construction barrier and fence
	auto NearSpawn = [](const FVector2D& P, float Dist)
	{
		for (const FVector2D& S : GSCampus::SpawnPoints()) if (FVector2D::Distance(P, S) < Dist) return true;
		return false;
	};
	int32 Lawn = 0;
	for (int32 Try = 0; Try < 600 && Lawn < 8; ++Try)
	{
		const FVector2D P(Rand.FRandRange(-150.f, 150.f), Rand.FRandRange(-100.f, 130.f));
		if (Inside(P, 5.f) || OnRoad(P, 3.f) || InPlaza(P, 2.f) || NearSpawn(P, 4.f)) continue;
		if (NearPerimeter(P, 3.f) || IsReserved(P)) continue;
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
		else AddProp(TEXT("ParkBench"), WorldT, P.X, P.Y, 0.f, Y);
		++Lawn;
	}

	// The back yard between the wings, behind the back door of Lysgaarden: fixed places, cover on the south route.
	AddModel(TEXT("SM_Car_Van"), FTransform(FRotator(0.f, 20.f, 0.f), FVector(-14.f, 40.f, 0.f) * CmPerM), TEXT("CarPaint_White"), true);
	AddModel(TEXT("SM_Car_Estate"), FTransform(FRotator(0.f, 195.f, 0.f), FVector(16.f, 33.f, 0.f) * CmPerM), TEXT("CarPaint_Grey"), true);
	for (const float X : { -30.f, 30.f })
	{
		AddProp(TEXT("BikeRack"), WorldT, X, 20.f, 0.f, 0.f);
		AddProp(TEXT("BikeRack"), WorldT, X, 24.f, 0.f, 0.f);
	}
	AddProp(TEXT("Dumpster"), WorldT, 36.f, 50.f, 0.f, 90.f);
	AddProp(TEXT("Dumpster"), WorldT, 38.5f, 50.f, 0.f, 90.f);
	AddProp(TEXT("Pallet"), WorldT, -36.f, 48.f, 0.f, 0.f);
	AddProp(TEXT("Crates"), WorldT, -34.f, 49.5f, 0.f, 0.f);
	AddProp(TEXT("PicnicTable"), WorldT, -8.f, 28.f, 0.f, 0.f);
	AddProp(TEXT("PicnicTable"), WorldT, 9.f, 46.f, 0.f, 0.f);
	AddProp(TEXT("Planter"), WorldT, -6.f, 18.f, 0.f, 0.f);
	AddProp(TEXT("Planter"), WorldT, 6.f, 18.f, 0.f, 0.f);
	AddProp(TEXT("ParkBench"), WorldT, 0.f, 55.f, 0.f, -90.f);
}
