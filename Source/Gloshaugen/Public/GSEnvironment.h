#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSTypes.h"
#include "GSEnvironment.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class USkyAtmosphereComponent;
class UVolumetricCloudComponent;
class UInstancedStaticMeshComponent;
class UPostProcessComponent;
class AGSCampusBuilder;

/**
 * Sun, moon, sky, fog, rain and snow. Spawned locally on each client.
 * Reads weather and clock from AGSGameState so everyone sees the same thing.
 */
UCLASS(NotPlaceable)
class GLOSHAUGEN_API AGSEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AGSEnvironment();
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	TWeakObjectPtr<AGSCampusBuilder> Campus;

protected:
	void FindOrCreateSky();
	void UpdatePostProcess(float Elevation, EGSWeather Weather, EGSWeather Prev, float Blend);
	void UpdateParticles(float DeltaSeconds, UInstancedStaticMeshComponent* ISM, TArray<FVector>& Points, float Amount, bool bSnow, const FVector& CamLoc);

	UPROPERTY() TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY() TObjectPtr<UDirectionalLightComponent> Moon;
	UPROPERTY() TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY() TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY() TObjectPtr<UVolumetricCloudComponent> Clouds;
	UPROPERTY() TObjectPtr<UPostProcessComponent> PostProcess;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> RainISM;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> SnowISM;

	TArray<FVector> RainDrops;  // world positions (cm)
	TArray<FVector> SnowFlakes;
	float Time = 0.f;
	float IndoorCheckTimer = 0.f;
	float IndoorFactor = 1.f;   // 1 = outside, 0 = under a roof
	bool bIndoorTarget = false;

	static constexpr int32 NumDrops = 900;
	static constexpr float BoxHalf = 1800.f;  // cm around the camera
	static constexpr float BoxHeight = 1400.f;
};
