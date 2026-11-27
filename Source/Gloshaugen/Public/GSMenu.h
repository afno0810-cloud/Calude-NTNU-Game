#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "InputCoreTypes.h"
#include "GSMenu.generated.h"

class AHUD;
class UCanvas;
class APlayerController;
class AGSPlayerController;

/** Gives the menu's text fields real characters (æ, ø, å, capitals). Set in DefaultEngine.ini. Without it the
 *  fields still take digits, '.', ':' and A–Z from the keys. */
UCLASS()
class GLOSHAUGEN_API UGSGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()
public:
	virtual bool InputChar(FViewport* InViewport, int32 ControllerId, TCHAR Character) override;
};

/** One line of the controls overview (also for the H overview in GSHelp later). */
struct FGSControlRow { FString Label; FString Key; bool bRebindable = false; };

/**
 * Main menu, settings, key bindings, team panel and confirm dialogs. Drawn on the canvas through
 * AHUD::OnHUDPostRender; mouse and keys come through the viewport's own override hooks, so nothing in the
 * HUD or the player controller has to know about it.
 */
namespace GSMenu
{
	/** Post-render hook + network/travel failure events. Once per process; safe to call many times. */
	GLOSHAUGEN_API void EnsureHooked();
	/** Opens the main menu (also from a match). */
	GLOSHAUGEN_API void OpenMain(AGSPlayerController* PC);
	/** "Avslutte spillet?" dialog. */
	GLOSHAUGEN_API void AskQuit(AGSPlayerController* PC);
	/** The main menu covers the screen (GSHelp and the HUD stay hidden). */
	GLOSHAUGEN_API bool IsFullscreen(const APlayerController* PC);
	/** A GSMenu panel or dialog has the mouse and keyboard. */
	GLOSHAUGEN_API bool IsModal(const APlayerController* PC);
	/** The key an action is on right now, Norwegian name ("E", "Mellomrom"). Action = "IA_Interact" or own "GS_Team". */
	GLOSHAUGEN_API FString KeyName(const APlayerController* PC, FName Action);
	/** The key an action is on right now (same lookup as KeyName; also "GS_Team" and "GS_Fly"). Invalid when it has none. */
	GLOSHAUGEN_API FKey KeyFor(const APlayerController* PC, FName Action);
	/** Every control with its current key, in the order of the controls page. */
	GLOSHAUGEN_API TArray<FGSControlRow> Controls(const APlayerController* PC);
	/** Called by UGSGameViewportClient::InputChar; true = the menu took the character. */
	bool HandleChar(UGameViewportClient* VC, TCHAR Ch);
}
