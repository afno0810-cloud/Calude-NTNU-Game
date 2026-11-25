#include "GSAudio.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

namespace
{
	bool CanPlay(const UObject* Context)
	{
		const UWorld* W = Context ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
		return W && W->GetNetMode() != NM_DedicatedServer;
	}

	USoundAttenuation* Attenuation(float RangeM)
	{
		// One shared attenuation object per range (rounded to 10 m), kept alive for the whole session.
		static TMap<int32, USoundAttenuation*> Cache;
		const int32 Key = FMath::Max(1, FMath::RoundToInt(RangeM / 10.f));
		if (USoundAttenuation** Found = Cache.Find(Key)) return *Found;
		USoundAttenuation* A = NewObject<USoundAttenuation>();
		A->AddToRoot();
		FSoundAttenuationSettings& S = A->Attenuation;
		S.bAttenuate = true;
		S.bSpatialize = true;
		S.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
		S.AttenuationShape = EAttenuationShape::Sphere;
		S.AttenuationShapeExtents = FVector(300.f, 0.f, 0.f);
		S.FalloffDistance = Key * 1000.f;
		// air absorbs the high frequencies: far away sounds get dull
		S.bAttenuateWithLPF = true;
		S.LPFRadiusMin = Key * 150.f;
		S.LPFRadiusMax = Key * 1000.f;
		Cache.Add(Key, A);
		return A;
	}
}

USoundBase* GSAudio::Get(const FString& Name)
{
	static TMap<FString, TWeakObjectPtr<USoundBase>> Cache;
	static TSet<FString> Missing;
	if (TWeakObjectPtr<USoundBase>* Found = Cache.Find(Name))
	{
		if (Found->IsValid()) return Found->Get();
	}
	if (Missing.Contains(Name)) return nullptr;
	USoundBase* S = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Sounds/S_GS_%s.S_GS_%s"), *Name, *Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!S)
	{
		Missing.Add(Name);
		return nullptr;
	}
	Cache.Add(Name, S);
	return S;
}

void GSAudio::PlayAt(const UObject* Context, const FString& Name, const FVector& Location, float Volume, float RangeM, float Pitch)
{
	if (!CanPlay(Context)) return;
	if (USoundBase* S = Get(Name))
	{
		UGameplayStatics::PlaySoundAtLocation(Context, S, Location, Volume, Pitch, 0.f, Attenuation(RangeM));
	}
}

void GSAudio::Play2D(const UObject* Context, const FString& Name, float Volume, float Pitch)
{
	if (!CanPlay(Context)) return;
	if (USoundBase* S = Get(Name))
	{
		UGameplayStatics::PlaySound2D(Context, S, Volume, Pitch);
	}
}

FString GSAudio::Variant(const TCHAR* Name, int32 Count)
{
	return FString::Printf(TEXT("%s_%d"), Name, FMath::RandRange(1, FMath::Max(1, Count)));
}

const TCHAR* GSAudio::ShotName(EGSWeaponClass Class, bool bSilent)
{
	if (bSilent) return TEXT("Shot_Silenced");
	switch (Class)
	{
	case EGSWeaponClass::Pistol:   return TEXT("Shot_Pistol");
	case EGSWeaponClass::SMG:      return TEXT("Shot_SMG");
	case EGSWeaponClass::Sniper:   return TEXT("Shot_Sniper");
	case EGSWeaponClass::Shotgun:  return TEXT("Shot_Shotgun");
	case EGSWeaponClass::LMG:      return TEXT("Shot_LMG");
	case EGSWeaponClass::Launcher: return TEXT("Shot_Launcher");
	case EGSWeaponClass::Bow:      return TEXT("Melee_Swing");
	case EGSWeaponClass::Melee:    return TEXT("Melee_Swing");
	default:                       return TEXT("Shot_Rifle");
	}
}

float GSAudio::ShotRange(EGSWeaponClass Class, bool bSilent)
{
	if (bSilent || Class == EGSWeaponClass::Bow || Class == EGSWeaponClass::Melee) return 40.f;
	switch (Class)
	{
	case EGSWeaponClass::Pistol: case EGSWeaponClass::SMG: return 200.f;
	case EGSWeaponClass::Sniper: case EGSWeaponClass::Launcher: return 450.f;
	default: return 300.f;
	}
}

void GSAudio::PlayShotFor(const UObject* Context, EGSWeaponClass Class, bool bSilent, const FVector& Location)
{
	if (!CanPlay(Context)) return;
	const UWorld* W = GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull);
	float DistM = 0.f;
	if (const APlayerController* PC = W->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager) DistM = FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), Location) / 100.f;
	}
	const float Pitch = FMath::FRandRange(0.95f, 1.05f);
	const float Range = ShotRange(Class, bSilent);
	if (bSilent || DistM < 120.f)
	{
		PlayAt(Context, ShotName(Class, bSilent), Location, 1.f, Range, Pitch);
	}
	if (!bSilent && DistM > 60.f)
	{
		// rolling echo off the hills, louder the further away the shot is (up to the shot's range)
		const float Far = FMath::Clamp((DistM - 60.f) / 120.f, 0.f, 1.f);
		PlayAt(Context, TEXT("Shot_Distant"), Location, 0.4f + 0.6f * Far, Range * 1.5f, Pitch);
	}
}
