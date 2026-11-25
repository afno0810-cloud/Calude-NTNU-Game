#pragma once

#include "CoreMinimal.h"
#include "GSItems.h"

class USoundBase;

/**
 * Sounds by name: /Game/Sounds/S_GS_<Name> (made by Tools/Audio/gen_sounds.py, imported by gs_sounds.py).
 * A recorded sound imported with the same name replaces ours. Missing sounds are silently skipped,
 * and nothing plays on a dedicated server.
 */
namespace GSAudio
{
	USoundBase* Get(const FString& Name);
	/** 3D sound that fades out over RangeM meters, muffled (low-pass) at a distance. */
	void PlayAt(const UObject* Context, const FString& Name, const FVector& Location, float Volume = 1.f, float RangeM = 50.f, float Pitch = 1.f);
	/** Non-positional sound for the local player (own shots, hit marker). */
	void Play2D(const UObject* Context, const FString& Name, float Volume = 1.f, float Pitch = 1.f);
	/** Random variant: Name_1 .. Name_Count */
	FString Variant(const TCHAR* Name, int32 Count);
	const TCHAR* ShotName(EGSWeaponClass Class, bool bSilent);
	/** How far (m) a shot of this class is heard. */
	float ShotRange(EGSWeaponClass Class, bool bSilent);
	/** Shot heard by everyone else: full shot nearby, a muffled distant boom far away. */
	void PlayShotFor(const UObject* Context, EGSWeaponClass Class, bool bSilent, const FVector& Location);
}
