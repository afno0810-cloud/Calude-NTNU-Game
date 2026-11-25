#pragma once

#include "CoreMinimal.h"
#include "GSTypes.generated.h"

UENUM(BlueprintType)
enum class EGSWeaponId : uint8
{
	None,
	Pistol,   // M1911 / Kongsberg Colt
	AG3,
	MAX UMETA(Hidden)
};

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

/** Static stats for a weapon type. Add a new row in GSWeapons::Get to add a weapon. */
struct FGSWeaponStats
{
	const TCHAR* Name = TEXT("");
	float Damage = 20.f;
	float HeadshotMultiplier = 2.f;
	float FireInterval = 0.2f;      // seconds between shots
	bool bAutomatic = false;
	int32 MagSize = 10;
	int32 StartReserve = 30;
	int32 MaxReserve = 90;
	float ReloadTime = 2.f;
	float Range = 20000.f;          // cm
	float HipSpread = 2.0f;         // degrees
	float AimSpread = 0.3f;         // degrees
	float RecoilPitch = 0.8f;       // degrees per shot
	float AimFOV = 65.f;
	FLinearColor Color = FLinearColor::Gray;
	FVector MeshScale = FVector(0.5f, 0.06f, 0.1f); // first person block (cube 1m)
};

namespace GSWeapons
{
	inline const FGSWeaponStats& Get(EGSWeaponId Id)
	{
		static FGSWeaponStats None;
		static FGSWeaponStats Pistol = [] {
			FGSWeaponStats S;
			S.Name = TEXT("Pistol (M1911)");
			S.Damage = 28.f; S.FireInterval = 0.18f; S.bAutomatic = false;
			S.MagSize = 7; S.StartReserve = 21; S.MaxReserve = 63; S.ReloadTime = 1.6f;
			S.Range = 8000.f; S.HipSpread = 1.6f; S.AimSpread = 0.35f; S.RecoilPitch = 1.4f; S.AimFOV = 75.f;
			S.Color = FLinearColor(0.08f, 0.08f, 0.09f);
			S.MeshScale = FVector(0.22f, 0.04f, 0.12f);
			return S;
		}();
		static FGSWeaponStats AG3 = [] {
			FGSWeaponStats S;
			S.Name = TEXT("AG3");
			S.Damage = 38.f; S.FireInterval = 0.1f; S.bAutomatic = true;
			S.MagSize = 20; S.StartReserve = 60; S.MaxReserve = 140; S.ReloadTime = 2.4f;
			S.Range = 30000.f; S.HipSpread = 3.0f; S.AimSpread = 0.25f; S.RecoilPitch = 1.1f; S.AimFOV = 55.f;
			S.Color = FLinearColor(0.12f, 0.13f, 0.1f);
			S.MeshScale = FVector(0.95f, 0.05f, 0.11f);
			return S;
		}();
		switch (Id)
		{
		case EGSWeaponId::Pistol: return Pistol;
		case EGSWeaponId::AG3: return AG3;
		default: return None;
		}
	}
}

USTRUCT(BlueprintType)
struct FGSWeaponSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) EGSWeaponId Id = EGSWeaponId::None;
	UPROPERTY(BlueprintReadOnly) int32 Ammo = 0;     // in magazine
	UPROPERTY(BlueprintReadOnly) int32 Reserve = 0;
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
