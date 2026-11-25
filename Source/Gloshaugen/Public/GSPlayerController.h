#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GSTypes.h"
#include "GSPlayerController.generated.h"

class UInputAction;
class AGSTerminal;
class AGSSecurityCamera;
class UInputMappingContext;

/** Input actions are created in code so the project needs no input assets. */
UCLASS()
class GLOSHAUGEN_API AGSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGSPlayerController();

	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;

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
	UPROPERTY() TObjectPtr<UInputAction> NextWeaponAction;
	UPROPERTY() TObjectPtr<UInputAction> ScoreboardAction;
	UPROPERTY() TObjectPtr<UInputAction> InspectAction;
	UPROPERTY() TObjectPtr<UInputAction> ReadyAction;
	UPROPERTY() TObjectPtr<UInputAction> MenuAction;

	/** Game menu (Esc or F10): resume, host, join, graphics, resolution, ray tracing, mouse sensitivity, quit. */
	bool bMenuOpen = false;
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

protected:
	bool ServerCanUse(const AGSTerminal* T) const;
	void SendTrackPulse();
	TWeakObjectPtr<AGSTerminal> ActiveTerminal;
	float ServerNextTrackTime = 0.f;
	int32 TrackPulsesLeft = 0;
	FTimerHandle TrackTimer;

	void ShowScoreboard() { bShowScoreboard = true; }
	void HideScoreboard() { bShowScoreboard = false; }
	void ToggleInventory() { bShowInventory = !bShowInventory; }
};
