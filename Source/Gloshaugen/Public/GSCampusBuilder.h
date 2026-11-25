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

	UInstancedStaticMeshComponent* GetISM(class UStaticMesh* Mesh, const FLinearColor& Color, bool bCollide);

	UPROPERTY() TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> ISMs;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> StreetLights;
	bool bLightsOn = true;

	TArray<FBox2D> Footprints; // building areas (m), used to keep trees out of buildings
};
