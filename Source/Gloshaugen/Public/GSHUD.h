#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GSHUD.generated.h"

/** Simple canvas HUD: crosshair, health, ammo, clock, weather, kill feed and scoreboard. */
UCLASS()
class GLOSHAUGEN_API AGSHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	void DrawCrosshair(class AGSCharacter* C);
	void DrawBars(class AGSCharacter* C);
	void DrawAmmo(class AGSCharacter* C);
	void DrawTopRight();
	void DrawKillFeed();
	void DrawScoreboard();
	void DrawDeathScreen();
	void DrawHelp();

	void Text(const FString& S, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bCenter = false, bool bRight = false);
	void Box(float X, float Y, float W, float H, const FLinearColor& Color);

	float StartTime = -1.f;
};
