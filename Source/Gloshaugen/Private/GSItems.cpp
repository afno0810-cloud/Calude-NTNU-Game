#include "GSItems.h"
#include "Engine/StaticMesh.h"
#include "UObject/UObjectGlobals.h"

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

UStaticMesh* GSItems::GetMesh(FName Id)
{
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

FName GSItems::RandomLoot(FRandomStream& Rng, EGSRarity MinRarity)
{
	// How common each rarity and category is.
	static const float RarityWeight[] = { 40.f, 28.f, 17.f, 9.f, 4.5f, 1.5f };
	static const float CatWeight[] = { 24.f, 6.f, 4.f, 22.f, 10.f, 12.f, 8.f, 14.f }; // order of EGSItemCat

	float Total = 0.f;
	for (const FGSItemDef& D : All())
	{
		if (D.Rarity < MinRarity) continue;
		Total += RarityWeight[(int32)D.Rarity] * CatWeight[(int32)D.Cat];
	}
	float Pick = Rng.FRandRange(0.f, Total);
	for (const FGSItemDef& D : All())
	{
		if (D.Rarity < MinRarity) continue;
		Pick -= RarityWeight[(int32)D.Rarity] * CatWeight[(int32)D.Cat];
		if (Pick <= 0.f) return D.Id;
	}
	return All().Last().Id;
}

int32 GSItems::PickupCount(const FGSItemDef& Def, FRandomStream& Rng)
{
	if (Def.Kind == FName(TEXT("grapple"))) return Def.Charges;
	if (Def.Cat == EGSItemCat::Throwable) return Rng.RandRange(1, 2);
	if (Def.Cat == EGSItemCat::Food && Def.Rarity <= EGSRarity::Uncommon) return Rng.RandRange(1, 2);
	return 1;
}
