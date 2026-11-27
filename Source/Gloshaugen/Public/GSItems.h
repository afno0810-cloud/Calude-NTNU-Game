#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

class UStaticMesh;

/**
 * Everything you can find on Gloshaugen: weapons, armor, food, drugs/boosts, medicine, grenades and gadgets.
 * The data lives in Tools/ItemCatalog/catalog.py and is generated into GSItemCatalog.gen.inl.
 * Items are referred to by FName id everywhere (replication, pickups, inventory).
 */
enum class EGSItemCat : uint8 { Weapon, Armor, Helmet, Food, Drug, Medicine, Throwable, Gadget, MAX };
enum class EGSRarity : uint8 { Common, Uncommon, Rare, Epic, Legendary, Mythic, MAX };
enum class EGSWeaponClass : uint8 { Pistol, SMG, Rifle, Sniper, Shotgun, LMG, Launcher, Bow, Melee };
enum class EGSFireMode : uint8 { Hitscan, Pellets, Projectile, Melee };
/** Where a loot spot lies (Helgasetr): decides what kind of things it holds. Campus = the old outdoor map. */
enum class EGSLootPlace : uint8 { Campus, Corridor, Medical, Canteen, Crate };

/** Multipliers from boosts and side effects. 1 = normal. */
struct FGSEffectMods
{
	float Speed = 1.f, StaminaDrain = 1.f, StaminaRegen = 1.f, Recoil = 1.f, Spread = 1.f, Damage = 1.f, Taken = 1.f;
	float Jump = 1.f, Reload = 1.f, Hunger = 1.f, Thirst = 1.f;
	float Regen = 0.f, Drain = 0.f, Lifesteal = 0.f;   // HP per second / fraction
	bool bNightVision = false, bWallhack = false, bPhoenix = false, bSilent = false, bFlashImmune = false, bGhost = false, bGlide = false;
	int32 Visual = 0; // 0 none, 1 blur, 2 colors, 3 shake, 4 green haze, 5 psychedelic, 6 bright, 7 night vision

	void Combine(const FGSEffectMods& O)
	{
		Speed *= O.Speed; StaminaDrain *= O.StaminaDrain; StaminaRegen *= O.StaminaRegen; Recoil *= O.Recoil; Spread *= O.Spread;
		Damage *= O.Damage; Taken *= O.Taken; Jump *= O.Jump; Reload *= O.Reload; Hunger *= O.Hunger; Thirst *= O.Thirst;
		Regen += O.Regen; Drain += O.Drain; Lifesteal += O.Lifesteal;
		bNightVision |= O.bNightVision; bWallhack |= O.bWallhack; bPhoenix |= O.bPhoenix; bSilent |= O.bSilent;
		bFlashImmune |= O.bFlashImmune; bGhost |= O.bGhost; bGlide |= O.bGlide;
		if (O.Visual != 0) Visual = O.Visual;
	}
};

struct FGSEffectDef
{
	FName Id;
	FString Name;
	float Duration = 10.f;
	FGSEffectMods Mods;
	FName After;   // side effect that starts when this one ends
};

struct FGSWeaponStats
{
	EGSWeaponClass Class = EGSWeaponClass::Pistol;
	EGSFireMode Mode = EGSFireMode::Hitscan;
	float Damage = 20.f;
	float HeadshotMultiplier = 2.f;
	float FireInterval = 0.2f;      // seconds between shots
	bool bAutomatic = false;
	int32 MagSize = 10;
	int32 StartReserve = 30;
	int32 MaxReserve = 90;
	float ReloadTime = 2.f;
	float Range = 20000.f;          // cm (reach for melee)
	float HipSpread = 2.0f;         // degrees
	float AimSpread = 0.3f;         // degrees
	float RecoilPitch = 0.8f;       // degrees per shot
	float AimFOV = 65.f;
	int32 Pellets = 1;
	float MeleeDamage = 25.f;       // quick melee (V)
	float ProjSpeed = 0.f;          // cm/s
	float ProjGravity = 0.f;        // gravity scale
	float Radius = 0.f;             // explosion radius cm
	bool bSilent = false;
	FName Caliber;                  // ammo type it uses ("556", "12g" ...). None = no reloads from boxes
	float MuzzleX = 50.f;           // cm from model origin to muzzle
	float BoreZ = 0.f;              // cm, height of the barrel axis at the muzzle (suppressors, lasers)
	float SightZ = 5.f;             // cm, height of the iron-sight line (optics sit on it)
	FVector FPHip = FVector(30.f, 12.f, -18.f);  // first person position relative to camera
	FVector FPAim = FVector(20.f, 0.f, -6.f);

	bool UsesAmmo() const { return Mode != EGSFireMode::Melee; }
};

struct FGSItemDef
{
	FName Id;
	FString Name;
	FString Desc;
	EGSItemCat Cat = EGSItemCat::Gadget;
	EGSRarity Rarity = EGSRarity::Common;
	FString MeshPath;
	int32 MaxStack = 1;

	FGSWeaponStats W;                     // weapons
	float Armor = 0.f, Absorb = 0.f, SpeedMul = 1.f;   // armor / helmets
	float UseTime = 1.f;                  // seconds to eat / inject / apply
	float Heal = 0.f, Hunger = 0.f, Thirst = 0.f, StaminaGain = 0.f, Poison = 0.f;
	FName Effect;                         // boost started when used
	FName Kind;                           // throwable / gadget behaviour
	float ThrowDamage = 0.f, ThrowRadius = 0.f, Fuse = 0.f;
	int32 Slots = 0, Charges = 1;
	FName Caliber;                        // ammo boxes: which caliber
	int32 Rounds = 0;                     // ammo boxes: rounds in one box

	bool IsWeapon() const { return Cat == EGSItemCat::Weapon; }
	bool IsWearable() const { return Cat == EGSItemCat::Armor || Cat == EGSItemCat::Helmet || Kind == FName(TEXT("backpack")); }
};

namespace GSItems
{
	GLOSHAUGEN_API const TArray<FGSItemDef>& All();
	GLOSHAUGEN_API const FGSItemDef* Find(FName Id);
	GLOSHAUGEN_API const FGSEffectDef* FindEffect(FName Id);
	/** Weapon stats, or a default "fists" entry if Id is not a weapon. */
	GLOSHAUGEN_API const FGSWeaponStats& Weapon(FName Id);
	GLOSHAUGEN_API const FString& Name(FName Id);
	/** Blender model, or nullptr if it has not been imported (then a colored block is used). */
	GLOSHAUGEN_API UStaticMesh* GetMesh(FName Id);
	/** Fixes for replacement models from Fab (Config/GSMeshOverrides.ini). Identity when the item uses its own model. */
	GLOSHAUGEN_API FRotator MeshRotation(FName Id);
	GLOSHAUGEN_API FVector MeshOffset(FName Id);
	GLOSHAUGEN_API float MeshScale(FName Id);

	GLOSHAUGEN_API FLinearColor RarityColor(EGSRarity R);
	GLOSHAUGEN_API const TCHAR* RarityName(EGSRarity R);
	GLOSHAUGEN_API const TCHAR* CategoryName(EGSItemCat C);

	/** Random loot. MinRarity lets locked crates give only good stuff. */
	GLOSHAUGEN_API FName RandomLoot(FRandomStream& Rng, EGSRarity MinRarity = EGSRarity::Common);
	/**
	 * Random loot for a place (room kind). Only MinRarity..MaxRarity. Depth 0..1 (near a base .. far from both)
	 * makes rarer things likelier. Outside Campus: no parachutes, drones or wall vision, and ammo follows how many
	 * of the possible weapons use each caliber. (Campus, Mythic, 0) is exactly the old draw.
	 */
	GLOSHAUGEN_API FName RandomLoot(FRandomStream& Rng, EGSRarity MinRarity, EGSRarity MaxRarity, EGSLootPlace Place, float Depth);
	/** How many of the item a pickup holds (grenades come in pairs, grapples have charges ...). */
	GLOSHAUGEN_API int32 PickupCount(const FGSItemDef& Def, FRandomStream& Rng);

	extern GLOSHAUGEN_API const FName Pistol;   // starting weapon
}
