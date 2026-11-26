#pragma once

#include "CoreMinimal.h"

class AHUD;
class UCanvas;

/**
 * On-screen help for Capture the Flag: the status line at the top (score and where each flag is),
 * your own line under it (you carry the flag, the last flag event, you came in during a match), the lobby intro,
 * the H overview "Taster og regler" (every key as the player set it, PC and 3D printer keys, the CTF rules) and
 * short hints that come up when they matter (at most twice each, remembered between sessions).
 * Team choice is in GSMenu.
 */
namespace GSHelp
{
	/** Call once per frame for the local player's HUD: CTF status line, lobby intro, H overview, hints. */
	GLOSHAUGEN_API void Draw(AHUD* HUD, UCanvas* Canvas);
	/** Draws through AHUD::OnHUDPostRender for as long as no HUD calls Draw itself. Safe to call many times. */
	GLOSHAUGEN_API void EnsureHooked();
	/** Forget which hints were shown (and that the overview was opened), so they come again. */
	GLOSHAUGEN_API void ResetHints();
}
