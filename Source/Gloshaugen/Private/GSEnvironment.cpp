#include "GSEnvironment.h"
#include "GSGameState.h"
#include "GSCampusBuilder.h"
#include "GSUtils.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/SkyAtmosphereComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	struct FWeatherLook
	{
		float SunLux;
		float SkyIntensity;
		float FogDensity;
		FLinearColor FogColor;
	};

	FWeatherLook LookFor(EGSWeather W)
	{
		switch (W)
		{
		case EGSWeather::Clear:  return { 10.f, 1.0f, 0.006f, FLinearColor(0.45f, 0.55f, 0.7f) };
		case EGSWeather::Cloudy: return { 3.5f, 0.8f, 0.02f,  FLinearColor(0.5f, 0.52f, 0.55f) };
		case EGSWeather::Fog:    return { 2.5f, 0.7f, 0.30f,  FLinearColor(0.6f, 0.62f, 0.64f) };
		case EGSWeather::Rain:   return { 1.8f, 0.6f, 0.06f,  FLinearColor(0.38f, 0.4f, 0.43f) };
		case EGSWeather::Snow:   return { 3.0f, 0.9f, 0.10f,  FLinearColor(0.7f, 0.72f, 0.76f) };
		default:                 return { 10.f, 1.0f, 0.01f,  FLinearColor(0.45f, 0.55f, 0.7f) };
		}
	}

	FWeatherLook Lerp(const FWeatherLook& A, const FWeatherLook& B, float T)
	{
		return { FMath::Lerp(A.SunLux, B.SunLux, T), FMath::Lerp(A.SkyIntensity, B.SkyIntensity, T),
			FMath::Lerp(A.FogDensity, B.FogDensity, T), FMath::Lerp(A.FogColor, B.FogColor, T) };
	}
}

AGSEnvironment::AGSEnvironment()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetAtmosphereSunLight(true);
	Sun->SetAtmosphereSunLightIndex(0);
	Sun->SetIntensity(10.f);
	Sun->SetLightColor(FLinearColor(1.f, 0.96f, 0.9f));
	Sun->SetLightSourceAngle(0.5357f);
	Sun->SetEnableLightShaftBloom(true);
	Sun->SetBloomScale(0.1f);
	Sun->SetBloomThreshold(4.f);
	Sun->bCastCloudShadows = true;
	Sun->SetCastShadows(true);

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetupAttachment(RootComponent);
	Moon->SetMobility(EComponentMobility::Movable);
	Moon->SetAtmosphereSunLight(true);
	Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetIntensity(0.f);
	Moon->SetLightColor(FLinearColor(0.6f, 0.7f, 1.f));
	Moon->SetCastShadows(false);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SourceType = ESkyLightSourceType::SLS_CapturedScene;

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(RootComponent);

	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Clouds"));
	Clouds->SetupAttachment(RootComponent);
	Clouds->SetLayerBottomAltitude(1.8f);   // km
	Clouds->SetLayerHeight(8.f);

	// Whole-world post process: realistic camera look (exposure, bloom, grain, vignette, weather colour grading)
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(RootComponent);
	PostProcess->bUnbound = true;
	{
		FPostProcessSettings& S = PostProcess->Settings;
		S.bOverride_BloomIntensity = true;            S.BloomIntensity = 0.45f;
		S.bOverride_VignetteIntensity = true;         S.VignetteIntensity = 0.32f;
		S.bOverride_FilmGrainIntensity = true;        S.FilmGrainIntensity = 0.06f;
		S.bOverride_SceneFringeIntensity = true;      S.SceneFringeIntensity = 0.12f;
		S.bOverride_MotionBlurAmount = true;          S.MotionBlurAmount = 0.25f;
		S.bOverride_MotionBlurMax = true;             S.MotionBlurMax = 2.f;
		S.bOverride_AutoExposureSpeedUp = true;       S.AutoExposureSpeedUp = 2.5f;
		S.bOverride_AutoExposureSpeedDown = true;     S.AutoExposureSpeedDown = 1.2f;
		S.bOverride_LumenSceneLightingQuality = true; S.LumenSceneLightingQuality = 1.f;
		S.bOverride_LumenFinalGatherQuality = true;   S.LumenFinalGatherQuality = 1.f;
		S.bOverride_LumenReflectionQuality = true;    S.LumenReflectionQuality = 1.f;
		S.bOverride_ColorSaturation = true;
		S.bOverride_ColorContrast = true;
		S.bOverride_ColorGain = true;
		S.bOverride_WhiteTemp = true;
	}

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(RootComponent);
	Fog->SetVolumetricFog(true);
	Fog->SetVolumetricFogScatteringDistribution(0.6f);
	Fog->SetFogHeightFalloff(0.08f);
	Fog->SetFogMaxOpacity(0.97f);

	RainISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Rain"));
	RainISM->SetupAttachment(RootComponent);
	RainISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RainISM->SetCastShadow(false);
	RainISM->SetMobility(EComponentMobility::Movable);

	SnowISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Snow"));
	SnowISM->SetupAttachment(RootComponent);
	SnowISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SnowISM->SetCastShadow(false);
	SnowISM->SetMobility(EComponentMobility::Movable);
}

void AGSEnvironment::BeginPlay()
{
	Super::BeginPlay();
	FindOrCreateSky();

	// The engine's ready-made volumetric clouds (ships with every UE5 install)
	if (UMaterialInterface* CloudMat = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		Clouds->SetMaterial(CloudMat);
	}

	RainISM->SetStaticMesh(GSUtils::Cube());
	SnowISM->SetStaticMesh(GSUtils::Cube());
	GSUtils::SetColor(RainISM, FLinearColor(0.55f, 0.6f, 0.68f));
	GSUtils::SetColor(SnowISM, FLinearColor(0.95f, 0.95f, 0.97f));

	FRandomStream Rand(7);
	for (int32 I = 0; I < NumDrops; ++I)
	{
		const FVector P(Rand.FRandRange(-BoxHalf, BoxHalf), Rand.FRandRange(-BoxHalf, BoxHalf), Rand.FRandRange(-BoxHeight * 0.3f, BoxHeight));
		RainDrops.Add(P);
		SnowFlakes.Add(P + FVector(37.f, -53.f, 0.f));
		RainISM->AddInstance(FTransform(FRotator::ZeroRotator, P, FVector::ZeroVector), true);
		SnowISM->AddInstance(FTransform(FRotator::ZeroRotator, P, FVector::ZeroVector), true);
	}
}

void AGSEnvironment::FindOrCreateSky()
{
	// The level may already contain a sun, sky or fog (e.g. the "Basic" template).
	// We drive our own, so remove the level's versions to avoid double lighting.
	TArray<AActor*> ToRemove;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* A = *It;
		if (A == this) continue;
		if (A->IsA<ADirectionalLight>() || A->IsA<ASkyLight>() || A->IsA<AExponentialHeightFog>()
			|| A->FindComponentByClass<USkyAtmosphereComponent>() || A->FindComponentByClass<UVolumetricCloudComponent>())
		{
			ToRemove.Add(A);
		}
	}
	for (AActor* A : ToRemove)
	{
		A->Destroy();
	}
}

void AGSEnvironment::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;

	const AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>();
	const float Hours = GS ? GS->GetTimeOfDayHours() : 12.f;
	const EGSWeather Weather = GS ? GS->Weather : EGSWeather::Clear;
	const EGSWeather Prev = GS ? GS->PreviousWeather : EGSWeather::Clear;
	const float Blend = GS ? GS->GetWeatherBlend() : 1.f;
	const FWeatherLook Look = Lerp(LookFor(Prev), LookFor(Weather), Blend);

	// Sun path: Trondheim, low sun. Noon at 12:00, highest ~48 degrees.
	const float Elevation = 48.f * FMath::Sin((Hours - 6.f) / 24.f * 2.f * PI);
	const float Azimuth = Hours / 24.f * 360.f + 180.f;
	Sun->SetWorldRotation(FRotator(-Elevation, Azimuth, 0.f));
	Moon->SetWorldRotation(FRotator(-FMath::Max(20.f, -Elevation), Azimuth + 180.f, 0.f));

	const float DayFactor = FMath::SmoothStep(-4.f, 8.f, Elevation);
	Sun->SetIntensity(Look.SunLux * DayFactor);
	Moon->SetIntensity(0.35f * (1.f - DayFactor) * (Weather == EGSWeather::Clear ? 1.f : 0.4f));
	SkyLight->SetIntensity(Look.SkyIntensity * FMath::Lerp(0.35f, 1.f, DayFactor));

	UpdatePostProcess(Elevation, Weather, Prev, Blend);

	Fog->SetFogDensity(Look.FogDensity);
	Fog->SetFogInscatteringColor(Look.FogColor * FMath::Lerp(0.06f, 1.f, DayFactor));

	if (AGSCampusBuilder* C = Campus.Get())
	{
		C->SetStreetLightsOn(Elevation < 4.f || Weather == EGSWeather::Fog);
	}

	// Precipitation around the local camera
	FVector CamLoc = FVector::ZeroVector;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager) CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	}

	IndoorCheckTimer -= DeltaSeconds;
	if (IndoorCheckTimer <= 0.f)
	{
		IndoorCheckTimer = 0.3f;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GSRoofCheck), false);
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) Params.AddIgnoredActor(PC->GetPawn());
		bIndoorTarget = GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, CamLoc + FVector(0.f, 0.f, 6000.f), ECC_Visibility, Params);
	}
	IndoorFactor = FMath::FInterpTo(IndoorFactor, bIndoorTarget ? 0.f : 1.f, DeltaSeconds, 3.f);

	auto Amount = [&](EGSWeather Kind)
	{
		return (Weather == Kind ? Blend : 0.f) + (Prev == Kind ? 1.f - Blend : 0.f);
	};
	UpdateParticles(DeltaSeconds, RainISM, RainDrops, Amount(EGSWeather::Rain) * IndoorFactor, false, CamLoc);
	UpdateParticles(DeltaSeconds, SnowISM, SnowFlakes, Amount(EGSWeather::Snow) * IndoorFactor, true, CamLoc);
}

void AGSEnvironment::UpdatePostProcess(float Elevation, EGSWeather Weather, EGSWeather Prev, float Blend)
{
	// Colour grading per weather, plus warm light at sunrise/sunset ("golden hour") and blue night.
	struct FGrade { float Saturation, Contrast, Temp; FLinearColor Gain; };
	auto GradeFor = [](EGSWeather W) -> FGrade
	{
		switch (W)
		{
		case EGSWeather::Clear:  return { 1.08f, 1.04f, 6300.f, FLinearColor(1.02f, 1.f, 0.97f) };
		case EGSWeather::Cloudy: return { 0.92f, 1.0f,  6800.f, FLinearColor(0.98f, 0.99f, 1.01f) };
		case EGSWeather::Fog:    return { 0.80f, 0.94f, 7000.f, FLinearColor(0.97f, 0.99f, 1.02f) };
		case EGSWeather::Rain:   return { 0.78f, 1.05f, 7200.f, FLinearColor(0.95f, 0.98f, 1.03f) };
		case EGSWeather::Snow:   return { 0.85f, 1.02f, 7600.f, FLinearColor(0.97f, 1.f, 1.05f) };
		default:                 return { 1.f, 1.f, 6500.f, FLinearColor::White };
		}
	};
	const FGrade A = GradeFor(Prev), B = GradeFor(Weather);
	float Sat = FMath::Lerp(A.Saturation, B.Saturation, Blend);
	float Con = FMath::Lerp(A.Contrast, B.Contrast, Blend);
	float Temp = FMath::Lerp(A.Temp, B.Temp, Blend);
	FLinearColor Gain = FMath::Lerp(A.Gain, B.Gain, Blend);

	const float Golden = 1.f - FMath::SmoothStep(2.f, 14.f, FMath::Abs(Elevation));   // near the horizon
	const float Night = 1.f - FMath::SmoothStep(-8.f, 2.f, Elevation);
	Temp = FMath::Lerp(Temp, 5200.f, Golden * 0.6f);
	Temp = FMath::Lerp(Temp, 8500.f, Night * 0.7f);
	Sat = FMath::Lerp(Sat, Sat * 0.7f, Night);
	Gain = FMath::Lerp(Gain, FLinearColor(1.06f, 0.98f, 0.9f), Golden * 0.5f);

	FPostProcessSettings& S = PostProcess->Settings;
	S.ColorSaturation = FVector4(Sat, Sat, Sat, 1.f);
	S.ColorContrast = FVector4(Con, Con, Con, 1.f);
	S.ColorGain = FVector4(Gain.R, Gain.G, Gain.B, 1.f);
	S.WhiteTemp = Temp;
}

void AGSEnvironment::UpdateParticles(float DeltaSeconds, UInstancedStaticMeshComponent* ISM, TArray<FVector>& Points,
	float AmountIn, bool bSnow, const FVector& CamLoc)
{
	const int32 Visible = FMath::RoundToInt(FMath::Clamp(AmountIn, 0.f, 1.f) * Points.Num());
	if (Visible == 0 && !ISM->IsVisible()) return;
	ISM->SetVisibility(Visible > 0);
	if (Visible == 0) return;

	const float Fall = bSnow ? 140.f : 1100.f;
	const FVector Wind = bSnow ? FVector(40.f, 20.f, 0.f) : FVector(80.f, 30.f, 0.f);
	const FVector Scale = bSnow ? FVector(0.035f) : FVector(0.008f, 0.008f, 0.45f);
	const FRotator Tilt = bSnow ? FRotator::ZeroRotator : FRotator(4.f, 20.f, 0.f);

	auto Wrap = [](float V, float Center, float Half)
	{
		float D = V - Center;
		const float Range = Half * 2.f;
		D = FMath::Fmod(D + Half, Range);
		if (D < 0.f) D += Range;
		return Center + D - Half;
	};

	TArray<FTransform> Transforms;
	Transforms.SetNum(Points.Num());
	for (int32 I = 0; I < Points.Num(); ++I)
	{
		FVector& P = Points[I];
		P += (Wind + FVector(0.f, 0.f, -Fall)) * DeltaSeconds;
		if (bSnow)
		{
			P.X += FMath::Sin(Time * 1.3f + I) * 30.f * DeltaSeconds;
			P.Y += FMath::Cos(Time * 1.1f + I * 0.7f) * 30.f * DeltaSeconds;
		}
		P.X = Wrap(P.X, CamLoc.X, BoxHalf);
		P.Y = Wrap(P.Y, CamLoc.Y, BoxHalf);
		if (P.Z < CamLoc.Z - BoxHeight * 0.3f || P.Z > CamLoc.Z + BoxHeight)
		{
			P.Z = CamLoc.Z + FMath::FRandRange(BoxHeight * 0.6f, BoxHeight);
		}
		Transforms[I] = FTransform(Tilt, P, I < Visible ? Scale : FVector::ZeroVector);
	}
	ISM->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
}
