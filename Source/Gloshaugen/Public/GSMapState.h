#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Engine/NetSerialization.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Subsystems/WorldSubsystem.h"
#include "GSMapState.generated.h"

class AGSDoor;
class AGSPickup;
class AGSCharacter;
class APlayerController;
class AGSMapState;

/**
 * Interaktiv verden: one replicated state for everything in the map that can be opened, broken or moved
 * (doors now; glass in 1b, breach walls and props in round 2). Only parts that differ from their default are in the list,
 * so a late client gets the whole picture in one small packet. The doors themselves are local actors built the same way
 * on every machine from the map data (same key everywhere); the list only says how they stand.
 */

/** Part keys: category in the top 8 bits, id in the low 24. */
namespace GSPart
{
	constexpr int32 Glass = 1;
	constexpr int32 Door = 2;
	constexpr int32 Breach = 3;
	constexpr int32 Prop = 4;
	constexpr int32 Device = 5;
	/** Test room ids start here in every category (the map's own ids are far below). */
	constexpr int32 TestIdBase = 0xF00000;
	inline int32 MakeKey(int32 Cat, int32 Id) { return (Cat << 24) | (Id & 0xFFFFFF); }
	inline int32 CatOf(int32 Key) { return (Key >> 24) & 0xFF; }
	inline int32 IdOf(int32 Key) { return Key & 0xFFFFFF; }
}

/** Kind of breakable glass (1b). */
UENUM()
enum class EGSGlass : uint8 { Railing, Wall, DoorPane };

/** Kind of door. */
UENUM()
enum class EGSDoorArt : uint8 { Wood, GlassCode, Double, Fire };

/** Door lock, bits 0-1 of FGSPartState::State. */
namespace GSLock
{
	constexpr uint8 Open = 0;      // not locked
	constexpr uint8 Locked = 1;
	constexpr uint8 Broken = 2;    // forced, kicked in or shot: can never lock again this match
	constexpr uint8 Mask = 3;
	constexpr uint8 PaneBroken = 1 << 2;   // the glass in a glass door is gone (1b)
	constexpr uint8 LeafGone = 1 << 3;     // the leaf is blown out (round 2)
}

/** Door flags (FGSPartState::Flags). */
namespace GSDoorFlag
{
	constexpr uint8 Picking = 1 << 0;    // someone is picking the code lock (clicks for everyone near)
	constexpr uint8 Forcing = 1 << 1;    // someone is breaking the code lock open (loud)
}

/** Why a part last changed (FGSPartState::Cause): picks the sound and the speed of the swing. */
namespace GSCause
{
	constexpr uint8 Use = 0;
	constexpr uint8 Kick = 1;
	constexpr uint8 Blast = 2;
	constexpr uint8 Card = 3;
	constexpr uint8 Pick = 4;
	constexpr uint8 Force = 5;
	constexpr uint8 Bullet = 6;
	constexpr uint8 AutoClose = 7;   // fire door closing by itself
	constexpr uint8 Bot = 8;
	constexpr uint8 Escape = 9;      // opened from the inside of a locked door (handle / push bar)
	constexpr uint8 Melee = 10;
	constexpr uint8 Flags = 11;      // only Flags changed (picking started/stopped)
}

/** One door from the map data (or the test room). Centimetres. */
struct FGSDoorDef
{
	int32 Id = 0;
	FVector BaseCm = FVector::ZeroVector;   // floor point in the middle of the opening
	float WidthCm = 100.f;
	float HeightCm = 210.f;
	float YawDeg = 0.f;                     // the way the front (outside) faces
	EGSDoorArt Art = EGSDoorArt::Wood;
	bool bHingeLeft = true;                 // seen from the front
	bool bLocked = false;                   // locked at the start of a match
	bool bOpen = false;                     // standing open at the start of a match
	FString Room;                           // the room behind the door ("Grupperom 2.1"), for the hint
};

/** What the bots (and anyone else) may read about a door. */
struct FGSDoorInfo
{
	int32 Key = 0;
	FVector LocationCm = FVector::ZeroVector;   // floor point in the middle of the opening
	FVector Normal = FVector::ForwardVector;    // the front (outside)
	float WidthCm = 100.f;
	EGSDoorArt Art = EGSDoorArt::Wood;
	uint8 Lock = 0;                             // GSLock
	bool bOpen = false;
	AGSDoor* Door = nullptr;
};

struct FGSPartArray;

/** The state of one part that is not at its default. */
USTRUCT()
struct FGSPartState : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY() int32 Key = 0;
	/** Door: bits 0-1 lock (GSLock), bit 2 door glass broken (1b), bit 3 leaf blown out (round 2). Glass (1b): 0 whole, 1 cracked, 2 gone. */
	UPROPERTY() uint8 State = 0;
	/** Door: -100..100 = percent of 95 degrees; the sign is the side it swings to (+ = front). */
	UPROPERTY() int8 Open = 0;
	/** Door: kicks/blows taken. Glass (1b): damage. */
	UPROPERTY() uint8 Health = 0;
	/** Door: GSDoorFlag. */
	UPROPERTY() uint8 Flags = 0;
	/** GSCause of the last change. */
	UPROPERTY() uint8 Cause = 0;
	UPROPERTY() FVector_NetQuantizeNormal HitDir = FVector::ZeroVector;
	/** Server world time of the last change. */
	UPROPERTY() float ChangedAt = 0.f;

	void PreReplicatedRemove(const FGSPartArray& InArray);
	void PostReplicatedAdd(const FGSPartArray& InArray);
	void PostReplicatedChange(const FGSPartArray& InArray);
};

USTRUCT()
struct FGSPartArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY() TArray<FGSPartState> Items;

	/** Not replicated: the actor that holds this list. */
	AGSMapState* Owner = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FGSPartState, FGSPartArray>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FGSPartArray> : public TStructOpsTypeTraitsBase2<FGSPartArray>
{
	enum { WithNetDeltaSerializer = true };
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FGSOnWalkableChanged, UWorld* /*World*/, const FBox& /*BoxCm*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FGSOnNoise, UWorld* /*World*/, const FVector& /*LocCm*/, float /*LoudnessM*/);

/** The one replicated state for the interactive map. Spawned by the server (UGSMapStateSubsystem). */
UCLASS(NotPlaceable)
class GLOSHAUGEN_API AGSMapState : public AInfo
{
	GENERATED_BODY()

public:
	AGSMapState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void PostNetReceive() override;

	/** The map state of this world, or null (clients: before it has arrived). */
	static AGSMapState* Get(const UObject* Ctx);
	/** Server: everything back to how the map starts (also called by the game mode at match start). */
	static void ResetForMatch(UWorld* World);

	// --- Server ---
	/** The part as it is now: the list entry, or the part's default. False if the key is unknown. */
	bool GetPart(int32 Key, FGSPartState& Out) const;
	/** Server: change a part. Fn changes the copy; the list, the local door and every client follow. */
	void SetPart(int32 Key, TFunctionRef<void(FGSPartState&)> Fn);
	void DoResetForMatch();
	void DoEnterTestRoom(APlayerController* PC, bool bAll);
	float ServerNow() const;

	// --- Every machine ---
	AGSDoor* FindDoor(int32 Key) const;
	const TArray<TWeakObjectPtr<AActor>>& GetUsables() const { return Usables; }
	AActor* FindUsable(int32 Key) const;
	const TMap<int32, TWeakObjectPtr<AGSDoor>>& GetDoors() const { return Doors; }
	/** Server world time, also on clients (0 until the game state has arrived). */
	float GetServerTime() const;
	bool IsRegistryBuilt() const { return bRegistryBuilt; }

	/** Called by the list callbacks on clients. */
	void OnPartReplicated(const FGSPartState& Item, bool bRemoved);

	/** Door sounds with fallbacks while the real ones are missing. Plays locally (never on a dedicated server). */
	static void PlayPartSound(const UObject* Ctx, const TCHAR* Name, const FVector& Loc, float Volume = 1.f, float RangeM = 25.f, float Pitch = 1.f);

	UPROPERTY(Replicated) FGSPartArray Parts;
	UPROPERTY(ReplicatedUsing = OnRep_ResetCount) int32 ResetCount = 0;
	UPROPERTY(ReplicatedUsing = OnRep_TestRoom) bool bTestRoom = false;

protected:
	UFUNCTION() void OnRep_ResetCount();
	UFUNCTION() void OnRep_TestRoom();

	void TryBuildRegistry();
	void BuildRegistry();
	void BuildTestRoom();
	AGSDoor* SpawnDoor(const FGSDoorDef& Def, int32 Key);
	void ApplyLocal(const FGSPartState& Item, bool bFresh);
	void ApplyAllLocal();
	void ResetLocal();
	void ResetPartLocal(int32 Key);
	void ServerSlowTick();     // 0.5 s: phase watch
	void ServerInteractors();  // 1 s: one AGSInteractor per human player
	void ServerBots();         // 5 Hz: bots open unlocked doors
	void SpawnTestPickups();

	TMap<int32, TWeakObjectPtr<AGSDoor>> Doors;
	TArray<TWeakObjectPtr<AActor>> Usables;
	TSet<int32> PendingKeys;
	bool bRegistryBuilt = false;
	bool bTestRoomBuilt = false;
	bool bFirstRepDone = false;
	float RegistryTries = 0.f;
	uint8 LastPhase = 255;
	UPROPERTY() TObjectPtr<AActor> TestRoomGeo;
	TArray<TWeakObjectPtr<AGSPickup>> TestPickups;
	FTimerHandle RegistryTimer, SlowTimer, InteractorTimer, BotTimer;
};

/** Spawns AGSMapState on the server in every game world. */
UCLASS()
class GLOSHAUGEN_API UGSMapStateSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
};

/**
 * The calls other teams make (Kamp: bullets, blasts, melee; Spillbarhet: reset, test room; Bots: doors, noise).
 * All are static, safe to call before the map exists, and do nothing on clients (except the delegates and queries).
 */
namespace GSMapState
{
	/** Server: a blast (grenade). Doors within RadiusCm (with a free line) fly open away from it, their locks break. */
	GLOSHAUGEN_API void ApplyBlast(UWorld* World, const FVector& OriginCm, float RadiusCm, float Strength);
	/** Server: a real bullet hit something that is not a figure. A code lock that is hit breaks; so does the lock of a
	 *  locked wooden door hit right by the handle. */
	GLOSHAUGEN_API void ApplyBulletHit(UWorld* World, const FHitResult& Hit, float Damage);
	/** Server: a melee swing from Start to End. A door leaf takes it as one kick, a code lock as one blow (two break it).
	 *  Returns the distance (cm from Start) to the first whole glass pane along the swing, BIG_NUMBER when none (1a: always). */
	GLOSHAUGEN_API float ApplyMelee(AActor* User, const FVector& Start, const FVector& End, float Damage);
	/** Bullet path From -> To (every machine, real and shown bullets): glass along it (1b). Returns the speed factor (1 = none). */
	GLOSHAUGEN_API float BulletSegment(UWorld* World, const FVector& From, const FVector& To, const FVector& Vel, float Damage, bool bReal);
	/** A lasting bullet hole where a shown bullet hit (1b). */
	GLOSHAUGEN_API void AddBulletMark(UWorld* World, const FHitResult& Hit);
	/** Server: everything back to the start of a match. */
	GLOSHAUGEN_API void ResetForMatch(UWorld* World);
	/** Server: build the door test room (on every machine) and move this player (or everyone) into it. */
	GLOSHAUGEN_API void EnterTestRoom(APlayerController* PC, bool bAll = false);
	/** False when a closed, locked door stands within 1 m of PosCm. */
	GLOSHAUGEN_API bool IsDoorPassable(UWorld* World, const FVector& PosCm);
	GLOSHAUGEN_API void ForEachDoor(UWorld* World, TFunctionRef<void(const FGSDoorInfo&)> Fn);
	/** Server, when a door is unlocked or broken (round 1b: glass gone, round 2: walls blown): the box that changed. */
	GLOSHAUGEN_API FGSOnWalkableChanged& OnWalkableChanged();
	/** Server: something loud happened (kick, breaking a lock, blast). LoudnessM = how far it is heard. */
	GLOSHAUGEN_API FGSOnNoise& OnNoise();
}
