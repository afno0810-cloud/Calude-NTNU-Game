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

	void Build();

	/** Street lights, switched on at night by the environment. */
	void SetStreetLightsOn(bool bOn);

	/** The hand-made Hovedbygningen model (main block, Lysgaarden and tower), or null if not imported yet. */
	static class UStaticMesh* MainBuildingMesh();

protected:
	/** Adds a box. Center in cm, size in METERS. */
	void AddBox(const FVector& CenterCm, const FRotator& Rot, const FVector& SizeM, const FLinearColor& Color, bool bCollide = true);
	void AddMesh(class UStaticMesh* Mesh, const FTransform& T, const FLinearColor& Color, bool bCollide);

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
	/** Outdoor campus props: benches, bike racks, bins, bus stops, signs, scooters, robots ... */
	void BuildCampusProps();

	/** Places SM_Prop_<Name> (from Tools/Blender/props.py). Local meters in building transform BT, yaw in degrees. Skipped if not imported. */
	void AddProp(const TCHAR* Name, const FTransform& BT, float X, float Y, float Z, float Yaw, bool bCollide = true, float Scale = 1.f);
	static class UStaticMesh* PropMesh(const TCHAR* Name);
	int32 NumProps = 0;

	UInstancedStaticMeshComponent* GetISM(class UStaticMesh* Mesh, const FLinearColor& Color, bool bCollide);

	UPROPERTY() TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> ISMs;
	UPROPERTY() TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> PropISMs; // keep the mesh's own materials
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> StreetLights;
	bool bLightsOn = true;

	TArray<FBox2D> Footprints; // building areas (m), used to keep trees out of buildings
};
