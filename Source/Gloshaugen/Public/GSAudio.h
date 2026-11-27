#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GSItems.h"
#include "GSAudio.generated.h"

class USoundBase;
struct FHitResult;

/** The room around a point, measured with traces (GSAudio::RoomAt). */
struct FGSRoom
{
	float RT60 = 0.f;        // seconds of reverb (Sabine-like estimate)
	float AreaM2 = 0.f;      // floor area seen from the point
	float HeightM = 0.f;     // floor to ceiling (0 = no ceiling)
	float LongM = 0.f;       // longest and shortest of the four widths through the point
	float ShortM = 0.f;
	bool bOutside = true;
	/** 0 Small, 1 Corridor, 2 Large, 3 Stair, 4 Outside */
	uint8 Preset = 4;
};

/**
 * Sounds by name: /Game/Sounds/S_GS_<Name> (Tools/Audio: prep_sounds.py makes the recorded ones, gen_sounds.py the
 * synthetic ones; Content/Python/gs_sounds.py imports them). A name may have variants S_GS_<Name>_1 .. _16: they are
 * found by themselves and drawn at random, never the same twice in a row; without variants S_GS_<Name> plays.
 * Missing sounds are silently skipped, and nothing plays on a dedicated server.
 *
 * Every sound belongs to a class by its name (Shot_ = Weapon, Step_ = Step, Impact_ = Impact, ...) that sets its
 * priority, how many may play at once and how the mix treats it. 3D sounds are muffled behind walls (occlusion) and
 * get the reverb of the room the listener stands in (measured with traces, see UGSAudioWorld).
 */
namespace GSAudio
{
	/** The sound to play for Name (a random variant if it has any), or null. */
	USoundBase* Get(const FString& Name);
	/** True if Name or any variant of it exists. */
	bool Has(const FString& Name);
	/** The asset name of a variant to play now ("Shot_Rifle_Close_3"), never the same as last time; "" if none. */
	FString Pick(const FString& Name);
	/** 3D sound that fades out over RangeM meters, muffled (low-pass) at a distance and behind walls. */
	void PlayAt(const UObject* Context, const FString& Name, const FVector& Location, float Volume = 1.f, float RangeM = 50.f, float Pitch = 1.f);
	/** As PlayAt, but heard DelayS later (the speed of sound). Under 0.03 s it plays at once. */
	void PlayAtDelayed(const UObject* Context, const FString& Name, const FVector& Location, float Volume, float RangeM, float Pitch, float DelayS);
	/** Non-positional sound for the local player (own shots, hit marker). */
	void Play2D(const UObject* Context, const FString& Name, float Volume = 1.f, float Pitch = 1.f);
	/** Menu sounds (other teams): 2D, never ducked by the mix, no pitch change. */
	void PlayUI(const UObject* Context, const TCHAR* Name, float Volume = 1.f);
	/** Radio messages (flag radio): 2D, highest priority, never ducked, a squelch when the channel closes. */
	void PlayRadio(const UObject* Context, const TCHAR* Name, float Volume = 1.f, float Pitch = 1.f);
	/** Old way of naming a random variant: Name_1 .. Name_Count (Pick finds them by itself now). */
	FString Variant(const TCHAR* Name, int32 Count);
	const TCHAR* ShotName(EGSWeaponClass Class, bool bSilent);
	/** How far (m) a shot of this class is heard. */
	float ShotRange(EGSWeaponClass Class, bool bSilent);
	/** Shot heard by everyone else: close crack, mid and far layers by distance, the tail of the shooter's room. */
	void PlayShotFor(const UObject* Context, EGSWeaponClass Class, bool bSilent, const FVector& Location);
	/** The shooter's own shot: close-up recording, the mechanism and the tail of his own room from the muzzle. */
	void PlayOwnShot(const UObject* Context, EGSWeaponClass Class, bool bSilent, const FVector& Muzzle);
	/** Reload / cycling part ("Mag_Out", "Mag_In", "Bolt") for this weapon type, with the old name as fallback. */
	FString MechName(EGSWeaponClass Class, const TCHAR* Part);
	/** Grenade, rocket, mine (bFlash: a stun grenade: short bang and long ringing inside its radius). */
	void PlayExplosion(UWorld* World, const FVector& Location, float RadiusCm, bool bFlash = false);

	/** What a trace hit is made of: Stone, Wood, Lino, Metal, Concrete, Carpet, Grass, Glass, Plaster, Dirt. */
	FName SurfaceAt(const FHitResult& Hit);
	/** Floor under Feet (a short trace down, physical material included). Concrete if nothing is found. */
	FName SurfaceBelow(const AActor* Self, const FVector& Feet);
	/** Footstep / landing / slide name for a surface ("Step", Stone) -> "Step_Stone", with the old names as fallback. */
	FString StepName(const TCHAR* Kind, FName Surface);

	/** The room around Location (traces against the static world). Cached per caller for 0.5 s when Cache is set. */
	FGSRoom RoomAt(const UWorld* World, const FVector& Location, const UObject* Cache = nullptr);
	/** Hook for the map's sound zones (Grafikk, plan-12): false until they exist. */
	bool ZoneRT60(const UWorld* World, const FVector& Location, float& OutRT60);
	const TCHAR* RoomName(uint8 Preset);
}

/** Per world: measures the listener's room 4 times a second and switches the reverb, plays delayed sounds. */
UCLASS()
class GLOSHAUGEN_API UGSAudioWorld : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	struct FDelayed
	{
		float At = 0.f;
		FString Name;
		FVector Location = FVector::ZeroVector;
		float Volume = 1.f, RangeM = 50.f, Pitch = 1.f;
		uint8 Kind = 0;   // 0 3D sound, 1 2D sound, 2 end of the blast mix
	};
	void Enqueue(const FDelayed& D);
	/** Sounds started by GSAudio that are still playing (estimate from their lengths). */
	int32 ActiveEstimate() const;
	/** A sound started: its class (GSAudioImpl::ESnd) and length in seconds. */
	void NoteStarted(uint8 Category, float LengthS);
	/** The engine's own count of sounds playing on this world's audio device (-1 without one). */
	int32 EngineActive() const;
	/** Everything but steps and radio ducked and muffled for Seconds (a blast close by). */
	void PushBlastMix(float Seconds);
	int32 QueueNum() const { return Queue.Num(); }

	TMap<TWeakObjectPtr<const UObject>, TPair<float, FGSRoom>> RoomCache;
	FGSRoom ListenerRoom;
	int32 MaxQueue = 0;
	int32 MaxActive = 0;
	int32 MaxEngineActive = 0;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGSAudioWorld, STATGROUP_Tickables); }
	virtual void Deinitialize() override;
protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
	{
		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
	}
private:
	TArray<FDelayed> Queue;
	struct FStarted { uint8 Category = 0; float End = 0.f; };
	TArray<FStarted> Ends;       // sounds we started: class and end time
	int32 MixPushes = 0;
	float NextRoom = 0.f;
	float NextLog = 10.f;
	uint8 ActivePreset = 255;
};
