#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GSMapState.h"
#include "GSBreakable.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

/**
 * Interaktiv verden 1b: breakable glass, glass bits and shards, lasting bullet holes.
 *
 * Glass panes are instances in one UGSGlassSet per glass type and source (map, test room), local on every machine and
 * built from the same data with the same keys; AGSMapState's replicated list says which are cracked or gone. A pane that
 * is gone is scaled to 0 (the engine removes its body, InstancedMeshComponentBodies.cpp) and never removed, so instance
 * indices never move. Every instance update here passes bMarkRenderStateDirty = false (no proxy rebuild per pane).
 *
 * Tempered glass (walls, glass doors) crumbles at once; laminated glass (railings) cracks where it is hit and gives way
 * at 100 damage. Bits in the air (AGSDebris), shards on the floor (UGSInstancePool "litter", max 3000) and bullet holes
 * (UGSInstancePool "marks", max 400 for the whole machine) are local and cosmetic.
 */

/** Visual kinds in the marks pool (bullet holes) and the litter pool (glass on the floor). */
namespace GSMarkVis
{
	constexpr int32 Core = 0;        // dark hole
	constexpr int32 Crater = 1;      // plaster / concrete
	constexpr int32 Splinter = 2;    // wood
	constexpr int32 MetalRing = 3;   // metal
	constexpr int32 GlassStreak = 4; // crack streaks in laminated glass
}
namespace GSLitterVis
{
	constexpr int32 Granules = 0;
	constexpr int32 Clump = 1;
	constexpr int32 ShardA = 2;      // .. ShardA + 3
}

/** One kind of breakable glass from one source. Collision before register; instances never removed. */
UCLASS(NotBlueprintable)
class GLOSHAUGEN_API UGSGlassSet : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	/** Before RegisterComponent: mesh, material, collision (Destructible), tags. */
	void Setup(EGSGlass InType);
	/** Adds a pane: centre, yaw, size (cm: length, thickness, height). Returns the instance index. */
	int32 AddPane(int32 Key, const FVector& CenterCm, float YawDeg, const FVector& SizeCm);
	int32 InstanceOfKey(int32 Key) const;
	int32 KeyOfInstance(int32 Index) const;
	void SetPaneShown(int32 Key, bool bShown);
	bool IsPaneShown(int32 Key) const;
	/** The whole pane (scale = size in cm / 100 of the unit cube). */
	bool GetPaneTransform(int32 Key, FTransform& Out) const;

	EGSGlass Type = EGSGlass::Wall;

protected:
	TArray<int32> Keys;
	TMap<int32, int32> IndexOf;
	TArray<FTransform> Whole;
	TBitArray<> Shown;
	FVector MeshScaleFix = FVector(1.f);
};

/**
 * A ring buffer of small instanced things (bullet holes, shards). Each entry owns 1-8 instances in one or more ISMs
 * (one ISM per visual and parent; parent = a moving component like a door leaf, or the world). When full, the oldest
 * entry gives its instances to the new one. Removed instances are scaled to 0 and reused.
 */
UCLASS(NotBlueprintable)
class GLOSHAUGEN_API UGSInstancePool : public USceneComponent
{
	GENERATED_BODY()

public:
	struct FPiece
	{
		int32 Visual = 0;
		FTransform World;
	};

	void Configure(int32 InCap, int32 InPerKeyCap, float InCullCm);
	/** Visual N = the Nth call. Mesh or material may be null (cube / flat colour). Scale is relative to a 100 cm cube. */
	int32 AddVisual(UStaticMesh* Mesh, UMaterialInterface* Mat, const FLinearColor& Fallback);
	/** A new entry. Parent: the component it sits on when that moves (door leaf), else null. Key: 0 or a part key. */
	int32 Add(const TArray<FPiece>& Pieces, int32 Key, USceneComponent* Parent);
	void RemoveKey(int32 Key);
	void ClearAll();
	int32 NumLive() const { return LiveCount; }
	int32 NumInstancesShown() const { return ShownInstances; }
	int32 CountKey(int32 Key) const { return KeyCount.FindRef(Key); }
	int32 GetCap() const { return Cap; }

protected:
	struct FVisual
	{
		TWeakObjectPtr<UStaticMesh> Mesh;
		TWeakObjectPtr<UMaterialInterface> Mat;
		FLinearColor Color = FLinearColor::White;
		FVector ScaleFix = FVector(1.f);   // a model's size vs the unit cube
	};
	struct FInst
	{
		TWeakObjectPtr<UInstancedStaticMeshComponent> ISM;
		int32 Index = INDEX_NONE;
	};
	struct FEntry
	{
		TArray<FInst> Insts;
		int32 Key = 0;
		uint32 Serial = 0;
		bool bLive = false;
	};

	UInstancedStaticMeshComponent* GetISM(int32 Visual, USceneComponent* Parent);
	void Evict(int32 Slot);
	void HideInst(const FInst& I);

	int32 Cap = 400;
	int32 PerKeyCap = 12;
	float CullCm = 3000.f;
	TArray<FVisual> Visuals;
	TArray<FEntry> Entries;
	TArray<int32> FreeSlots;
	/** Oldest first: (slot, serial). A copy whose serial no longer matches is stale (the slot was reused). */
	TArray<TPair<int32, uint32>> Fifo;
	int32 Head = 0;
	uint32 NextSerial = 1;
	int32 LiveCount = 0;
	int32 ShownInstances = 0;
	TMap<int32, int32> KeyCount;
	TMap<TPair<TWeakObjectPtr<USceneComponent>, int32>, TWeakObjectPtr<UInstancedStaticMeshComponent>> ISMByParent;
	TMap<TWeakObjectPtr<UInstancedStaticMeshComponent>, TArray<int32>> FreeInsts;
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> AllISMs;
};

/** Glass bits flying from one break (local). After 3.5 s every bit becomes a shard in the litter pool. */
UCLASS(NotPlaceable)
class GLOSHAUGEN_API AGSDebris : public AActor
{
	GENERATED_BODY()

public:
	AGSDebris();
	/** Spawns the bits over the pane (centre, rotation, size in cm) flying along HitDir. */
	void Init(int32 InKey, const FTransform& Pane, const FVector& SizeCm, bool bTempered, const FVector& HitDir);
	/** Turns the oldest N bits into shards now (the machine-wide cap). Returns how many. */
	int32 SettleSome(int32 N);
	int32 NumLive() const;

protected:
	struct FBit
	{
		TObjectPtr<UStaticMeshComponent> Comp;
		int32 Visual = 0;
	};
	void Check();
	void SettleBit(FBit& B);

	TArray<FBit> Bits;
	int32 Key = 0;
	float Born = 0.f;
	FTimerHandle CheckTimer;
};

namespace GSBreakable
{
	/** Server: damage a glass pane (or a glass door's pane). Cause: GSCause (Bullet, Melee, Blast; Use = console).
	 *  Bullet: Amount = bullet damage. Blast: Amount = Strength x (1 - d/R2). Melee: fixed. */
	GLOSHAUGEN_API void DamageGlass(UWorld* World, int32 Key, float Amount, uint8 Cause, const FVector& Dir);
	/** Server: a hit on a Destructible component (glass set instance or a door's pane) -> DamageGlass. False if not glass. */
	GLOSHAUGEN_API bool DamageGlassHit(UWorld* World, const FHitResult& Hit, float Amount, uint8 Cause, const FVector& Dir);
	/** The part key of a glass hit (glass set instance or door pane), 0 if none. */
	GLOSHAUGEN_API int32 GlassKeyOfHit(const FHitResult& Hit);
	/** Local bullet hole; bForce skips the 30 per second limit (tests). False when the surface takes no holes. */
	GLOSHAUGEN_API bool AddMark(UWorld* World, const FHitResult& Hit, bool bForce);
}
