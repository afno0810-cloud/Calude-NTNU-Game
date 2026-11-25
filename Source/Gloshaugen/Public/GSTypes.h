#pragma once

#include "CoreMinimal.h"
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

/** Attachment bits stored on a weapon slot. */
namespace GSAttach
{
	constexpr uint8 Suppressor = 1 << 0;
	constexpr uint8 RedDot = 1 << 1;
	constexpr uint8 Scope = 1 << 2;
	constexpr uint8 ExtMag = 1 << 3;
	constexpr uint8 Laser = 1 << 4;
}

/** A weapon the player carries. Id is a row in the item catalog (GSItems). */
USTRUCT(BlueprintType)
struct FGSWeaponSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName Id;
	UPROPERTY(BlueprintReadOnly) int32 Ammo = 0;     // in magazine
	UPROPERTY(BlueprintReadOnly) int32 Reserve = 0;
	UPROPERTY(BlueprintReadOnly) uint8 Attachments = 0;
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
