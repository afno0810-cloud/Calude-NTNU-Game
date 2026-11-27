#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GSTypes.generated.h"

UENUM(BlueprintType)
enum class EGSWeather : uint8
{
	Clear,
	Cloudy,
	Fog,
	Rain,
	Snow,
	MAX UMETA(Hidden)
};

UENUM()
enum class EGSMatchPhase : uint8
{
	Lobby,
	Countdown,
	Playing,
	Ended,
	/** "Test kartet": walk and fly freely, no match, no damage (Spillbarhet, GSExplore). */
	Explore,
};

/** Attachment bits stored on a weapon slot. */
namespace GSAttach
{
	constexpr uint16 Suppressor = 1 << 0;
	constexpr uint16 RedDot = 1 << 1;
	constexpr uint16 Scope = 1 << 2;      // 4x
	constexpr uint16 ExtMag = 1 << 3;
	constexpr uint16 Laser = 1 << 4;
	constexpr uint16 Holo = 1 << 5;
	constexpr uint16 Scope8x = 1 << 6;
	constexpr uint16 Drum = 1 << 7;
	constexpr uint16 Compensator = 1 << 8;
	constexpr uint16 Grip = 1 << 9;
	// Only one of each group fits on a gun at a time
	constexpr uint16 SightGroup = RedDot | Scope | Holo | Scope8x;
	constexpr uint16 MagGroup = ExtMag | Drum;
	constexpr uint16 MuzzleGroup = Suppressor | Compensator;
}

/** Special rounds loaded in a gun. */
namespace GSAmmoType
{
	constexpr uint8 Normal = 0;
	constexpr uint8 ArmorPiercing = 1;
	constexpr uint8 HollowPoint = 2;
	constexpr uint8 Incendiary = 3;
	constexpr uint8 Tracer = 4;
	constexpr uint8 Shock = 5;
}

/** A weapon the player carries. Id is a row in the item catalog (GSItems). */
USTRUCT(BlueprintType)
struct FGSWeaponSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName Id;
	UPROPERTY(BlueprintReadOnly) int32 Ammo = 0;     // in magazine
	UPROPERTY(BlueprintReadOnly) int32 Reserve = 0;
	UPROPERTY() uint16 Attachments = 0; // GSAttach bits (uint16 is not Blueprint-visible)
	UPROPERTY(BlueprintReadOnly) uint8 AmmoType = 0;   // GSAmmoType
};

/** A stack of items in the backpack. */
USTRUCT(BlueprintType)
struct FGSInvStack
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName Id;
	UPROPERTY(BlueprintReadOnly) int32 Count = 0;
};

/** A boost / side effect that is running. EndTime is server world time. */
USTRUCT(BlueprintType)
struct FGSActiveEffect
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName Id;
	UPROPERTY(BlueprintReadOnly) float EndTime = 0.f;
	UPROPERTY(BlueprintReadOnly) uint8 Doses = 1;
	UPROPERTY(BlueprintReadOnly) float StartTime = 0.f;   // server time of the first dose; 0 = EndTime - Duration
};

/** A player position sent to someone using the tracker on a PC. */
USTRUCT()
struct FGSTrackedPlayer
{
	GENERATED_BODY()
	UPROPERTY() FString Name;
	UPROPERTY() FVector_NetQuantize Location = FVector::ZeroVector;
	UPROPERTY() bool bSelf = false;
};

USTRUCT(BlueprintType)
struct FGSKillFeedEntry
{
	GENERATED_BODY()

	UPROPERTY() FString Killer;
	UPROPERTY() FString Victim;
	UPROPERTY() FString Weapon;
	UPROPERTY() bool bHeadshot = false;
	UPROPERTY() float ServerTime = 0.f;
};

/** Sent by the server to a player who just died: who killed them, with what, from how far, and how hurt the killer was. */
USTRUCT()
struct FGSDeathInfo
{
	GENERATED_BODY()

	UPROPERTY() FString Killer;              // empty = no player (hunger, fall, your own grenade ...)
	UPROPERTY() FString Weapon;              // weapon name, or the cause of death ("Sult", "Overdose", "Verden" ...)
	UPROPERTY() float DistanceM = 0.f;
	UPROPERTY() float KillerHealth = 0.f;
	UPROPERTY() float KillerMaxHealth = 100.f;
	UPROPERTY() float KillerArmor = 0.f;     // vest + helmet points left
	UPROPERTY() bool bHeadshot = false;
	/** Where the killer stood when you died (the death cam turns there if his pawn is too far away to be seen). */
	UPROPERTY() FVector_NetQuantize KillerLocation = FVector::ZeroVector;
	UPROPERTY() bool bHasKillerLocation = false;
	/** KillerHealth/MaxHealth/Armor are real (false: grenade or mine from a body that has died since). */
	UPROPERTY() bool bHasKillerHealth = false;
};

namespace GSNames
{
	inline const TCHAR* Weather(EGSWeather W)
	{
		switch (W)
		{
		case EGSWeather::Clear: return TEXT("Klart");
		case EGSWeather::Cloudy: return TEXT("Overskyet");
		case EGSWeather::Fog: return TEXT("T\u00E5ke");
		case EGSWeather::Rain: return TEXT("Regn");
		case EGSWeather::Snow: return TEXT("Sn\u00F8");
		default: return TEXT("");
		}
	}
}

/** What a hit did to the body, for the blood everyone sees (AGSCharacter::MulticastWoundFX). */
namespace GSWound
{
	constexpr uint8 Flesh = 0;    // bullet stayed in the body: blood at the wound, drops on the floor
	constexpr uint8 Through = 1;  // went through: also spatter on whatever is behind
	constexpr uint8 Head = 2;     // head, through
	constexpr uint8 Armor = 3;    // stopped by vest/helmet: dust and fibre, no blood
	constexpr uint8 Blast = 4;    // explosion / fragments
	constexpr uint8 Blade = 5;    // melee
}
