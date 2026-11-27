#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

class UStaticMesh;
struct FGSActiveEffect;   // GSTypes.h (not included here, so files that only need the catalog do not rebuild with it)
struct FGSEffectMods;
namespace GSItems
{
	/** Caps on stacked boosts (round 7): speed and damage at most +10 %, damage taken at least 0.85 ... */
	GLOSHAUGEN_API void ClampMods(FGSEffectMods& M);
}

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
/** What kind of drug an effect is (round 7). None = not a drug (medicine, food, game effects). */
enum class EGSDrugClass : uint8 { None, Stimulant, Opioid, Depressant, Alcohol, Nicotine, Caffeine, Cannabis, Antidote };

/** Multipliers from boosts and side effects. 1 = normal. */
struct FGSEffectMods
{
	float Speed = 1.f, StaminaDrain = 1.f, StaminaRegen = 1.f, Recoil = 1.f, Spread = 1.f, Damage = 1.f, Taken = 1.f;
	float Jump = 1.f, Reload = 1.f, Hunger = 1.f, Thirst = 1.f;
	float Regen = 0.f, Drain = 0.f, Lifesteal = 0.f;   // HP per second / fraction
	bool bNightVision = false, bWallhack = false, bPhoenix = false, bSilent = false, bFlashImmune = false, bGhost = false, bGlide = false;
	// 0 none, 1 blur, 2 colors, 3 shake, 4 green haze, 5 psychedelic, 6 bright, 7 night vision, 8 red/warm, 9 white/overexposed,
	// 10 tunnel vision, 11 double vision
	int32 Visual = 0;
	bool bNumb = false, bCough = false;   // dulls pain (limbs) / coughing when sprinting

	/** Stacks O onto this without caps (GSItems::CombineActive adds them all, then caps once at the end). */
	void CombineRaw(const FGSEffectMods& O)
	{
		Speed *= O.Speed; StaminaDrain *= O.StaminaDrain; StaminaRegen *= O.StaminaRegen; Recoil *= O.Recoil; Spread *= O.Spread;
		Damage *= O.Damage; Taken *= O.Taken; Jump *= O.Jump; Reload *= O.Reload; Hunger *= O.Hunger; Thirst *= O.Thirst;
		Regen += O.Regen; Drain += O.Drain; Lifesteal += O.Lifesteal;
		bNightVision |= O.bNightVision; bWallhack |= O.bWallhack; bPhoenix |= O.bPhoenix; bSilent |= O.bSilent;
		bFlashImmune |= O.bFlashImmune; bGhost |= O.bGhost; bGlide |= O.bGlide;
		bNumb |= O.bNumb; bCough |= O.bCough;
		if (O.Visual != 0) Visual = O.Visual;
	}

	/** Stacks O onto this and keeps the result within the caps (GSItems::ClampMods). A guard until Kamp calls
	 *  GSItems::CombineActive, which caps once at the end (capping after every step depends on the order). */
	void Combine(const FGSEffectMods& O)
	{
		CombineRaw(O);
		GSItems::ClampMods(*this);
	}
};

struct FGSEffectDef
{
	FName Id;
	FString Name;
	float Duration = 10.f;
	FGSEffectMods Mods;
	FName After;   // side effect that starts when this one ends
	float Onset = 0.f, Fade = 0.f;   // seconds to full effect / seconds of come-down at the end (plan-07b)
	float Load = 0.f;                // how hard one dose is on the body (overdose, plan-07b)
	EGSDrugClass Class = EGSDrugClass::None;
};

/** One screen effect and how strong it is now (0..1), for the HUD. */
struct FGSVisualLayer
{
	int32 Visual = 0;
	float Strength = 0.f;
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
	float Treat = 0.f;                    // medicine: how much a hurt leg or arm is treated (0..1)
	FName UseAnim;                        // how it is used (model arch or e.g. "cigarette")
	bool bNoLoot = false;                 // never in loot (supernatural boosts); stays in the catalog

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

	/** How strong an active effect is now, 0..1 (onset, fade). Until plan-07b: 1 while it lasts, else 0. */
	GLOSHAUGEN_API float EffectStrength(const FGSEffectDef& Def, const FGSActiveEffect& Active, float Now);
	/** All active effects stacked by strength and capped once at the end (the GetMods of plan-07b). */
	GLOSHAUGEN_API FGSEffectMods CombineActive(const TArray<FGSActiveEffect>& Effects, float Now);
	/** The load on the body from all drugs (overdose at 3). Until plan-07b: 0. */
	GLOSHAUGEN_API float DrugLoad(const TArray<FGSActiveEffect>& Effects, float Now);
	/** Screen effects of all active effects with their strength. Until plan-07b: one layer per Visual at strength 1. */
	GLOSHAUGEN_API void ActiveVisuals(const TArray<FGSActiveEffect>& Effects, float Now, TArray<FGSVisualLayer>& Out);

	extern GLOSHAUGEN_API const FName Pistol;   // starting weapon
}
