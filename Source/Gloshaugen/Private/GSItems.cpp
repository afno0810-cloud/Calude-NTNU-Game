#include "GSItems.h"
#include "GSTypes.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Engine/StaticMesh.h"
#include "UObject/UObjectGlobals.h"
#include "HAL/IConsoleManager.h"

#include "GSItemCatalog.gen.inl"

namespace
{
	struct FCatalog
	{
		TArray<FGSItemDef> Items;
		TArray<FGSEffectDef> Effects;
		TMap<FName, int32> ItemIndex;
		TMap<FName, int32> EffectIndex;

		FCatalog()
		{
			GSBuildCatalog(Items, Effects);
			for (int32 I = 0; I < Items.Num(); ++I) ItemIndex.Add(Items[I].Id, I);
			for (int32 I = 0; I < Effects.Num(); ++I) EffectIndex.Add(Effects[I].Id, I);
		}
	};

	const FCatalog& Cat()
	{
		static FCatalog C;
		return C;
	}
}

const FName GSItems::Pistol(TEXT("M1911"));

const TArray<FGSItemDef>& GSItems::All() { return Cat().Items; }

const FGSItemDef* GSItems::Find(FName Id)
{
	if (Id.IsNone()) return nullptr;
	const int32* I = Cat().ItemIndex.Find(Id);
	return I ? &Cat().Items[*I] : nullptr;
}

const FGSEffectDef* GSItems::FindEffect(FName Id)
{
	if (Id.IsNone()) return nullptr;
	const int32* I = Cat().EffectIndex.Find(Id);
	return I ? &Cat().Effects[*I] : nullptr;
}

const FGSWeaponStats& GSItems::Weapon(FName Id)
{
	static const FGSWeaponStats Fists = [] {
		FGSWeaponStats S;
		S.Class = EGSWeaponClass::Melee; S.Mode = EGSFireMode::Melee; S.Damage = 15.f; S.FireInterval = 0.5f; S.Range = 150.f;
		return S;
	}();
	const FGSItemDef* D = Find(Id);
	return D && D->IsWeapon() ? D->W : Fists;
}

const FString& GSItems::Name(FName Id)
{
	static const FString Unknown(TEXT("?"));
	const FGSItemDef* D = Find(Id);
	return D ? D->Name : Unknown;
}

namespace
{
	/** A Fab / Megascans model that replaces the item's own Blender model. */
	struct FMeshOverride
	{
		FString Path;
		FRotator Rot = FRotator::ZeroRotator;
		FVector Offset = FVector::ZeroVector;
		float Scale = 1.f;
		bool bLoaded = false;
		TWeakObjectPtr<UStaticMesh> Mesh;
	};

	/**
	 * Config/GSMeshOverrides.ini, one line per item:
	 *   AK47=/Game/Fab/AK/SM_AK47.SM_AK47 | yaw=90 pitch=0 roll=0 | scale=1.0 | offset=0,0,0
	 * Lines starting with ; are comments. Written by Content/Python/init_unreal.py, can be edited by hand.
	 */
	TMap<FName, FMeshOverride>& Overrides()
	{
		static TMap<FName, FMeshOverride> Map;
		static bool bRead = false;
		if (bRead) return Map;
		bRead = true;
		TArray<FString> Lines;
		FFileHelper::LoadFileToStringArray(Lines, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("GSMeshOverrides.ini")));
		for (FString Line : Lines)
		{
			Line.TrimStartAndEndInline();
			if (Line.IsEmpty() || Line.StartsWith(TEXT(";")) || Line.StartsWith(TEXT("["))) continue;
			FString Key, Rest;
			if (!Line.Split(TEXT("="), &Key, &Rest)) continue;
			TArray<FString> Parts;
			Rest.ParseIntoArray(Parts, TEXT("|"));
			if (Parts.Num() == 0) continue;
			FMeshOverride O;
			O.Path = Parts[0].TrimStartAndEnd();
			for (int32 I = 1; I < Parts.Num(); ++I)
			{
				TArray<FString> Tokens;
				Parts[I].ParseIntoArrayWS(Tokens);
				for (const FString& T : Tokens)
				{
					FString K, V;
					if (!T.Split(TEXT("="), &K, &V)) continue;
					K = K.ToLower();
					if (K == TEXT("yaw")) O.Rot.Yaw = FCString::Atof(*V);
					else if (K == TEXT("pitch")) O.Rot.Pitch = FCString::Atof(*V);
					else if (K == TEXT("roll")) O.Rot.Roll = FCString::Atof(*V);
					else if (K == TEXT("scale")) O.Scale = FMath::Max(0.001f, FCString::Atof(*V));
					else if (K == TEXT("offset"))
					{
						TArray<FString> XYZ;
						V.ParseIntoArray(XYZ, TEXT(","));
						if (XYZ.Num() == 3) O.Offset = FVector(FCString::Atof(*XYZ[0]), FCString::Atof(*XYZ[1]), FCString::Atof(*XYZ[2]));
					}
				}
			}
			if (!O.Path.IsEmpty()) Map.Add(FName(*Key.TrimStartAndEnd()), O);
		}
		return Map;
	}

	/** The override for Id if its model actually loads. */
	FMeshOverride* ActiveOverride(FName Id)
	{
		FMeshOverride* O = Overrides().Find(Id);
		if (!O) return nullptr;
		if (!O->bLoaded)
		{
			O->bLoaded = true;
			O->Mesh = LoadObject<UStaticMesh>(nullptr, *O->Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
		return O->Mesh.IsValid() ? O : nullptr;
	}
}

FRotator GSItems::MeshRotation(FName Id) { const FMeshOverride* O = ActiveOverride(Id); return O ? O->Rot : FRotator::ZeroRotator; }
FVector GSItems::MeshOffset(FName Id) { const FMeshOverride* O = ActiveOverride(Id); return O ? O->Offset : FVector::ZeroVector; }
float GSItems::MeshScale(FName Id) { const FMeshOverride* O = ActiveOverride(Id); return O ? O->Scale : 1.f; }

UStaticMesh* GSItems::GetMesh(FName Id)
{
	if (FMeshOverride* O = ActiveOverride(Id)) return O->Mesh.Get();
	static TMap<FName, TWeakObjectPtr<UStaticMesh>> Cache;
	static TSet<FName> Missing;
	const FGSItemDef* D = Find(Id);
	if (!D || D->MeshPath.IsEmpty() || Missing.Contains(Id)) return nullptr;
	if (TWeakObjectPtr<UStaticMesh>* Found = Cache.Find(Id))
	{
		if (Found->IsValid()) return Found->Get();
	}
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *D->MeshPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Mesh) { Missing.Add(Id); return nullptr; }
	Cache.Add(Id, Mesh);
	return Mesh;
}

FLinearColor GSItems::RarityColor(EGSRarity R)
{
	switch (R)
	{
	case EGSRarity::Common: return FLinearColor(0.62f, 0.62f, 0.62f);
	case EGSRarity::Uncommon: return FLinearColor(0.2f, 0.85f, 0.25f);
	case EGSRarity::Rare: return FLinearColor(0.15f, 0.5f, 1.f);
	case EGSRarity::Epic: return FLinearColor(0.65f, 0.25f, 0.95f);
	case EGSRarity::Legendary: return FLinearColor(1.f, 0.72f, 0.1f);
	case EGSRarity::Mythic: return FLinearColor(1.f, 0.12f, 0.12f);
	default: return FLinearColor::White;
	}
}

const TCHAR* GSItems::RarityName(EGSRarity R)
{
	switch (R)
	{
	case EGSRarity::Common: return TEXT("Vanlig");
	case EGSRarity::Uncommon: return TEXT("Uvanlig");
	case EGSRarity::Rare: return TEXT("Sjelden");
	case EGSRarity::Epic: return TEXT("Episk");
	case EGSRarity::Legendary: return TEXT("Legendarisk");
	case EGSRarity::Mythic: return TEXT("Mytisk");
	default: return TEXT("");
	}
}

const TCHAR* GSItems::CategoryName(EGSItemCat C)
{
	switch (C)
	{
	case EGSItemCat::Weapon: return TEXT("V\u00E5pen");
	case EGSItemCat::Armor: return TEXT("Vest");
	case EGSItemCat::Helmet: return TEXT("Hjelm");
	case EGSItemCat::Food: return TEXT("Mat");
	case EGSItemCat::Drug: return TEXT("Boost");
	case EGSItemCat::Medicine: return TEXT("Medisin");
	case EGSItemCat::Throwable: return TEXT("Granat");
	case EGSItemCat::Gadget: return TEXT("Utstyr");
	default: return TEXT("");
	}
}

namespace GSItemsImpl
{
	static const float RarityWeight[] = { 40.f, 28.f, 17.f, 9.f, 4.5f, 1.5f };
	// Groups: EGSItemCat in order, plus ammo (ammo boxes are gadgets in the catalog, but are needed far more often).
	enum { GroupAmmo = (int32)EGSItemCat::MAX, NumGroups };
	// Share of all loot per group and place (EGSLootPlace order). Campus is the old outdoor map, tuned for 20 players
	// on ~410 spots. Helgasetr: corridors arm you, medical rooms heal you, the canteen feeds you, crates hold weapons.
	static const float GroupWeight[][NumGroups] = {
		//  Weapon Armor Helmet Food Drug Medicine Throwable Gadget Ammo
		{ 20.f,  7.f,  5.f, 20.f,  8.f, 13.f,  7.f,  8.f, 12.f },   // Campus
		{ 30.f,  6.f,  5.f,  4.f,  3.f, 18.f,  8.f,  4.f, 22.f },   // Corridor
		{ 10.f,  2.f,  2.f,  4.f,  6.f, 60.f,  2.f,  4.f, 10.f },   // Medical
		{ 18.f,  3.f,  2.f, 35.f,  2.f, 15.f,  5.f,  0.f, 20.f },   // Canteen
		{ 45.f, 12.f,  8.f,  0.f,  3.f, 15.f, 12.f,  3.f,  2.f },   // Crate
	};
	static const FName KindAmmo(TEXT("ammo"));
	static const FName KindDrone(TEXT("drone"));
	static const FName IdParachute(TEXT("Parachute"));

	int32 GroupOf(const FGSItemDef& D)
	{
		return (D.Kind == KindAmmo || D.Rounds > 0) ? (int32)GroupAmmo : (int32)D.Cat;
	}

	/** Never: supernatural boosts (bNoLoot, round 7). Indoors (Helgasetr) also no parachute (nothing to jump from) and no
	 *  drones (wall vision is not realistic). */
	bool Excluded(const FGSItemDef& D, EGSLootPlace Place)
	{
		return D.bNoLoot || (Place != EGSLootPlace::Campus && (D.Id == IdParachute || D.Kind == KindDrone));
	}

	/**
	 * Ammo weights outside Campus: a caliber is as likely as the weapons that use it (their rarity weights summed,
	 * only weapons that can drop here). The generic ammo box gets the mean of the calibers that can drop.
	 */
	void AmmoWeights(EGSRarity MinRarity, EGSRarity MaxRarity, EGSLootPlace Place, TArray<TPair<const FGSItemDef*, float>>& Out)
	{
		TMap<FName, float> ByCaliber;
		for (const FGSItemDef& D : GSItems::All())
		{
			if (!D.IsWeapon() || D.W.Caliber.IsNone() || D.Rarity < MinRarity || D.Rarity > MaxRarity || Excluded(D, Place)) continue;
			ByCaliber.FindOrAdd(D.W.Caliber) += RarityWeight[FMath::Min((int32)D.Rarity, 5)];
		}
		float Sum = 0.f;
		int32 NonZero = 0;
		const FGSItemDef* Box = nullptr;
		for (const FGSItemDef& D : GSItems::All())
		{
			// MinRarity does not apply to ammo (it is Common/Uncommon): a Rare crate may still hold a box for its guns.
			if (GroupOf(D) != GroupAmmo || D.Rarity > MaxRarity || Excluded(D, Place)) continue;
			if (D.Caliber.IsNone()) { Box = &D; continue; }
			const float* W = ByCaliber.Find(D.Caliber);
			if (!W || *W <= 0.f) continue;
			Out.Add({ &D, *W });
			Sum += *W;
			++NonZero;
		}
		if (Box && NonZero > 0) Out.Add({ Box, Sum / NonZero });
	}
}

FName GSItems::RandomLoot(FRandomStream& Rng, EGSRarity MinRarity)
{
	return RandomLoot(Rng, MinRarity, EGSRarity::Mythic, EGSLootPlace::Campus, 0.f);
}

FName GSItems::RandomLoot(FRandomStream& Rng, EGSRarity MinRarity, EGSRarity MaxRarity, EGSLootPlace Place, float Depth)
{
	using namespace GSItemsImpl;
	// Three draws: first what kind of thing it is, then how rare, then which item of that kind and rarity.
	// (Weighting every item on its own made loot follow the size of the catalog: 59 weapons drowned 5 medicines.)
	// With (Campus, Mythic, Depth 0) every weight and every random call is the same as before Helgasetr.
	const bool bIndoor = Place != EGSLootPlace::Campus;
	const float* PlaceWeight = GroupWeight[FMath::Clamp((int32)Place, 0, (int32)UE_ARRAY_COUNT(GroupWeight) - 1)];
	if (MaxRarity < MinRarity) MaxRarity = MinRarity;

	// 1) Group, among those that have anything in MinRarity..MaxRarity.
	const int32 NumRarities = (int32)EGSRarity::MAX;
	int32 Count[NumGroups][(int32)EGSRarity::MAX] = {};
	for (const FGSItemDef& D : All())
	{
		if (D.Rarity >= MinRarity && D.Rarity <= MaxRarity && !Excluded(D, Place)) ++Count[GroupOf(D)][(int32)D.Rarity];
	}
	TArray<TPair<const FGSItemDef*, float>> Ammo;
	float AmmoTotal = 0.f;
	if (bIndoor)
	{
		AmmoWeights(MinRarity, MaxRarity, Place, Ammo);
		for (const TPair<const FGSItemDef*, float>& A : Ammo) AmmoTotal += A.Value;
	}
	auto GroupTotal = [&](int32 G) -> int32
	{
		if (bIndoor && G == GroupAmmo) return AmmoTotal > 0.f ? 1 : 0;
		int32 N = 0;
		for (int32 R = 0; R < NumRarities; ++R) N += Count[G][R];
		return N;
	};
	auto Usable = [&](int32 G) { return PlaceWeight[G] > 0.f && GroupTotal(G) > 0; };
	float Total = 0.f;
	for (int32 G = 0; G < NumGroups; ++G) if (Usable(G)) Total += PlaceWeight[G];
	if (Total <= 0.f) return All().Last().Id;
	int32 Group = INDEX_NONE;
	float Pick = Rng.FRandRange(0.f, Total);
	for (int32 G = 0; G < NumGroups; ++G)
	{
		if (!Usable(G)) continue;
		Group = G;
		Pick -= PlaceWeight[G];
		if (Pick <= 0.f) break;
	}

	// Indoor ammo: one caliber, weighted by the weapons that use it (no rarity draw).
	if (bIndoor && Group == GroupAmmo)
	{
		Pick = Rng.FRandRange(0.f, AmmoTotal);
		for (const TPair<const FGSItemDef*, float>& A : Ammo)
		{
			Pick -= A.Value;
			if (Pick <= 0.f) return A.Key->Id;
		}
		return Ammo.Last().Key->Id;
	}

	// 2) Rarity, among those the group has. Deeper (further from the bases) makes rare things likelier.
	const float DepthBoost = 1.f + 0.6f * FMath::Clamp(Depth, 0.f, 1.f);
	float RarityW[(int32)EGSRarity::MAX] = {};
	Total = 0.f;
	for (int32 R = 0; R < NumRarities; ++R)
	{
		if (Count[Group][R] == 0) continue;
		RarityW[R] = Depth > 0.f ? RarityWeight[R] * FMath::Pow(DepthBoost, (float)R) : RarityWeight[R];
		Total += RarityW[R];
	}
	int32 Rarity = 0;
	Pick = Rng.FRandRange(0.f, Total);
	for (int32 R = 0; R < NumRarities; ++R)
	{
		if (Count[Group][R] == 0) continue;
		Rarity = R;
		Pick -= RarityW[R];
		if (Pick <= 0.f) break;
	}

	// 3) One of the items in that group with that rarity, all equally likely.
	int32 Index = Rng.RandRange(0, Count[Group][Rarity] - 1);
	for (const FGSItemDef& D : All())
	{
		if (D.Rarity < MinRarity || D.Rarity > MaxRarity || Excluded(D, Place) || GroupOf(D) != Group || (int32)D.Rarity != Rarity) continue;
		if (Index-- == 0) return D.Id;
	}
	return All().Last().Id;
}

namespace GSItemsImpl
{
	/** gs.loot.stats [N]: draws N times per place, min rarity and depth (max Legendary, like Helgasetr) and logs the shares. */
	void LootStats(const TArray<FString>& Args)
	{
		const int32 N = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 100, 200000) : 2000;
		static const TCHAR* PlaceNames[] = { TEXT("Campus"), TEXT("Corridor"), TEXT("Medical"), TEXT("Canteen"), TEXT("Crate") };
		static const TCHAR* GroupNames[NumGroups] = { TEXT("weapon"), TEXT("armor"), TEXT("helmet"), TEXT("food"), TEXT("drug"),
			TEXT("medicine"), TEXT("throwable"), TEXT("gadget"), TEXT("ammo") };
		const EGSRarity Mins[] = { EGSRarity::Common, EGSRarity::Uncommon, EGSRarity::Rare };
		const float Depths[] = { 0.f, 1.f };
		const FName Rocket(TEXT("Ammo_rocket")), Bmg(TEXT("Ammo_50bmg"));
		UE_LOG(LogTemp, Display, TEXT("gs.loot.stats: %d trekninger per linje, Max Legendary"), N);
		for (int32 P = (int32)EGSLootPlace::Corridor; P <= (int32)EGSLootPlace::Crate; ++P)
		{
			for (EGSRarity Min : Mins)
			{
				for (float Depth : Depths)
				{
					FRandomStream Rng(1234 + P * 100 + (int32)Min * 10 + (int32)Depth);
					int32 Groups[NumGroups] = {};
					int32 EpicPlus = 0, Forbidden = 0, Banned = 0;
					TMap<FName, int32> Calibers;
					for (int32 I = 0; I < N; ++I)
					{
						const FName Id = GSItems::RandomLoot(Rng, Min, EGSRarity::Legendary, (EGSLootPlace)P, Depth);
						const FGSItemDef* D = GSItems::Find(Id);
						if (!D) continue;
						++Groups[GroupOf(*D)];
						if (D->Rarity >= EGSRarity::Epic) ++EpicPlus;
						if (Id == Rocket || Id == Bmg) ++Forbidden;
						if (Excluded(*D, (EGSLootPlace)P) || D->Rarity > EGSRarity::Legendary) ++Banned;
						if (GroupOf(*D) == GroupAmmo) ++Calibers.FindOrAdd(D->Caliber.IsNone() ? FName(TEXT("ammobox")) : D->Caliber);
					}
					FString Line;
					for (int32 G = 0; G < NumGroups; ++G) Line += FString::Printf(TEXT("%s %.1f%% "), GroupNames[G], 100.f * Groups[G] / N);
					Calibers.ValueSort([](int32 A, int32 B) { return A > B; });
					FString Top;
					int32 Shown = 0;
					for (const TPair<FName, int32>& C : Calibers)
					{
						if (Shown++ == 3) break;
						Top += FString::Printf(TEXT("%s %d "), *C.Key.ToString(), C.Value);
					}
					UE_LOG(LogTemp, Display, TEXT("gs.loot.stats %s Min=%s Depth=%.0f: %s| Epic+ %.1f%% | rakett/.50 %d | utelatt %d | ammo topp: %s"),
						PlaceNames[P], GSItems::RarityName(Min), Depth, *Line, 100.f * EpicPlus / N, Forbidden, Banned, *Top);
				}
			}
		}
	}

	static FAutoConsoleCommand LootStatsCommand(TEXT("gs.loot.stats"),
		TEXT("gs.loot.stats [N=2000]: fordelingen av Helgasetr-loot per sted, minste sjeldenhet og dybde"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&LootStats));
}

int32 GSItems::PickupCount(const FGSItemDef& Def, FRandomStream& Rng)
{
	if (Def.Kind == FName(TEXT("grapple"))) return Def.Charges;
	if (Def.Cat == EGSItemCat::Throwable) return Rng.RandRange(1, 2);
	if (Def.Cat == EGSItemCat::Food && Def.Rarity <= EGSRarity::Uncommon) return Rng.RandRange(1, 2);
	return 1;
}

// ------------------------------------------------------------------ effects (round 7)

void GSItems::ClampMods(FGSEffectMods& M)
{
	// However many things you take: at most 10 % faster or harder hitting, at least 85 % of the damage taken ...
	M.Speed = FMath::Min(M.Speed, 1.10f);
	M.Taken = FMath::Max(M.Taken, 0.85f);
	M.StaminaDrain = FMath::Max(M.StaminaDrain, 0.5f);
	M.StaminaRegen = FMath::Min(M.StaminaRegen, 1.6f);
	M.Recoil = FMath::Max(M.Recoil, 0.75f);
	M.Spread = FMath::Max(M.Spread, 0.75f);
	M.Damage = FMath::Min(M.Damage, 1.10f);
	M.Jump = FMath::Min(M.Jump, 1.05f);
	M.Reload = FMath::Max(M.Reload, 0.85f);
	M.Regen = FMath::Min(M.Regen, 1.0f);
	M.Lifesteal = FMath::Min(M.Lifesteal, 0.f);
}

// Simple versions until plan-07b (onset, fade, load and overdose come there). They build and can be called now.
float GSItems::EffectStrength(const FGSEffectDef& Def, const FGSActiveEffect& Active, float Now)
{
	return Now < Active.EndTime ? 1.f : 0.f;
}

FGSEffectMods GSItems::CombineActive(const TArray<FGSActiveEffect>& Effects, float Now)
{
	FGSEffectMods M;
	for (const FGSActiveEffect& E : Effects)
	{
		const FGSEffectDef* Def = FindEffect(E.Id);
		if (Def && EffectStrength(*Def, E, Now) > 0.f) M.CombineRaw(Def->Mods);
	}
	ClampMods(M);
	return M;
}

float GSItems::DrugLoad(const TArray<FGSActiveEffect>& Effects, float Now)
{
	return 0.f;
}

void GSItems::ActiveVisuals(const TArray<FGSActiveEffect>& Effects, float Now, TArray<FGSVisualLayer>& Out)
{
	for (const FGSActiveEffect& E : Effects)
	{
		const FGSEffectDef* Def = FindEffect(E.Id);
		if (!Def || Def->Mods.Visual == 0) continue;
		const float Strength = EffectStrength(*Def, E, Now);
		if (Strength > 0.f) Out.Add({ Def->Mods.Visual, Strength });
	}
}

namespace GSItemsImpl
{
	/** gs.fx.test: checks the drugs and boosts (part 1 of plan-07: caps, loot, ids). Logs PASS/FAIL per line. */
	void FxTest(const TArray<FString>& Args)
	{
		int32 Passed = 0, Run = 0;
		auto Report = [&](const TCHAR* Name, bool bOk, const FString& Detail)
		{
			++Run;
			if (bOk) ++Passed;
			UE_LOG(LogTemp, Display, TEXT("gs.fx.test %s: %s %s"), Name, bOk ? TEXT("PASS") : TEXT("FAIL"), *Detail);
		};
		auto WithinCaps = [](const FGSEffectMods& M)
		{
			const float Eps = 1e-4f;
			return M.Speed <= 1.10f + Eps && M.Taken >= 0.85f - Eps && M.StaminaDrain >= 0.5f - Eps && M.StaminaRegen <= 1.6f + Eps
				&& M.Recoil >= 0.75f - Eps && M.Spread >= 0.75f - Eps && M.Damage <= 1.10f + Eps && M.Jump <= 1.05f + Eps
				&& M.Reload >= 0.85f - Eps && M.Regen <= 1.0f + Eps && M.Lifesteal <= Eps;
		};
		auto ModsText = [](const FGSEffectMods& M)
		{
			return FString::Printf(TEXT("fart %.3f, skade tatt %.3f, stamina-bruk %.3f, stamina-henting %.3f, rekyl %.3f, spredning %.3f, skade %.3f, hopp %.3f, lading %.3f, regen %.2f, lifesteal %.2f"),
				M.Speed, M.Taken, M.StaminaDrain, M.StaminaRegen, M.Recoil, M.Spread, M.Damage, M.Jump, M.Reload, M.Regen, M.Lifesteal);
		};

		// (h) every effect at once, through Combine and through CombineActive
		{
			FGSEffectMods M;
			TArray<FGSActiveEffect> Active;
			for (const FGSEffectDef& E : Cat().Effects)
			{
				M.Combine(E.Mods);
				FGSActiveEffect& A = Active.AddDefaulted_GetRef();
				A.Id = E.Id;
				A.EndTime = 100.f;
			}
			const FGSEffectMods Stacked = GSItems::CombineActive(Active, 10.f);
			Report(TEXT("(h) tak"), WithinCaps(M) && WithinCaps(Stacked),
				FString::Printf(TEXT("alle %d effekter samtidig: %s"), Cat().Effects.Num(), *ModsText(Stacked)));
		}

		// (i) nothing with bNoLoot comes out of 20 000 draws per place
		{
			const EGSLootPlace Places[] = { EGSLootPlace::Medical, EGSLootPlace::Corridor, EGSLootPlace::Canteen, EGSLootPlace::Crate };
			int32 NoLootCount = 0, NoLootItems = 0, Draws = 0;
			FString Found;
			for (const FGSItemDef& D : GSItems::All()) if (D.bNoLoot) ++NoLootItems;
			for (EGSLootPlace P : Places)
			{
				FRandomStream Rng(777 + (int32)P);
				for (int32 I = 0; I < 20000; ++I, ++Draws)
				{
					// Max Mythic and full depth: the rarest things are possible too
					const FGSItemDef* D = GSItems::Find(GSItems::RandomLoot(Rng, EGSRarity::Common, EGSRarity::Mythic, P, (I % 2) ? 1.f : 0.f));
					if (D && D->bNoLoot) { ++NoLootCount; if (Found.Len() < 200) Found += D->Id.ToString() + TEXT(" "); }
				}
			}
			Report(TEXT("(i) ikke i loot"), NoLootItems == 13 && NoLootCount == 0,
				FString::Printf(TEXT("%d ting med bNoLoot (skal være 13), %d av %d trekninger var slike %s"), NoLootItems, NoLootCount, Draws, *Found));
		}

		// (j) the ids Kamp's code uses exist, and food and drugs never heal
		{
			static const TCHAR* Effects[] = { TEXT("Foniks"), TEXT("Glide"), TEXT("Avslort"), TEXT("Blendet"), TEXT("Matforgiftning"),
				TEXT("Smertestillende"), TEXT("Brennende"), TEXT("Sjokk"), TEXT("Adrenalin"), TEXT("Turbo") };
			static const TCHAR* Items[] = { TEXT("Plaster"), TEXT("Bandage"), TEXT("Medkit"), TEXT("SurgeryKit") };
			FString Missing;
			for (const TCHAR* Id : Effects) if (!GSItems::FindEffect(FName(Id))) Missing += FString(Id) + TEXT(" ");
			for (const TCHAR* Id : Items) if (!GSItems::Find(FName(Id))) Missing += FString(Id) + TEXT(" ");
			FString Heals;
			for (const FGSItemDef& D : GSItems::All())
			{
				if ((D.Cat == EGSItemCat::Food || D.Cat == EGSItemCat::Drug) && D.Heal != 0.f) Heals += D.Id.ToString() + TEXT(" ");
			}
			Report(TEXT("(j) Id-er og helse"), Missing.IsEmpty() && Heals.IsEmpty(),
				FString::Printf(TEXT("mangler: [%s], mat/stoff som leger: [%s]"), *Missing, *Heals));
		}

		UE_LOG(LogTemp, Display, TEXT("gs.fx.test: %d/%d OK"), Passed, Run);
	}

	static FAutoConsoleCommand FxTestCommand(TEXT("gs.fx.test"),
		TEXT("gs.fx.test: selvtest av stoffer og boosts (tak, loot, Id-er, ingen helse fra mat og stoffer)"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&FxTest));
}
