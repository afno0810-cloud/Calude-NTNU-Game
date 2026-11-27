#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSCampus.h"
#include "GSCampusBuilder.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;

/**
 * Builds the whole Gloshaugen blockout at runtime from GSCampus data.
 * Spawned locally on every machine (server and clients), so nothing here is replicated:
 * everyone builds the exact same geometry.
 */
UCLASS(NotPlaceable)
class GLOSHAUGEN_API AGSCampusBuilder : public AActor
{
	GENERATED_BODY()

public:
	AGSCampusBuilder();

	/** Lays out the whole map. Virtual: AGSHelgasetrBuilder builds Helgasetr instead (GSCampus::bHelgasetr). */
	virtual void Build();

	/** Street lights, switched on at night by the environment. */
	virtual void SetStreetLightsOn(bool bOn);

	/** The hand-made Hovedbygningen model (main block and Lysgaarden), or null if not imported yet. */
	static class UStaticMesh* MainBuildingMesh();

protected:
	/** Adds a box. Center in cm, size in METERS. */
	void AddBox(const FVector& CenterCm, const FRotator& Rot, const FVector& SizeM, const FLinearColor& Color, bool bCollide = true);
	void AddMesh(class UStaticMesh* Mesh, const FTransform& T, const FLinearColor& Color, bool bCollide, bool bShadow = true);
	/** Box with one of our PBR materials (MI_GS_<MatName>, made by Content/Python/gs_materials.py); flat colour if it is missing. */
	void AddBoxMat(const FVector& CenterCm, const FRotator& Rot, const FVector& SizeM, const TCHAR* MatName, const FLinearColor& Fallback, bool bCollide);
	static class UMaterialInterface* GSMaterial(const TCHAR* Name);
	/** "IT-bygget" -> "IT_bygget": the asset name used by Tools/Blender/buildings.py. */
	static FString SafeName(const FString& Name);
	void BuildSidewalks();
	/** Paved squares: the forecourt in front of the main entrance and the courtyard between the wings. */
	void BuildPlazas();
	/** Low concrete kerbs along both edges of every walking path. */
	void BuildPathEdges();
	/** Closes the play area: site fences all the way round PlayArea(), closed roads, invisible walls on the edges,
	 *  containers, a fenced rig area behind each base, and the dig sites and parked cars that break the long sight lines. */
	void BuildPerimeter();
	/** Site fence panels (3.5 m each, squeezed a little to fit) from A to B; the front faces (D.Y, -D.X) for the direction D = A -> B.
	 *  bMixed: see-through panels where a road crosses and on every fourth panel, a warning sign on every eighth;
	 *  otherwise every panel has a tarpaulin. Returns the number of panels. */
	int32 FenceRun(const FVector2D& A, const FVector2D& B, bool bMixed);
	/** True when P (m) is closer than Margin to any site fence set by FenceRun. */
	bool NearPerimeter(const FVector2D& P, float Margin) const;
	/** True inside one of the Reserved circles (dig sites, parked cars, road closures). */
	bool IsReserved(const FVector2D& P, float Extra = 0.f) const;
	TArray<TPair<FVector2D, FVector2D>> FenceSegs;   // every fence run (m)
	TArray<FVector> Reserved;                        // X, Y (m) and radius (m): nothing random is placed here
	/** Invisible walls exactly on the edges of PlayArea(): the fence is 2.0 m and players climb 2.2 m. */
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> PerimeterBlock;

	void BuildGround();
	void BuildRoads();
	void BuildBuilding(const GSCampus::FBuilding& B);
	void BuildWallSide(const FTransform& BuildingT, float CenterX, float CenterY, float Length, bool bAlongX,
		float Height, float FloorHeight, int32 Floors, bool bDoors, const FLinearColor& Color);
	void BuildTreesAndProps();
	void BuildStreetLights();

	/** Furniture: classrooms, offices, labs, cafeterias, libraries ... in every enterable building. */
	void FurnishBuilding(const GSCampus::FBuilding& B);
	void FurnishMainBuilding(const GSCampus::FBuilding& B);
	/** Fills one room (local building meters, floor height Z) with a random room type. PadCorridor keeps the -Y or +Y side free for doors. */
	void FurnishRoom(const FTransform& BT, float X0, float X1, float Y0, float Y1, float Z, FRandomStream& Rand, const TArray<FVector>& Keep);
	/** Outdoor campus props: benches, bike racks, bins, bus stops, signs, scooters ... and the furnished back yard. */
	void BuildCampusProps();

	/** Places SM_Prop_<Name> (from Tools/Blender/props.py). Local meters in building transform BT, yaw in degrees. Skipped if not imported. */
	void AddProp(const TCHAR* Name, const FTransform& BT, float X, float Y, float Z, float Yaw, bool bCollide = true, float Scale = 1.f);
	/** The same with a ready world transform (cm), so a prop can be stretched along one axis. */
	void AddPropWorld(const TCHAR* Name, const FTransform& WorldT, bool bCollide);
	static class UStaticMesh* PropMesh(const TCHAR* Name);
	/** Any model in /Game/Models by asset name (trees, cars ...); null if not imported. */
	static class UStaticMesh* ModelMesh(const TCHAR* Name);
	/** Instanced model in world space. PaintMat (MI_GS_<PaintMat>) replaces the "CarPaint" slot. */
	void AddModel(const TCHAR* Name, const FTransform& WorldT, const TCHAR* PaintMat, bool bCollide);
	/** Invisible cylinder so tree trunks block players and bullets while leaves do not. */
	void AddTrunkCollider(const FVector2D& P, float Radius, float Height);
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> TrunkColliders;
	int32 NumProps = 0;

	UInstancedStaticMeshComponent* GetISM(class UStaticMesh* Mesh, const FLinearColor& Color, bool bCollide, bool bShadow);

	/** Instances are collected per group while the campus is laid out and added in one go at the end of Build():
	 *  one AddInstances per group is far cheaper than tens of thousands of AddInstance calls. Nothing in Build()
	 *  may read instances back before FlushPending(). */
	void Queue(UInstancedStaticMeshComponent* ISM, const FTransform& WorldT);
	void FlushPending();
	TMap<UInstancedStaticMeshComponent*, TArray<FTransform>> Pending;

	UPROPERTY() TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> ISMs;
	UPROPERTY() TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> PropISMs; // keep the mesh's own materials
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> StreetLights;
	bool bLightsOn = true;

	TArray<FBox2D> Footprints; // building areas (m), used to keep trees out of buildings
};
