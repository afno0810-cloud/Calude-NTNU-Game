#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GSHUD.generated.h"

class AGSCharacter;

/** Canvas HUD: crosshair, health/armor/hunger/thirst, ammo, backpack bar, boosts, pickups, kill feed, scoreboard, inventory. */
UCLASS()
class GLOSHAUGEN_API AGSHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** A gunshot was heard here (drawn on the radio radar if you carry a walkie-talkie). */
	void AddShotMarker(const FVector& Where);

protected:
	void DrawCrosshair(AGSCharacter* C);
	void DrawBars(AGSCharacter* C);
	void DrawAmmo(AGSCharacter* C);
	void DrawQuickbar(AGSCharacter* C);
	void DrawEffects(AGSCharacter* C);
	void DrawPrompt(AGSCharacter* C);
	void DrawUseProgress(AGSCharacter* C);
	void DrawToast(AGSCharacter* C);
	void DrawWallhack(AGSCharacter* C, float Range);
	void DrawRadar(AGSCharacter* C);
	void DrawCompass(AGSCharacter* C);
	/** Always-on minimap (player-up, rotates with you): roads, buildings with their names, loot beams, you. */
	void DrawMinimap(AGSCharacter* C);
	/** Fills a convex polygon (screen space) clipped to a rectangle. */
	void FillClipped(TArray<FVector2D> Poly, const FBox2D& Clip, const FLinearColor& Color);
	void DrawInventory(AGSCharacter* C);
	void DrawBinoculars();
	void DrawTopRight();
	void DrawKillFeed();
	void DrawScoreboard();
	void DrawDeathScreen();
	void DrawHelp();
	void DrawTerminal(AGSCharacter* C, class AGSPlayerController* PC);
	void DrawPrinter(AGSCharacter* C, class AGSPlayerController* PC, class AGSTerminal* T);
	void DrawComputer(AGSCharacter* C, class AGSPlayerController* PC, class AGSTerminal* T);
	void DrawTrackerMap(class AGSPlayerController* PC, float X, float Y, float W, float H);

	void Text(const FString& S, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bCenter = false, bool bRight = false);
	void Box(float X, float Y, float W, float H, const FLinearColor& Color);
	void Frame(float X, float Y, float W, float H, const FLinearColor& Color, float T = 2.f);
	void Bar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Color);

	float StartTime = -1.f;
	TArray<TPair<FVector, float>> Shots;
};
