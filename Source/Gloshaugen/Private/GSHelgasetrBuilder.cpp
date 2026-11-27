#include "GSHelgasetrBuilder.h"
#include "GSHelgasetr.h"
#include "GSCampus.h"
#include "GSUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Materials/MaterialInterface.h"

namespace GSHelgBuild
{
	constexpr float Cm = 100.f;
	constexpr float TileM = 4.f;   // box faces carry one texture repeat; cut long boxes so a wall or floor keeps its scale

	/** Flat colour when an MI_GS_ material is missing. Same order as MATS in Tools/Blender/helgasetr_plan.py. */
	const FLinearColor Fallback[] = {
		FLinearColor(0.85f, 0.85f, 0.83f), FLinearColor(0.60f, 0.60f, 0.58f), FLinearColor(0.62f, 0.48f, 0.32f),
		FLinearColor(0.35f, 0.25f, 0.18f), FLinearColor(0.75f, 0.74f, 0.70f), FLinearColor(0.90f, 0.45f, 0.10f),
		FLinearColor(0.95f, 0.75f, 0.10f), FLinearColor(0.30f, 0.20f, 0.12f), FLinearColor(0.08f, 0.08f, 0.09f),
		FLinearColor(0.60f, 0.75f, 0.80f), FLinearColor(0.10f, 0.10f, 0.11f), FLinearColor(1.00f, 1.00f, 0.95f),
		FLinearColor(0.70f, 0.05f, 0.05f), FLinearColor(0.05f, 0.15f, 0.70f), FLinearColor(0.10f, 0.35f, 0.20f),
		FLinearColor(0.20f, 0.21f, 0.23f), FLinearColor(0.15f, 0.45f, 0.42f), FLinearColor(0.50f, 0.50f, 0.48f),
	};

	int32 MatIndex(const TCHAR* Name)
	{
		for (int32 I = 0; I < UE_ARRAY_COUNT(GSHelgasetr::MatNames); ++I)
		{
			if (FCString::Strcmp(GSHelgasetr::MatNames[I], Name) == 0) return I;
		}
		return 0;
	}

	bool IsGlass(const TCHAR* Name) { return FCString::Strcmp(Name, TEXT("Glass")) == 0; }
	bool IsEmissive(const TCHAR* Name) { return FCString::Strcmp(Name, TEXT("LightPanel_In")) == 0; }
}

void AGSHelgasetrBuilder::SetupCollision(UPrimitiveComponent* C, uint8 Coll)
{
	switch (Coll)
	{
	case GSHelgasetr::Coll_Solid:
		C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		break;
	case GSHelgasetr::Coll_High:
		// Over Plan 4 in the atrium: stops bullets, the camera and the rain check (Visibility), but nobody can stand
		// on it and the grapple hook (ECC_WorldStatic channel) passes (K10).
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		C->SetCollisionObjectType(ECC_WorldStatic);
		C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		C->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
		break;
	case GSHelgasetr::Coll_Blocker:
		C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		C->SetCollisionObjectType(ECC_WorldStatic);
		C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		C->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		C->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
		break;
	default:
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	}
}

UInstancedStaticMeshComponent* AGSHelgasetrBuilder::NewCubeISM(const TCHAR* MatName, uint8 Coll, bool bShadow)
{
	UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this);
	ISM->SetMobility(EComponentMobility::Static);
	ISM->SetStaticMesh(GSUtils::Cube());
	ISM->SetupAttachment(RootComponent);
	SetupCollision(ISM, Coll);
	const bool bGlass = GSHelgBuild::IsGlass(MatName);
	if (!bShadow || bGlass) ISM->SetCastShadow(false);
	if (bGlass)
	{
		// One cheap glass: no shadow, no distance field, not in ray tracing (K19).
		ISM->bAffectDistanceFieldLighting = false;
		ISM->bVisibleInRayTracing = false;
		ISM->ComponentTags.Add(TEXT("Mat_Glass"));
	}
	if (Coll == GSHelgasetr::Coll_Blocker)
	{
		ISM->SetVisibility(false);
		ISM->SetHiddenInGame(true);
		ISM->SetCastShadow(false);
		ISM->bAffectDistanceFieldLighting = false;
	}
	ISM->RegisterComponent();
	if (UMaterialInterface* M = GSMaterial(MatName))
	{
		ISM->SetMaterial(0, M);
	}
	else
	{
		const int32 Index = GSHelgBuild::MatIndex(MatName);
		GSUtils::SetColor(ISM, GSHelgBuild::Fallback[FMath::Clamp(Index, 0, int32(UE_ARRAY_COUNT(GSHelgBuild::Fallback)) - 1)]);
	}
	AddInstanceComponent(ISM);
	return ISM;
}

UInstancedStaticMeshComponent* AGSHelgasetrBuilder::StructISM(uint8 Mat, uint8 Coll, bool bShadow)
{
	const int32 Key = Mat * 16 + Coll * 2 + (bShadow ? 1 : 0);
	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = StructISMs.Find(Key)) return *Found;
	const int32 NumMats = UE_ARRAY_COUNT(GSHelgasetr::MatNames);
	UInstancedStaticMeshComponent* ISM = NewCubeISM(GSHelgasetr::MatNames[FMath::Clamp(int32(Mat), 0, NumMats - 1)], Coll, bShadow);
	StructISMs.Add(Key, ISM);
	return ISM;
}

void AGSHelgasetrBuilder::QueueBoxM(UInstancedStaticMeshComponent* ISM, const FVector& CentreM, const FVector& SizeM, const FRotator& Rot, bool bTile)
{
	if (!ISM || SizeM.X <= 0.002f || SizeM.Y <= 0.002f || SizeM.Z <= 0.002f) return;
	const int32 NX = (bTile && Rot.IsNearlyZero()) ? FMath::Max(1, FMath::CeilToInt(SizeM.X / GSHelgBuild::TileM - 0.01f)) : 1;
	const int32 NY = (bTile && Rot.IsNearlyZero()) ? FMath::Max(1, FMath::CeilToInt(SizeM.Y / GSHelgBuild::TileM - 0.01f)) : 1;
	const FVector Tile(SizeM.X / NX, SizeM.Y / NY, SizeM.Z);
	for (int32 I = 0; I < NX; ++I)
	{
		for (int32 J = 0; J < NY; ++J)
		{
			const FVector C(CentreM.X - SizeM.X * 0.5f + Tile.X * (I + 0.5f), CentreM.Y - SizeM.Y * 0.5f + Tile.Y * (J + 0.5f), CentreM.Z);
			Queue(ISM, FTransform(Rot, C * GSHelgBuild::Cm, Tile));
			QueuedCount.FindOrAdd(ISM)++;
			++NumBoxes;
		}
	}
}

void AGSHelgasetrBuilder::Build()
{
	const double StartTime = FPlatformTime::Seconds();
	BuildGroundAndSky();
	BuildStructure();
	BuildGlassPanels();
	BuildPieces();
	if (GetNetMode() != NM_DedicatedServer)
	{
		BuildLights();
		BuildSigns();
	}
	// Everything above only queued its instances: add them now, one call per component (indices = queue order).
	FlushPending();

	UE_LOG(LogTemp, Log, TEXT("Helgasetr: bygget %d klosser i %d grupper, %d glassfelt, %d lettvegger, %d dekning, %d døråpninger, %d lys (%.2f s)"),
		NumBoxes, StructISMs.Num() + PieceISMs.Num(), GlassById.Num(), BreachById.Num(), CoverById.Num(),
		GSHelgasetr::DoorSlots().Num(), IndoorLights.Num(), FPlatformTime::Seconds() - StartTime);
	ProbeGameModeTraces();

	// Once, a few seconds in: where the bodies actually stand (helps to see a bad spawn in the log).
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		for (TActorIterator<APawn> It(GetWorld()); It; ++It)
		{
			const FVector L = It->GetActorLocation() / 100.f;
			const TCHAR* Zone = GSHelgasetr::ZoneAt(FVector2D(L.X, L.Y));
			UE_LOG(LogTemp, Log, TEXT("Helgasetr: %s står i (%.1f, %.1f, %.1f) m, %s, %s"), *It->GetName(), L.X, L.Y, L.Z,
				*GSHelgasetr::LevelName(GSHelgasetr::LevelAt(L.Z - 0.9f)), Zone ? Zone : TEXT("ute"));
		}
	}), 3.f, false);
}

void AGSHelgasetrBuilder::BuildGroundAndSky()
{
	// Asphalt round the building (top at -5 cm, the Plan 1 floor is at 0). Seen through the windows and the glass roof.
	QueueBoxM(StructISM(GSHelgBuild::MatIndex(TEXT("W_Asphalt")), GSHelgasetr::Coll_Solid, false),
		FVector(0.f, 0.f, -0.55f), FVector(400.f, 400.f, 1.f), FRotator::ZeroRotator, false);
	// Horizon (town, hills and the fjord far out), as on the old campus.
	if (UStaticMesh* Backdrop = ModelMesh(TEXT("SM_Backdrop")))
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetMobility(EComponentMobility::Static);
		C->SetStaticMesh(Backdrop);
		C->SetupAttachment(RootComponent);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->bAffectDistanceFieldLighting = false;
		C->RegisterComponent();
		AddInstanceComponent(C);
	}
}

void AGSHelgasetrBuilder::BuildStructure()
{
	for (const GSHelgasetr::FBoxM& B : GSHelgasetr::Boxes())
	{
		if (UseModelFor(B.Bit)) continue;
		const TCHAR* MatName = GSHelgasetr::MatNames[B.Mat];
		const bool bGlass = GSHelgBuild::IsGlass(MatName);
		const FRotator Rot(B.Pitch, B.Yaw, B.Roll);
		// Flat things on the ground floor and lights cannot shade anything worth a shadow.
		const bool bLow = B.Cz + B.Sz * 0.5f < 0.3f && Rot.IsNearlyZero();
		const bool bShadow = !bGlass && !bLow && !GSHelgBuild::IsEmissive(MatName) && B.Coll != GSHelgasetr::Coll_None
			&& B.Coll != GSHelgasetr::Coll_Blocker;
		QueueBoxM(StructISM(B.Mat, B.Coll, bShadow), FVector(B.Cx, B.Cy, B.Cz), FVector(B.Sx, B.Sy, B.Sz), Rot, !bGlass);
	}
}

void AGSHelgasetrBuilder::BuildGlassPanels()
{
	// One component per panel type (rekkverk / vegg), never shared with anything else (K14): instance N of the
	// list goes in at its queue position, and GlassById keeps id -> (component, index).
	UInstancedStaticMeshComponent* PerType[2] = { nullptr, nullptr };
	for (const GSHelgasetr::FPanelM& P : GSHelgasetr::GlassPanels())
	{
		const int32 Type = FMath::Clamp(int32(P.Type), 0, 1);
		if (!PerType[Type])
		{
			UInstancedStaticMeshComponent* ISM = NewCubeISM(TEXT("Glass"), GSHelgasetr::Coll_None, false);
			// Blocks bodies and things, lets bullets and the camera through until it can break (round 5).
			ISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ISM->SetCollisionObjectType(ECC_WorldStatic);
			ISM->SetCollisionResponseToAllChannels(ECR_Ignore);
			ISM->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
			ISM->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
			ISM->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
			ISM->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
			ISM->ComponentTags.Add(TEXT("GS_Int_Glass"));
			ISM->ComponentTags.Add(Type == 0 ? FName(TEXT("Glass_Rekkverk")) : FName(TEXT("Glass_Vegg")));
			PerType[Type] = ISM;
			PieceISMs.Add(ISM);
		}
		UInstancedStaticMeshComponent* ISM = PerType[Type];
		const int32 Index = QueuedCount.FindRef(ISM);
		QueueBoxM(ISM, FVector(P.Cx, P.Cy, P.Cz), FVector(P.Len, P.Thick, P.Height), FRotator(0.f, P.Yaw, 0.f), false);
		GlassById.Add(P.Id, TPair<UInstancedStaticMeshComponent*, int32>(ISM, Index));
	}
}

void AGSHelgasetrBuilder::BuildPieces()
{
	// Breach walls (lettvegger): own component, instance index = id.
	UInstancedStaticMeshComponent* Breach = nullptr;
	for (const GSHelgasetr::FPieceM& P : GSHelgasetr::BreachWalls())
	{
		if (!Breach)
		{
			Breach = NewCubeISM(GSHelgasetr::MatNames[P.Mat], GSHelgasetr::Coll_Solid, true);
			Breach->ComponentTags.Add(TEXT("Mat_Plaster"));
			Breach->ComponentTags.Add(TEXT("GS_Int_Breach"));
			PieceISMs.Add(Breach);
		}
		const int32 Index = QueuedCount.FindRef(Breach);
		QueueBoxM(Breach, FVector((P.X0 + P.X1) * 0.5f, (P.Y0 + P.Y1) * 0.5f, (P.Z0 + P.Z1) * 0.5f),
			FVector(P.X1 - P.X0, P.Y1 - P.Y0, P.Z1 - P.Z0), FRotator::ZeroRotator, false);
		BreachById.Add(P.Id, TPair<UInstancedStaticMeshComponent*, int32>(Breach, Index));
	}
	// Cover (planters, cabinets, sofa groups, booths, desks): one component per material, id -> (component, index).
	TMap<uint8, UInstancedStaticMeshComponent*> PerMat;
	for (const GSHelgasetr::FPieceM& P : GSHelgasetr::Cover())
	{
		UInstancedStaticMeshComponent*& ISM = PerMat.FindOrAdd(P.Mat);
		if (!ISM)
		{
			ISM = NewCubeISM(GSHelgasetr::MatNames[P.Mat], GSHelgasetr::Coll_Solid, true);
			ISM->ComponentTags.Add(TEXT("GS_Int_Cover"));
			PieceISMs.Add(ISM);
		}
		const int32 Index = QueuedCount.FindRef(ISM);
		QueueBoxM(ISM, FVector((P.X0 + P.X1) * 0.5f, (P.Y0 + P.Y1) * 0.5f, (P.Z0 + P.Z1) * 0.5f),
			FVector(P.X1 - P.X0, P.Y1 - P.Y0, P.Z1 - P.Z0), FRotator::ZeroRotator, false);
		CoverById.Add(P.Id, TPair<UInstancedStaticMeshComponent*, int32>(ISM, Index));
	}
}

void AGSHelgasetrBuilder::BuildLights()
{
	using namespace GSHelgBuild;
	UInstancedStaticMeshComponent* Glow = StructISM(MatIndex(TEXT("LightPanel_In")), GSHelgasetr::Coll_None, false);
	UInstancedStaticMeshComponent* Wire = StructISM(MatIndex(TEXT("MetalDark_In")), GSHelgasetr::Coll_None, false);
	auto Ceiling = [](float ZM)
	{
		const int32 L = GSHelgasetr::LevelAt(ZM);
		return (L + 1 < GSHelgasetr::NumLevels ? GSHelgasetr::LevelZM[L + 1] : GSHelgasetr::RoofZM) - GSHelgasetr::SlabM;
	};
	TArray<FVector2D> PanelSpots;
	for (const GSHelgasetr::FLightM& L : GSHelgasetr::Lights())
	{
		const FVector P(L.X, L.Y, L.Z);
		if (L.Kind == 0)
		{
			// Big round LED ring (Ø 2.4 m) on three long wires from the roof truss, one soft rect light down.
			const float R = L.RadiusM;
			for (int32 K = 0; K < 12; ++K)
			{
				const float A = 2.f * PI * K / 12.f;
				QueueBoxM(Glow, P + FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * R, FVector(2.f * PI * R / 12.f * 1.04f, 0.12f, 0.07f),
					FRotator(0.f, FMath::RadiansToDegrees(A) + 90.f, 0.f), false);
			}
			for (int32 K = 0; K < 3; ++K)
			{
				const float A = 2.f * PI * K / 3.f + 0.5f;
				const float Top = GSHelgasetr::RoofZM - 0.45f;
				QueueBoxM(Wire, FVector(L.X + FMath::Cos(A) * R, L.Y + FMath::Sin(A) * R, (L.Z + Top) * 0.5f),
					FVector(0.015f, 0.015f, Top - L.Z), FRotator::ZeroRotator, false);
			}
			URectLightComponent* Rect = NewObject<URectLightComponent>(this);
			Rect->SetupAttachment(RootComponent);
			Rect->SetMobility(EComponentMobility::Movable);
			Rect->SetWorldLocationAndRotation((P - FVector(0.f, 0.f, 0.05f)) * Cm, FRotator(-90.f, 0.f, 0.f));
			Rect->SetSourceWidth(R * 180.f);
			Rect->SetSourceHeight(R * 180.f);
			Rect->SetIntensityUnits(ELightUnits::Lumens);
			Rect->SetIntensity(6000.f);
			Rect->SetAttenuationRadius(2600.f);
			Rect->SetUseTemperature(true);
			Rect->SetTemperature(4000.f);
			Rect->SetCastShadows(false);
			Rect->MaxDrawDistance = 4000.f;
			Rect->MaxDistanceFadeRange = 800.f;
			Rect->RegisterComponent();
			IndoorLights.Add(Rect);
		}
		else
		{
			// Corridor light: a short-range point light (<= 6 m, so it does not shine through walls into the next room
			// or the floor above, K20) under a round glowing panel in the ceiling.
			const float CeilZ = Ceiling(L.Z);
			QueueBoxM(Glow, FVector(L.X, L.Y, CeilZ - 0.02f), FVector(0.6f, 0.6f, 0.04f), FRotator::ZeroRotator, false);
			PanelSpots.Add(FVector2D(L.X, L.Y));
			UPointLightComponent* Point = NewObject<UPointLightComponent>(this);
			Point->SetupAttachment(RootComponent);
			Point->SetMobility(EComponentMobility::Movable);
			Point->SetWorldLocation(P * Cm);
			Point->SetIntensityUnits(ELightUnits::Lumens);
			Point->SetIntensity(1800.f);
			Point->SetAttenuationRadius(FMath::Min(L.RadiusM, 6.f) * Cm);
			Point->SetSourceRadius(25.f);
			Point->SetUseTemperature(true);
			Point->SetTemperature(3800.f);
			Point->SetCastShadows(false);
			Point->MaxDrawDistance = 4000.f;
			Point->MaxDistanceFadeRange = 800.f;
			Point->RegisterComponent();
			IndoorLights.Add(Point);
		}
	}
	// Round downlights every 3 m in the ring corridor ceilings of Plan 2-4 (emissive only: Lumen spreads the glow).
	for (int32 Level = 1; Level <= 3; ++Level)
	{
		const float CeilZ = GSHelgasetr::LevelZM[Level + 1] - GSHelgasetr::SlabM - 0.015f;
		for (const GSHelgasetr::FSegM& S : GSHelgasetr::Corridors())
		{
			const FVector2D A(S.AX, S.AY), B(S.BX, S.BY);
			const float Len = FVector2D::Distance(A, B);
			for (float D = 1.5f; D < Len; D += 3.f)
			{
				const FVector2D Q = A + (B - A) * (D / Len);
				if (Level == 1 && FMath::Abs(Q.Y) < 17.f && Q.X < 0.f) continue;   // Plan 2 west: over the stair hole
				bool bNearPanel = false;
				for (const FVector2D& Spot : PanelSpots) bNearPanel |= FVector2D::Distance(Spot, Q) < 1.f;
				if (bNearPanel) continue;
				QueueBoxM(Glow, FVector(Q.X, Q.Y, CeilZ), FVector(0.25f, 0.25f, 0.03f), FRotator::ZeroRotator, false);
			}
		}
	}
}

void AGSHelgasetrBuilder::BuildSigns()
{
	// "SØR" / "NORD" in team colour on the corridor wall between the two doors of each base.
	for (const GSHelgasetr::FSignM& S : GSHelgasetr::Signs())
	{
		UTextRenderComponent* T = NewObject<UTextRenderComponent>(this);
		T->SetupAttachment(RootComponent);
		T->SetMobility(EComponentMobility::Static);
		T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		T->SetText(FText::FromString(S.Text));
		T->SetTextRenderColor(S.Team == 0 ? FColor(200, 20, 20) : FColor(20, 60, 210));
		T->SetWorldSize(55.f);
		T->SetHorizontalAlignment(EHTA_Center);
		T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetCastShadow(false);
		T->SetWorldLocationAndRotation(FVector(S.X, S.Y, S.Z) * GSHelgBuild::Cm, FRotator(0.f, S.Yaw, 0.f));
		T->RegisterComponent();
		AddInstanceComponent(T);
	}
}

void AGSHelgasetrBuilder::ProbeGameModeTraces() const
{
	// Exactly the traces of AGSGameMode::SpawnFlags (object type WorldStatic from Flag +-3 m) and PickSpawnTransform
	// (channel WorldStatic from +4 m to -3 m, then a 42/92 capsule 1 m over the hit), on the real map data.
	UWorld* World = GetWorld();
	if (!World) return;
	const TArray<GSCampus::FBase>& Bases = GSCampus::Bases();
	int32 FlagsOK = 0, SpawnsOK = 0, SpawnsAll = 0;
	// Bodies are not the map: ignore any pawn that is already there.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSHelgasetrProbe), false);
	for (TActorIterator<APawn> It(World); It; ++It) Params.AddIgnoredActor(*It);
	TArray<FString> Bad;
	for (int32 I = 0; I < Bases.Num(); ++I)
	{
		const GSCampus::FBase& B = Bases[I];
		const float FloorCm = (GSHelgasetr::Bases().IsValidIndex(I) ? GSHelgasetr::Bases()[I].Z : B.Flag.Z) * 100.f;
		const FVector Base = B.Flag * 100.f;
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Base + FVector(0.f, 0.f, 300.f), Base - FVector(0.f, 0.f, 300.f), FCollisionObjectQueryParams(ECC_WorldStatic), Params)
			&& FMath::Abs(Hit.ImpactPoint.Z - FloorCm) < 10.f)
		{
			++FlagsOK;
		}
		else
		{
			Bad.Add(FString::Printf(TEXT("flagg %s (treff %.0f cm, venter %.0f)"), B.Name, Hit.bBlockingHit ? Hit.ImpactPoint.Z : -1.f, FloorCm));
		}
		for (const FVector2D& S : B.Spawns)
		{
			++SpawnsAll;
			FVector Loc(S.X * 100.f, S.Y * 100.f, Base.Z);
			FHitResult SHit;
			if (!World->LineTraceSingleByChannel(SHit, Loc + FVector(0.f, 0.f, 400.f), Loc - FVector(0.f, 0.f, 300.f), ECC_WorldStatic, Params))
			{
				Bad.Add(FString::Printf(TEXT("spawn %s (%.1f, %.1f): sporet traff ingenting"), B.Name, S.X, S.Y));
				continue;
			}
			Loc.Z = SHit.ImpactPoint.Z + 100.f;
			if (FMath::Abs(SHit.ImpactPoint.Z - FloorCm) > 10.f)
			{
				Bad.Add(FString::Printf(TEXT("spawn %s (%.1f, %.1f): traff %.0f cm (%s / %s), ikke gulvet"), B.Name, S.X, S.Y, SHit.ImpactPoint.Z,
					*GetNameSafe(SHit.GetActor()), *GetNameSafe(SHit.GetComponent())));
				continue;
			}
			if (World->OverlapBlockingTestByChannel(Loc, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(42.f, 92.f), Params))
			{
				TArray<FOverlapResult> Over;
				World->OverlapMultiByChannel(Over, Loc, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(42.f, 92.f), Params);
				Bad.Add(FString::Printf(TEXT("spawn %s (%.1f, %.1f): kapselen sitter fast i %s"), B.Name, S.X, S.Y,
					Over.Num() > 0 ? *GetNameSafe(Over[0].GetComponent()) : TEXT("?")));
				continue;
			}
			++SpawnsOK;
		}
	}
	if (Bad.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Helgasetr: flagg %d/%d og spawn %d/%d står riktig"), FlagsOK, Bases.Num(), SpawnsOK, SpawnsAll);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Helgasetr: flagg %d/%d og spawn %d/%d står riktig. Feil: %s"), FlagsOK, Bases.Num(), SpawnsOK, SpawnsAll,
			*FString::Join(Bad, TEXT("; ")));
	}
}

bool AGSHelgasetrBuilder::FindGlassPanel(int32 Id, UInstancedStaticMeshComponent*& OutComp, int32& OutIndex) const
{
	if (const TPair<UInstancedStaticMeshComponent*, int32>* P = GlassById.Find(Id)) { OutComp = P->Key; OutIndex = P->Value; return true; }
	return false;
}

bool AGSHelgasetrBuilder::FindBreachWall(int32 Id, UInstancedStaticMeshComponent*& OutComp, int32& OutIndex) const
{
	if (const TPair<UInstancedStaticMeshComponent*, int32>* P = BreachById.Find(Id)) { OutComp = P->Key; OutIndex = P->Value; return true; }
	return false;
}

bool AGSHelgasetrBuilder::FindCover(int32 Id, UInstancedStaticMeshComponent*& OutComp, int32& OutIndex) const
{
	if (const TPair<UInstancedStaticMeshComponent*, int32>* P = CoverById.Find(Id)) { OutComp = P->Key; OutIndex = P->Value; return true; }
	return false;
}
