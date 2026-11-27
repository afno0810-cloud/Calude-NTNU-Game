#include "GSAudio.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundMix.h"
#include "Sound/ReverbEffect.h"
#include "AudioDeviceManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "HAL/IConsoleManager.h"
#include "UObject/Package.h"

// Named namespace (not anonymous): unity builds put several .cpp files together (see laerdom.md).
namespace GSAudioImpl
{
	/** Sound classes, by the start of the sound's name. */
	enum class ESnd : uint8 { Weapon, Step, Impact, Body, Blast, Radio, Ambience, Mech, Num };

	ESnd Category(const FString& N)
	{
		auto Is = [&N](const TCHAR* P) { return N.StartsWith(P, ESearchCase::CaseSensitive); };
		if (Is(TEXT("Shot_")) || Is(TEXT("Tail_")) || Is(TEXT("Bullet_"))) return ESnd::Weapon;
		if (Is(TEXT("Step_")) || Is(TEXT("Land")) || Is(TEXT("Gear_")) || Is(TEXT("Slide_"))) return ESnd::Step;
		if (Is(TEXT("Explosion_")) || Is(TEXT("Debris_"))) return ESnd::Blast;
		if (Is(TEXT("Flag_")) || Is(TEXT("Radio_")) || Is(TEXT("UI_")) || Is(TEXT("Hit_Marker")) || Is(TEXT("Ear_")) || Is(TEXT("Heartbeat"))) return ESnd::Radio;
		if (Is(TEXT("Amb_"))) return ESnd::Ambience;
		if (Is(TEXT("Hit_")) || Is(TEXT("Armor_"))) return ESnd::Body;
		if (Is(TEXT("Impact")) || Is(TEXT("Glass_")) || Is(TEXT("Door_")) || Is(TEXT("Lock_")) || Is(TEXT("Keypad_"))) return ESnd::Impact;
		return ESnd::Mech;   // shells, reload, bolt, pump, dry fire, melee swing, pickup
	}

	bool Jitter(ESnd C) { return C != ESnd::Radio && C != ESnd::Ambience; }

	USoundClass* ClassFor(ESnd C)
	{
		static USoundClass* Classes[uint8(ESnd::Num)] = {};
		USoundClass*& K = Classes[uint8(C)];
		if (K) return K;
		static const TCHAR* Names[] = { TEXT("GS_Weapon"), TEXT("GS_Step"), TEXT("GS_Impact"), TEXT("GS_Body"), TEXT("GS_Blast"), TEXT("GS_Radio"), TEXT("GS_Ambience"), TEXT("GS_Mech") };
		K = NewObject<USoundClass>(GetTransientPackage(), Names[uint8(C)], RF_Transient);
		K->AddToRoot();
		// Footsteps and the radio must always be heard (Andreas): they play even when the voice limit is reached.
		K->Properties.bAlwaysPlay = C == ESnd::Step || C == ESnd::Radio;
		K->Properties.bIsUISound = C == ESnd::Radio;
		// Own (2D) shots get a little of the room's reverb too.
		K->Properties.Default2DReverbSendAmount = C == ESnd::Weapon ? 0.25f : 0.f;
		if (GEngine)
		{
			if (FAudioDeviceManager* M = GEngine->GetAudioDeviceManager()) M->RegisterSoundClass(K);
		}
		return K;
	}

	USoundConcurrency* ConcurrencyFor(ESnd C)
	{
		static USoundConcurrency* Items[uint8(ESnd::Num)] = {};
		USoundConcurrency*& K = Items[uint8(C)];
		if (K) return K;
		int32 Max = 16;
		EMaxConcurrentResolutionRule::Type Rule = EMaxConcurrentResolutionRule::StopQuietest;
		switch (C)
		{
		case ESnd::Weapon: Max = 24; Rule = EMaxConcurrentResolutionRule::StopOldest; break;
		case ESnd::Step:   Max = 16; break;
		case ESnd::Impact: Max = 12; break;
		case ESnd::Body:   Max = 8; break;
		case ESnd::Blast:  Max = 8; Rule = EMaxConcurrentResolutionRule::StopOldest; break;
		case ESnd::Mech:   Max = 6; break;   // brass: 40 casings must never drown a footstep
		case ESnd::Radio:  Max = 8; Rule = EMaxConcurrentResolutionRule::StopOldest; break;
		default:           Max = 8; break;
		}
		K = NewObject<USoundConcurrency>(GetTransientPackage(), NAME_None, RF_Transient);
		K->AddToRoot();
		K->Concurrency.MaxCount = Max;
		K->Concurrency.ResolutionRule = Rule;
		K->Concurrency.bLimitToOwner = false;
		return K;
	}

	int32 CapFor(ESnd C) { return ConcurrencyFor(C)->Concurrency.MaxCount; }

	float PriorityFor(ESnd C)
	{
		switch (C)
		{
		case ESnd::Radio: return 4.f;
		case ESnd::Step:  return 3.f;
		case ESnd::Weapon: case ESnd::Blast: return 2.f;
		default: return 1.f;
		}
	}

	/** One shared attenuation object per range (rounded to 10 m) and occlusion on/off, alive for the session. */
	USoundAttenuation* Attenuation(float RangeM, bool bOcclude)
	{
		static TMap<int32, USoundAttenuation*> Cache;
		const int32 Key = FMath::Max(1, FMath::RoundToInt(RangeM / 10.f));
		const int32 CacheKey = Key * 2 + (bOcclude ? 1 : 0);
		if (USoundAttenuation** Found = Cache.Find(CacheKey)) return *Found;
		USoundAttenuation* A = NewObject<USoundAttenuation>(GetTransientPackage(), NAME_None, RF_Transient);
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
		// a wall between: dull and lower, never gone (the engine traces each playing sound asynchronously)
		S.bEnableOcclusion = bOcclude;
		S.OcclusionTraceChannel = ECC_Visibility;
		S.OcclusionLowPassFilterFrequency = 900.f;
		S.OcclusionVolumeAttenuation = 0.35f;   // about -9 dB
		S.OcclusionInterpolationTime = 0.15f;
		S.bUseComplexCollisionForOcclusion = false;
		// more of the room's reverb the further away the sound is
		S.bEnableReverbSend = true;
		S.ReverbSendMethod = EReverbSendMethod::Linear;
		S.ReverbWetLevelMin = 0.2f;
		S.ReverbWetLevelMax = 0.8f;
		S.ReverbDistanceMin = 300.f;
		S.ReverbDistanceMax = Key * 1000.f;
		Cache.Add(CacheKey, A);
		return A;
	}

	USoundMix* BlastMix()
	{
		static USoundMix* Mix = nullptr;
		if (Mix) return Mix;
		Mix = NewObject<USoundMix>(GetTransientPackage(), TEXT("GS_Mix_Trykkbolge"), RF_Transient);
		Mix->AddToRoot();
		// Everything but footsteps and the radio: -12 dB and muffled at 600 Hz (Andreas: steps and flag always heard).
		for (ESnd C : { ESnd::Weapon, ESnd::Impact, ESnd::Body, ESnd::Blast, ESnd::Mech, ESnd::Ambience })
		{
			FSoundClassAdjuster A;
			A.SoundClassObject = ClassFor(C);
			A.VolumeAdjuster = 0.25f;
			A.PitchAdjuster = 1.f;
			A.LowPassFilterFrequency = 600.f;
			A.bApplyToChildren = false;
			Mix->SoundClassEffects.Add(A);
		}
		Mix->FadeInTime = 0.05f;
		Mix->FadeOutTime = 1.0f;
		Mix->Duration = -1.f;   // until popped
		return Mix;
	}

	UReverbEffect* Reverb(uint8 Preset)
	{
		static UReverbEffect* Effects[5] = {};
		Preset = FMath::Min<uint8>(Preset, 4);
		UReverbEffect*& R = Effects[Preset];
		if (R) return R;
		R = NewObject<UReverbEffect>(GetTransientPackage(), NAME_None, RF_Transient);
		R->AddToRoot();
		struct FP { float Density, Diffusion, Gain, GainHF, Decay, HFRatio, ReflGain, ReflDelay, LateGain, LateDelay; };
		static const FP P[5] = {
			{ 0.8f, 0.9f, 0.40f, 0.60f, 0.5f, 0.70f, 0.50f, 0.004f, 1.0f, 0.010f },   // small room: short, soft walls eat the top
			{ 1.0f, 0.6f, 0.45f, 0.85f, 1.1f, 0.85f, 0.90f, 0.008f, 1.3f, 0.020f },   // corridor: hard, flutter, 1 s
			{ 1.0f, 1.0f, 0.50f, 0.70f, 2.4f, 0.60f, 0.40f, 0.025f, 1.4f, 0.040f },   // atrium / hall: long and dark
			{ 1.0f, 0.4f, 0.55f, 0.90f, 2.0f, 0.90f, 1.20f, 0.015f, 1.6f, 0.030f },   // stairwell: hard, ringing echoes
			{ 0.3f, 0.3f, 0.25f, 0.50f, 1.2f, 0.50f, 0.15f, 0.080f, 0.5f, 0.100f },   // outside: a few late slaps
		};
		const FP& V = P[Preset];
		R->Density = V.Density; R->Diffusion = V.Diffusion; R->Gain = V.Gain; R->GainHF = V.GainHF;
		R->DecayTime = V.Decay; R->DecayHFRatio = V.HFRatio; R->ReflectionsGain = V.ReflGain; R->ReflectionsDelay = V.ReflDelay;
		R->LateGain = V.LateGain; R->LateDelay = V.LateDelay; R->AirAbsorptionGainHF = 0.994f;
		return R;
	}

	UWorld* WorldOf(const UObject* Context)
	{
		return Context && GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
	}

	bool CanPlay(const UObject* Context)
	{
		const UWorld* W = WorldOf(Context);
		return W && W->GetNetMode() != NM_DedicatedServer;
	}

	UGSAudioWorld* Sub(const UWorld* W)
	{
		return W ? W->GetSubsystem<UGSAudioWorld>() : nullptr;
	}

	bool Listener(const UWorld* W, FVector& Out)
	{
		const APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
		if (!PC || !PC->PlayerCameraManager) return false;
		Out = PC->PlayerCameraManager->GetCameraLocation();
		return true;
	}

	USoundBase* LoadExact(const FString& Name)
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
		// The loaded copy gets its class and priority (the transient class is never saved into the asset; no Modify()).
		const ESnd C = Category(Name);
		S->SoundClassObject = ClassFor(C);
		S->Priority = PriorityFor(C);
		Cache.Add(Name, S);
		return S;
	}

	int32 VariantCount(const FString& Name)
	{
		static TMap<FString, int32> Counts;
		if (const int32* N = Counts.Find(Name)) return *N;
		int32 N = 0;
		while (N < 16 && LoadExact(FString::Printf(TEXT("%s_%d"), *Name, N + 1))) ++N;
		Counts.Add(Name, N);
		return N;
	}

	TMap<FString, int32>& LastVariants()
	{
		static TMap<FString, int32> Last;
		return Last;
	}

	void Note(const UObject* Context, ESnd C, USoundBase* S)
	{
		if (UGSAudioWorld* A = Sub(WorldOf(Context))) A->NoteStarted(FMath::Min(S->GetDuration(), 6.f) + float(uint8(C)) * 100.f);
	}

	USoundBase* Play3D(const UObject* Context, const FString& Name, const FVector& Loc, float Volume, float RangeM, float Pitch, bool bOcclude)
	{
		if (!CanPlay(Context)) return nullptr;
		const FString N = GSAudio::Pick(Name);
		USoundBase* S = N.IsEmpty() ? nullptr : LoadExact(N);
		if (!S) return nullptr;
		const ESnd C = Category(Name);
		if (Jitter(C))
		{
			Volume *= FMath::FRandRange(0.93f, 1.f);
			Pitch *= FMath::FRandRange(0.98f, 1.02f);
		}
		UGameplayStatics::PlaySoundAtLocation(Context, S, Loc, FRotator::ZeroRotator, Volume, Pitch, 0.f,
			Attenuation(RangeM, bOcclude && C != ESnd::Radio), ConcurrencyFor(C), nullptr, nullptr);
		Note(Context, C, S);
		return S;
	}

	USoundBase* Play2DImpl(const UObject* Context, const FString& Name, float Volume, float Pitch)
	{
		if (!CanPlay(Context)) return nullptr;
		const FString N = GSAudio::Pick(Name);
		USoundBase* S = N.IsEmpty() ? nullptr : LoadExact(N);
		if (!S) return nullptr;
		const ESnd C = Category(Name);
		if (Jitter(C))
		{
			Volume *= FMath::FRandRange(0.95f, 1.f);
			Pitch *= FMath::FRandRange(0.985f, 1.015f);
		}
		UGameplayStatics::PlaySound2D(Context, S, Volume, Pitch, 0.f, ConcurrencyFor(C));
		Note(Context, C, S);
		return S;
	}

	float Smooth(float A, float B, float X) { return FMath::SmoothStep(A, B, X); }

	const TCHAR* ShotKey(EGSWeaponClass Class, bool bSilent)
	{
		if (bSilent) return TEXT("Silenced");
		switch (Class)
		{
		case EGSWeaponClass::Pistol:   return TEXT("Pistol");
		case EGSWeaponClass::SMG:      return TEXT("SMG");
		case EGSWeaponClass::Sniper:   return TEXT("Sniper");
		case EGSWeaponClass::Shotgun:  return TEXT("Shotgun");
		case EGSWeaponClass::LMG:      return TEXT("LMG");
		case EGSWeaponClass::Launcher: return TEXT("Launcher");
		default:                       return TEXT("Rifle");
		}
	}

	bool Blocked(const UWorld* W, const FVector& From, const FVector& To, const UObject* Ignore)
	{
		FCollisionQueryParams P(SCENE_QUERY_STAT(GSAudioOcclusion), false);
		if (const AActor* A = Cast<AActor>(Ignore)) P.AddIgnoredActor(A);
		if (const APlayerController* PC = W->GetFirstPlayerController())
		{
			if (PC->GetPawn()) P.AddIgnoredActor(PC->GetPawn());
		}
		return W->LineTraceTestByChannel(From, To, ECC_Visibility, P);
	}

	void ActivateRoom(UWorld* W, const FGSRoom& R)
	{
		UReverbEffect* E = Reverb(R.Preset);
		// the measured decay within the preset's character (a long corridor rings longer than a short one)
		static const float Lo[5] = { 0.3f, 0.7f, 1.6f, 1.5f, 0.8f };
		static const float Hi[5] = { 0.8f, 1.4f, 3.0f, 2.8f, 1.6f };
		E->DecayTime = FMath::Clamp(R.RT60, Lo[R.Preset], Hi[R.Preset]);
		UGameplayStatics::ActivateReverbEffect(W, E, TEXT("GSRoom"), 1.f, 1.f, 0.4f);
	}
}


// ---------------------------------------------------------------------------------------------------------------------
// Names and variants

FString GSAudio::Pick(const FString& Name)
{
	using namespace GSAudioImpl;
	const int32 N = VariantCount(Name);
	if (N <= 0) return LoadExact(Name) ? Name : FString();
	int32& Last = LastVariants().FindOrAdd(Name, 0);
	int32 I = FMath::RandRange(1, N);
	if (N > 1 && I == Last) I = I % N + 1;   // never the same twice in a row
	Last = I;
	return FString::Printf(TEXT("%s_%d"), *Name, I);
}

bool GSAudio::Has(const FString& Name)
{
	using namespace GSAudioImpl;
	return VariantCount(Name) > 0 || LoadExact(Name) != nullptr;
}

USoundBase* GSAudio::Get(const FString& Name)
{
	using namespace GSAudioImpl;
	const FString N = Pick(Name);
	return N.IsEmpty() ? nullptr : LoadExact(N);
}

FString GSAudio::Variant(const TCHAR* Name, int32 Count)
{
	using namespace GSAudioImpl;
	return FString::Printf(TEXT("%s_%d"), Name, FMath::RandRange(1, FMath::Max(1, Count)));
}

// ---------------------------------------------------------------------------------------------------------------------
// Playing

void GSAudio::PlayAt(const UObject* Context, const FString& Name, const FVector& Location, float Volume, float RangeM, float Pitch)
{
	using namespace GSAudioImpl;
	Play3D(Context, Name, Location, Volume, RangeM, Pitch, true);
}

void GSAudio::PlayAtDelayed(const UObject* Context, const FString& Name, const FVector& Location, float Volume, float RangeM, float Pitch, float DelayS)
{
	using namespace GSAudioImpl;
	if (!CanPlay(Context)) return;
	UGSAudioWorld* A = Sub(WorldOf(Context));
	if (DelayS < 0.03f || !A)
	{
		Play3D(Context, Name, Location, Volume, RangeM, Pitch, true);
		return;
	}
	UGSAudioWorld::FDelayed D;
	D.At = A->GetWorld()->GetTimeSeconds() + DelayS;
	D.Name = Name;
	D.Location = Location;
	D.Volume = Volume;
	D.RangeM = RangeM;
	D.Pitch = Pitch;
	A->Enqueue(D);
}

void GSAudio::Play2D(const UObject* Context, const FString& Name, float Volume, float Pitch)
{
	using namespace GSAudioImpl;
	Play2DImpl(Context, Name, Volume, Pitch);
}

void GSAudio::PlayUI(const UObject* Context, const TCHAR* Name, float Volume)
{
	using namespace GSAudioImpl;
	Play2DImpl(Context, Name, Volume, 1.f);
}

void GSAudio::PlayRadio(const UObject* Context, const TCHAR* Name, float Volume, float Pitch)
{
	using namespace GSAudioImpl;
	USoundBase* S = Play2DImpl(Context, Name, Volume, Pitch);
	UGSAudioWorld* A = S ? Sub(WorldOf(Context)) : nullptr;
	if (!A || !Has(TEXT("Radio_Squelch"))) return;
	// the channel closes with a squelch when the message is over
	UGSAudioWorld::FDelayed D;
	D.At = A->GetWorld()->GetTimeSeconds() + FMath::Max(0.05f, S->GetDuration() / FMath::Max(0.1f, Pitch));
	D.Name = TEXT("Radio_Squelch");
	D.Volume = Volume * 0.6f;
	D.Kind = 1;
	A->Enqueue(D);
}

// ---------------------------------------------------------------------------------------------------------------------
// Shots

const TCHAR* GSAudio::ShotName(EGSWeaponClass Class, bool bSilent)
{
	using namespace GSAudioImpl;
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
	using namespace GSAudioImpl;
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
	using namespace GSAudioImpl;
	if (!CanPlay(Context)) return;
	UWorld* W = WorldOf(Context);
	const float Range = ShotRange(Class, bSilent);
	const float Pitch = FMath::FRandRange(0.97f, 1.03f);
	if (Class == EGSWeaponClass::Bow || Class == EGSWeaponClass::Melee)
	{
		PlayAt(Context, ShotName(Class, bSilent), Location, 1.f, Range, Pitch);
		return;
	}
	FVector Ear;
	const bool bEar = Listener(W, Ear);
	const float DistM = bEar ? FVector::Dist(Ear, Location) / 100.f : 0.f;
	const float Delay = DistM / 343.f;   // the bang arrives after the flash
	const FString K = ShotKey(Class, bSilent);
	const FString Close = FString::Printf(TEXT("Shot_%s_Close"), *K);

	if (!Has(Close))
	{
		// no recorded layers for this class (launcher): the single sound as before (without the old hill echo)
		PlayAtDelayed(Context, ShotName(Class, bSilent), Location, 1.f, Range, Pitch, Delay);
	}
	else
	{
		// close (crack + mechanism) under 25 m, mid 25-60 m, a dull boom beyond 60 m or behind walls; soft hand-overs
		float WClose = 1.f - Smooth(20.f, 30.f, DistM);
		float WMid = Smooth(20.f, 30.f, DistM) * (1.f - Smooth(55.f, 65.f, DistM));
		float WFar = Smooth(55.f, 65.f, DistM);
		if (bSilent)
		{
			WClose = 1.f; WMid = 0.f; WFar = 0.f;
		}
		else if (bEar && DistM > 8.f && Blocked(W, Ear, Location, Context))
		{
			WFar = FMath::Max(WFar, 0.6f);
		}
		struct FL { FString Name; float W; };
		TArray<FL, TInlineAllocator<3>> Layers;
		const FString Mid = FString::Printf(TEXT("Shot_%s_Mid"), *K);
		const FString Far = FString::Printf(TEXT("Shot_%s_Far"), *K);
		Layers.Add({ Close, WClose });
		Layers.Add({ Has(Mid) ? Mid : Close, WMid });
		Layers.Add({ Has(Far) ? Far : FString(TEXT("Shot_Distant")), WFar });
		Layers.Sort([](const FL& A, const FL& B) { return A.W > B.W; });
		for (int32 I = 0; I < 2; ++I)   // never more than two layers at once
		{
			if (Layers[I].W > 0.02f) PlayAtDelayed(Context, Layers[I].Name, Location, Layers[I].W, Range, Pitch, Delay);
		}
		if (bSilent) PlayAtDelayed(Context, TEXT("Shot_Silenced_Mech"), Location, 0.5f, 15.f, Pitch, Delay);
	}

	if (!bSilent)
	{
		// the shooter's room rings: a short slap in a group room, a long roll in the atrium, echoes in a stairwell
		const FGSRoom R = RoomAt(W, Location, Context);
		const FString Tail = FString::Printf(TEXT("Tail_%s"), RoomName(R.Preset));
		if (UGSAudioWorld* A = Sub(W))
		{
			UGSAudioWorld::FDelayed D;
			D.At = W->GetTimeSeconds() + Delay;
			D.Name = Tail;
			D.Location = Location;
			D.Volume = 0.5f + 0.3f * FMath::Clamp(R.RT60 / 3.f, 0.f, 1.f);
			D.RangeM = Range * 0.8f;
			D.Pitch = Pitch;
			D.Kind = 3;   // the tail fills the room: not muffled by the wall in between
			A->Enqueue(D);
		}
	}
}

void GSAudio::PlayOwnShot(const UObject* Context, EGSWeaponClass Class, bool bSilent, const FVector& Muzzle)
{
	using namespace GSAudioImpl;
	if (!CanPlay(Context)) return;
	const float Pitch = FMath::FRandRange(0.97f, 1.03f);
	if (Class == EGSWeaponClass::Bow || Class == EGSWeaponClass::Melee)
	{
		Play2D(Context, ShotName(Class, bSilent), 1.f, Pitch);
		return;
	}
	const FString K = ShotKey(Class, bSilent);
	const FString FP = FString::Printf(TEXT("Shot_%s_FP"), *K);
	if (Has(FP)) Play2D(Context, FP, bSilent ? 0.75f : 1.f, Pitch);
	else Play2D(Context, ShotName(Class, bSilent), bSilent ? 0.6f : 1.f, Pitch);
	const FString Mech = FString::Printf(TEXT("Shot_%s_Mech"), *K);
	if (Has(Mech)) Play2D(Context, Mech, bSilent ? 0.6f : 0.45f, Pitch);
	if (!bSilent)
	{
		// the tail of your own room comes from the muzzle, so direction and reverb fit where you stand
		UWorld* W = WorldOf(Context);
		const FGSRoom R = RoomAt(W, Muzzle, Context);
		Play3D(Context, FString::Printf(TEXT("Tail_%s"), RoomName(R.Preset)), Muzzle, 0.55f + 0.25f * FMath::Clamp(R.RT60 / 3.f, 0.f, 1.f),
			ShotRange(Class, false) * 0.8f, Pitch, false);
	}
}

FString GSAudio::MechName(EGSWeaponClass Class, const TCHAR* Part)
{
	using namespace GSAudioImpl;
	const FString P(Part);
	FString Typed;
	if (Class == EGSWeaponClass::Shotgun)
	{
		Typed = P == TEXT("Bolt") ? TEXT("Pump_Fwd") : TEXT("Shell_Load");
	}
	else
	{
		const TCHAR* Type = (Class == EGSWeaponClass::Pistol || Class == EGSWeaponClass::SMG) ? TEXT("Pistol")
			: Class == EGSWeaponClass::Sniper ? TEXT("Sniper") : TEXT("Rifle");
		Typed = FString::Printf(TEXT("%s_%s"), Part, Type);
	}
	if (Has(Typed)) return Typed;
	if (P == TEXT("Mag_Out")) return TEXT("Reload_Out");
	if (P == TEXT("Mag_In")) return TEXT("Reload_In");
	return TEXT("Slide");
}

// ---------------------------------------------------------------------------------------------------------------------
// Explosions

void GSAudio::PlayExplosion(UWorld* World, const FVector& Location, float RadiusCm, bool bFlash)
{
	using namespace GSAudioImpl;
	if (!CanPlay(World)) return;
	UGSAudioWorld* A = Sub(World);
	FVector Ear;
	const bool bEar = Listener(World, Ear);
	const float DistM = bEar ? FVector::Dist(Ear, Location) / 100.f : 100.f;
	const float Delay = DistM / 343.f;
	if (bFlash)
	{
		// stun grenade: a short sharp bang, and inside its radius a long ringing and everything muffled for 4 s
		PlayAtDelayed(World, Has(TEXT("Explosion_Close")) ? TEXT("Explosion_Close") : TEXT("Shot_Pistol"), Location, 0.6f, 300.f, 1.5f, Delay);
		if (DistM * 100.f < FMath::Max(RadiusCm, 800.f))
		{
			Play2D(World, TEXT("Ear_Ring"), 0.7f, 1.f);
			if (A)
			{
				for (int32 I = 1; I <= 2; ++I)
				{
					UGSAudioWorld::FDelayed D;
					D.At = World->GetTimeSeconds() + 1.3f * I;
					D.Name = TEXT("Ear_Ring");
					D.Volume = 0.7f - 0.2f * I;
					D.Kind = 1;
					A->Enqueue(D);
				}
				A->PushBlastMix(4.f);
			}
		}
		return;
	}
	const FGSRoom R = RoomAt(World, Location + FVector(0.f, 0.f, 50.f));
	FString Name = DistM < 15.f ? TEXT("Explosion_Close") : !R.bOutside ? TEXT("Explosion_Indoor") : TEXT("Explosion_Far");
	if (!Has(Name)) Name = TEXT("Shot_Launcher");
	PlayAtDelayed(World, Name, Location, 1.f, 600.f, Name == TEXT("Shot_Launcher") ? 0.6f : FMath::FRandRange(0.94f, 1.04f), Delay);
	if (A)
	{
		UGSAudioWorld::FDelayed D;
		D.At = World->GetTimeSeconds() + Delay;
		D.Name = FString::Printf(TEXT("Tail_%s"), RoomName(R.Preset));
		D.Location = Location;
		D.Volume = 1.f;
		D.RangeM = 400.f;
		D.Kind = 3;
		A->Enqueue(D);
	}
	PlayAtDelayed(World, TEXT("Debris_Fall"), Location, 0.8f, 40.f, 1.f, Delay + 0.4f);   // stones and glass coming down
	if (A && bEar && DistM < 10.f)
	{
		// the pressure wave: everything but steps and radio goes dull, the ears ring (4 s close, 1.5 s at 10 m)
		A->PushBlastMix(FMath::Lerp(4.f, 1.5f, DistM / 10.f));
		Play2D(World, TEXT("Ear_Ring"), 0.25f + 0.5f * (1.f - DistM / 10.f), 1.f);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Surfaces

FName GSAudio::SurfaceAt(const FHitResult& Hit)
{
	using namespace GSAudioImpl;
	struct FKey { const TCHAR* Word; const TCHAR* Surface; };
	if (const UPhysicalMaterial* PM = Hit.PhysMaterial.Get())
	{
		// Grafikk's physical materials (PM_GS_*), Norwegian names
		static const FKey PMs[] = { { TEXT("Stein"), TEXT("Stone") }, { TEXT("Tre"), TEXT("Wood") }, { TEXT("Stal"), TEXT("Metal") },
			{ TEXT("Betong"), TEXT("Concrete") }, { TEXT("Stoff"), TEXT("Carpet") }, { TEXT("Glass"), TEXT("Glass") },
			{ TEXT("Gips"), TEXT("Plaster") }, { TEXT("Lino"), TEXT("Lino") }, { TEXT("Gress"), TEXT("Grass") }, { TEXT("Jord"), TEXT("Dirt") } };
		const FString N = PM->GetName();
		if (N.StartsWith(TEXT("PM_GS_")))
		{
			for (const FKey& K : PMs)
			{
				if (N.Contains(K.Word)) return FName(K.Surface);
			}
		}
	}
	const UPrimitiveComponent* C = Hit.GetComponent();
	if (!C) return FName(TEXT("Concrete"));
	if (C->GetCollisionObjectType() == ECC_Destructible) return FName(TEXT("Glass"));   // Interaktiv's breakable panes
	const UMaterialInterface* M = C->GetMaterial(0);
	if (!M) return FName(TEXT("Concrete"));
	static const FKey Mats[] = {
		{ TEXT("Terrazzo"), TEXT("Stone") }, { TEXT("Marble"), TEXT("Stone") }, { TEXT("Slate"), TEXT("Stone") }, { TEXT("Stone"), TEXT("Stone") },
		{ TEXT("Paving"), TEXT("Stone") }, { TEXT("Tile"), TEXT("Stone") },
		{ TEXT("Wood"), TEXT("Wood") }, { TEXT("Parkett"), TEXT("Wood") }, { TEXT("Parquet"), TEXT("Wood") }, { TEXT("Plank"), TEXT("Wood") }, { TEXT("Bark"), TEXT("Wood") },
		{ TEXT("Lino"), TEXT("Lino") }, { TEXT("Floor"), TEXT("Lino") }, { TEXT("Rubber"), TEXT("Lino") },
		{ TEXT("Steel"), TEXT("Metal") }, { TEXT("Metal"), TEXT("Metal") }, { TEXT("Plate"), TEXT("Metal") }, { TEXT("Gunmetal"), TEXT("Metal") },
		{ TEXT("Copper"), TEXT("Metal") }, { TEXT("CarPaint"), TEXT("Metal") }, { TEXT("Chrome"), TEXT("Metal") },
		{ TEXT("Fabric"), TEXT("Carpet") }, { TEXT("Carpet"), TEXT("Carpet") }, { TEXT("Leather"), TEXT("Carpet") },
		{ TEXT("Grass"), TEXT("Grass") }, { TEXT("Leaves"), TEXT("Grass") }, { TEXT("Needles"), TEXT("Grass") },
		{ TEXT("Soil"), TEXT("Dirt") }, { TEXT("Gravel"), TEXT("Dirt") }, { TEXT("Tarp"), TEXT("Dirt") },
		{ TEXT("Glass"), TEXT("Glass") }, { TEXT("Screen"), TEXT("Glass") },
		{ TEXT("Plaster"), TEXT("Plaster") }, { TEXT("Paint"), TEXT("Plaster") }, { TEXT("Plastic"), TEXT("Plaster") }, { TEXT("Trim"), TEXT("Plaster") },
	};
	const FString N = M->GetName();
	for (const FKey& K : Mats)
	{
		if (N.Contains(K.Word)) return FName(K.Surface);
	}
	return FName(TEXT("Concrete"));   // concrete, brick, asphalt and anything unknown
}

FName GSAudio::SurfaceBelow(const AActor* Self, const FVector& Feet)
{
	using namespace GSAudioImpl;
	const UWorld* W = Self ? Self->GetWorld() : nullptr;
	if (!W) return FName(TEXT("Concrete"));
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSStep), false, Self);
	Params.bReturnPhysicalMaterial = true;
	if (W->LineTraceSingleByChannel(Hit, Feet + FVector(0.f, 0.f, 20.f), Feet - FVector(0.f, 0.f, 60.f), ECC_Visibility, Params)) return SurfaceAt(Hit);
	return FName(TEXT("Concrete"));
}

FString GSAudio::StepName(const TCHAR* Kind, FName Surface)
{
	using namespace GSAudioImpl;
	const FString K(Kind);
	FString S = Surface.ToString();
	if (S == TEXT("Plaster")) S = TEXT("Concrete");
	if (S == TEXT("Dirt")) S = TEXT("Grass");
	const bool bSoft = S == TEXT("Grass") || S == TEXT("Carpet");
	if (K == TEXT("Slide")) return bSoft ? TEXT("Slide_Soft") : TEXT("Slide_Hard");
	const FString Typed = FString::Printf(TEXT("%s_%s"), Kind, *S);
	if (Has(Typed)) return Typed;
	if (K == TEXT("Step")) return S == TEXT("Grass") ? TEXT("Step_Grass") : TEXT("Step_Hard");
	return K;   // "Land"
}

// ---------------------------------------------------------------------------------------------------------------------
// Rooms

const TCHAR* GSAudio::RoomName(uint8 Preset)
{
	using namespace GSAudioImpl;
	static const TCHAR* Names[] = { TEXT("Small"), TEXT("Corridor"), TEXT("Large"), TEXT("Stair"), TEXT("Out") };
	return Names[FMath::Min<uint8>(Preset, 4)];
}

bool GSAudio::ZoneRT60(const UWorld* World, const FVector& Location, float& OutRT60)
{
	using namespace GSAudioImpl;
	// plan-12: ask GSHelgasetr::SoundZones() (Grafikk) for the zone at Location and return its RT60.
	return false;
}

FGSRoom GSAudio::RoomAt(const UWorld* World, const FVector& Location, const UObject* Cache)
{
	using namespace GSAudioImpl;
	FGSRoom R;
	if (!World) return R;
	UGSAudioWorld* A = Sub(World);
	const float Now = World->GetTimeSeconds();
	if (Cache && A)
	{
		if (const TPair<float, FGSRoom>* Found = A->RoomCache.Find(Cache))
		{
			if (Now - Found->Key < 0.5f) return Found->Value;   // automatic fire must not trace 9 times per bullet
		}
	}
	// 8 level traces 45 degrees apart (up to 40 m), one up (15 m) and one down, against the static world only
	// (object traces: player capsules never count as walls).
	FCollisionObjectQueryParams Obj(ECC_WorldStatic);
	FCollisionQueryParams P(SCENE_QUERY_STAT(GSRoom), false);
	if (const AActor* Self = Cast<AActor>(Cache)) P.AddIgnoredActor(Self);
	FHitResult Hit;
	float D[8];
	FVector2D Pts[8];
	float Sum = 0.f;
	for (int32 I = 0; I < 8; ++I)
	{
		const float Ang = I * PI / 4.f;
		const FVector Dir(FMath::Cos(Ang), FMath::Sin(Ang), 0.f);
		D[I] = World->LineTraceSingleByObjectType(Hit, Location, Location + Dir * 4000.f, Obj, P) ? Hit.Distance : 4000.f;
		Pts[I] = FVector2D(Dir) * D[I] / 100.f;
		Sum += D[I];
	}
	const bool bCeiling = World->LineTraceSingleByObjectType(Hit, Location, Location + FVector(0.f, 0.f, 1500.f), Obj, P);
	const float Up = bCeiling ? Hit.Distance : 1500.f;
	const float Down = World->LineTraceSingleByObjectType(Hit, Location, Location - FVector(0.f, 0.f, 500.f), Obj, P) ? Hit.Distance : 160.f;
	float Area = 0.f, Perim = 0.f;
	for (int32 I = 0; I < 8; ++I)
	{
		const FVector2D& P0 = Pts[I];
		const FVector2D& P1 = Pts[(I + 1) % 8];
		Area += P0.X * P1.Y - P1.X * P0.Y;
		Perim += FVector2D::Distance(P0, P1);
	}
	R.AreaM2 = FMath::Abs(Area) * 0.5f;
	R.HeightM = (Up + Down) / 100.f;
	R.LongM = 0.f;
	R.ShortM = 1000.f;
	for (int32 I = 0; I < 4; ++I)
	{
		const float Wd = (D[I] + D[I + 4]) / 100.f;
		R.LongM = FMath::Max(R.LongM, Wd);
		R.ShortM = FMath::Min(R.ShortM, Wd);
	}
	R.bOutside = !bCeiling || Sum / 8.f > 3000.f;
	// Sabine: RT60 = 0.161 V / (alpha S), with an average absorption of 0.1 (hard school interiors)
	const float V = R.AreaM2 * R.HeightM;
	const float S = 2.f * R.AreaM2 + Perim * R.HeightM;
	R.RT60 = R.bOutside ? 1.2f : FMath::Clamp(0.161f * V / (0.1f * FMath::Max(S, 1.f)), 0.3f, 3.f);
	float Zone = 0.f;
	if (ZoneRT60(World, Location, Zone)) R.RT60 = Zone;
	if (R.bOutside) R.Preset = 4;
	else if (R.HeightM > 8.f && R.AreaM2 < 60.f) R.Preset = 3;                                 // stairwell
	else if (R.ShortM < 4.f && R.LongM / FMath::Max(R.ShortM, 0.5f) > 3.f) R.Preset = 1;       // corridor
	else if (R.RT60 > 1.6f || R.AreaM2 > 250.f || R.HeightM > 6.f) R.Preset = 2;               // atrium / hall
	else { R.Preset = 0; R.RT60 = FMath::Min(R.RT60, 0.8f); }                                 // small room
	if (Cache && A)
	{
		if (A->RoomCache.Num() > 64) A->RoomCache.Reset();
		A->RoomCache.Add(Cache, TPair<float, FGSRoom>(Now, R));
	}
	return R;
}

// ---------------------------------------------------------------------------------------------------------------------
// UGSAudioWorld

void UGSAudioWorld::Enqueue(const FDelayed& D)
{
	using namespace GSAudioImpl;
	if (Queue.Num() >= 64)
	{
		// the oldest sound goes (never the end of a blast mix); never more than 64 waiting
		const int32 Old = Queue.IndexOfByPredicate([](const FDelayed& Q) { return Q.Kind != 2; });
		Queue.RemoveAt(Old == INDEX_NONE ? 0 : Old);
	}
	Queue.Add(D);
	MaxQueue = FMath::Max(MaxQueue, Queue.Num());
}

void UGSAudioWorld::NoteStarted(float Encoded)
{
	using namespace GSAudioImpl;
	// Encoded = category * 100 + length (kept as one float so the header needs no enum)
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const int32 Cat = FMath::FloorToInt(Encoded / 100.f);
	Ends.Add(Now + (Encoded - Cat * 100.f) + Cat * 1000000.f);
	if (Ends.Num() > 512) Ends.RemoveAt(0, 128);
	MaxActive = FMath::Max(MaxActive, ActiveEstimate());
}

int32 UGSAudioWorld::ActiveEstimate() const
{
	using namespace GSAudioImpl;
	// sounds still playing, each category capped by its concurrency limit (as the engine does)
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	int32 Per[uint8(ESnd::Num)] = {};
	for (float E : Ends)
	{
		const int32 Cat = FMath::Clamp(FMath::FloorToInt(E / 1000000.f), 0, int32(ESnd::Num) - 1);
		if (E - Cat * 1000000.f > Now) ++Per[Cat];
	}
	int32 Total = 0;
	for (int32 C = 0; C < int32(ESnd::Num); ++C) Total += FMath::Min(Per[C], CapFor(ESnd(C)));
	return Total;
}

void UGSAudioWorld::PushBlastMix(float Seconds)
{
	using namespace GSAudioImpl;
	UWorld* W = GetWorld();
	if (!W) return;
	UGameplayStatics::PushSoundMixModifier(W, BlastMix());
	++MixPushes;
	FDelayed D;
	D.At = W->GetTimeSeconds() + Seconds;
	D.Kind = 2;
	// the end of the mix must never be dropped from a full queue: put it first in line
	Queue.Insert(D, 0);
}

void UGSAudioWorld::Tick(float DeltaTime)
{
	using namespace GSAudioImpl;
	UWorld* W = GetWorld();
	if (!W || W->GetNetMode() == NM_DedicatedServer) return;
	const float Now = W->GetTimeSeconds();

	for (int32 I = 0; I < Queue.Num();)
	{
		if (Queue[I].At > Now) { ++I; continue; }
		const FDelayed D = Queue[I];
		Queue.RemoveAt(I);
		switch (D.Kind)
		{
		case 0: Play3D(W, D.Name, D.Location, D.Volume, D.RangeM, D.Pitch, true); break;
		case 1: Play2DImpl(W, D.Name, D.Volume, D.Pitch); break;
		case 2:
			if (MixPushes > 0)
			{
				UGameplayStatics::PopSoundMixModifier(W, BlastMix());
				--MixPushes;
			}
			break;
		default: Play3D(W, D.Name, D.Location, D.Volume, D.RangeM, D.Pitch, false); break;
		}
	}

	if (Now >= NextRoom)
	{
		NextRoom = Now + 0.25f;
		FVector Ear;
		if (Listener(W, Ear))
		{
			const FGSRoom R = GSAudio::RoomAt(W, Ear);
			ListenerRoom = R;
			if (R.Preset != ActivePreset)
			{
				UE_LOG(LogTemp, Log, TEXT("GSAudio: rom %s -> %s (RT60 %.1f s, %.0f m2, takhoyde %.1f m)"),
					ActivePreset == 255 ? TEXT("-") : GSAudio::RoomName(ActivePreset), GSAudio::RoomName(R.Preset), R.RT60, R.AreaM2, R.HeightM);
				ActivePreset = R.Preset;
				ActivateRoom(W, R);
			}
		}
	}

	if (Now >= NextLog)
	{
		NextLog = Now + 10.f;
		if (MaxActive > 0 || MaxQueue > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("GSAudio: maks aktive lyder siste 10 s: %d, maks i kø: %d"), MaxActive, MaxQueue);
			MaxActive = 0;
			MaxQueue = 0;
		}
	}
}

void UGSAudioWorld::Deinitialize()
{
	using namespace GSAudioImpl;
	if (UWorld* W = GetWorld())
	{
		for (; MixPushes > 0; --MixPushes) UGameplayStatics::PopSoundMixModifier(W, BlastMix());
	}
	Queue.Reset();
	RoomCache.Reset();
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------------------------------------------------
// Self test (headless):  GSAudio.Selvtest  -> 100 shots from a point 6 m ahead of the listener over 10 s, logs results

namespace GSAudioImpl
{
	void SelfTest(const TArray<FString>& Args, UWorld* World)
	{
		if (!World) return;
		int32 Repeats = 0;
		FString Prev;
		const int32 Variants = VariantCount(TEXT("Shot_Rifle_Close"));
		for (int32 I = 0; I < 100; ++I)
		{
			const FString N = GSAudio::Pick(TEXT("Shot_Rifle_Close"));
			if (!N.IsEmpty() && N == Prev) ++Repeats;
			Prev = N;
		}
		UE_LOG(LogTemp, Log, TEXT("GSAudio Selvtest: Shot_Rifle_Close har %d varianter, samme variant to ganger på rad: %d av 100"), Variants, Repeats);
		static const TCHAR* Check[] = { TEXT("Shot_Rifle_Close"), TEXT("Shot_Rifle_Mid"), TEXT("Shot_Rifle_Far"), TEXT("Shot_Rifle_FP"),
			TEXT("Shot_Sniper_Close"), TEXT("Shot_Shotgun_Close"), TEXT("Shot_Silenced_Close"), TEXT("Tail_Corridor"), TEXT("Tail_Large"),
			TEXT("Step_Stone"), TEXT("Step_Wood"), TEXT("Step_Metal"), TEXT("Impact_Glass"), TEXT("Explosion_Close"), TEXT("Debris_Fall"),
			TEXT("Mag_In_Rifle"), TEXT("Bolt_Sniper"), TEXT("Glass_Shatter"), TEXT("Door_Kick"), TEXT("UI_Click"), TEXT("Radio_Squelch") };
		FString Missing;
		for (const TCHAR* C : Check)
		{
			if (!GSAudio::Has(C)) Missing += FString(C) + TEXT(" ");
		}
		UE_LOG(LogTemp, Log, TEXT("GSAudio Selvtest: mangler: %s"), Missing.IsEmpty() ? TEXT("ingen") : *Missing);
		FVector Ear = FVector(0.f, 0.f, 200.f);
		Listener(World, Ear);
		const FGSRoom R = GSAudio::RoomAt(World, Ear);
		UE_LOG(LogTemp, Log, TEXT("GSAudio Selvtest: rommet her: %s RT60 %.2f s, %.0f m2, takhoyde %.1f m, %.1f x %.1f m"),
			GSAudio::RoomName(R.Preset), R.RT60, R.AreaM2, R.HeightM, R.LongM, R.ShortM);
		UGSAudioWorld* A = Sub(World);
		if (!A) return;
		// queue limit: 100 far-away sounds at once must leave at most 64 waiting
		for (int32 I = 0; I < 100; ++I) GSAudio::PlayAtDelayed(World, TEXT("Shot_Rifle_Far"), Ear + FVector(30000.f, 0.f, 0.f), 0.f, 400.f, 1.f, 1.f);
		UE_LOG(LogTemp, Log, TEXT("GSAudio Selvtest: kø etter 100 forsinkede lyder: %d (maks 64)"), A->QueueNum());
		// 100 rifle shots, 10 a second, from 6 m in front of the listener
		const FVector From = Ear + FVector(600.f, 0.f, 0.f);
		TSharedRef<int32> Count = MakeShared<int32>(0);
		TSharedRef<int32> Peak = MakeShared<int32>(0);
		TSharedRef<FTimerHandle> H = MakeShared<FTimerHandle>();
		TWeakObjectPtr<UWorld> WW(World);
		World->GetTimerManager().SetTimer(*H, FTimerDelegate::CreateLambda([WW, Count, Peak, H, From]()
		{
			UWorld* W = WW.Get();
			if (!W) return;
			GSAudio::PlayShotFor(W, EGSWeaponClass::Rifle, false, From);
			if (UGSAudioWorld* S = Sub(W)) *Peak = FMath::Max(*Peak, S->ActiveEstimate());
			if (++*Count >= 100)
			{
				W->GetTimerManager().ClearTimer(*H);
				if (UGSAudioWorld* S = Sub(W))
				{
					UE_LOG(LogTemp, Log, TEXT("GSAudio Selvtest: 100 skudd ferdig. Maks aktive lyder: %d (maks 48), i kø nå: %d (maks 64)"), *Peak, S->QueueNum());
				}
			}
		}), 0.1f, true);
	}

	FAutoConsoleCommandWithWorldAndArgs GCmdAudioTest(TEXT("GSAudio.Selvtest"),
		TEXT("Lydmotoren: varianter, kø, rom og 100 skudd (logger resultatet)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SelfTest));
}
