#include "GSHelp.h"
#include "GSCharacter.h"
#include "GSFlag.h"
#include "GSGameState.h"
#include "GSItems.h"
#include "GSMenu.h"
#include "GSPickup.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"
#include "GSTerminal.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "InputCoreTypes.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
	// Same look as AGSHUD::Text / Box: medium font with a black shadow, translucent tiles. Sober, no blinking.
	const FLinearColor PanelColor(0.02f, 0.025f, 0.03f, 0.78f);
	const FLinearColor TextColor(0.82f, 0.83f, 0.84f);
	const FLinearColor DimColor(0.62f, 0.64f, 0.66f);

	FLinearColor TeamText(uint8 Team)
	{
		return FMath::Lerp(GSTeam::Color(Team), FLinearColor::White, 0.45f);
	}

	float TextWidth(const FString& S, float Scale)
	{
		UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
		return Font ? Font->GetStringSize(*S) * Scale : 0.f;
	}

	float LineHeight(float Scale)
	{
		UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
		return Font ? Font->GetMaxCharHeight() * Scale : 18.f * Scale;
	}

	/** Align: 0 left, 1 center, 2 right. */
	void Text(UCanvas* Canvas, const FString& S, float X, float Y, const FLinearColor& Color, float Scale, int32 Align = 0)
	{
		UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
		if (!Font) return;
		const float W = Font->GetStringSize(*S) * Scale;
		if (Align == 1) X -= W * 0.5f;
		if (Align == 2) X -= W;
		Canvas->SetDrawColor(FColor(0, 0, 0, uint8(160 * Color.A)));
		Canvas->DrawText(Font, S, X + 1.5f, Y + 1.5f, Scale, Scale);
		Canvas->SetDrawColor(Color.ToFColor(true));
		Canvas->DrawText(Font, S, X, Y, Scale, Scale);
	}

	void Box(UCanvas* Canvas, float X, float Y, float W, float H, const FLinearColor& Color)
	{
		FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(W, H), Color);
		Tile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Tile);
	}

	const AGSFlag* FindFlag(UWorld* World, uint8 Team)
	{
		for (TActorIterator<AGSFlag> It(World); It; ++It)
		{
			if (It->Team == Team) return *It;
		}
		return nullptr;
	}

	FString FlagStatus(const AGSFlag* Flag)
	{
		if (!Flag) return FString();
		switch (Flag->State)
		{
		case EGSFlagState::Carried:
			return FString::Printf(TEXT("bæres av %s"), Flag->Carrier ? *Flag->Carrier->GetPlayerName() : TEXT("?"));
		case EGSFlagState::Dropped:
			return TEXT("ligger ute");
		default:
			return TEXT("hjemme");
		}
	}

	FString FlagNameCap(uint8 Team)
	{
		return Team == GSTeam::Red ? TEXT("Det røde flagget") : TEXT("Det blå flagget");
	}

	/** HUDs that called GSHelp::Draw themselves recently: the post-render hook leaves them alone. */
	TMap<TWeakObjectPtr<const AHUD>, uint64>& LastDirectFrame()
	{
		static TMap<TWeakObjectPtr<const AHUD>, uint64> Map;
		return Map;
	}

	// ------------------------------------------------------------------ H overview and hints: state
	// (Distinct names: a unity build can put this file next to other files' anonymous namespaces.)
	const TCHAR* const HelpIniSection = TEXT("Gloshaugen.Help");
	const FName HelpOverviewKey(TEXT("OverviewOpened"));
	constexpr int32 HelpMaxShows = 2;           // each hint (and the H reminder) at most this many times
	constexpr float HelpHintSeconds = 7.f;
	constexpr float HelpHintFade = 0.4f;
	constexpr float HelpHintPause = 6.f;        // quiet time after a hint
	constexpr float HelpCountAfter = 2.f;       // a hint counts as read once it has stood this long
	constexpr float HelpRepeatSeconds = 60.f;   // the same hint never comes back sooner
	constexpr float HelpJoinedSeconds = 10.f;

	/** Per HUD: several PIE windows share these statics. Times are World->GetTimeSeconds(). */
	struct FHelpState
	{
		bool bOpen = false; uint64 KeyFrame = 0;
		FName HintId; FString HintText; float HintStart = -100.f; bool bHintCounted = false; float NextHintAt = 0.f;
		float LastUpdate = -100.f;
		TMap<FName, float> LastShown;      // per hint: when it was last on screen (long enough to count)
		TSet<FName> SeenFalse;             // hints whose condition has been false since they were last shown
		float JoinedAt = -1.f; bool bFirstDrawDone = false; bool bJoinedMid = false;
		uint8 PhaseGroup = 255; float PhaseGroupStart = 0.f;   // 0 lobby + countdown, 1 playing, 2 ended
	};

	TMap<TWeakObjectPtr<const AHUD>, FHelpState>& HelpStates()
	{
		static TMap<TWeakObjectPtr<const AHUD>, FHelpState> Map;
		return Map;
	}

	FHelpState& HelpStateFor(const AHUD* HUD)
	{
		TMap<TWeakObjectPtr<const AHUD>, FHelpState>& Map = HelpStates();
		if (FHelpState* Found = Map.Find(HUD)) return *Found;
		if (Map.Num() > 16)
		{
			for (auto It = Map.CreateIterator(); It; ++It)
			{
				if (!It.Key().IsValid()) It.RemoveCurrent();
			}
		}
		return Map.Add(HUD, FHelpState());
	}

	/** How many times each hint was shown, saved in GameUserSettings.ini. Read once per id, not every frame. */
	TMap<FName, int32>& HelpCounts()
	{
		static TMap<FName, int32> Map;
		return Map;
	}

	int32 HelpCount(FName Id)
	{
		TMap<FName, int32>& Map = HelpCounts();
		if (const int32* N = Map.Find(Id)) return *N;
		int32 V = 0;
		if (GConfig) GConfig->GetInt(HelpIniSection, *Id.ToString(), V, GGameUserSettingsIni);
		return Map.Add(Id, FMath::Max(0, V));
	}

	void HelpCountUp(FName Id)
	{
		const int32 N = HelpCount(Id) + 1;
		HelpCounts().Add(Id, N);
		if (GConfig)
		{
			GConfig->SetInt(HelpIniSection, *Id.ToString(), N, GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);   // only when a number changes
		}
	}

	FString HelpKey(const APlayerController* PC, const TCHAR* Action, const TCHAR* Default)
	{
		const FString K = GSMenu::KeyName(PC, FName(Action));
		return K.IsEmpty() ? FString(Default) : K;   // before the mapping context exists
	}

	TArray<FString> HelpWrap(const FString& S, float Scale, float MaxW)
	{
		TArray<FString> Lines, Words;
		S.ParseIntoArray(Words, TEXT(" "), true);
		FString Line;
		for (const FString& Word : Words)
		{
			const FString Try = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
			if (!Line.IsEmpty() && TextWidth(Try, Scale) > MaxW) { Lines.Add(Line); Line = Word; }
			else Line = Try;
		}
		if (!Line.IsEmpty()) Lines.Add(Line);
		return Lines;
	}

	/** The CTF rules as they are in the game today (plan-04 adds overtime and the revealed carrier here). */
	TArray<FString> RuleLines(const AGSGameState* /*GS: overtime in plan-04*/)
	{
		TArray<FString> L;
		L.Add(TEXT("Ta motstanderens flagg ved å gå helt bort til det, og bær det til din egen base."));
		L.Add(TEXT("Du får bare poeng når ditt eget flagg står hjemme."));
		L.Add(TEXT("Bæreren går saktere."));
		L.Add(TEXT("Faller bæreren, blir flagget liggende. Motstanderne kan ta det opp; eget lag sender det hjem ved å gå bort til det."));
		L.Add(FString::Printf(TEXT("Første lag til %d erobringer vinner, ellers flest når %d minutter er gått."),
			AGSGameState::CaptureLimit, int32(AGSGameState::MatchMinutes)));
		L.Add(TEXT("Du kommer tilbake ved egen base. Det du bar, ligger igjen der du falt."));
		return L;
	}

	/** The overview "Taster og regler". bDraw = false only measures. Returns the height. */
	float LayoutOverview(UCanvas* Canvas, const APlayerController* PC, const AGSGameState* GS,
		float X, float Y, float W, float S, float RowH, bool bDraw)
	{
		const float Pad = 22.f, IX = X + Pad, IW = W - 2.f * Pad;
		const FLinearColor KeyColor(0.85f, 0.65f, 0.25f);
		const float Top = Y;
		Y += 16.f;
		if (bDraw)
		{
			Text(Canvas, TEXT("Taster og regler"), IX, Y, FLinearColor::White, 1.f);
			Text(Canvas, TEXT("H lukker"), IX + IW, Y + 3.f, DimColor, 0.75f, 2);
		}
		Y += LineHeight(1.f) + 10.f;

		// --- keys, in two columns ---
		const TArray<FGSControlRow> Rows = GSMenu::Controls(PC);
		const int32 Half = FMath::Max(1, (Rows.Num() + 1) / 2);
		const float Gap = 28.f, ColW = (IW - Gap) * 0.5f;
		if (bDraw)
		{
			for (int32 I = 0; I < Rows.Num(); ++I)
			{
				const float RX = IX + (I / Half) * (ColW + Gap);
				const float RY = Y + (I % Half) * RowH;
				const float KeyW = TextWidth(Rows[I].Key, S);
				FString Label = Rows[I].Label;
				const float Room = ColW - KeyW - 12.f;
				while (Label.Len() > 4 && TextWidth(Label, S) > Room) Label.LeftChopInline(1);   // tiny windows only
				Text(Canvas, Label, RX, RY, TextColor, S);
				Text(Canvas, Rows[I].Key, RX + ColW, RY, KeyColor, S, 2);
			}
		}
		Y += Half * RowH + 10.f;

		// --- PC and 3D printer: the keys TerminalInput really takes (it is fed by the player's own actions) ---
		const float TS = S * 0.9f, TLH = LineHeight(TS) + 3.f;
		const FString PcLine = FString::Printf(TEXT("På en PC:  %s kameraer · %s spor spillere · %s tilbake til menyen · %s / %s neste / forrige kamera · %s logg ut"),
			*HelpKey(PC, TEXT("IA_Slot1"), TEXT("1")), *HelpKey(PC, TEXT("IA_Slot2"), TEXT("2")), *HelpKey(PC, TEXT("IA_Slot3"), TEXT("3")),
			*HelpKey(PC, TEXT("IA_NextItem"), TEXT("Q")), *HelpKey(PC, TEXT("IA_UseItem"), TEXT("F")), *HelpKey(PC, TEXT("IA_Interact"), TEXT("E")));
		const FString PrinterLine = FString::Printf(TEXT("På en 3D-printer:  %s / %s neste, %s forrige (eller musehjulet) · %s eller %s print · %s gå vekk"),
			*HelpKey(PC, TEXT("IA_NextItem"), TEXT("Q")), *HelpKey(PC, TEXT("IA_Slot2"), TEXT("2")), *HelpKey(PC, TEXT("IA_Slot1"), TEXT("1")),
			*HelpKey(PC, TEXT("IA_UseItem"), TEXT("F")), *HelpKey(PC, TEXT("IA_Fire"), TEXT("Venstre musknapp")), *HelpKey(PC, TEXT("IA_Interact"), TEXT("E")));
		for (const FString* Para : { &PcLine, &PrinterLine })
		{
			for (const FString& L : HelpWrap(*Para, TS, IW))
			{
				if (bDraw) Text(Canvas, L, IX, Y, DimColor, TS);
				Y += TLH;
			}
		}
		Y += 10.f;

		// --- Capture the Flag ---
		if (bDraw) Text(Canvas, TEXT("Capture the Flag"), IX, Y, FLinearColor::White, 0.9f);
		Y += LineHeight(0.9f) + 4.f;
		const float RLH = LineHeight(S) + 3.f;
		for (const FString& Rule : RuleLines(GS))
		{
			const TArray<FString> Lines = HelpWrap(Rule, S, IW - 14.f);
			for (int32 I = 0; I < Lines.Num(); ++I)
			{
				if (bDraw)
				{
					if (I == 0) Text(Canvas, TEXT("·"), IX, Y, DimColor, S);
					Text(Canvas, Lines[I], IX + 14.f, Y, TextColor, S);
				}
				Y += RLH;
			}
		}
		return Y + 14.f - Top;
	}

	void DrawOverview(UCanvas* Canvas, const APlayerController* PC, const AGSGameState* GS)
	{
		const float SX = Canvas->SizeX, SY = Canvas->SizeY;
		const float W = FMath::Min(820.f, SX - 64.f);
		if (W < 200.f) return;
		float S = 0.8f, RowH = 22.f;
		float H = LayoutOverview(Canvas, PC, GS, 0.f, 0.f, W, S, RowH, false);
		if (H > SY - 40.f)
		{
			S = 0.72f; RowH = 19.f;
			H = LayoutOverview(Canvas, PC, GS, 0.f, 0.f, W, S, RowH, false);
		}
		const float X = (SX - W) * 0.5f, Y = FMath::Max(20.f, (SY - H) * 0.5f);
		Box(Canvas, X, Y, W, H, PanelColor.CopyWithNewOpacity(0.9f));
		LayoutOverview(Canvas, PC, GS, X, Y, W, S, RowH, true);
	}

	/** The hint on screen goes away. Before it was read (2 s) it does not count and sets no 60 s block. */
	void HelpStopHint(FHelpState& St, float Now)
	{
		if (St.HintId.IsNone()) return;
		St.NextHintAt = St.bHintCounted ? Now + HelpHintPause : Now + 1.f;
		St.HintId = NAME_None;
		St.HintText.Empty();
		St.bHintCounted = false;
	}

	/** One hint: when it applies (Cond) and whether it may come outside a running match. */
	struct FHelpHint { FName Id; bool bCond; bool bLobbyToo; };

	/** Every hint in priority order, with its condition right now. C is the live pawn. */
	void HelpHints(UWorld* World, const AGSCharacter* C, const AGSPlayerState* PS, TArray<FHelpHint>& Out)
	{
		const uint8 MyTeam = PS ? PS->Team : GSTeam::None;
		const AGSFlag* Own = MyTeam != GSTeam::None ? FindFlag(World, MyTeam) : nullptr;
		const AGSFlag* Enemy = MyTeam != GSTeam::None ? FindFlag(World, GSTeam::Other(MyTeam)) : nullptr;
		const bool bCarrying = PS && PS->CarriedFlagTeam != 0;

		const AGSPickup* P = C->GetFocusPickup();
		const FGSItemDef* PD = P ? GSItems::Find(P->ItemId) : nullptr;
		const AGSTerminal* T = C->GetFocusTerminal();

		bool bEmpty = false;
		if (const FGSWeaponSlot* W = C->GetCurrentWeapon())
		{
			const FGSWeaponStats S = C->GetStats(*W);
			// The reserve lives in the weapon slot; ammo boxes for the caliber in the sack refill it on reload.
			const bool bMore = W->Reserve > 0 || (!S.Caliber.IsNone() && C->CountItem(FName(*(FString(TEXT("Ammo_")) + S.Caliber.ToString()))) > 0);
			bEmpty = S.UsesAmmo() && W->Ammo <= 0 && bMore && !C->bReloading;
		}

		Out.Reset();
		Out.Add({ TEXT("OwnTaken"), Own && Own->State == EGSFlagState::Carried, false });
		Out.Add({ TEXT("Carry"), bCarrying, false });
		Out.Add({ TEXT("EnemyFlag"), !bCarrying && Enemy && Enemy->State != EGSFlagState::Carried
			&& FVector::Dist(Enemy->GetFlagLocation(), C->GetActorLocation()) < 3000.f, false });
		Out.Add({ TEXT("Weapon"), P && !P->bLocked && PD && PD->IsWeapon(), true });
		Out.Add({ TEXT("Item"), P && !P->bLocked && PD && !PD->IsWeapon(), true });
		Out.Add({ TEXT("Crate"), P && P->bLocked, true });
		Out.Add({ TEXT("Computer"), T && T->Kind == EGSTerminalKind::Computer, true });
		Out.Add({ TEXT("Printer"), T && T->Kind == EGSTerminalKind::Printer, true });
		Out.Add({ TEXT("Empty"), bEmpty, true });
		// LegInjury / ArmInjury: 0 fine, 1 badly shot (about 13 HP lost in the leg or 12 in the arm gives 0.3).
		Out.Add({ TEXT("Limb"), C->LegInjury > 0.3f || C->ArmInjury > 0.3f, false });
		Out.Add({ TEXT("Hurt"), C->Health < 60.f, false });
	}

	FString HelpHintText(const APlayerController* PC, FName Id)
	{
		auto K = [PC](const TCHAR* Action, const TCHAR* Default) { return HelpKey(PC, Action, Default); };
		if (Id == FName(TEXT("OwnTaken"))) return TEXT("Flagget deres er tatt. Stopp bæreren, så blir det liggende.");
		if (Id == FName(TEXT("Carry"))) return TEXT("Med flagget går du saktere. Hold deg til dekning og få følge av laget.");
		if (Id == FName(TEXT("EnemyFlag"))) return TEXT("Gå helt bort til flagget for å ta det.");
		if (Id == FName(TEXT("Weapon")))
			return FString::Printf(TEXT("%s tar våpenet. Du bærer fire. %s–%s eller musehjulet bytter."),
				*K(TEXT("IA_Interact"), TEXT("E")), *K(TEXT("IA_Slot1"), TEXT("1")), *K(TEXT("IA_Slot4"), TEXT("4")));
		if (Id == FName(TEXT("Item")))
			return FString::Printf(TEXT("%s legger tingen i sekken. %s blar, %s bruker, %s slipper."),
				*K(TEXT("IA_Interact"), TEXT("E")), *K(TEXT("IA_NextItem"), TEXT("Q")), *K(TEXT("IA_UseItem"), TEXT("F")), *K(TEXT("IA_DropItem"), TEXT("G")));
		if (Id == FName(TEXT("Crate"))) return TEXT("Låst kasse. Adgangskort åpner den med en gang, dirkesett tar noen sekunder.");
		if (Id == FName(TEXT("Computer")))
			return FString::Printf(TEXT("%s: på PC-en ser du gjennom overvåkningskameraene."), *K(TEXT("IA_Interact"), TEXT("E")));
		if (Id == FName(TEXT("Printer")))
			return FString::Printf(TEXT("%s: printeren lager utstyr av filament du finner som loot."), *K(TEXT("IA_Interact"), TEXT("E")));
		if (Id == FName(TEXT("Empty"))) return FString::Printf(TEXT("Tomt magasin. %s lader."), *K(TEXT("IA_Reload"), TEXT("R")));
		if (Id == FName(TEXT("Limb"))) return TEXT("Skadd bein gjør deg treg, skadd arm gjør siktet ustøtt. Bandasje eller førstehjelpsskrin hjelper.");
		if (Id == FName(TEXT("Hurt")))
			return FString::Printf(TEXT("Du er skadet. Velg medisin med %s og bruk den med %s. Det tar tid, så finn dekning først."),
				*K(TEXT("IA_NextItem"), TEXT("Q")), *K(TEXT("IA_UseItem"), TEXT("F")));
		return FString();
	}

	/** Picks, counts, ends and draws the hint. bAllowed = nothing covers the screen and the pawn lives. */
	void HelpUpdateHints(UCanvas* Canvas, UWorld* World, const APlayerController* PC, const AGSCharacter* C,
		const AGSPlayerState* PS, const AGSGameState* GS, FHelpState& St, float Now, bool bAllowed)
	{
		const bool bPlaying = GS->Phase == EGSMatchPhase::Playing;
		const bool bLobby = GS->Phase == EGSMatchPhase::Lobby || GS->Phase == EGSMatchPhase::Countdown;
		if (!bPlaying && !bLobby) bAllowed = false;
		// Not drawn for a while (main menu, a PC, the hook stopped): an unread hint does not count.
		if (!St.HintId.IsNone() && !St.bHintCounted && Now - St.LastUpdate > 0.5f) HelpStopHint(St, Now);
		St.LastUpdate = Now;

		TArray<FHelpHint> Hints;
		if (C && !C->bDead) HelpHints(World, C, PS, Hints);
		for (const FHelpHint& H : Hints)
		{
			if (!H.bCond) St.SeenFalse.Add(H.Id);
		}

		if (!St.HintId.IsNone())
		{
			const float Age = Now - St.HintStart;
			if (!bAllowed) { HelpStopHint(St, Now); return; }
			if (!St.bHintCounted && Age >= HelpCountAfter)
			{
				St.bHintCounted = true;
				HelpCountUp(St.HintId);
				St.LastShown.Add(St.HintId, Now);
				St.SeenFalse.Remove(St.HintId);
			}
			if (Age >= HelpHintSeconds) { HelpStopHint(St, Now); return; }
		}
		else
		{
			if (!bAllowed || Now < St.NextHintAt) return;
			for (const FHelpHint& H : Hints)
			{
				if (!H.bCond || (!bPlaying && !H.bLobbyToo) || HelpCount(H.Id) >= HelpMaxShows) continue;
				if (const float* Last = St.LastShown.Find(H.Id))
				{
					if (!St.SeenFalse.Contains(H.Id) || Now - *Last < HelpRepeatSeconds) continue;
				}
				St.HintId = H.Id;
				St.HintText = HelpHintText(PC, H.Id);
				St.HintStart = Now;
				St.bHintCounted = false;
				break;
			}
			if (St.HintId.IsNone()) return;
		}

		// --- draw: one line (two when long), centred under the middle of the screen, a quiet tile behind ---
		const float Age = Now - St.HintStart;
		const float A = FMath::Clamp(Age / HelpHintFade, 0.f, 1.f) * FMath::Clamp((HelpHintSeconds - Age) / HelpHintFade, 0.f, 1.f);
		if (A <= 0.f || St.HintText.IsEmpty()) return;
		TArray<FString> Lines;
		if (St.HintText.Len() > 70)
		{
			// Split at the sentence end nearest the middle, else at the space nearest the middle.
			const int32 Mid = St.HintText.Len() / 2;
			int32 Best = INDEX_NONE;
			for (int32 Pass = 0; Pass < 2 && Best == INDEX_NONE; ++Pass)
			{
				for (int32 I = 1; I < St.HintText.Len() - 1; ++I)
				{
					const bool bCut = St.HintText[I] == TEXT(' ') && (Pass == 1 || St.HintText[I - 1] == TEXT('.'));
					if (bCut && (Best == INDEX_NONE || FMath::Abs(I - Mid) < FMath::Abs(Best - Mid))) Best = I;
				}
			}
			if (Best != INDEX_NONE && FMath::Abs(Best - Mid) < St.HintText.Len() / 3)
			{
				Lines.Add(St.HintText.Left(Best));
				Lines.Add(St.HintText.RightChop(Best + 1));
			}
		}
		if (Lines.Num() == 0) Lines.Add(St.HintText);
		const float S = 0.85f, LH = LineHeight(S) + 2.f;
		float W = 0.f;
		for (const FString& L : Lines) W = FMath::Max(W, TextWidth(L, S));
		const float H = Lines.Num() * LH;
		const float CX = Canvas->SizeX * 0.5f, SY = Canvas->SizeY;
		// Under the pickup / PC prompt (centre + 60 .. about + 130) and above the compass and the sack bar (bottom 152 px).
		float Y = FMath::Max(SY * 0.68f, SY * 0.5f + 150.f);
		Y = FMath::Min(Y, SY - 170.f - H);
		Box(Canvas, CX - W * 0.5f - 10.f, Y - 10.f, W + 20.f, H + 20.f, PanelColor * FLinearColor(1.f, 1.f, 1.f, A));
		for (const FString& L : Lines)
		{
			Text(Canvas, L, CX, Y, TextColor.CopyWithNewOpacity(0.92f * A), S, 1);
			Y += LH;
		}
	}

	void DrawImpl(AHUD* HUD, UCanvas* Canvas)
	{
		if (!HUD || !Canvas || Canvas->SizeX <= 0) return;
		APlayerController* PC = HUD->GetOwningPlayerController();
		UWorld* World = HUD->GetWorld();
		if (!PC || !PC->IsLocalController() || !World) return;
		const AGSGameState* GS = World->GetGameState<AGSGameState>();
		AGSPlayerState* PS = PC->GetPlayerState<AGSPlayerState>();
		if (!GS) return;
		FHelpState& St = HelpStateFor(HUD);
		const float Now = World->GetTimeSeconds();

		// Came in during a match? Decided on the first draw that has a game state.
		if (!St.bFirstDrawDone)
		{
			St.bFirstDrawDone = true;
			St.bJoinedMid = GS->Phase == EGSMatchPhase::Playing;
		}
		const uint8 Group = GS->Phase == EGSMatchPhase::Playing ? 1 : (GS->Phase == EGSMatchPhase::Ended ? 2 : 0);
		if (Group != St.PhaseGroup) { St.PhaseGroup = Group; St.PhaseGroupStart = Now; }

		if (GSMenu::IsFullscreen(PC))   // the main menu covers the screen
		{
			St.bOpen = false;
			HelpUpdateHints(Canvas, World, PC, nullptr, PS, GS, St, Now, false);
			return;
		}
		const AGSPlayerController* GPC = Cast<AGSPlayerController>(PC);
		const AGSCharacter* Pawn = Cast<AGSCharacter>(PC->GetPawn());

		// --- H: the overview "Taster og regler" (not F1: that is "wireframe" in development builds) ---
		if (!GPC || GPC->bMenuOpen || GPC->IsOnTerminal() || GPC->bShowInventory || GSMenu::IsModal(PC))
		{
			St.bOpen = false;
		}
		else if (GFrameCounter != St.KeyFrame && PC->WasInputKeyJustPressed(EKeys::H))
		{
			St.KeyFrame = GFrameCounter;   // Draw and the post-render hook can both run in one frame
			St.bOpen = !St.bOpen;
			if (St.bOpen && HelpCount(HelpOverviewKey) < HelpMaxShows) HelpCountUp(HelpOverviewKey);
		}
		if (GPC && (GPC->bMenuOpen || GPC->IsOnTerminal()))
		{
			HelpUpdateHints(Canvas, World, PC, nullptr, PS, GS, St, Now, false);
			return;
		}

		const bool bLobby = GS->Phase == EGSMatchPhase::Lobby || GS->Phase == EGSMatchPhase::Countdown;
		const float CX = Canvas->SizeX * 0.5f;

		// --- 1. Status line at the top: score and where each flag is ---
		{
			const float W = 520.f, Y = 8.f;
			const float S1 = 0.9f, S2 = 0.65f;
			const float H1 = LineHeight(S1), H2 = LineHeight(S2);
			const float H = FMath::Max(34.f, 3.f + H1 + H2 + 1.f);
			Box(Canvas, CX - W * 0.5f, Y, W, H, PanelColor);
			const float Row1 = Y + 2.f, Row2 = Y + 2.f + H1;
			const float SideOffset = 70.f;   // team names sit this far from the middle
			Text(Canvas, TEXT("RØDT"), CX - SideOffset, Row1, TeamText(GSTeam::Red), S1, 2);
			Text(Canvas, TEXT("BLÅTT"), CX + SideOffset, Row1, TeamText(GSTeam::Blue), S1, 0);
			if (bLobby)
			{
				Text(Canvas, TEXT("Capture the Flag"), CX, Row1 + 1.f, TextColor, 0.75f, 1);
			}
			else
			{
				Text(Canvas, FString::Printf(TEXT("%d – %d"), GS->RedScore, GS->BlueScore), CX, Row1, FLinearColor::White, S1 * 1.1f, 1);
				Text(Canvas, FlagStatus(FindFlag(World, GSTeam::Red)), CX - SideOffset, Row2, DimColor, S2, 2);
				Text(Canvas, FlagStatus(FindFlag(World, GSTeam::Blue)), CX + SideOffset, Row2, DimColor, S2, 0);
			}

			// --- 2. Own line under the status line (below the match clock while playing) ---
			const float OwnY = GS->Phase == EGSMatchPhase::Playing ? 84.f : Y + H + 8.f;
			if (GS->Phase == EGSMatchPhase::Playing && PS)
			{
				FString Line;
				FLinearColor Col = TextColor;
				const AGSFlag* Own = FindFlag(World, PS->Team);
				int32 OwnLines = 0;   // own lines taken, so the "you came in" line goes under them
				if (PS->CarriedFlagTeam != 0)
				{
					Line = TEXT("Du bærer flagget. Bær det til basen din.");
					Col = TeamText(PS->CarriedFlagTeam);
					if (Own && Own->State != EGSFlagState::Home)
					{
						Text(Canvas, Line, CX, OwnY, Col, 0.8f, 1);
						Line = TEXT("Flagget ditt er borte. Du får ikke poeng før det er hentet hjem.");
						Col = TextColor;
						Text(Canvas, Line, CX, OwnY + LineHeight(0.8f) + 2.f, Col, 0.8f, 1);
						Line.Empty();
						OwnLines = 2;
					}
				}
				else if (GS->LastFlagEvent.Event != 0 && World->GetTimeSeconds() - GS->LastFlagEvent.LocalTime < 6.f)
				{
					const AGSGameState::FLastFlagEvent& E = GS->LastFlagEvent;
					Col = TeamText(E.FlagTeam);
					if (E.Who.IsEmpty())
					{
						// Nobody to name: the flag went home by itself, or its carrier left the game.
						Line = E.Event == GSFlagEvent::Returned ? FString::Printf(TEXT("%s er tilbake på basen"), *FlagNameCap(E.FlagTeam))
							: E.Event == GSFlagEvent::Dropped ? FString::Printf(TEXT("%s ligger ute"), *FlagNameCap(E.FlagTeam))
							: FlagNameCap(E.FlagTeam);
					}
					else
					{
						const TCHAR* Verb = E.Event == GSFlagEvent::Taken ? TEXT("tok")
							: E.Event == GSFlagEvent::Dropped ? TEXT("mistet")
							: E.Event == GSFlagEvent::Returned ? TEXT("hentet hjem")
							: TEXT("erobret");
						Line = FString::Printf(TEXT("%s %s %s"), *E.Who, Verb, GSTeam::FlagName(E.FlagTeam));
					}
				}
				if (!Line.IsEmpty()) { Text(Canvas, Line, CX, OwnY, Col, 0.8f, 1); OwnLines = 1; }

				// Came in during the match: the 10 s start once the team is known (else it would say "uten lag").
				if (St.bJoinedMid && St.JoinedAt < 0.f && PS->Team != GSTeam::None) St.JoinedAt = Now;
				if (St.JoinedAt >= 0.f && Now - St.JoinedAt < HelpJoinedSeconds)
				{
					const FString Joined = FString::Printf(TEXT("Kampen er i gang. Du er på %s. Stillingen er %d–%d."),
						*FString(GSTeam::Name(PS->Team)).ToLower(), GS->RedScore, GS->BlueScore);
					Text(Canvas, Joined, CX, OwnY + OwnLines * (LineHeight(0.8f) + 2.f), TeamText(PS->Team), 0.8f, 1);
				}
			}

			// --- 4. Match over ---
			if (GS->Phase == EGSMatchPhase::Ended)
			{
				const int32 R = GS->RedScore, B = GS->BlueScore;
				const FString Result = R > B ? FString::Printf(TEXT("Rødt lag vant %d–%d"), R, B)
					: B > R ? FString::Printf(TEXT("Blått lag vant %d–%d"), B, R)
					: FString::Printf(TEXT("Uavgjort %d–%d"), R, B);
				const FLinearColor Col = R > B ? TeamText(GSTeam::Red) : (B > R ? TeamText(GSTeam::Blue) : TextColor);
				Text(Canvas, Result, CX, OwnY, Col, 1.f, 1);
			}
		}

		// --- 3. Lobby: what Capture the Flag is, which team you are on ---
		if (bLobby && PS && !GSMenu::IsModal(PC))
		{
			const float X = 28.f, W = 400.f, S = 0.8f, LH = LineHeight(S) + 4.f;
			float Y = Canvas->SizeY * 0.30f;
			TArray<TPair<FString, FLinearColor>> Lines;
			if (GS->Phase == EGSMatchPhase::Lobby)
			{
				Lines.Emplace(FString::Printf(TEXT("%s: velg lag."), *GSMenu::KeyName(PC, TEXT("GS_Team"))), DimColor);
			}
			Lines.Emplace(TEXT("Ta motstanderens flagg ved å gå bort til det,"), TextColor);
			Lines.Emplace(TEXT("og bær det til din egen base."), TextColor);
			Lines.Emplace(TEXT("Du får bare poeng når ditt eget flagg står hjemme."), TextColor);
			Lines.Emplace(TEXT("Dør du, blir flagget liggende. Eget flagg hentes"), TextColor);
			Lines.Emplace(TEXT("hjem ved å gå bort til det."), TextColor);
			Lines.Emplace(FString::Printf(TEXT("Første lag til %d erobringer vinner, ellers flest"), AGSGameState::CaptureLimit), TextColor);
			Lines.Emplace(FString::Printf(TEXT("når %d minutter er gått."), int32(AGSGameState::MatchMinutes)), TextColor);
			Lines.Emplace(FString::Printf(TEXT("Oppvarming nå, ingen skade. Trykk %s når du er klar."), *GSMenu::KeyName(PC, TEXT("IA_Ready"))), DimColor);
			if (GS->Phase == EGSMatchPhase::Lobby && GS->PhaseEndsAt > 0.f)
			{
				Lines.Emplace(FString::Printf(TEXT("Starter av seg selv om %d s."), FMath::CeilToInt(GS->PhaseSecondsLeft())), DimColor);
			}
			Lines.Emplace(TEXT("H viser alle taster og regler."), DimColor);
			const float TitleH = LineHeight(1.f) + 8.f;
			const float H = 14.f + TitleH + LH * (Lines.Num() + 1) + 10.f;
			Box(Canvas, X, Y, W, H, PanelColor);
			Box(Canvas, X, Y, 3.f, H, GSTeam::Color(PS->Team).CopyWithNewOpacity(0.9f));
			Y += 12.f;
			Text(Canvas, TEXT("Capture the Flag"), X + 16.f, Y, FLinearColor::White, 1.f);
			Y += TitleH;
			const FString Prefix = TEXT("Du er på ");
			Text(Canvas, Prefix, X + 16.f, Y, TextColor, S);
			const FString TeamLabel = FString(GSTeam::Name(PS->Team)).ToLower();
			Text(Canvas, TeamLabel + TEXT("."), X + 16.f + TextWidth(Prefix, S), Y, PS->Team ? TeamText(PS->Team) : DimColor, S);
			Y += LH;
			for (const TPair<FString, FLinearColor>& L : Lines)
			{
				Text(Canvas, L.Key, X + 16.f, Y, L.Value, S);
				Y += LH;
			}
		}
		// The team key (L) is taken by GSMenu, which opens the team panel.

		// --- 5. Reminder "H: taster og regler": the first 45 s of the lobby and 20 s of a match, until H was used twice.
		// Bottom centre under the sack bar: the ammo box owns the bottom right corner, the health bars the bottom left.
		if (!St.bOpen && HelpCount(HelpOverviewKey) < HelpMaxShows && Group != 2
			&& Now - St.PhaseGroupStart < (Group == 0 ? 45.f : 20.f) && !GSMenu::IsModal(PC))
		{
			Text(Canvas, TEXT("H: taster og regler"), CX, Canvas->SizeY - 24.f, DimColor, 0.7f, 1);
		}

		// --- 6. The H overview over everything else here, or else the hint ---
		const bool bHintAllowed = Pawn && !Pawn->bDead && !St.bOpen && GPC && !GPC->bShowInventory && !GPC->bShowScoreboard
			&& !GSMenu::IsModal(PC);
		HelpUpdateHints(Canvas, World, PC, Pawn, PS, GS, St, Now, bHintAllowed);
		if (St.bOpen) DrawOverview(Canvas, PC, GS);
	}

	void OnPostRender(AHUD* HUD, UCanvas* Canvas)
	{
		if (!HUD || !Canvas) return;
		if (const uint64* Last = LastDirectFrame().Find(HUD))
		{
			if (GFrameCounter - *Last <= 2) return;   // the HUD draws us itself
		}
		DrawImpl(HUD, Canvas);
	}
}

namespace GSHelp
{
	void Draw(AHUD* HUD, UCanvas* Canvas)
	{
		if (!HUD) return;
		TMap<TWeakObjectPtr<const AHUD>, uint64>& Map = LastDirectFrame();
		Map.Add(HUD, GFrameCounter);
		if (Map.Num() > 16)
		{
			for (auto It = Map.CreateIterator(); It; ++It)
			{
				if (!It.Key().IsValid()) It.RemoveCurrent();
			}
		}
		DrawImpl(HUD, Canvas);
	}

	void EnsureHooked()
	{
		static bool bHooked = false;
		if (bHooked) return;
		bHooked = true;
		AHUD::OnHUDPostRender.AddStatic(&OnPostRender);
	}

	void ResetHints()
	{
		if (GConfig)
		{
			GConfig->EmptySection(HelpIniSection, GGameUserSettingsIni);   // also OverviewOpened
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		HelpCounts().Reset();
		for (auto& Pair : HelpStates())
		{
			Pair.Value.LastShown.Reset();
			Pair.Value.SeenFalse.Reset();
		}
	}
}
