#pragma once

#include "CoreMinimal.h"
#include "GSCampusBuilder.h"
#include "GSHelgasetrBuilder.generated.h"

class UInstancedStaticMeshComponent;
class ULocalLightComponent;

/**
 * Builds Helgasetr at runtime from GSHelgasetr.h (generated from Tools/Blender/helgasetr_plan.py), on every machine,
 * nothing replicated. Round 4a: the whole building as boxes (walls with door holes, slabs, one box per step, rails,
 * glass boxes, saw-tooth glass roof with truss), so the map is playable without importing anything.
 * Round 4b: the Blender model (Tools/Blender/building_helgasetr.py) replaces the boxes part by part where it exists.
 *
 * Interactive pieces are NOT part of the structure: breakable glass panels, breach walls (lettvegger) and cover each
 * live in their own instanced components, instance N = id N, with an id -> (component, index) table
 * (FindGlassPanel / FindBreachWall / FindCover). Instances are never removed (round 5 scales them to 0).
 * Tags: glass "Mat_Glass" + "GS_Int_Glass", breach walls "Mat_Plaster" + "GS_Int_Breach", cover "GS_Int_Cover".
 */
UCLASS(NotPlaceable)
class GLOSHAUGEN_API AGSHelgasetrBuilder : public AGSCampusBuilder
{
	GENERATED_BODY()

public:
	virtual void Build() override;
	/** No street lights indoors; the indoor lights stay on day and night (the building is open until 23). */
	virtual void SetStreetLightsOn(bool bOn) override {}

	bool FindGlassPanel(int32 Id, UInstancedStaticMeshComponent*& OutComp, int32& OutIndex) const;
	bool FindBreachWall(int32 Id, UInstancedStaticMeshComponent*& OutComp, int32& OutIndex) const;
	bool FindCover(int32 Id, UInstancedStaticMeshComponent*& OutComp, int32& OutIndex) const;

protected:
	/** Round 4b: true when the Blender model part (GSHelgasetr::ModelParts()) is in /Game/Models and placed, so its
	 *  boxes are skipped. Blocks are the fallback per part (K13): a part that is missing never leaves a hole. */
	bool UseModelPart(int32 Part) const { return ModelPartUsed.IsValidIndex(Part) && ModelPartUsed[Part]; }

	void BuildGroundAndSky();
	/** Places every model part that has been imported (before BuildStructure, which then skips their boxes). */
	void BuildModelParts();
	void BuildStructure();
	void BuildGlassPanels();
	void BuildPieces();
	void BuildLights();
	void BuildSigns();
	/** Runs the exact flag and spawn traces of AGSGameMode on the built map and logs the result (no game change). */
	void ProbeGameModeTraces() const;

	/** Instanced cube for one material and collision kind. bShadow false for glass, emissive and flat things. */
	UInstancedStaticMeshComponent* StructISM(uint8 Mat, uint8 Coll, bool bShadow);
	UInstancedStaticMeshComponent* NewCubeISM(const TCHAR* MatName, uint8 Coll, bool bShadow);
	/** Queues one box (m); straight boxes longer than 4 m are cut in tiles so the textures keep their scale. */
	void QueueBoxM(UInstancedStaticMeshComponent* ISM, const FVector& CentreM, const FVector& SizeM, const FRotator& Rot, bool bTile);
	static void SetupCollision(UPrimitiveComponent* C, uint8 Coll);

	UPROPERTY() TMap<int32, TObjectPtr<UInstancedStaticMeshComponent>> StructISMs;
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> PieceISMs;   // glass, breach walls, cover
	UPROPERTY() TArray<TObjectPtr<ULocalLightComponent>> IndoorLights;

	TMap<int32, TPair<UInstancedStaticMeshComponent*, int32>> GlassById;
	TMap<int32, TPair<UInstancedStaticMeshComponent*, int32>> BreachById;
	TMap<int32, TPair<UInstancedStaticMeshComponent*, int32>> CoverById;
	/** Instances queued per component so far (= the index the next one gets after FlushPending). */
	TMap<UInstancedStaticMeshComponent*, int32> QueuedCount;

	int32 NumBoxes = 0;
	TArray<bool> ModelPartUsed;
};
