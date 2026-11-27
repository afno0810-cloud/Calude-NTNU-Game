#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GSTypes.h"
#include "GSPlayerController.generated.h"

class UInputAction;
class AGSTerminal;
class AGSSecurityCamera;
class UInputMappingContext;
class AGSCharacter;

/** Input actions are created in code so the project needs no input assets. */
UCLASS()
class GLOSHAUGEN_API AGSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGSPlayerController();

	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetPawn(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* P) override;

	// --- Death cam and killer card ---
	/** Server -> the player who died: look at your body, turn toward the killer, show who killed you and how. */
	UFUNCTION(Client, Reliable) void ClientDeathCam(AGSCharacter* Body, APawn* KillerPawn, const FGSDeathInfo& Info);
	FGSDeathInfo DeathInfo;
	float LocalDeathTime = -100.f;   // local GetTimeSeconds() when you died
	bool bHasDeathInfo = false;      // false until the first death, and again once you have a new body
	/** The body the death cam watches until you get a new pawn (server and owning client). */
	TWeakObjectPtr<AGSCharacter> DeathCamBody;

	void EnsureInputCreated();

	UPROPERTY() TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> LookAction;
	UPROPERTY() TObjectPtr<UInputAction> JumpAction;
	UPROPERTY() TObjectPtr<UInputAction> SprintAction;
	UPROPERTY() TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY() TObjectPtr<UInputAction> FireAction;
	UPROPERTY() TObjectPtr<UInputAction> AimAction;
	UPROPERTY() TObjectPtr<UInputAction> ReloadAction;
	UPROPERTY() TObjectPtr<UInputAction> Slot1Action;
	UPROPERTY() TObjectPtr<UInputAction> Slot2Action;
	UPROPERTY() TObjectPtr<UInputAction> Slot3Action;
	UPROPERTY() TObjectPtr<UInputAction> Slot4Action;
	UPROPERTY() TObjectPtr<UInputAction> InteractAction;
	UPROPERTY() TObjectPtr<UInputAction> UseItemAction;
	UPROPERTY() TObjectPtr<UInputAction> NextItemAction;
	UPROPERTY() TObjectPtr<UInputAction> DropItemAction;
	UPROPERTY() TObjectPtr<UInputAction> MeleeAction;
	UPROPERTY() TObjectPtr<UInputAction> InventoryAction;
	/** Mouse wheel as one axis (+ = up, - = down): flips through the backpack with the inventory open, else the weapons. */
	UPROPERTY() TObjectPtr<UInputAction> ScrollAction;
	UPROPERTY() TObjectPtr<UInputAction> ScoreboardAction;
	UPROPERTY() TObjectPtr<UInputAction> InspectAction;
	UPROPERTY() TObjectPtr<UInputAction> ReadyAction;
	UPROPERTY() TObjectPtr<UInputAction> MenuAction;

	/** Game menu (Esc, F10 or P; in the editor's Play only F10 and P): resume, host, join, graphics, resolution, ray tracing, mouse sensitivity, quit. */
	bool bMenuOpen = false;
	/** True while this menu or a GSMenu panel/dialog has the controls: every game action stops (releases still pass). */
	bool IsGameInputBlocked() const;
	/** Play in the editor: there Esc stops Play before the game sees it, so the texts say F10 instead. */
	bool IsEditorPlay() const;
	void ToggleMenu();
	void CloseMenu();
	/** A button on the menu was clicked (called by the HUD). */
	void MenuClick(FName Button);
	/** Mouse sensitivity multiplier, saved between sessions. */
	float MouseSensitivity = 1.f;
	/** Address used by the menu's "Bli med" button (the last one you joined). */
	FString LastJoinAddress = TEXT("127.0.0.1");
	int32 GetGraphicsLevel() const;
	int32 GetResolutionPercent() const;
	bool IsRayTracingOn() const;
	/** Lobby: Enter toggles ready. */
	void ToggleReady();
	UFUNCTION(Server, Reliable) void ServerSetReady(bool bReady);

	// --- PCs and 3D printers ---
	bool IsOnTerminal() const { return ActiveTerminal.IsValid(); }
	AGSTerminal* GetActiveTerminal() const { return ActiveTerminal.Get(); }
	void OpenTerminal(AGSTerminal* T);
	void CloseTerminal();
	/** A key pressed while a terminal is open ("1".."4", "Q", "F", "E", "Fire"). */
	void TerminalInput(const FString& Key);
	/** All security cameras, sorted by name. */
	TArray<AGSSecurityCamera*> GetCameras() const;
	bool IsLoggingIn() const;

	int32 TerminalMode = 0;   // PC: 0 menu, 1 cameras, 2 tracker
	int32 CameraIndex = 0;
	int32 RecipeIndex = 0;
	float TerminalOpenTime = -100.f;
	TArray<FGSTrackedPlayer> Tracked;
	float TrackedTime = -100.f;   // when the last positions came in (local time)
	float TrackUntil = -100.f;    // local time when tracking stops

	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(Server, Reliable) void ServerViewCamera(AGSTerminal* T, AGSSecurityCamera* Cam);
	UFUNCTION(Server, Reliable) void ServerTrack(AGSTerminal* T);
	UFUNCTION(Server, Reliable) void ServerPrint(AGSTerminal* T, int32 Recipe);
	UFUNCTION(Client, Reliable) void ClientTrackData(const TArray<FGSTrackedPlayer>& Data, float SecondsLeft);
	UFUNCTION(Client, Reliable) void ClientTerminalMessage(const FString& Message);

	bool bShowScoreboard = false;
	bool bShowInventory = false;

	/** Console: "Host" starts a listen server on this map. */
	UFUNCTION(Exec) void Host();
	/** Console: "Join 192.168.1.20" joins a friend's game. */
	UFUNCTION(Exec) void Join(const FString& Address);
	/** Console: "Grafikk 0".."Grafikk 4" = lav, middels, hoy, episk, film. */
	UFUNCTION(Exec) void Grafikk(int32 Level);
	/** Console: "Opplosning 50".."Opplosning 100" = how much of the screen resolution is rendered before TSR upscales it. */
	UFUNCTION(Exec) void Opplosning(int32 Percent);
	/** Console: "Straaling 1" = hardware ray tracing in Lumen (RTX), "Straaling 0" = software Lumen (faster). */
	UFUNCTION(Exec) void Straaling(int32 On);
	/** Console: "Bildefrekvens 60" = highest frame rate (30-240). Default 60, so a laptop does not run hot and throttle. Saved. */
	UFUNCTION(Exec) void Bildefrekvens(int32 Fps);
	int32 GetFrameLimit() const { return FrameLimit; }

protected:
	/** Your pawn, or your body while you are dead (what the view goes back to after a security camera). */
	AActor* GetHomeViewTarget() const;
	bool ServerCanUse(const AGSTerminal* T) const;
	void SendTrackPulse();
	TWeakObjectPtr<AGSTerminal> ActiveTerminal;
	float ServerNextTrackTime = 0.f;
	int32 TrackPulsesLeft = 0;
	FTimerHandle TrackTimer;

	/** Sets t.MaxFPS (works in the editor's Play too) and, in the packaged game, the saved user setting. */
	void ApplyFrameLimit(bool bSave, bool bFromConsole);
	int32 FrameLimit = 60;
	bool bFrameLimitSet = false;   // this controller put the cap on t.MaxFPS (taken off again in EndPlay in the editor)

	void ShowScoreboard() { bShowScoreboard = true; }
	void HideScoreboard() { bShowScoreboard = false; }
	void ToggleInventory() { bShowInventory = !bShowInventory; }
};
