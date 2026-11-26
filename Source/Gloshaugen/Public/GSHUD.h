#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GSHUD.generated.h"

class AGSCharacter;
class AGSGameState;
class APlayerState;

/** Canvas HUD: crosshair, health/armor/hunger/thirst, ammo, backpack bar, boosts, pickups, kill feed, team scoreboard,
 *  inventory, names over team mates and the radio line for flag events. The CTF status line is drawn by GSHelp. */
UCLASS()
class GLOSHAUGEN_API AGSHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** A gunshot was heard here (drawn on the radio radar if you carry a walkie-talkie). */
	void AddShotMarker(const FVector& Where);
	virtual void NotifyHitBoxClick(FName BoxName) override;

protected:
	void DrawCrosshair(AGSCharacter* C);
	/** Hit marker X around (CX, CY): white body hit, blue on armor, gold headshot, big red kill. */
	void DrawHitMarker(AGSCharacter* C, float CX, float CY);
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
	/** Lobby list with ready marks and teams, countdown, match clock and the result (which team won, or a draw). */
	void DrawMatch();
	/** Small name over team mates you can see within 25 m. Nothing is ever drawn over enemies. */
	void DrawTeamTags(AGSCharacter* C);
	/** A flag was taken, dropped, returned or captured (AGSGameState::OnFlagEvent): short radio sound and one line. */
	void HandleFlagEvent(uint8 Event, uint8 FlagTeam, const FVector& Where, APlayerState* Who);
	/** Black scope with a reticle while scoped in. */
	void DrawScope(AGSCharacter* C);
	/** Red wedges (up to 4) pointing to where recent hits came from; they turn as you turn. */
	void DrawDamageDirection(AGSCharacter* C);
	/** Being hit: blood-red screen edge (blue-grey off armor), white flash on headshots, red pulse with the
	 *  heartbeat at low health, short green edge when healed. */
	void DrawHurtOverlay(AGSCharacter* C);
	/** A glow along all four screen edges, Alpha at the edge fading to nothing Depth pixels in. */
	void EdgeGlow(const FLinearColor& Col, float Alpha, float Depth);
	/** Fills a convex polygon (screen space) clipped to a rectangle. */
	void FillClipped(TArray<FVector2D> Poly, const FBox2D& Clip, const FLinearColor& Color);
	/** Game menu (Esc): clickable buttons. */
	void DrawMenu(class AGSPlayerController* PC);
	bool MenuButton(FName Id, const FString& Label, float X, float Y, float W, float H, const FVector2D& Mouse, bool bAccent = false);
	void DrawInventory(AGSCharacter* C);
	void DrawBinoculars();
	void DrawTopRight();
	void DrawKillFeed();
	/** Two groups, one per team: team score, then captures, returns, kills, deaths and ping per player. Y < 0 = default place. */
	void DrawScoreboard(float Y = -1.f);
	void DrawDeathScreen();
	void DrawTerminal(AGSCharacter* C, class AGSPlayerController* PC);
	void DrawPrinter(AGSCharacter* C, class AGSPlayerController* PC, class AGSTerminal* T);
	void DrawComputer(AGSCharacter* C, class AGSPlayerController* PC, class AGSTerminal* T);
	void DrawTrackerMap(class AGSPlayerController* PC, float X, float Y, float W, float H);

	void Text(const FString& S, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bCenter = false, bool bRight = false);
	void Box(float X, float Y, float W, float H, const FLinearColor& Color);
	void Frame(float X, float Y, float W, float H, const FLinearColor& Color, float T = 2.f);
	void Bar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Color);

	TArray<TPair<FVector, float>> Shots;
	/** The game state whose flag events we listen to (it can arrive after the HUD on a client). */
	TWeakObjectPtr<AGSGameState> BoundGameState;
	FString RadioText;
	float RadioTime = -100.f;
	bool bRadioWarning = false;
};
