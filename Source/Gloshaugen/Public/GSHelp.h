#pragma once

#include "CoreMinimal.h"

class AHUD;
class UCanvas;

/**
 * On-screen help for Capture the Flag: the status line at the top (score and where each flag is),
 * your own line under it (you carry the flag, the last flag event) and the lobby intro. Team choice is in GSMenu.
 */
namespace GSHelp
{
	/** Call once per frame for the local player's HUD: CTF status line, lobby intro. */
	GLOSHAUGEN_API void Draw(AHUD* HUD, UCanvas* Canvas);
	/** Draws through AHUD::OnHUDPostRender for as long as no HUD calls Draw itself. Safe to call many times. */
	GLOSHAUGEN_API void EnsureHooked();
}
