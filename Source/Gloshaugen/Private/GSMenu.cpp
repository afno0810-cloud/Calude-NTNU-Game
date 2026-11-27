#include "GSMenu.h"
#include "GSCampus.h"
#include "GSExplore.h"
#include "GSGameMode.h"
#include "GSGameState.h"
#include "GSHelp.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"
#include "GSTypes.h"
#include "AudioDevice.h"
#include "AudioDeviceHandle.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Console.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/Font.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/HUD.h"
#include "GameFramework/HUDHitBox.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "InputMappingContext.h"
#include "IPAddress.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "SocketSubsystem.h"
#include "UnrealClient.h"

// A named namespace, not an anonymous one: GSHelp.cpp has helpers with the same names (Text, Box, OnPostRender),
// and a unity build puts both files in one translation unit.
namespace GSMenuImpl
{
	// ------------------------------------------------------------------ look (same family as GSHelp: sober, no blinking)
	const FLinearColor PanelColor(0.015f, 0.018f, 0.022f, 0.86f);
	const FLinearColor TextColor(0.82f, 0.83f, 0.84f);
	const FLinearColor DimColor(0.55f, 0.57f, 0.60f);
	const FLinearColor AccentColor(0.85f, 0.65f, 0.25f);
	const FLinearColor LockedColor(0.35f, 0.36f, 0.38f);
	const FLinearColor ReadyColor(0.45f, 0.7f, 0.45f);

	const TCHAR* const Sec = TEXT("Gloshaugen");
	const TCHAR* const KeySec = TEXT("Gloshaugen.Keys");
	const TCHAR* const TeamRowId = TEXT("GS_Team.L");
	const TCHAR* const FlyRowId = TEXT("GS_Fly.N");
	constexpr double ConnectTimeoutSeconds = 20.0;

	// Bots, Online and Explore are the three choices of the start menu; Goto and Light are pages inside "Test kartet".
	enum class EPage : uint8 { Root, Host, Join, Connecting, Settings, Controls, Bots, Online, Explore, Goto, Light };
	enum class EDialog : uint8 { None, Host, Join, Leave, Quit, Message, Restart };
	/** What EDialog::Host starts once confirmed. */
	enum class EStart : uint8 { Host, Bots, Explore };

	struct FHit
	{
		float X0 = 0.f, Y0 = 0.f, X1 = 0.f, Y1 = 0.f;
		FName Id;
		int32 Arg = 0;
	};

	struct FState
	{
		// --- per world (reset when World changes) ---
		TWeakObjectPtr<UWorld> World;
		bool bMainOpen = false, bMainDismissed = false, bTeamOpen = false, bTeamShownThisLobby = false;
		EGSMatchPhase LastPhase = EGSMatchPhase::Lobby;
		EPage Page = EPage::Root;
		EDialog Dialog = EDialog::None;
		FName Focus;                       // "Name" or "Address": the text field with the caret
		FString NameEdit, AddressEdit, PendingJoin;
		int32 ListenRow = INDEX_NONE;      // controls page: waiting for a new key
		FString KeyNote;                   // "J var brukt av ..., de har byttet"
		FString JoinNote;                  // "Skriv inn en adresse."
		FName DragSlider;                  // "Sens" / "Volume" while the left button is held
		float DragX = 0.f, DragW = 1.f;
		bool bWorldSetupDone = false, bNameSent = false;
		bool bModalApplied = false, bHudHidden = false;
		double TeamClickAt = -100.0;       // our last team button click (the server allows one switch per 2 s)
		TArray<FHit> Hits;                 // rebuilt every frame, read by the next click
		bool bPausedByUs = false;          // bot match / test: we paused the game while the menu is open
		EStart PendingStart = EStart::Host;
		// --- per window, kept across map changes (not touched by ResetWorld) ---
		bool bOpenMainAfterTravel = false; // "Til hovedmenyen": open the start menu in the next world, also in the editor's Play
		// --- per window (kept across map changes) ---
		FString MsgTitle, MsgText, MsgDetail;
		double MsgTime = 0.0;
		bool bMsgPending = false;          // shown as soon as there is a screen to show it on
		FString ConnectAddress;
		double ConnectStart = 0.0;
		bool bConnecting = false, bConnectFromMatch = false;
		TWeakObjectPtr<UWorld> ConnectWorld;   // the world we were in when the attempt started
		double LeavingUntil = 0.0;         // we left on purpose: no "connection lost" message
		bool bShift = false, bCtrl = false;
		FVector2D CanvasSize = FVector2D::ZeroVector;
		uint64 LastDrawFrame = 0;
		// Our input hooks on this window. The editor's "Play in selected viewport" binds its own key hook first,
		// so ours is put in front of it (the old one still runs), and checked every frame by its handle.
		FDelegateHandle KeyHook, AxisHook;
		bool bHookLive = false;            // our key hook is the one the window calls: without it nothing may be modal
		bool bCursorShown = false;         // the mouse cursor is on: without it nothing may be modal
		bool bPIENoteLogged = false;

		void ResetWorld(UWorld* NewWorld)
		{
			World = NewWorld;
			bMainOpen = bMainDismissed = bTeamOpen = bTeamShownThisLobby = false;
			LastPhase = EGSMatchPhase::Lobby;
			Page = EPage::Root;
			Dialog = EDialog::None;
			Focus = NAME_None;
			NameEdit.Empty(); AddressEdit.Empty(); PendingJoin.Empty();
			ListenRow = INDEX_NONE;
			KeyNote.Empty(); JoinNote.Empty();
			DragSlider = NAME_None;
			bWorldSetupDone = bNameSent = false;
			bModalApplied = bHudHidden = false;
			TeamClickAt = -100.0;
			Hits.Reset();
			bPausedByUs = false;           // a new world starts unpaused
			PendingStart = EStart::Host;
		}
	};

	TMap<TWeakObjectPtr<UGameViewportClient>, TSharedPtr<FState>>& States()
	{
		static TMap<TWeakObjectPtr<UGameViewportClient>, TSharedPtr<FState>> Map;
		return Map;
	}

	FState* FindState(const UGameViewportClient* VC)
	{
		if (!VC) return nullptr;
		const TSharedPtr<FState>* Found = States().Find(TWeakObjectPtr<UGameViewportClient>(const_cast<UGameViewportClient*>(VC)));
		return Found ? Found->Get() : nullptr;
	}

	FState* StateForVC(UGameViewportClient* VC)
	{
		if (!VC) return nullptr;
		if (FState* S = FindState(VC)) return S;
		TMap<TWeakObjectPtr<UGameViewportClient>, TSharedPtr<FState>>& Map = States();
		if (Map.Num() > 8)
		{
			for (auto It = Map.CreateIterator(); It; ++It)
			{
				if (!It.Key().IsValid()) It.RemoveCurrent();
			}
		}
		return Map.Add(VC, MakeShared<FState>()).Get();
	}

	/** Null on a dedicated server. */
	FState* StateFor(const UWorld* W)
	{
		return W ? StateForVC(W->GetGameViewport()) : nullptr;
	}

	bool IsModalState(const FState& S)
	{
		// Modal needs us to be drawing, our key hook to be the one the window calls (so Esc and clicks reach us) and the
		// mouse cursor to be on. If any of them is missing, no input is swallowed.
		return (S.bMainOpen || S.bTeamOpen || S.Dialog != EDialog::None) && GFrameCounter - S.LastDrawFrame <= 30
			&& S.bHookLive && S.bCursorShown;
	}

	// ------------------------------------------------------------------ the way out
	// In the editor's Play, Esc stops the whole Play session before the game sees the key (the editor's "Stop" command
	// runs first). F10 reaches the game there, and P as well (no Fn key needed on a laptop). So F10 does everything
	// Esc does, P does it when no text field has the caret, and every text names F10 instead of Esc in the editor.
	bool IsEditorPlay(const UWorld* W)
	{
		return W && W->WorldType == EWorldType::PIE;
	}

	/** Esc or F10: back / close, everywhere (also in a text field and while waiting for a new key). */
	bool IsBackKey(const FKey& K)
	{
		return K == EKeys::Escape || K == EKeys::F10;
	}

	/** "F10" in the editor's Play, "Esc" in the real game. */
	const TCHAR* BackKeyText(const UWorld* W)
	{
		return IsEditorPlay(W) ? TEXT("F10") : TEXT("Esc");
	}

	/** Every key that opens and closes the menu, for the texts. */
	const TCHAR* MenuKeysText(const UWorld* W)
	{
		return IsEditorPlay(W) ? TEXT("F10 / P") : TEXT("Esc / F10 / P");
	}

	// ------------------------------------------------------------------ config
	FString CfgString(const TCHAR* Section, const TCHAR* Key, const FString& Default = FString())
	{
		FString V;
		return (GConfig && GConfig->GetString(Section, Key, V, GGameUserSettingsIni)) ? V : Default;
	}

	void CfgFlush()
	{
		if (GConfig) GConfig->Flush(false, GGameUserSettingsIni);
	}

	float SavedVolume()
	{
		float V = 1.f;
		if (GConfig) GConfig->GetFloat(Sec, TEXT("Volume"), V, GGameUserSettingsIni);
		return FMath::Clamp(V, 0.f, 1.f);
	}

	void ApplyVolume(UWorld* W)
	{
		if (!W) return;
		if (FAudioDeviceHandle D = W->GetAudioDevice())
		{
			D->SetTransientPrimaryVolume(SavedVolume());
		}
	}

	/** Letters (also æøå), digits, space and - _ . only: no invisible or URL characters. Same rule as AGSGameMode::ChangeName. */
	bool NameChar(TCHAR Ch)
	{
		return Ch == TEXT(' ') || Ch == TEXT('-') || Ch == TEXT('_') || Ch == TEXT('.') || (Ch > 32 && FChar::IsAlnum(Ch));
	}

	FString CleanName(const FString& In)
	{
		FString Out;
		for (int32 I = 0; I < In.Len(); ++I)
		{
			const TCHAR Ch = In[I];
			if (!NameChar(Ch)) continue;
			if (Ch == TEXT(' ') && Out.EndsWith(TEXT(" "))) continue;   // one space at a time
			Out.AppendChar(Ch);
		}
		Out.TrimStartAndEndInline();
		Out.LeftInline(20);
		Out.TrimEndInline();
		return Out;
	}

	/** The saved player name; made up and saved the first time. */
	FString EnsureName()
	{
		FString Name = CleanName(CfgString(Sec, TEXT("PlayerName")));
		if (Name.IsEmpty())
		{
			Name = FString::Printf(TEXT("Spiller %d"), FMath::RandRange(100, 999));
			if (GConfig)
			{
				GConfig->SetString(Sec, TEXT("PlayerName"), *Name, GGameUserSettingsIni);
				CfgFlush();
			}
		}
		return Name;
	}

	const TArray<FString>& LocalAddresses()
	{
		static TArray<FString> Result;
		static bool bDone = false;
		if (bDone) return Result;
		bDone = true;
		if (ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
		{
			TArray<TSharedPtr<FInternetAddr>> Addrs;
			Sockets->GetLocalAdapterAddresses(Addrs);
			for (const TSharedPtr<FInternetAddr>& A : Addrs)
			{
				if (!A.IsValid()) continue;
				const FString Str = A->ToString(false);
				// IPv4 only, not loopback, not link-local
				if (Str.IsEmpty() || Str.Contains(TEXT(":")) || Str.StartsWith(TEXT("127.")) || Str.StartsWith(TEXT("169.254.")) || Str == TEXT("0.0.0.0")) continue;
				Result.AddUnique(Str);
				if (Result.Num() >= 3) break;
			}
		}
		return Result;
	}

	// ------------------------------------------------------------------ keys
	struct FKeyRow
	{
		FString Id;            // "IA_Jump.SpaceBar": action + the key the combat team gave it
		FName Action;
		int32 Map = INDEX_NONE;   // index in the mapping context; INDEX_NONE = our own (team panel)
		FKey Original;
		FKey Current;
	};

	/** The keys each mapping had before we touched it. */
	const TArray<FKey>& OriginalsFor(const UInputMappingContext* C)
	{
		static TMap<TWeakObjectPtr<const UInputMappingContext>, TArray<FKey>> Map;
		if (Map.Num() > 16)
		{
			for (auto It = Map.CreateIterator(); It; ++It)
			{
				if (!It.Key().IsValid()) It.RemoveCurrent();
			}
		}
		TArray<FKey>& Keys = Map.FindOrAdd(C);
		const TArray<FEnhancedActionKeyMapping>& Mappings = C->GetMappings();
		for (int32 I = Keys.Num(); I < Mappings.Num(); ++I) Keys.Add(Mappings[I].Key);
		return Keys;
	}

	bool IsFixedAction(FName Action)
	{
		return Action == FName(TEXT("IA_Look")) || Action == FName(TEXT("IA_Scroll")) || Action == FName(TEXT("IA_Menu"));
	}

	FKey TeamKey()
	{
		const FString Str = CfgString(KeySec, TeamRowId);
		if (!Str.IsEmpty())
		{
			const FKey K(*Str);
			if (K.IsValid() && K != EKeys::H) return K;   // H is the help overview's (an older save may have it)
		}
		return EKeys::L;
	}

	/** Free flying in "Test kartet" (GSExplore). Our own row like the team key; N unless changed. */
	FKey FlyKey()
	{
		const FString Str = CfgString(KeySec, FlyRowId);
		if (!Str.IsEmpty())
		{
			const FKey K(*Str);
			if (K.IsValid() && K != EKeys::H && K != EKeys::P && K != EKeys::Escape && K != EKeys::F10) return K;
		}
		return EKeys::N;
	}

	/** The key an action has now (after rebinding); invalid when it has none. */
	FKey ActionKey(const AGSPlayerController* PC, FName Action)
	{
		const UInputMappingContext* C = PC ? PC->MappingContext.Get() : nullptr;
		if (!C) return FKey();
		for (const FEnhancedActionKeyMapping& M : C->GetMappings())
		{
			if (M.Action && M.Action->GetFName() == Action) return M.Key;
		}
		return FKey();
	}

	/** The key is on this action now (any of its mappings, e.g. all four walking keys of IA_Move). */
	bool IsActionKey(const AGSPlayerController* PC, FName Action, const FKey& Key)
	{
		const UInputMappingContext* C = PC ? PC->MappingContext.Get() : nullptr;
		if (!C || !Key.IsValid()) return false;
		for (const FEnhancedActionKeyMapping& M : C->GetMappings())
		{
			if (M.Action && M.Action->GetFName() == Action && M.Key == Key) return true;
		}
		return false;
	}

	/** Every key that can be changed, in the combat team's mapping order, the team panel last. Reserved = keys that are taken for good. */
	void BuildRows(const AGSPlayerController* PC, TArray<FKeyRow>& Out, TArray<FKey>* Reserved = nullptr)
	{
		Out.Reset();
		if (Reserved)
		{
			Reserved->Reset();
			Reserved->Add(EKeys::Escape);
			Reserved->Add(EKeys::H);   // opens "Taster og regler" (GSHelp)
			Reserved->Add(EKeys::P);   // opens and closes the menu (also in the editor's Play)
			Reserved->Add(EKeys::F10);
		}
		const UInputMappingContext* C = PC ? PC->MappingContext.Get() : nullptr;
		if (C)
		{
			const TArray<FKey>& Orig = OriginalsFor(C);
			const TArray<FEnhancedActionKeyMapping>& Mappings = C->GetMappings();
			for (int32 I = 0; I < Mappings.Num() && I < Orig.Num(); ++I)
			{
				const FName Action = Mappings[I].Action ? Mappings[I].Action->GetFName() : NAME_None;
				const FKey& O = Orig[I];
				if (Action == NAME_None || IsFixedAction(Action) || O.IsAxis1D() || O.IsAxis2D() || O.IsAxis3D())
				{
					if (Reserved) Reserved->AddUnique(O);
					continue;
				}
				FKeyRow Row;
				Row.Action = Action;
				Row.Id = Action.ToString() + TEXT(".") + O.ToString();
				Row.Map = I;
				Row.Original = O;
				Row.Current = Mappings[I].Key;
				Out.Add(Row);
			}
		}
		FKeyRow Team;
		Team.Action = FName(TEXT("GS_Team"));
		Team.Id = TeamRowId;
		Team.Original = EKeys::L;
		Team.Current = TeamKey();
		Out.Add(Team);
		FKeyRow Fly;
		Fly.Action = FName(TEXT("GS_Fly"));
		Fly.Id = FlyRowId;
		Fly.Original = EKeys::N;
		Fly.Current = FlyKey();
		Out.Add(Fly);
	}

	FString KeyDisplay(const FKey& K)
	{
		static const TMap<FKey, FString> Names = {
			{ EKeys::SpaceBar, TEXT("Mellomrom") }, { EKeys::LeftShift, TEXT("V. Shift") }, { EKeys::RightShift, TEXT("H. Shift") },
			{ EKeys::LeftControl, TEXT("V. Ctrl") }, { EKeys::RightControl, TEXT("H. Ctrl") }, { EKeys::LeftAlt, TEXT("Alt") },
			{ EKeys::LeftMouseButton, TEXT("Venstre musknapp") }, { EKeys::RightMouseButton, TEXT("Høyre musknapp") },
			{ EKeys::MiddleMouseButton, TEXT("Midtre musknapp") }, { EKeys::ThumbMouseButton, TEXT("Musknapp 4") },
			{ EKeys::ThumbMouseButton2, TEXT("Musknapp 5") }, { EKeys::Escape, TEXT("Esc") }, { EKeys::BackSpace, TEXT("Rettetast") },
			{ EKeys::Up, TEXT("Pil opp") }, { EKeys::Down, TEXT("Pil ned") }, { EKeys::Left, TEXT("Pil venstre") },
			{ EKeys::Right, TEXT("Pil høyre") }, { EKeys::Enter, TEXT("Enter") }, { EKeys::Tab, TEXT("Tab") } };
		if (const FString* N = Names.Find(K)) return *N;
		return K.IsValid() ? K.GetDisplayName(false).ToString() : FString();
	}

	FString RowLabel(const FKeyRow& Row)
	{
		static const TMap<FString, FString> Labels = {
			{ TEXT("IA_Move.W"), TEXT("Gå fremover") }, { TEXT("IA_Move.S"), TEXT("Gå bakover") },
			{ TEXT("IA_Move.A"), TEXT("Gå til venstre") }, { TEXT("IA_Move.D"), TEXT("Gå til høyre") },
			{ TEXT("IA_Jump"), TEXT("Hopp / klatre") }, { TEXT("IA_Sprint"), TEXT("Løp") },
			{ TEXT("IA_Crouch.C"), TEXT("Huk (løp + huk = skli)") }, { TEXT("IA_Crouch.LeftControl"), TEXT("Huk (ekstra tast)") },
			{ TEXT("IA_Crouch"), TEXT("Huk") },
			{ TEXT("IA_Fire"), TEXT("Skyt") }, { TEXT("IA_Aim"), TEXT("Sikt") }, { TEXT("IA_Reload"), TEXT("Lad") },
			{ TEXT("IA_Slot1"), TEXT("Våpen 1") }, { TEXT("IA_Slot2"), TEXT("Våpen 2") },
			{ TEXT("IA_Slot3"), TEXT("Våpen 3") }, { TEXT("IA_Slot4"), TEXT("Våpen 4") },
			{ TEXT("IA_Interact"), TEXT("Plukk opp / bruk PC") }, { TEXT("IA_UseItem"), TEXT("Bruk valgt ting") },
			{ TEXT("IA_NextItem"), TEXT("Neste ting i sekken") }, { TEXT("IA_DropItem"), TEXT("Slipp valgt ting") },
			{ TEXT("IA_Melee"), TEXT("Slag") }, { TEXT("IA_Inventory"), TEXT("Inventar") },
			{ TEXT("IA_Scoreboard"), TEXT("Poengtavle (hold)") }, { TEXT("IA_Inspect"), TEXT("Se på våpenet") },
			{ TEXT("IA_Ready"), TEXT("Klar (lobby)") }, { TEXT("GS_Team"), TEXT("Velg lag (lobby)") },
			{ TEXT("GS_Fly"), TEXT("Fri flyging (test kartet)") } };
		if (const FString* L = Labels.Find(Row.Id)) return *L;
		const FString Action = Row.Action.ToString();
		if (const FString* L = Labels.Find(Action)) return *L;
		// Unknown action (new from the combat team): its name without the prefix.
		return Action.StartsWith(TEXT("IA_")) ? Action.RightChop(3) : Action;
	}

	void RebuildMappings(AGSPlayerController* PC)
	{
		if (!PC) return;
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Sub->RequestRebuildControlMappings();
			}
		}
	}

	/** Saved keys (or the originals) onto the mapping context. */
	void ApplySavedKeys(AGSPlayerController* PC)
	{
		UInputMappingContext* C = PC ? PC->MappingContext.Get() : nullptr;
		if (!C) return;
		TArray<FKeyRow> Rows;
		TArray<FKey> Reserved;
		BuildRows(PC, Rows, &Reserved);
		for (const FKeyRow& Row : Rows)
		{
			if (Row.Map == INDEX_NONE || Row.Map >= C->GetMappings().Num()) continue;
			FKey K = Row.Original;
			const FString Str = CfgString(KeySec, *Row.Id);
			if (!Str.IsEmpty())
			{
				const FKey Saved(*Str);
				if (Saved.IsValid() && !Reserved.Contains(Saved)) K = Saved;   // a key saved before it was reserved (H)
			}
			C->GetMapping(Row.Map).Key = K;
		}
		RebuildMappings(PC);
	}

	/** Saves one row (only when it differs from the original) and puts it on the mapping context. */
	void SetRowKey(AGSPlayerController* PC, const FKeyRow& Row, const FKey& K)
	{
		if (GConfig)
		{
			if (K == Row.Original) GConfig->RemoveKey(KeySec, *Row.Id, GGameUserSettingsIni);
			else GConfig->SetString(KeySec, *Row.Id, *K.ToString(), GGameUserSettingsIni);
		}
		UInputMappingContext* C = PC ? PC->MappingContext.Get() : nullptr;
		if (C && Row.Map != INDEX_NONE && Row.Map < C->GetMappings().Num())
		{
			C->GetMapping(Row.Map).Key = K;
		}
	}

	void AssignKey(AGSPlayerController* PC, FState& S, int32 RowIndex, const FKey& NewKey)
	{
		S.ListenRow = INDEX_NONE;
		TArray<FKeyRow> Rows;
		TArray<FKey> Reserved;
		BuildRows(PC, Rows, &Reserved);
		if (!Rows.IsValidIndex(RowIndex)) return;
		if (!NewKey.IsValid() || NewKey.IsGamepadKey() || NewKey.IsTouch() || NewKey.IsAxis1D() || NewKey.IsAxis2D() || NewKey.IsAxis3D()
			|| NewKey == EKeys::MouseScrollUp || NewKey == EKeys::MouseScrollDown || Reserved.Contains(NewKey))
		{
			S.KeyNote = TEXT("Esc, F10, P, H, musa og hjulet er satt av og kan ikke brukes.");
			UE_LOG(LogTemp, Log, TEXT("GSMenu: tasten %s kan ikke brukes"), *NewKey.ToString());
			return;
		}
		const FKeyRow& Row = Rows[RowIndex];
		UE_LOG(LogTemp, Log, TEXT("GSMenu: %s -> %s (var %s)"), *Row.Id, *NewKey.ToString(), *Row.Current.ToString());
		S.KeyNote.Empty();
		if (Row.Current == NewKey) return;
		for (int32 I = 0; I < Rows.Num(); ++I)
		{
			if (I != RowIndex && Rows[I].Current == NewKey)
			{
				SetRowKey(PC, Rows[I], Row.Current);   // the two swap
				S.KeyNote = FString::Printf(TEXT("%s var brukt av «%s». De har byttet."), *KeyDisplay(NewKey), *RowLabel(Rows[I]));
			}
		}
		SetRowKey(PC, Row, NewKey);
		CfgFlush();
		RebuildMappings(PC);
	}

	void ResetKeys(AGSPlayerController* PC, FState& S)
	{
		if (GConfig)
		{
			GConfig->EmptySection(KeySec, GGameUserSettingsIni);
			CfgFlush();
		}
		ApplySavedKeys(PC);
		S.ListenRow = INDEX_NONE;
		S.KeyNote = TEXT("Standardtastene er tilbake.");
	}

	// ------------------------------------------------------------------ drawing helpers
	UFont* MenuFont()
	{
		return GEngine ? GEngine->GetMediumFont() : nullptr;
	}

	float TextWidth(const FString& Str, float Scale)
	{
		UFont* Font = MenuFont();
		return Font ? Font->GetStringSize(*Str) * Scale : 0.f;
	}

	float LineHeight(float Scale)
	{
		UFont* Font = MenuFont();
		return Font ? Font->GetMaxCharHeight() * Scale : 18.f * Scale;
	}

	/** Align: 0 left, 1 center, 2 right. */
	void Text(UCanvas* Canvas, const FString& Str, float X, float Y, const FLinearColor& Color, float Scale, int32 Align = 0)
	{
		UFont* Font = MenuFont();
		if (!Font || Str.IsEmpty()) return;
		const float W = Font->GetStringSize(*Str) * Scale;
		if (Align == 1) X -= W * 0.5f;
		if (Align == 2) X -= W;
		Canvas->SetDrawColor(FColor(0, 0, 0, uint8(160 * Color.A)));
		Canvas->DrawText(Font, Str, X + 1.5f, Y + 1.5f, Scale, Scale);
		Canvas->SetDrawColor(Color.ToFColor(true));
		Canvas->DrawText(Font, Str, X, Y, Scale, Scale);
	}

	void Box(UCanvas* Canvas, float X, float Y, float W, float H, const FLinearColor& Color)
	{
		if (W <= 0.f || H <= 0.f) return;
		FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(W, H), Color);
		Tile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Tile);
	}

	void Frame(UCanvas* Canvas, float X, float Y, float W, float H, const FLinearColor& Color)
	{
		Box(Canvas, X, Y, W, 1.f, Color);
		Box(Canvas, X, Y + H - 1.f, W, 1.f, Color);
		Box(Canvas, X, Y, 1.f, H, Color);
		Box(Canvas, X + W - 1.f, Y, 1.f, H, Color);
	}

	TArray<FString> Wrap(const FString& Str, float Scale, float MaxW)
	{
		TArray<FString> Lines;
		TArray<FString> Words;
		Str.ParseIntoArray(Words, TEXT(" "), true);
		FString Line;
		for (const FString& Word : Words)
		{
			const FString Try = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
			if (!Line.IsEmpty() && TextWidth(Try, Scale) > MaxW)
			{
				Lines.Add(Line);
				Line = Word;
			}
			else
			{
				Line = Try;
			}
		}
		if (!Line.IsEmpty()) Lines.Add(Line);
		return Lines;
	}

	/** What every widget needs. Mouse is far off-screen for the layer under a dialog (no hover). */
	struct FUi
	{
		FState& S;
		UCanvas* C;
		FVector2D M;
		bool Over(float X, float Y, float W, float H) const { return M.X >= X && M.X <= X + W && M.Y >= Y && M.Y <= Y + H; }
		void Hit(FName Id, float X, float Y, float W, float H, int32 Arg = 0) const
		{
			FHit HitBox;
			HitBox.X0 = X; HitBox.Y0 = Y; HitBox.X1 = X + W; HitBox.Y1 = Y + H;
			HitBox.Id = Id; HitBox.Arg = Arg;
			S.Hits.Add(HitBox);
		}
	};

	/** Wrapped paragraph; returns the Y under it. */
	float Para(const FUi& U, const FString& Str, float X, float Y, float MaxW, const FLinearColor& Color, float Scale)
	{
		const float LH = LineHeight(Scale) + 3.f;
		for (const FString& Line : Wrap(Str, Scale, MaxW))
		{
			Text(U.C, Line, X, Y, Color, Scale);
			Y += LH;
		}
		return Y;
	}

	void Button(const FUi& U, FName Id, const FString& Label, float X, float Y, float W, float H,
		bool bEnabled = true, bool bSelected = false, int32 Arg = 0, bool bAccent = false, bool bCenter = false)
	{
		const bool bHover = bEnabled && U.Over(X, Y, W, H);
		Box(U.C, X, Y, W, H, FLinearColor(1.f, 1.f, 1.f, bHover ? 0.10f : 0.04f));
		if (bEnabled && (bHover || bSelected)) Box(U.C, X, Y, 3.f, H, AccentColor);
		const FLinearColor Col = !bEnabled ? LockedColor : (bAccent ? AccentColor : (bHover || bSelected ? FLinearColor::White : TextColor));
		const float TY = Y + (H - LineHeight(1.f)) * 0.5f;
		if (bCenter) Text(U.C, Label, X + W * 0.5f, TY, Col, 1.f, 1);
		else Text(U.C, Label, X + 16.f, TY, Col, 1.f);
		if (bEnabled) U.Hit(Id, X, Y, W, H, Arg);
	}

	void Field(const FUi& U, FName Id, const FString& Value, const FString& Placeholder, float X, float Y, float W, float H)
	{
		const bool bFocus = U.S.Focus == Id;
		Box(U.C, X, Y, W, H, FLinearColor(0.f, 0.f, 0.f, 0.45f));
		Frame(U.C, X, Y, W, H, bFocus ? AccentColor : FLinearColor(1.f, 1.f, 1.f, 0.18f));
		const float Scale = 0.95f;
		const float TY = Y + (H - LineHeight(Scale)) * 0.5f;
		const bool bCaret = bFocus && FMath::Fmod(FPlatformTime::Seconds(), 1.0) < 0.55;
		if (Value.IsEmpty() && !bFocus)
		{
			Text(U.C, Placeholder, X + 10.f, TY, DimColor, Scale);
		}
		else
		{
			// Wider than the field: show the end.
			FString Shown = Value;
			const float MaxW = W - 28.f;
			while (Shown.Len() > 1 && TextWidth(Shown, Scale) > MaxW) Shown.RightChopInline(1);
			Text(U.C, Shown, X + 10.f, TY, FLinearColor::White, Scale);
			if (bCaret) Box(U.C, X + 12.f + TextWidth(Shown, Scale), Y + 7.f, 2.f, H - 14.f, AccentColor);
		}
		U.Hit(Id, X, Y, W, H);
	}

	void Slider(const FUi& U, FName Id, float Value01, float X, float Y, float W)
	{
		const float H = 28.f;
		const float V = FMath::Clamp(Value01, 0.f, 1.f);
		Box(U.C, X, Y + H * 0.5f - 2.f, W, 4.f, FLinearColor(1.f, 1.f, 1.f, 0.2f));
		Box(U.C, X, Y + H * 0.5f - 2.f, W * V, 4.f, AccentColor);
		const bool bHot = U.S.DragSlider == Id || U.Over(X, Y, W, H);
		Box(U.C, X + W * V - 5.f, Y + 5.f, 10.f, 18.f, bHot ? FLinearColor::White : TextColor);
		U.Hit(Id, X, Y, W, H);
	}

	/** "<  value  >"; the arrows are buttons with Arg -1 / +1. */
	void Choice(const FUi& U, FName Id, const FString& ValueText, float X, float Y, float W, bool bEnabled = true)
	{
		const float H = 28.f, B = 30.f;
		Button(U, Id, TEXT("<"), X, Y, B, H, bEnabled, false, -1, false, true);
		Box(U.C, X + B + 4.f, Y, W - 2.f * B - 8.f, H, FLinearColor(0.f, 0.f, 0.f, 0.30f));
		Text(U.C, ValueText, X + W * 0.5f, Y + (H - LineHeight(0.9f)) * 0.5f, bEnabled ? FLinearColor::White : LockedColor, 0.9f, 1);
		Button(U, Id, TEXT(">"), X + W - B, Y, B, H, bEnabled, false, +1, false, true);
	}

	// ------------------------------------------------------------------ state changes
	/** Are we in a match with others (host or client)? Not the same as the net mode: while a connection is pending,
	 *  the engine reports NM_Client for a world that is still alone. */
	bool InMatch(const UWorld* W, const FState& S)
	{
		if (!W) return false;
		if (S.bConnecting && S.ConnectWorld.Get() == W) return S.bConnectFromMatch;
		const ENetMode Mode = W->GetNetMode();
		return Mode == NM_ListenServer || (Mode == NM_Client && W->GetNetDriver() != nullptr);
	}

	/** GSGameKind of this world: Bots or Explore only alone on this machine (the server allows nothing else). */
	uint8 LocalKind(const UWorld* W)
	{
		if (!W || W->GetNetMode() != NM_Standalone) return GSGameKind::Online;
		const AGSGameState* GS = W->GetGameState<AGSGameState>();
		return GS ? GS->GameKind : GSGameKind::Online;
	}

	/** A bot match or "Test kartet" (never a match with others). */
	bool InLocalGame(const UWorld* W)
	{
		return LocalKind(W) != GSGameKind::Online;
	}

	/** The port this host actually listens on: the engine quietly takes the next free one when 7777 is taken. */
	int32 HostPort(const UWorld* W)
	{
		const UNetDriver* Driver = (W && W->GetNetMode() == NM_ListenServer) ? W->GetNetDriver() : nullptr;
		if (Driver)
		{
			// "0.0.0.0:7778"
			const FString Addr = const_cast<UNetDriver*>(Driver)->LowLevelGetNetworkNumber();
			int32 Colon = INDEX_NONE;
			if (Addr.FindLastChar(TEXT(':'), Colon))
			{
				const int32 Port = FCString::Atoi(*Addr.RightChop(Colon + 1));
				if (Port > 0 && Port < 65536) return Port;
			}
		}
		return 7777;
	}

	/** "192.168.1.20", or "192.168.1.20:7778" when the port is not the usual one. */
	FString WithPort(const FString& Addr, int32 Port)
	{
		return Port == 7777 ? Addr : FString::Printf(TEXT("%s:%d"), *Addr, Port);
	}

	/** Mouse cursor and input mode follow whether anything of ours is open. */
	void SyncModal(AGSPlayerController* PC, FState& S)
	{
		if (!PC) return;
		const bool bWant = S.bMainOpen || S.bTeamOpen || S.Dialog != EDialog::None;
		if (bWant && (!S.bModalApplied || !PC->bShowMouseCursor))
		{
			if (PC->bMenuOpen) PC->CloseMenu();
			PC->bShowMouseCursor = true;
			FInputModeGameAndUI Mode;
			Mode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(Mode);
			// A key held while the panel opens must not hang: we swallow its release.
			if (!S.bModalApplied)
			{
				PC->FlushPressedKeys();
				UE_LOG(LogTemp, Log, TEXT("GSMenu: menyen tar mus og tastatur (musepeker på, spill og meny)"));
			}
			S.bModalApplied = true;
		}
		else if (!bWant && S.bModalApplied)
		{
			S.bModalApplied = false;
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
			PC->FlushPressedKeys();
			S.Focus = NAME_None;
			S.ListenRow = INDEX_NONE;
			S.DragSlider = NAME_None;
			UE_LOG(LogTemp, Log, TEXT("GSMenu: input sluppet til spillet (musepeker av, bare spill)"));
		}
		S.bCursorShown = PC->ShouldShowMouseCursor();
		// Bot match and test: the game stands still while the menu or a dialog is open. Never online. Only a pause we
		// set ourselves is lifted again (the input hooks are on the window, so the menu still gets keys while paused).
		{
			UWorld* W = PC->GetWorld();
			const bool bWantPause = W && (S.bMainOpen || S.Dialog != EDialog::None) && InLocalGame(W);
			if (bWantPause && !S.bPausedByUs)
			{
				if (!UGameplayStatics::IsGamePaused(PC) && UGameplayStatics::SetGamePaused(PC, true))
				{
					S.bPausedByUs = true;
					UE_LOG(LogTemp, Log, TEXT("GSMenu: spillet står på pause mens menyen er åpen"));
				}
			}
			else if (!bWantPause && S.bPausedByUs)
			{
				S.bPausedByUs = false;
				UGameplayStatics::SetGamePaused(PC, false);
				UE_LOG(LogTemp, Log, TEXT("GSMenu: pausen er slått av"));
			}
		}
		// The main menu covers everything: the combat HUD is not drawn under it.
		if (AHUD* HUD = PC->MyHUD)
		{
			if (S.bMainOpen && !S.bHudHidden) { HUD->bShowHUD = false; S.bHudHidden = true; }
			else if (!S.bMainOpen && S.bHudHidden) { HUD->bShowHUD = true; S.bHudHidden = false; }
		}
	}

	void CommitName(AGSPlayerController* PC, FState& S)
	{
		const FString Saved = EnsureName();
		const FString Clean = CleanName(S.NameEdit);
		if (Clean.IsEmpty() || Clean == Saved)
		{
			S.NameEdit = Saved;
			return;
		}
		S.NameEdit = Clean;
		if (GConfig)
		{
			GConfig->SetString(Sec, TEXT("PlayerName"), *Clean, GGameUserSettingsIni);
			CfgFlush();
		}
		if (PC && PC->PlayerState) PC->ServerChangeName(Clean);
	}

	void SetFocus(AGSPlayerController* PC, FState& S, FName NewFocus)
	{
		if (S.Focus == NewFocus) return;
		if (S.Focus == FName(TEXT("Name"))) CommitName(PC, S);
		S.Focus = NewFocus;
	}

	void SetPage(AGSPlayerController* PC, FState& S, EPage Page)
	{
		SetFocus(PC, S, NAME_None);
		S.Page = Page;
		S.ListenRow = INDEX_NONE;
		if (S.DragSlider != NAME_None)
		{
			S.DragSlider = NAME_None;   // the button may have been let go outside the window
			CfgFlush();
		}
		S.KeyNote.Empty();
		S.JoinNote.Empty();
		if (Page == EPage::Join || Page == EPage::Online)
		{
			if (S.AddressEdit.IsEmpty() && PC) S.AddressEdit = PC->LastJoinAddress;
			if (Page == EPage::Join) S.Focus = FName(TEXT("Address"));   // on the online page P still closes the menu
		}
	}

	void OpenMainImpl(AGSPlayerController* PC, FState& S, const TCHAR* Why = TEXT("kall"))
	{
		if (!S.bMainOpen) UE_LOG(LogTemp, Log, TEXT("GSMenu: hovedmenyen åpnes (%s)"), Why);
		S.bTeamOpen = false;
		S.bMainOpen = true;
		S.Page = S.bConnecting ? EPage::Connecting : EPage::Root;
		S.Focus = NAME_None;
		S.ListenRow = INDEX_NONE;
		SyncModal(PC, S);
	}

	void CloseMain(AGSPlayerController* PC, FState& S, const TCHAR* Why = TEXT("kall"))
	{
		if (S.bMainOpen) UE_LOG(LogTemp, Log, TEXT("GSMenu: hovedmenyen lukkes (%s)"), Why);
		SetFocus(PC, S, NAME_None);
		S.bMainOpen = false;
		S.bMainDismissed = true;
		S.Page = EPage::Root;
		SyncModal(PC, S);
	}

	void ShowMessage(FState& S, const FString& Title, const FString& Body, const FString& Detail)
	{
		S.MsgTitle = Title;
		S.MsgText = Body;
		S.MsgDetail = Detail;
		S.MsgTime = FPlatformTime::Seconds();
		S.bMsgPending = true;
		S.bConnecting = false;
	}

	void ReloadAlone(AGSPlayerController* PC, FState& S)
	{
		S.LeavingUntil = FPlatformTime::Seconds() + 10.0;
		UGameplayStatics::OpenLevel(PC, FName(*UGameplayStatics::GetCurrentLevelName(PC, true)));
	}

	// ------------------------------------------------------------------ the rules of a new game (bot page, host)
	static const int32 MinuteSteps[] = { 5, 10, 15, 20, 30 };
	static const int32 CapSteps[] = { 1, 2, 3, 5 };
	constexpr int32 NumMinuteSteps = 5, NumCapSteps = 4;
	static const TCHAR* const SkillNames[] = { TEXT("Lett"), TEXT("Middels"), TEXT("Vanskelig"), TEXT("Ekspert") };
	static const TCHAR* const SkillInfo[] = {
		TEXT("Lett: reagerer sent og bommer mye. Fint for å lære kartet."),
		TEXT("Middels: reagerer som en vanlig spiller, bommer ofte på lang avstand."),
		TEXT("Vanskelig: reagerer raskt, bruker dekning og treffer godt."),
		TEXT("Ekspert: som en erfaren spiller, og samarbeider om flagget.") };
	static const TCHAR* const TeamChoiceNames[] = { TEXT("Tilfeldig"), TEXT("Rødt lag"), TEXT("Blått lag") };

	int32 CfgInt(const TCHAR* Key, int32 Default)
	{
		int32 V = Default;
		if (GConfig) GConfig->GetInt(Sec, Key, V, GGameUserSettingsIni);
		return V;
	}

	void SetCfgInt(const TCHAR* Key, int32 V)
	{
		if (!GConfig) return;
		GConfig->SetInt(Sec, Key, V, GGameUserSettingsIni);
		CfgFlush();
	}

	int32 NearestStep(const int32* Steps, int32 Num, int32 Value)
	{
		int32 Best = 0;
		for (int32 I = 1; I < Num; ++I)
		{
			if (FMath::Abs(Steps[I] - Value) < FMath::Abs(Steps[Best] - Value)) Best = I;
		}
		return Best;
	}

	/** The saved choices, clamped the same way the server clamps them (AGSGameMode::InitGame). */
	struct FRules
	{
		int32 Team = 0, Allies = 4, Enemies = 5, Skill = 1, Minutes = 10, Caps = 3;
		bool bFF = false, bKit = true;
	};

	FRules BotRules()
	{
		FRules R;
		R.Team = FMath::Clamp(CfgInt(TEXT("BotTeam"), 0), 0, 2);
		R.Allies = FMath::Clamp(CfgInt(TEXT("BotAllies"), 4), 0, 9);
		R.Enemies = FMath::Clamp(CfgInt(TEXT("BotEnemies"), 5), 1, 10);
		R.Skill = FMath::Clamp(CfgInt(TEXT("BotSkill"), 1), 0, 3);
		R.Minutes = MinuteSteps[NearestStep(MinuteSteps, NumMinuteSteps, CfgInt(TEXT("BotMinutes"), 10))];
		R.Caps = CapSteps[NearestStep(CapSteps, NumCapSteps, CfgInt(TEXT("BotCaps"), 3))];
		R.bFF = CfgInt(TEXT("BotFF"), 0) != 0;
		R.bKit = CfgInt(TEXT("BotKit"), 1) != 0;
		return R;
	}

	FRules HostRules()
	{
		FRules R;
		R.Minutes = MinuteSteps[NearestStep(MinuteSteps, NumMinuteSteps, CfgInt(TEXT("HostMinutes"), 15))];
		R.Caps = CapSteps[NearestStep(CapSteps, NumCapSteps, CfgInt(TEXT("HostCaps"), 3))];
		R.bFF = CfgInt(TEXT("HostFF"), 0) != 0;
		R.bKit = false;
		return R;
	}

	/** One "<  >" press on a rule; saved at once. */
	void ChangeRule(const FString& Id, int32 Dir)
	{
		auto Step = [Dir](const TCHAR* Key, const int32* Steps, int32 Num, int32 Default)
		{
			const int32 I = FMath::Clamp(NearestStep(Steps, Num, CfgInt(Key, Default)) + Dir, 0, Num - 1);
			SetCfgInt(Key, Steps[I]);
		};
		auto Range = [Dir](const TCHAR* Key, int32 Lo, int32 Hi, int32 Default)
		{
			SetCfgInt(Key, FMath::Clamp(FMath::Clamp(CfgInt(Key, Default), Lo, Hi) + Dir, Lo, Hi));
		};
		auto Toggle = [](const TCHAR* Key, int32 Default) { SetCfgInt(Key, CfgInt(Key, Default) != 0 ? 0 : 1); };
		if (Id == TEXT("Rule.Bot.Team")) Range(TEXT("BotTeam"), 0, 2, 0);
		else if (Id == TEXT("Rule.Bot.Allies")) Range(TEXT("BotAllies"), 0, 9, 4);
		else if (Id == TEXT("Rule.Bot.Enemies")) Range(TEXT("BotEnemies"), 1, 10, 5);
		else if (Id == TEXT("Rule.Bot.Skill")) Range(TEXT("BotSkill"), 0, 3, 1);
		else if (Id == TEXT("Rule.Bot.Min")) Step(TEXT("BotMinutes"), MinuteSteps, NumMinuteSteps, 10);
		else if (Id == TEXT("Rule.Bot.Caps")) Step(TEXT("BotCaps"), CapSteps, NumCapSteps, 3);
		else if (Id == TEXT("Rule.Bot.FF")) Toggle(TEXT("BotFF"), 0);
		else if (Id == TEXT("Rule.Bot.Kit")) Toggle(TEXT("BotKit"), 1);
		else if (Id == TEXT("Rule.Host.Min")) Step(TEXT("HostMinutes"), MinuteSteps, NumMinuteSteps, 15);
		else if (Id == TEXT("Rule.Host.Caps")) Step(TEXT("HostCaps"), CapSteps, NumCapSteps, 3);
		else if (Id == TEXT("Rule.Host.FF")) Toggle(TEXT("HostFF"), 0);
	}

	/** Loads this map again with options (the server reads and clamps them, AGSGameMode::InitGame). */
	void OpenSameMap(AGSPlayerController* PC, FState& S, const FString& Options)
	{
		if (!PC) return;
		S.LeavingUntil = FPlatformTime::Seconds() + 10.0;   // leaving a match on purpose: no "connection lost"
		UGameplayStatics::OpenLevel(PC, FName(*UGameplayStatics::GetCurrentLevelName(PC, true)), true, Options);
	}

	void StartBots(AGSPlayerController* PC, FState& S)
	{
		const FRules R = BotRules();
		if (AGSGameState::bBotsAvailable)
		{
			UE_LOG(LogTemp, Log, TEXT("GSMenu: Spill mot bots startes (du + %d mot %d, %s, %d min, %d erobringer)"),
				R.Allies, R.Enemies, SkillNames[R.Skill], R.Minutes, R.Caps);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("GSMenu: Spill mot bots startes (botene finnes ikke ennå: du er alene, %d min, %d erobringer)"), R.Minutes, R.Caps);
		}
		OpenSameMap(PC, S, FString::Printf(TEXT("Mode=Bots?Team=%d?Allies=%d?Enemies=%d?Skill=%d?Min=%d?Caps=%d?FF=%d?Kit=%d"),
			R.Team, R.Allies, R.Enemies, R.Skill, R.Minutes, R.Caps, R.bFF ? 1 : 0, R.bKit ? 1 : 0));
	}

	void StartHost(AGSPlayerController* PC, FState& S)
	{
		const FRules R = HostRules();
		UE_LOG(LogTemp, Log, TEXT("GSMenu: vert startes (%d min, %d erobringer, venneild %s)"), R.Minutes, R.Caps, R.bFF ? TEXT("på") : TEXT("av"));
		OpenSameMap(PC, S, FString::Printf(TEXT("listen?Min=%d?Caps=%d?FF=%d"), R.Minutes, R.Caps, R.bFF ? 1 : 0));
	}

	void StartExplore(AGSPlayerController* PC, FState& S)
	{
		UE_LOG(LogTemp, Log, TEXT("GSMenu: Test kartet startes"));
		OpenSameMap(PC, S, TEXT("Mode=Explore"));
	}

	void StartPending(AGSPlayerController* PC, FState& S)
	{
		switch (S.PendingStart)
		{
		case EStart::Bots: StartBots(PC, S); break;
		case EStart::Explore: StartExplore(PC, S); break;
		default: StartHost(PC, S); break;
		}
	}

	void CancelConnect(AGSPlayerController* PC, UWorld* World, FState& S)
	{
		if (GEngine && World)
		{
			if (FWorldContext* Ctx = GEngine->GetWorldContextFromWorld(World))
			{
				Ctx->TravelURL.Empty();
				GEngine->CancelPending(World);
			}
		}
		const bool bWasConnecting = S.bConnecting;
		S.bConnecting = false;
		if (S.Page == EPage::Connecting) S.Page = EPage::Online;
		// The engine closed the old match's connection the moment the attempt started: that world is dead, load the map alone.
		if (bWasConnecting && S.bConnectFromMatch && PC && S.ConnectWorld.Get() == World)
		{
			S.bConnectFromMatch = false;
			ReloadAlone(PC, S);
		}
	}

	void StartJoin(AGSPlayerController* PC, UWorld* World, FState& S, const FString& Addr)
	{
		if (!PC) return;
		S.bConnectFromMatch = InMatch(World, S);   // before bConnecting is set
		S.bConnecting = true;
		S.LeavingUntil = 0.0;   // a new attempt: its own failures must be shown
		S.ConnectWorld = World;
		S.ConnectAddress = Addr;
		S.ConnectStart = FPlatformTime::Seconds();
		S.Page = EPage::Connecting;
		S.Focus = NAME_None;
		PC->Join(Addr);   // saves the address and travels
	}

	void TryJoin(AGSPlayerController* PC, UWorld* World, FState& S)
	{
		const FString Addr = S.AddressEdit.TrimStartAndEnd();
		if (Addr.IsEmpty()) { S.JoinNote = TEXT("Skriv inn en adresse."); return; }
		if (Addr.Contains(TEXT(" "))) { S.JoinNote = TEXT("Adressen kan ikke ha mellomrom."); return; }
		// Without '.' or ':' the engine reads the address as a map name and fails with a misleading map error.
		if (!Addr.Contains(TEXT(".")) && !Addr.Contains(TEXT(":"))) { S.JoinNote = TEXT("Sjekk adressen. Den skal se ut som 192.168.1.20."); return; }
		S.JoinNote.Empty();
		if (InMatch(World, S))
		{
			S.PendingJoin = Addr;
			S.Dialog = EDialog::Join;
		}
		else
		{
			StartJoin(PC, World, S, Addr);
		}
	}

	int32 TeamCountOf(const AGSGameState* GS, uint8 Team)
	{
		int32 N = 0;
		if (GS)
		{
			for (const APlayerState* P : GS->PlayerArray)
			{
				const AGSPlayerState* PS = Cast<AGSPlayerState>(P);
				if (PS && PS->Team == Team) ++N;
			}
		}
		return N;
	}

	/** Same rule as AGSGameMode::RequestTeam. Empty = allowed, else why not. */
	FString TeamJoinBlock(const AGSGameState* GS, const AGSPlayerState* PS, uint8 T)
	{
		if (!GS || !PS) return TEXT(" ");
		if (PS->Team == T) return TEXT("Du er her");
		if (GS->Phase != EGSMatchPhase::Lobby) return TEXT("Bare før nedtellingen");
		const int32 NewCount = TeamCountOf(GS, T);
		if (NewCount >= AGSGameMode::MaxPerTeam) return TEXT("Laget er fullt");
		const int32 OldCount = PS->Team == GSTeam::None ? TeamCountOf(GS, GSTeam::Other(T)) + 1 : TeamCountOf(GS, PS->Team);
		if (NewCount >= OldCount) return TEXT("Lagene blir skjeve");
		return FString();
	}

	// ------------------------------------------------------------------ settings
	static const int32 ResSteps[] = { 50, 60, 72, 85, 100 };
	// The combat team owns the cap (30-240, saved, set again on every map): no "unlimited".
	static const int32 FpsSteps[] = { 30, 60, 90, 120, 144, 240 };

	/** The graphics level shown and stepped from. Not the overall level: the engine gives -1 for that as soon as the
	 *  resolution scale is not the level's own value, and we keep the player's own resolution. One part instead,
	 *  which the overall level sets and nothing else here touches. */
	int32 GraphicsLevel(const UGameUserSettings* Gus)
	{
		return Gus ? FMath::Clamp(Gus->GetShadowQuality(), 0, 4) : 2;
	}

	int32 NearestIndex(const int32* Steps, int32 Num, int32 Value)
	{
		int32 Best = 0;
		for (int32 I = 1; I < Num; ++I)
		{
			if (FMath::Abs(Steps[I] - Value) < FMath::Abs(Steps[Best] - Value)) Best = I;
		}
		return Best;
	}

	void ChangeSetting(AGSPlayerController* PC, const FString& Id, int32 Dir)
	{
		UGameUserSettings* Gus = GEngine ? GEngine->GetGameUserSettings() : nullptr;
		if (!PC) return;
		if (Id == TEXT("Set.RT"))
		{
			const bool bOn = !PC->IsRayTracingOn();
			PC->Straaling(bOn ? 1 : 0);
			if (GConfig)
			{
				GConfig->SetBool(Sec, TEXT("RayTracing"), bOn, GGameUserSettingsIni);
				CfgFlush();
			}
			return;
		}
		if (Id == TEXT("Set.Fps"))
		{
			// The combat team's cap: it saves it and sets it again on every map, also in the editor's Play.
			const int32 N = int32(UE_ARRAY_COUNT(FpsSteps));
			const int32 Cur = NearestIndex(FpsSteps, N, PC->GetFrameLimit());
			const int32 I = FMath::Clamp(Cur + Dir, 0, N - 1);
			if (FpsSteps[I] != PC->GetFrameLimit()) PC->Bildefrekvens(FpsSteps[I]);
			return;
		}
		if (!Gus) return;
		if (Id == TEXT("Set.Graphics"))
		{
			// The overall level also resets the resolution scale: keep the player's own.
			const int32 Res = PC->GetResolutionPercent();
			Gus->SetOverallScalabilityLevel(FMath::Clamp(GraphicsLevel(Gus) + Dir, 0, 4));
			Gus->SetResolutionScaleValueEx(float(FMath::Clamp(Res, 50, 100)));
			Gus->ApplyNonResolutionSettings();
		}
		else if (Id == TEXT("Set.Res"))
		{
			const int32 I = FMath::Clamp(NearestIndex(ResSteps, int32(UE_ARRAY_COUNT(ResSteps)), PC->GetResolutionPercent()) + Dir, 0, int32(UE_ARRAY_COUNT(ResSteps)) - 1);
			Gus->SetResolutionScaleValueEx(float(ResSteps[I]));
			Gus->ApplyNonResolutionSettings();
		}
		else if (Id == TEXT("Set.VSync"))
		{
			Gus->SetVSyncEnabled(!Gus->IsVSyncEnabled());
			Gus->ApplyNonResolutionSettings();
		}
		else if (Id == TEXT("Set.Mode"))
		{
			if (GIsEditor) return;   // would resize the editor's own window
			static const EWindowMode::Type Modes[] = { EWindowMode::Fullscreen, EWindowMode::WindowedFullscreen, EWindowMode::Windowed };
			int32 I = 0;
			for (int32 J = 0; J < 3; ++J) { if (Modes[J] == Gus->GetFullscreenMode()) I = J; }
			I = FMath::Clamp(I + Dir, 0, 2);
			Gus->SetFullscreenMode(Modes[I]);
			const FIntPoint Desktop = Gus->GetDesktopResolution();
			if (Modes[I] != EWindowMode::Windowed)
			{
				if (Desktop.X > 0 && Desktop.Y > 0) Gus->SetScreenResolution(Desktop);
			}
			else if (Gus->GetScreenResolution() == Desktop)
			{
				Gus->SetScreenResolution(FIntPoint(1600, 900));
			}
			Gus->ApplyResolutionSettings(false);
		}
		else
		{
			return;
		}
		Gus->SaveSettings();
	}

	/** A slider moved: use the value at once; it is written to disk when the button is released. */
	void ApplySlider(AGSPlayerController* PC, UWorld* World, FName Id, float V01)
	{
		V01 = FMath::Clamp(V01, 0.f, 1.f);
		if (!GConfig) return;
		if (Id == FName(TEXT("Sens")))
		{
			const float V = FMath::Clamp(FMath::RoundToFloat((0.2f + V01 * 2.8f) / 0.05f) * 0.05f, 0.2f, 3.f);
			if (PC) PC->MouseSensitivity = V;
			GConfig->SetFloat(Sec, TEXT("MouseSensitivity"), V, GGameUserSettingsIni);
		}
		else if (Id == FName(TEXT("Volume")))
		{
			GConfig->SetFloat(Sec, TEXT("Volume"), FMath::RoundToFloat(V01 * 100.f) / 100.f, GGameUserSettingsIni);
			ApplyVolume(World);
		}
	}

	// ------------------------------------------------------------------ pages
	void DrawSettings(const FUi& U, AGSPlayerController* PC, float X, float Y, float W, float H)
	{
		const UGameUserSettings* Gus = GEngine ? GEngine->GetGameUserSettings() : nullptr;
		const float RowH = FMath::Clamp((H - 110.f) / 9.f, 30.f, 40.f);
		const float LX = X + 24.f;
		const float CX = X + W * 0.45f;
		const float CW = FMath::Min(W * 0.55f - 24.f, 340.f);
		const float S = 0.85f;
		auto Label = [&](const TCHAR* Str) { Text(U.C, Str, LX, Y + (28.f - LineHeight(S)) * 0.5f, TextColor, S); };

		static const TCHAR* Levels[] = { TEXT("Lav"), TEXT("Middels"), TEXT("Høy"), TEXT("Episk"), TEXT("Film") };
		Label(TEXT("Grafikk"));
		Choice(U, TEXT("Set.Graphics"), Levels[GraphicsLevel(Gus)], CX, Y, CW);
		Y += RowH;

		Label(TEXT("Oppløsning"));
		Choice(U, TEXT("Set.Res"), FString::Printf(TEXT("%d %%"), PC->GetResolutionPercent()), CX, Y, CW);
		Y += RowH;

		Label(TEXT("Strålesporing (RTX)"));
		Choice(U, TEXT("Set.RT"), PC->IsRayTracingOn() ? TEXT("På") : TEXT("Av"), CX, Y, CW);
		Y += RowH;

		Label(TEXT("Skjermmodus"));
		{
			const EWindowMode::Type Mode = Gus ? Gus->GetFullscreenMode() : EWindowMode::Windowed;
			FString Name = Mode == EWindowMode::Fullscreen ? TEXT("Fullskjerm") : (Mode == EWindowMode::WindowedFullscreen ? TEXT("Fullskjerm i vindu") : TEXT("Vindu"));
			if (GIsEditor) Name = TEXT("(gjelder i ferdig spill)");
			Choice(U, TEXT("Set.Mode"), Name, CX, Y, CW, !GIsEditor);
		}
		Y += RowH;

		Label(TEXT("Vertikal synk"));
		Choice(U, TEXT("Set.VSync"), (Gus && Gus->IsVSyncEnabled()) ? TEXT("På") : TEXT("Av"), CX, Y, CW);
		Y += RowH;

		Label(TEXT("Maks bildefrekvens"));
		Choice(U, TEXT("Set.Fps"), FString::Printf(TEXT("%d"), PC->GetFrameLimit()), CX, Y, CW);
		Y += RowH;

		Label(TEXT("Musfølsomhet"));
		Slider(U, TEXT("Sens"), (PC->MouseSensitivity - 0.2f) / 2.8f, CX, Y, CW - 70.f);
		Text(U.C, FString::Printf(TEXT("%.2f"), PC->MouseSensitivity), CX + CW, Y + (28.f - LineHeight(S)) * 0.5f, TextColor, S, 2);
		Y += RowH;

		Label(TEXT("Lydstyrke"));
		const float Vol = SavedVolume();
		Slider(U, TEXT("Volume"), Vol, CX, Y, CW - 70.f);
		Text(U.C, FString::Printf(TEXT("%d %%"), FMath::RoundToInt(Vol * 100.f)), CX + CW, Y + (28.f - LineHeight(S)) * 0.5f, TextColor, S, 2);
		Y += RowH;

		// The name is edited in the left panel (one field, one caret).
		Label(TEXT("Taster"));
		Button(U, TEXT("Nav.Controls"), TEXT("Endre taster ..."), CX, Y, CW, 30.f);
		Y += RowH + 6.f;

		Text(U.C, TEXT("Alt lagres med en gang."), LX, Y, DimColor, 0.75f);
	}

	void DrawControls(const FUi& U, AGSPlayerController* PC, float X, float Y, float W, float H)
	{
		TArray<FKeyRow> Rows;
		BuildRows(PC, Rows);
		const float LX = X + 24.f, MaxW = W - 48.f;
		const UWorld* KeyWorld = PC ? PC->GetWorld() : nullptr;
		const FString Help = FString::Printf(TEXT("Klikk en linje og trykk den nye tasten (%s avbryter). Faste: mus = se rundt · musehjul = bla i våpnene, eller i sekken når inventaret er åpent · %s = meny · H = taster og regler."),
			BackKeyText(KeyWorld), MenuKeysText(KeyWorld));
		const AGSGameState* RulesGS = KeyWorld ? KeyWorld->GetGameState<AGSGameState>() : nullptr;
		const FString Rules = FString::Printf(TEXT("Ta motstanderens flagg og bær det til din egen base. Du får bare poeng når ditt eget flagg står hjemme. Første lag til %d erobringer vinner."),
			RulesGS ? int32(RulesGS->CaptureGoal) : AGSGameState::CaptureLimit);
		auto ParaH = [](const FString& Str, float Scale, float Width) { return Wrap(Str, Scale, Width).Num() * (LineHeight(Scale) + 3.f); };
		// What goes under the rows: help text, the note, the reset button, and (if there is room) the CTF rules.
		const float FooterH = 8.f + ParaH(Help, 0.72f, MaxW) + 2.f + LineHeight(0.8f) + 8.f + 44.f;
		const float RulesH = LineHeight(0.9f) + 4.f + ParaH(Rules, 0.78f, MaxW);
		// Small window (the editor's Play on a laptop): first drop the rules, then go to three columns.
		constexpr float GoodRowH = 22.f;
		int32 Cols = 2;
		bool bRules = true;
		auto PerColFor = [&](int32 C) { return FMath::Max(1, (Rows.Num() + C - 1) / C); };
		auto RowHFor = [&](int32 C, bool bWithRules) { return (H - FooterH - (bWithRules ? RulesH : 0.f)) / PerColFor(C); };
		if (RowHFor(2, true) < GoodRowH) bRules = false;
		if (!bRules && RowHFor(2, false) < GoodRowH && (MaxW - 40.f) / 3.f >= 210.f) Cols = 3;
		const int32 PerCol = PerColFor(Cols);
		const float RowH = FMath::Clamp(RowHFor(Cols, bRules), 16.f, 28.f);
		const float S = FMath::Clamp(0.85f * RowH / 26.f, 0.62f, 0.85f);
		const float ColW = (MaxW - 20.f * (Cols - 1)) / Cols;
		const float KeyW = FMath::Min(170.f, ColW * 0.48f);
		const float Top = Y;
		for (int32 I = 0; I < Rows.Num(); ++I)
		{
			const int32 Col = I / PerCol;
			const float RX = X + 24.f + Col * (ColW + 20.f);
			const float RY = Top + (I % PerCol) * RowH;
			const bool bListen = U.S.ListenRow == I;
			const bool bHover = U.Over(RX, RY, ColW, RowH - 2.f);
			if (bHover || bListen) Box(U.C, RX, RY, ColW, RowH - 2.f, FLinearColor(1.f, 1.f, 1.f, 0.07f));
			const float TY = RY + (RowH - 2.f - LineHeight(S)) * 0.5f;
			Text(U.C, RowLabel(Rows[I]), RX + 6.f, TY, TextColor, S);
			const float KX = RX + ColW - KeyW;
			Box(U.C, KX, RY + 1.f, KeyW, RowH - 4.f, FLinearColor(0.f, 0.f, 0.f, 0.40f));
			if (bListen)
			{
				Frame(U.C, KX, RY + 1.f, KeyW, RowH - 4.f, AccentColor);
				Text(U.C, TEXT("Trykk en tast ..."), KX + KeyW * 0.5f, TY, AccentColor, S, 1);
			}
			else
			{
				const bool bChanged = Rows[I].Current != Rows[I].Original;
				Text(U.C, KeyDisplay(Rows[I].Current), KX + KeyW * 0.5f, TY, bChanged ? AccentColor : FLinearColor::White, S, 1);
			}
			U.Hit(TEXT("Key.Row"), RX, RY, ColW, RowH - 2.f, I);
		}
		Y = Top + PerCol * RowH + 8.f;
		Y = Para(U, Help, LX, Y, MaxW, DimColor, 0.72f);
		Text(U.C, U.S.KeyNote, LX, Y + 2.f, AccentColor, 0.8f);
		Y += 2.f + LineHeight(0.8f) + 8.f;
		{
			const float BW = FMath::Min(280.f, (MaxW - 12.f) * 0.5f);
			Button(U, TEXT("Key.Reset"), TEXT("Tilbakestill alle taster"), LX, Y, BW, 32.f);
			Button(U, TEXT("Help.Reset"), TEXT("Vis hintene på nytt"), LX + BW + 12.f, Y, BW, 32.f);
		}
		Y += 44.f;
		if (!bRules) return;   // the rules are also shown in the lobby (GSHelp)
		Text(U.C, TEXT("Capture the Flag"), LX, Y, FLinearColor::White, 0.9f);
		Y += LineHeight(0.9f) + 4.f;
		Para(U, Rules, LX, Y, MaxW, TextColor, 0.78f);
	}

	/** "Label   <  value  >"; returns the Y of the next row. */
	float RuleRow(const FUi& U, const TCHAR* Label, const TCHAR* Id, const FString& Value, float LX, float CX, float CW, float Y, float RowH, bool bEnabled = true)
	{
		const float S = 0.85f;
		Text(U.C, Label, LX, Y + (28.f - LineHeight(S)) * 0.5f, bEnabled ? TextColor : LockedColor, S);
		Choice(U, FName(Id), Value, CX, Y, CW, bEnabled);
		return Y + RowH;
	}

	FLinearColor TeamTextColor(uint8 T)
	{
		return FMath::Lerp(GSTeam::Color(T), FLinearColor::White, 0.45f);
	}

	/** Host: addresses for friends, the port, the three rules and the start button. Returns the Y under it. */
	float DrawHostPart(const FUi& U, float IX, float Y, float MaxW, bool bMatch)
	{
		const float TS = 0.85f;
		Y = Para(U, TEXT("Du blir vert for en ny kamp. Vennene dine kobler seg til deg."), IX, Y, MaxW, TextColor, TS) + 6.f;
		const TArray<FString>& Addrs = LocalAddresses();
		if (Addrs.Num() > 0)
		{
			Text(U.C, TEXT("Venner på samme nett skriver inn:"), IX, Y, TextColor, TS);
			Y += LineHeight(TS) + 4.f;
			for (const FString& A : Addrs)
			{
				Text(U.C, A, IX + 12.f, Y, FLinearColor::White, 1.05f);
				Y += LineHeight(1.05f) + 2.f;
			}
		}
		else
		{
			Text(U.C, TEXT("Fant ingen nettverksadresse."), IX, Y, DimColor, TS);
			Y += LineHeight(TS) + 4.f;
		}
		Y += 4.f;
		Y = Para(U, TEXT("Over internett må port 7777 (UDP) åpnes i ruteren din. Første gang spør Windows om brannmuren: trykk Tillat."),
			IX, Y, MaxW, DimColor, 0.78f) + 8.f;
		const FRules R = HostRules();
		const float CX = IX + FMath::Min(230.f, MaxW * 0.42f);
		const float CW = FMath::Min(MaxW - (CX - IX), 300.f);
		Y = RuleRow(U, TEXT("Kamplengde"), TEXT("Rule.Host.Min"), FString::Printf(TEXT("%d min"), R.Minutes), IX, CX, CW, Y, 34.f);
		Y = RuleRow(U, TEXT("Erobringer for å vinne"), TEXT("Rule.Host.Caps"), FString::FromInt(R.Caps), IX, CX, CW, Y, 34.f);
		Y = RuleRow(U, TEXT("Venneild"), TEXT("Rule.Host.FF"), R.bFF ? TEXT("På") : TEXT("Av"), IX, CX, CW, Y, 34.f);
		if (bMatch) Y = Para(U, TEXT("Kampen du er i nå avsluttes."), IX, Y + 2.f, MaxW, AccentColor, TS);
		Button(U, TEXT("Host.Go"), TEXT("Start som vert"), IX, Y + 6.f, FMath::Min(300.f, MaxW), 42.f, true, false, 0, true);
		return Y + 6.f + 42.f + 12.f;
	}

	/** Join: the address field, connect, the last address and how it works. Returns the Y under it. */
	float DrawJoinPart(const FUi& U, AGSPlayerController* PC, float IX, float Y, float MaxW)
	{
		const float TS = 0.85f;
		Text(U.C, TEXT("Adresse"), IX, Y, DimColor, 0.8f);
		Y += LineHeight(0.8f) + 4.f;
		const float FW = FMath::Min(420.f, MaxW);
		Field(U, TEXT("Address"), U.S.AddressEdit, TEXT("192.168.1.20"), IX, Y, FW, 36.f);
		Y += 36.f + 10.f;
		Button(U, TEXT("Join.Go"), TEXT("Koble til"), IX, Y, FMath::Min(200.f, FW), 42.f, true, false, 0, true);
		if (PC && !PC->LastJoinAddress.IsEmpty() && PC->LastJoinAddress != U.S.AddressEdit)
		{
			Button(U, TEXT("Join.Last"), FString::Printf(TEXT("Siste: %s"), *PC->LastJoinAddress), IX + FMath::Min(200.f, FW) + 10.f, Y,
				FMath::Min(300.f, MaxW - 210.f), 42.f);
		}
		Y += 42.f + 8.f;
		Text(U.C, U.S.JoinNote, IX, Y, AccentColor, TS);
		Y += LineHeight(TS) + 4.f;
		return Para(U, TEXT("Skriv adressen verten ser under «Start som vert», for eksempel 192.168.1.20. Ctrl+V limer inn."), IX, Y, MaxW, DimColor, 0.78f);
	}

	void DrawBotsPage(const FUi& U, UWorld* World, float IX, float Y, float MaxW, float Bottom, bool bMatch)
	{
		const FRules R = BotRules();
		const bool bAvail = AGSGameState::bBotsAvailable;
		const float CX = IX + FMath::Min(250.f, MaxW * 0.42f);
		const float CW = FMath::Min(MaxW - (CX - IX), 330.f);
		const float RowH = FMath::Clamp((Bottom - Y - 150.f) / 8.f, 30.f, 38.f);
		Y = RuleRow(U, TEXT("Ditt lag"), TEXT("Rule.Bot.Team"), TeamChoiceNames[R.Team], IX, CX, CW, Y, RowH);
		Y = RuleRow(U, TEXT("Bots på ditt lag"), TEXT("Rule.Bot.Allies"), FString::FromInt(R.Allies), IX, CX, CW, Y, RowH, bAvail);
		Y = RuleRow(U, TEXT("Bots på motstanderlaget"), TEXT("Rule.Bot.Enemies"), FString::FromInt(R.Enemies), IX, CX, CW, Y, RowH, bAvail);
		Y = RuleRow(U, TEXT("Vanskelighetsgrad"), TEXT("Rule.Bot.Skill"), SkillNames[R.Skill], IX, CX, CW, Y, RowH, bAvail);
		Y = RuleRow(U, TEXT("Kamplengde"), TEXT("Rule.Bot.Min"), FString::Printf(TEXT("%d min"), R.Minutes), IX, CX, CW, Y, RowH);
		Y = RuleRow(U, TEXT("Erobringer for å vinne"), TEXT("Rule.Bot.Caps"), FString::FromInt(R.Caps), IX, CX, CW, Y, RowH);
		Y = RuleRow(U, TEXT("Venneild"), TEXT("Rule.Bot.FF"), R.bFF ? TEXT("På") : TEXT("Av"), IX, CX, CW, Y, RowH);
		Y = RuleRow(U, TEXT("Startutstyr"), TEXT("Rule.Bot.Kit"), R.bKit ? TEXT("Gevær, pistol og bandasjer") : TEXT("Bare det du finner"), IX, CX, CW, Y, RowH);
		Y += 4.f;
		if (bAvail)
		{
			Text(U.C, SkillInfo[R.Skill], IX, Y, DimColor, 0.78f);
			Y += LineHeight(0.78f) + 3.f;
			Text(U.C, FString::Printf(TEXT("Lagene: du + %d mot %d"), R.Allies, R.Enemies), IX, Y, DimColor, 0.78f);
			Y += LineHeight(0.78f) + 6.f;
		}
		else
		{
			// Not pretending: there are no bots yet. The match runs, alone on the map.
			Y = Para(U, TEXT("Bots kommer i neste oppdatering. Nå spiller du alene på kartet: fin måte å lære rutene og flaggene på."),
				IX, Y, MaxW, AccentColor, 0.8f) + 4.f;
		}
		if (bMatch) Y = Para(U, TEXT("Kampen du er i nå avsluttes."), IX, Y, MaxW, AccentColor, 0.8f);
		const bool bRestart = LocalKind(World) == GSGameKind::Bots;
		Button(U, TEXT("Bots.Go"), bRestart ? TEXT("Start på nytt med disse") : TEXT("Start kampen"), IX, Y + 4.f, FMath::Min(320.f, MaxW), 44.f, true, false, 0, true);
	}

	void DrawExplorePage(const FUi& U, AGSPlayerController* PC, UWorld* World, float IX, float Y, float MaxW, bool bMatch)
	{
		const float TS = 0.85f;
		Y = Para(U, TEXT("Gå fritt rundt i kartet. Ingen kamp, ingen skade og ingen sult."), IX, Y, MaxW, TextColor, TS) + 6.f;
		Y = Para(U, FString::Printf(TEXT("%s slår fri flyging av og på: du flyr gjennom vegger, %s opp og %s ned. I menyen (%s) kan du flytte deg til basene og stille klokka og været."),
			*KeyDisplay(FlyKey()), *GSMenu::KeyName(PC, TEXT("IA_Jump")), *GSMenu::KeyName(PC, TEXT("IA_Crouch")), MenuKeysText(World)),
			IX, Y, MaxW, DimColor, 0.8f) + 8.f;
		if (bMatch) Y = Para(U, TEXT("Kampen du er i nå avsluttes."), IX, Y, MaxW, AccentColor, TS) + 4.f;
		Button(U, TEXT("Explore.Go"), TEXT("Gå inn i kartet"), IX, Y + 4.f, FMath::Min(320.f, MaxW), 44.f, true, false, 0, true);
	}

	void DrawGotoPage(const FUi& U, AGSPlayerController* PC, float IX, float Y, float MaxW)
	{
		const TArray<GSCampus::FBase>& Bases = GSCampus::Bases();
		const float BW = FMath::Min(260.f, (MaxW - 12.f) * 0.5f);
		for (uint8 T = GSTeam::Red; T <= GSTeam::Blue; ++T)
		{
			const int32 I = T == GSTeam::Blue ? 1 : 0;   // same order as AGSGameMode::BasePointM
			if (!Bases.IsValidIndex(I)) continue;
			Text(U.C, FString::Printf(TEXT("%s  ·  %s"), GSTeam::Name(T), Bases[I].Building), IX, Y, TeamTextColor(T), 0.95f);
			Y += LineHeight(0.95f) + 6.f;
			Button(U, TEXT("Goto.Flag"), TEXT("Til flagget"), IX, Y, BW, 38.f, true, false, T);
			Button(U, TEXT("Goto.Spawn"), TEXT("Til startplassen"), IX + BW + 12.f, Y, BW, 38.f, Bases[I].Spawns.Num() > 0, false, T);
			Y += 38.f + 16.f;
		}
		Button(U, TEXT("Goto.Back"), TEXT("Der du sto sist"), IX, Y, BW, 38.f, GSExplore::HasPreviousSpot(PC));
		Y += 38.f + 12.f;
		Para(U, TEXT("Du settes ned på gulvet der, og fri flyging slås av."), IX, Y, MaxW, DimColor, 0.78f);
	}

	static const int32 ClockSteps[] = { 6, 8, 10, 12, 14, 16, 18, 20, 22, 0 };
	constexpr int32 NumClockSteps = 10;

	int32 NearestClockStep(float Hours)
	{
		int32 Best = 0;
		float BestD = 100.f;
		for (int32 I = 0; I < NumClockSteps; ++I)
		{
			float D = FMath::Abs(Hours - ClockSteps[I]);
			D = FMath::Min(D, 24.f - D);   // round the clock
			if (D < BestD) { BestD = D; Best = I; }
		}
		return Best;
	}

	void DrawLightPage(const FUi& U, UWorld* World, float IX, float Y, float MaxW)
	{
		const AGSGameState* GS = World ? World->GetGameState<AGSGameState>() : nullptr;
		const float Hours = GS ? GS->GetTimeOfDayHours() : 12.f;
		const int32 H = FMath::FloorToInt(Hours) % 24, M = FMath::FloorToInt(FMath::Frac(Hours) * 60.f);
		const float CX = IX + FMath::Min(200.f, MaxW * 0.4f);
		const float CW = FMath::Min(MaxW - (CX - IX), 300.f);
		Y = RuleRow(U, TEXT("Klokka"), TEXT("Light.Clock"), FString::Printf(TEXT("%02d:%02d"), H, M), IX, CX, CW, Y, 38.f, GS != nullptr);
		Y = RuleRow(U, TEXT("Vær"), TEXT("Light.Weather"), GS ? FString(GSNames::Weather(GS->Weather)) : FString(), IX, CX, CW, Y, 38.f, GS != nullptr);
		Y += 6.f;
		Para(U, FString::Printf(TEXT("Klokka går videre herfra (et døgn tar %d minutter). Været blir stående til du endrer det."), int32(AGSGameState::DayLengthMinutes)),
			IX, Y, MaxW, DimColor, 0.78f);
	}

	void DrawMain(const FUi& U, AGSPlayerController* PC, UWorld* World)
	{
		FState& S = U.S;
		const float SX = U.C->SizeX, SY = U.C->SizeY;
		const bool bMatch = InMatch(World, S);
		Box(U.C, 0.f, 0.f, SX, SY, FLinearColor(0.f, 0.f, 0.f, 0.30f));

		// --- left panel ---
		const float PW = FMath::Min(FMath::Max(460.f, SX * 0.30f), SX * 0.48f);
		Box(U.C, 0.f, 0.f, PW, SY, PanelColor);
		const float LX = 48.f, BW = PW - 96.f;
		float Y = SY * 0.12f;
		Text(U.C, TEXT("HELGASETR SHOOTOUT"), LX, Y, FLinearColor::White, FMath::Min(1.9f, BW / FMath::Max(1.f, TextWidth(TEXT("HELGASETR SHOOTOUT"), 1.f))));
		Y += LineHeight(1.9f) + 4.f;
		Text(U.C, TEXT("Capture the Flag · Helgasetr, NTNU"), LX, Y, DimColor, 0.8f);
		Y += LineHeight(0.8f) + 18.f;
		Text(U.C, TEXT("Navn"), LX, Y, DimColor, 0.8f);
		Y += LineHeight(0.8f) + 4.f;
		Field(U, TEXT("Name"), S.NameEdit, TEXT("Navnet ditt"), LX, Y, BW, 36.f);
		Y += 36.f + 22.f;
		Y = FMath::Max(Y, SY * 0.32f);

		struct FNav { const TCHAR* Id; FString Label; bool bSelected; bool bBig; float GapBefore; };
		TArray<FNav> Nav;
		const uint8 Kind = LocalKind(World);
		const bool bOnline = S.Page == EPage::Online || S.Page == EPage::Host || S.Page == EPage::Join || S.Page == EPage::Connecting;
		if (bMatch)
		{
			Nav.Add(FNav{ TEXT("Nav.Resume"), TEXT("Tilbake til kampen"), false, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Host"), TEXT("Ny kamp som vert"), bOnline, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Join"), TEXT("Bli med i en annen"), false, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Settings"), TEXT("Innstillinger"), S.Page == EPage::Settings, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Controls"), TEXT("Kontroller"), S.Page == EPage::Controls, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Leave"), TEXT("Forlat kampen"), false, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Quit"), TEXT("Avslutt"), false, false, 0.f });
		}
		else if (Kind == GSGameKind::Bots)
		{
			Nav.Add(FNav{ TEXT("Nav.Resume"), TEXT("Tilbake til kampen"), false, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Restart"), TEXT("Start kampen på nytt"), false, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Bots"), TEXT("Endre kampen ..."), S.Page == EPage::Bots, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Settings"), TEXT("Innstillinger"), S.Page == EPage::Settings, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Controls"), TEXT("Kontroller"), S.Page == EPage::Controls, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.ToMain"), TEXT("Til hovedmenyen"), false, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Quit"), TEXT("Avslutt"), false, false, 0.f });
		}
		else if (Kind == GSGameKind::Explore)
		{
			const FString Fly = FString::Printf(TEXT("Fri flyging: %s (%s)"), GSExplore::IsFlying(PC) ? TEXT("på") : TEXT("av"), *KeyDisplay(FlyKey()));
			Nav.Add(FNav{ TEXT("Nav.Resume"), TEXT("Tilbake"), false, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Fly"), Fly, false, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Goto"), TEXT("Gå til ..."), S.Page == EPage::Goto, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Light"), TEXT("Lys og vær ..."), S.Page == EPage::Light, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Settings"), TEXT("Innstillinger"), S.Page == EPage::Settings, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Controls"), TEXT("Kontroller"), S.Page == EPage::Controls, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.ToMain"), TEXT("Til hovedmenyen"), false, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Quit"), TEXT("Avslutt"), false, false, 0.f });
		}
		else
		{
			// The start menu: three clear choices, then the rest.
			Nav.Add(FNav{ TEXT("Nav.Bots"), TEXT("Spill mot bots"), S.Page == EPage::Bots, true, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Online"), TEXT("Spill online"), bOnline, true, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Explore"), TEXT("Test kartet"), S.Page == EPage::Explore, true, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Settings"), TEXT("Innstillinger"), S.Page == EPage::Settings, false, 12.f });
			Nav.Add(FNav{ TEXT("Nav.Controls"), TEXT("Kontroller"), S.Page == EPage::Controls, false, 0.f });
			Nav.Add(FNav{ TEXT("Nav.Quit"), TEXT("Avslutt"), false, false, 0.f });
		}
		float Extra = 0.f;
		for (const FNav& N : Nav) Extra += N.GapBefore + (N.bBig ? 6.f : 0.f);
		const float BH = FMath::Clamp((SY - Y - 70.f - Extra) / Nav.Num() - 6.f, 30.f, 44.f);
		for (const FNav& N : Nav)
		{
			Y += N.GapBefore;
			const float H = BH + (N.bBig ? 6.f : 0.f);
			// While connecting, only Cancel (on the right) and Quit make sense.
			const bool bEnabled = !S.bConnecting || FCString::Strcmp(N.Id, TEXT("Nav.Quit")) == 0;
			Button(U, N.Id, N.Label, LX, Y, BW, H, bEnabled, N.bSelected);
			Y += H + 6.f;
		}
		if (Kind != GSGameKind::Online && S.bPausedByUs)
		{
			Text(U.C, TEXT("Spillet står på pause"), LX, SY - 58.f, AccentColor, 0.8f);
		}
		Text(U.C, IsEditorPlay(World) ? FString(TEXT("F10 eller P: lukk menyen (Esc stopper Play i editoren)")) : FString(TEXT("Esc, F10 eller P: lukk menyen")),
			LX, SY - 36.f, DimColor, 0.75f);

		if (S.Page == EPage::Root) return;

		// --- right panel ---
		const float X = PW + 40.f, PY = SY * 0.12f;
		const float W = FMath::Min(SX - X - 48.f, 900.f), H = SY * 0.76f;
		if (W < 200.f) return;
		Box(U.C, X, PY, W, H, PanelColor);
		const float IX = X + 24.f, MaxW = W - 48.f, TS = 0.85f;
		Y = PY + 18.f;
		const TCHAR* Title = S.Page == EPage::Host ? (bMatch ? TEXT("Ny kamp som vert") : TEXT("Start som vert"))
			: S.Page == EPage::Join ? TEXT("Bli med")
			: S.Page == EPage::Online ? TEXT("Spill online")
			: S.Page == EPage::Bots ? (Kind == GSGameKind::Bots ? TEXT("Endre kampen") : TEXT("Spill mot bots"))
			: S.Page == EPage::Explore ? TEXT("Test kartet")
			: S.Page == EPage::Goto ? TEXT("Gå til")
			: S.Page == EPage::Light ? TEXT("Lys og vær")
			: S.Page == EPage::Connecting ? TEXT("Kobler til")
			: S.Page == EPage::Settings ? TEXT("Innstillinger") : TEXT("Kontroller");
		Text(U.C, Title, IX, Y, FLinearColor::White, 1.2f);
		Y += LineHeight(1.2f) + 14.f;

		switch (S.Page)
		{
		case EPage::Host:
			DrawHostPart(U, IX, Y, MaxW, bMatch);
			break;
		case EPage::Join:
			DrawJoinPart(U, PC, IX, Y, MaxW);
			break;
		case EPage::Online:
		{
			if (PY + H - Y >= 620.f)
			{
				// Room for both: start as host above, join below. No tabs, fewer clicks.
				Text(U.C, TEXT("Start som vert"), IX, Y, FLinearColor::White, 1.f);
				Y += LineHeight(1.f) + 8.f;
				Y = DrawHostPart(U, IX, Y, MaxW, bMatch);
				Box(U.C, IX, Y, MaxW, 1.f, FLinearColor(1.f, 1.f, 1.f, 0.12f));
				Y += 14.f;
				Text(U.C, TEXT("Bli med"), IX, Y, FLinearColor::White, 1.f);
				Y += LineHeight(1.f) + 8.f;
				DrawJoinPart(U, PC, IX, Y, MaxW);
			}
			else
			{
				// A low window (the editor's Play on a laptop): join first, the host part on its own page.
				Text(U.C, TEXT("Bli med"), IX, Y, FLinearColor::White, 1.f);
				Y += LineHeight(1.f) + 8.f;
				Y = DrawJoinPart(U, PC, IX, Y, MaxW) + 12.f;
				Button(U, TEXT("Nav.HostPage"), TEXT("Start som vert ..."), IX, Y, FMath::Min(300.f, MaxW), 40.f);
			}
			break;
		}
		case EPage::Bots:
			DrawBotsPage(U, World, IX, Y, MaxW, PY + H, bMatch);
			break;
		case EPage::Explore:
			DrawExplorePage(U, PC, World, IX, Y, MaxW, bMatch);
			break;
		case EPage::Goto:
			DrawGotoPage(U, PC, IX, Y, MaxW);
			break;
		case EPage::Light:
			DrawLightPage(U, World, IX, Y, MaxW);
			break;
		case EPage::Connecting:
		{
			const int32 Secs = FMath::Max(0, FMath::FloorToInt(float(FPlatformTime::Seconds() - S.ConnectStart)));
			Text(U.C, FString::Printf(TEXT("Kobler til %s ... %d s"), *S.ConnectAddress, Secs), IX, Y, TextColor, 1.f);
			Y += LineHeight(1.f) + 8.f;
			Y = Para(U, FString::Printf(TEXT("Forsøket gis opp etter %d sekunder."), int32(ConnectTimeoutSeconds)), IX, Y, MaxW, DimColor, TS) + 10.f;
			Button(U, TEXT("Conn.Cancel"), TEXT("Avbryt"), IX, Y, FMath::Min(200.f, MaxW), 44.f);
			break;
		}
		case EPage::Settings:
			DrawSettings(U, PC, X, Y, W, PY + H - Y);
			break;
		case EPage::Controls:
			DrawControls(U, PC, X, Y, W, PY + H - Y);
			break;
		default:
			break;
		}
	}

	void DrawTeam(const FUi& U, AGSPlayerController* PC, UWorld* World)
	{
		const AGSGameState* GS = World->GetGameState<AGSGameState>();
		const AGSPlayerState* PS = PC->GetPlayerState<AGSPlayerState>();
		if (!GS) return;
		const float SX = U.C->SizeX, SY = U.C->SizeY;
		const int32 Most = FMath::Max(TeamCountOf(GS, GSTeam::Red), TeamCountOf(GS, GSTeam::Blue));
		const int32 RowsShown = FMath::Clamp(Most, 4, 10);
		const bool bHost = World->GetNetMode() == NM_ListenServer && LocalAddresses().Num() > 0;
		const float W = FMath::Min(760.f, SX - 64.f);
		const float RowH = 26.f;
		const float H = 18.f + 34.f + 26.f + 34.f + RowsShown * RowH + 8.f + 36.f + 22.f + 12.f + 40.f + (bHost ? 26.f : 0.f) + 30.f;
		const float X = (SX - W) * 0.5f;
		float Y = FMath::Max(16.f, (SY - H) * 0.5f);
		Box(U.C, X, Y, W, H, PanelColor);
		Frame(U.C, X, Y, W, H, FLinearColor(1.f, 1.f, 1.f, 0.12f));
		U.Hit(NAME_None, X, Y, W, H);   // clicks on the panel itself do nothing
		Y += 18.f;
		Text(U.C, TEXT("Velg lag"), X + 24.f, Y, FLinearColor::White, 1.2f);
		Y += 34.f;
		{
			FString Info = TEXT("Oppvarming - ingen skade.");
			if (GS->Phase == EGSMatchPhase::Countdown) Info = FString::Printf(TEXT("Kampen starter om %d s."), FMath::CeilToInt(GS->PhaseSecondsLeft()));
			else if (GS->Phase == EGSMatchPhase::Lobby && GS->PhaseEndsAt > 0.f) Info += FString::Printf(TEXT(" Starter av seg selv om %d s."), FMath::CeilToInt(GS->PhaseSecondsLeft()));
			Text(U.C, Info, X + 24.f, Y, DimColor, 0.85f);
		}
		Y += 26.f;

		const float ColW = (W - 48.f - 16.f) * 0.5f;
		const float ColsTop = Y;
		for (uint8 T = GSTeam::Red; T <= GSTeam::Blue; ++T)
		{
			const float CX = X + 24.f + (T - 1) * (ColW + 16.f);
			float CY = ColsTop;
			Box(U.C, CX, CY, ColW, 34.f, GSTeam::Color(T).CopyWithNewOpacity(0.85f));
			Text(U.C, FString::Printf(TEXT("%s  %d/%d"), GSTeam::Name(T), TeamCountOf(GS, T), AGSGameMode::MaxPerTeam), CX + 12.f, CY + (34.f - LineHeight(0.95f)) * 0.5f, FLinearColor::White, 0.95f);
			CY += 34.f;
			Box(U.C, CX, CY, ColW, RowsShown * RowH, FLinearColor(0.f, 0.f, 0.f, 0.25f));
			int32 Shown = 0;
			for (const APlayerState* P : GS->PlayerArray)
			{
				const AGSPlayerState* Other = Cast<AGSPlayerState>(P);
				if (!Other || Other->Team != T || Shown >= RowsShown) continue;
				const float RY = CY + Shown * RowH;
				if (Other == PS) Box(U.C, CX, RY, ColW, RowH, FLinearColor(1.f, 1.f, 1.f, 0.08f));
				FString Name = Other->GetPlayerName();
				while (Name.Len() > 3 && TextWidth(Name, 0.85f) > ColW - 80.f) Name.LeftChopInline(1);
				Text(U.C, Name, CX + 12.f, RY + (RowH - LineHeight(0.85f)) * 0.5f, Other == PS ? FLinearColor::White : TextColor, 0.85f);
				if (Other->bReady) Text(U.C, TEXT("klar"), CX + ColW - 12.f, RY + (RowH - LineHeight(0.8f)) * 0.5f, ReadyColor, 0.8f, 2);
				++Shown;
			}
			CY += RowsShown * RowH + 8.f;
			FString Block = TeamJoinBlock(GS, PS, T);
			// The server takes one switch per player every TeamSwitchCooldown: say so instead of a click that does nothing.
			if (Block.IsEmpty() && FPlatformTime::Seconds() - U.S.TeamClickAt < AGSGameMode::TeamSwitchCooldown) Block = TEXT("Vent litt");
			Button(U, TEXT("Team.Join"), T == GSTeam::Red ? TEXT("Bli med på rødt lag") : TEXT("Bli med på blått lag"), CX, CY, ColW, 36.f, Block.IsEmpty(), false, T);
			Text(U.C, Block, CX + 16.f, CY + 38.f, DimColor, 0.75f);
		}
		Y = ColsTop + 34.f + RowsShown * RowH + 8.f + 36.f + 22.f + 12.f;

		const bool bCanReady = PS && GS->Phase != EGSMatchPhase::Playing && GS->Phase != EGSMatchPhase::Ended;
		Button(U, TEXT("Team.Ready"), (PS && PS->bReady) ? TEXT("Ikke klar likevel") : TEXT("Klar"), X + 24.f, Y, ColW, 40.f, bCanReady, false, 0, true);
		Button(U, TEXT("Team.Close"), TEXT("Varm opp"), X + 24.f + ColW + 16.f, Y, ColW, 40.f);
		Y += 40.f + 8.f;
		if (bHost)
		{
			Text(U.C, FString::Printf(TEXT("Venner blir med på: %s"), *WithPort(LocalAddresses()[0], HostPort(World))), X + 24.f, Y, TextColor, 0.85f);
			Y += 26.f;
		}
		// The panel opens by itself in the lobby: say plainly how to get out of it (walking closes it too, see OnKey).
		Text(U.C, FString::Printf(TEXT("%s = klar · %s, P, %s eller gå (WASD) lukker · %s åpner igjen"), *GSMenu::KeyName(PC, TEXT("IA_Ready")),
			BackKeyText(World), *GSMenu::KeyName(PC, TEXT("GS_Team")), *GSMenu::KeyName(PC, TEXT("GS_Team"))),
			X + 24.f, Y, TextColor, 0.8f);
	}

	/** "Du er vert. Kampen avsluttes for alle 3 andre." / "Du forlater kampen du er i." / empty when alone. */
	FString LeaveText(UWorld* World, const FState& S)
	{
		if (!InMatch(World, S)) return FString();
		if (World->GetNetMode() == NM_ListenServer)
		{
			const AGameStateBase* GS = World->GetGameState();
			const int32 Others = GS ? GS->PlayerArray.Num() - 1 : 0;
			if (Others == 1) return TEXT("Du er vert. Kampen avsluttes for den andre spilleren.");
			if (Others > 1) return FString::Printf(TEXT("Du er vert. Kampen avsluttes for alle %d andre."), Others);
		}
		return TEXT("Du forlater kampen du er i.");
	}

	void DrawDialog(const FUi& U, UWorld* World)
	{
		FState& S = U.S;
		const float SX = U.C->SizeX, SY = U.C->SizeY;
		FString Title, Body, Detail, Yes;
		bool bCancel = true;
		switch (S.Dialog)
		{
		case EDialog::Host: Title = TEXT("Starte en ny kamp?"); Body = LeaveText(World, S); Yes = TEXT("Ja, start ny"); break;
		case EDialog::Join: Title = TEXT("Bli med i en annen kamp?"); Body = LeaveText(World, S); Yes = TEXT("Ja, koble til"); break;
		case EDialog::Leave:
			if (InLocalGame(World))
			{
				Title = TEXT("Tilbake til hovedmenyen?");
				Body = LocalKind(World) == GSGameKind::Explore ? TEXT("Testen avsluttes.") : TEXT("Kampen avsluttes.");
				Yes = TEXT("Ja, til hovedmenyen");
			}
			else
			{
				Title = TEXT("Forlate kampen?"); Body = LeaveText(World, S); Yes = TEXT("Ja, forlat");
			}
			break;
		case EDialog::Restart:
			Title = TEXT("Starte kampen på nytt?");
			Body = TEXT("Kampen du er i nå avsluttes, og en ny starter med valgene på siden «Endre kampen».");
			Yes = TEXT("Ja, start på nytt");
			break;
		case EDialog::Quit: Title = TEXT("Avslutte spillet?"); Body = LeaveText(World, S); Yes = TEXT("Avslutt"); break;
		case EDialog::Message: Title = S.MsgTitle; Body = S.MsgText; Detail = S.MsgDetail; Yes = TEXT("OK"); bCancel = false; break;
		default: return;
		}
		Box(U.C, 0.f, 0.f, SX, SY, FLinearColor(0.f, 0.f, 0.f, 0.45f));
		const float W = FMath::Min(560.f, SX - 32.f), MaxW = W - 48.f;
		const TArray<FString> BodyLines = Wrap(Body, 0.9f, MaxW);
		const TArray<FString> DetailLines = Wrap(Detail, 0.75f, MaxW);
		const float BodyLH = LineHeight(0.9f) + 3.f, DetailLH = LineHeight(0.75f) + 2.f;
		const float H = 22.f + LineHeight(1.2f) + 14.f + BodyLines.Num() * BodyLH + (DetailLines.Num() > 0 ? 6.f + DetailLines.Num() * DetailLH : 0.f) + 18.f + 42.f + 22.f;
		const float X = (SX - W) * 0.5f;
		float Y = (SY - H) * 0.5f;
		Box(U.C, X, Y, W, H, FLinearColor(0.015f, 0.018f, 0.022f, 0.96f));
		Frame(U.C, X, Y, W, H, FLinearColor(1.f, 1.f, 1.f, 0.18f));
		Y += 22.f;
		Text(U.C, Title, X + 24.f, Y, FLinearColor::White, 1.2f);
		Y += LineHeight(1.2f) + 14.f;
		for (const FString& L : BodyLines) { Text(U.C, L, X + 24.f, Y, TextColor, 0.9f); Y += BodyLH; }
		if (DetailLines.Num() > 0)
		{
			Y += 6.f;
			for (const FString& L : DetailLines) { Text(U.C, L, X + 24.f, Y, DimColor, 0.75f); Y += DetailLH; }
		}
		Y += 18.f;
		const float BW = (MaxW - 12.f) * 0.5f;
		Button(U, TEXT("Dlg.Yes"), Yes, X + 24.f, Y, BW, 42.f, true, false, 0, true);
		if (bCancel) Button(U, TEXT("Dlg.No"), TEXT("Avbryt"), X + 24.f + BW + 12.f, Y, BW, 42.f);
	}

	// ------------------------------------------------------------------ actions
	void DialogYes(AGSPlayerController* PC, UWorld* World, FState& S)
	{
		const EDialog D = S.Dialog;
		S.Dialog = EDialog::None;
		switch (D)
		{
		case EDialog::Host:
			StartPending(PC, S);   // host, bots or test, with the choices from the menu
			break;
		case EDialog::Restart:
			StartBots(PC, S);
			break;
		case EDialog::Join:
			if (!S.bMainOpen) OpenMainImpl(PC, S);
			StartJoin(PC, World, S, S.PendingJoin);
			break;
		case EDialog::Leave:
			// The start menu opens in the next world, also in the editor's Play (it does not open by itself there).
			S.bOpenMainAfterTravel = true;
			ReloadAlone(PC, S);
			break;
		case EDialog::Quit:
			UKismetSystemLibrary::QuitGame(PC, PC, EQuitPreference::Quit, false);
			break;
		case EDialog::Message:
			S.bMsgPending = false;
			break;
		default:
			break;
		}
	}

	void DialogNo(FState& S)
	{
		if (S.Dialog == EDialog::Message) S.bMsgPending = false;
		S.Dialog = EDialog::None;
	}

	void CloseTeam(FState& S)
	{
		S.bTeamOpen = false;
	}

	void HandleClick(AGSPlayerController* PC, UWorld* World, FState& S, const FHit& Hit)
	{
		const FString Id = Hit.Id.ToString();
		const bool bField = Id == TEXT("Name") || Id == TEXT("Address");
		if (!bField) SetFocus(PC, S, NAME_None);

		if (Hit.Id == NAME_None) return;
		if (bField) { SetFocus(PC, S, Hit.Id); return; }
		if (Id == TEXT("Dlg.Yes")) { DialogYes(PC, World, S); return; }
		if (Id == TEXT("Dlg.No")) { DialogNo(S); return; }

		// A new game: confirmed first when it ends a game in progress (online, a bot match or the test).
		auto Begin = [&](EStart What)
		{
			S.PendingStart = What;
			if (InMatch(World, S)) S.Dialog = EDialog::Host;
			else if (What == EStart::Bots && LocalKind(World) == GSGameKind::Bots) S.Dialog = EDialog::Restart;
			else if (InLocalGame(World)) S.Dialog = EDialog::Host;
			else StartPending(PC, S);
		};

		if (Id == TEXT("Nav.Host") || Id == TEXT("Nav.Online")) SetPage(PC, S, EPage::Online);
		else if (Id == TEXT("Nav.Join")) { SetPage(PC, S, EPage::Online); S.Focus = FName(TEXT("Address")); }
		else if (Id == TEXT("Nav.HostPage")) SetPage(PC, S, EPage::Host);
		else if (Id == TEXT("Nav.Bots")) SetPage(PC, S, EPage::Bots);
		else if (Id == TEXT("Nav.Explore")) SetPage(PC, S, EPage::Explore);
		else if (Id == TEXT("Nav.Goto")) SetPage(PC, S, EPage::Goto);
		else if (Id == TEXT("Nav.Light")) SetPage(PC, S, EPage::Light);
		else if (Id == TEXT("Nav.Settings")) SetPage(PC, S, EPage::Settings);
		else if (Id == TEXT("Nav.Controls")) SetPage(PC, S, EPage::Controls);
		else if (Id == TEXT("Nav.Resume")) CloseMain(PC, S, *Id);
		else if (Id == TEXT("Nav.Restart")) S.Dialog = EDialog::Restart;
		else if (Id == TEXT("Nav.Fly"))
		{
			CloseMain(PC, S, *Id);   // fly at once: the menu closes and the pause ends
			GSExplore::ToggleFly(PC);
		}
		else if (Id == TEXT("Nav.Leave") || Id == TEXT("Nav.ToMain")) S.Dialog = EDialog::Leave;
		else if (Id == TEXT("Nav.Quit")) S.Dialog = EDialog::Quit;
		else if (Id == TEXT("Host.Go")) Begin(EStart::Host);
		else if (Id == TEXT("Bots.Go")) Begin(EStart::Bots);
		else if (Id == TEXT("Explore.Go")) Begin(EStart::Explore);
		else if (Id.StartsWith(TEXT("Rule."))) ChangeRule(Id, Hit.Arg);
		else if (Id == TEXT("Goto.Flag") || Id == TEXT("Goto.Spawn"))
		{
			const TArray<GSCampus::FBase>& Bases = GSCampus::Bases();
			const int32 I = Hit.Arg == GSTeam::Blue ? 1 : 0;
			if (Bases.IsValidIndex(I))
			{
				const GSCampus::FBase& B = Bases[I];
				FVector Target = B.Flag * 100.f;
				if (Id == TEXT("Goto.Spawn") && B.Spawns.Num() > 0) Target = FVector(B.Spawns[0].X * 100.f, B.Spawns[0].Y * 100.f, B.Flag.Z * 100.f);
				UE_LOG(LogTemp, Log, TEXT("GSMenu: gå til %s, %s"), GSTeam::Name(uint8(Hit.Arg)), Id == TEXT("Goto.Flag") ? TEXT("flagget") : TEXT("startplassen"));
				CloseMain(PC, S, *Id);
				GSExplore::Teleport(PC, Target, B.SpawnYaw);
			}
		}
		else if (Id == TEXT("Goto.Back"))
		{
			CloseMain(PC, S, *Id);
			GSExplore::TeleportBack(PC);
		}
		else if (Id == TEXT("Light.Clock"))
		{
			const AGSGameState* GS = World->GetGameState<AGSGameState>();
			const int32 Cur = NearestClockStep(GS ? GS->GetTimeOfDayHours() : 12.f);
			GSExplore::SetClock(PC, float(ClockSteps[(Cur + Hit.Arg + NumClockSteps) % NumClockSteps]));
		}
		else if (Id == TEXT("Light.Weather"))
		{
			const AGSGameState* GS = World->GetGameState<AGSGameState>();
			const int32 N = int32(EGSWeather::MAX);
			const int32 Cur = GS ? int32(GS->Weather) : 0;
			GSExplore::SetWeather(PC, EGSWeather(uint8((Cur + Hit.Arg + N) % N)));
		}
		else if (Id == TEXT("Join.Go")) TryJoin(PC, World, S);
		else if (Id == TEXT("Join.Last")) { S.AddressEdit = PC->LastJoinAddress; S.Focus = FName(TEXT("Address")); }
		else if (Id == TEXT("Conn.Cancel")) CancelConnect(PC, World, S);
		else if (Id.StartsWith(TEXT("Set."))) ChangeSetting(PC, Id, Hit.Arg);
		else if (Id == TEXT("Sens") || Id == TEXT("Volume"))
		{
			S.DragSlider = Hit.Id;
			S.DragX = Hit.X0;
			S.DragW = FMath::Max(1.f, Hit.X1 - Hit.X0);
		}
		else if (Id == TEXT("Key.Row")) { S.ListenRow = Hit.Arg; S.KeyNote.Empty(); }
		else if (Id == TEXT("Key.Reset")) ResetKeys(PC, S);
		else if (Id == TEXT("Help.Reset"))
		{
			GSHelp::ResetHints();
			S.ListenRow = INDEX_NONE;
			S.KeyNote = TEXT("Hintene vises på nytt.");
		}
		else if (Id == TEXT("Team.Join"))
		{
			if (AGSPlayerState* PS = PC->GetPlayerState<AGSPlayerState>())
			{
				PS->ServerRequestTeam(uint8(Hit.Arg));   // the server decides
				S.TeamClickAt = FPlatformTime::Seconds();
			}
		}
		else if (Id == TEXT("Team.Ready")) PC->ToggleReady();
		else if (Id == TEXT("Team.Close")) CloseTeam(S);
	}

	void EndDrag(FState& S)
	{
		if (S.DragSlider != NAME_None)
		{
			S.DragSlider = NAME_None;
			CfgFlush();
		}
	}

	/** Esc: the way out that always works. Dialog, text field, page and the menu itself close in one press, and the
	 *  controls go back to the game (only "waiting for a new key" takes Esc as "cancel", see OnKey). */
	void Back(AGSPlayerController* PC, UWorld* World, FState& S, const TCHAR* Why)
	{
		UE_LOG(LogTemp, Log, TEXT("GSMenu: %s lukker alt (meny %d, lagpanel %d, dialog %d)"), Why, S.bMainOpen ? 1 : 0, S.bTeamOpen ? 1 : 0, int32(S.Dialog));
		if (S.Dialog != EDialog::None) DialogNo(S);
		S.Dialog = EDialog::None;
		if (S.bMainOpen && S.Page == EPage::Connecting) CancelConnect(PC, World, S);
		if (S.bMainOpen) CloseMain(PC, S, Why);
		SetFocus(PC, S, NAME_None);
		if (S.bTeamOpen) CloseTeam(S);
	}

	bool AddressChar(TCHAR Ch)
	{
		return (Ch >= TEXT('0') && Ch <= TEXT('9')) || (Ch >= TEXT('a') && Ch <= TEXT('z')) || (Ch >= TEXT('A') && Ch <= TEXT('Z'))
			|| Ch == TEXT('.') || Ch == TEXT(':') || Ch == TEXT('-');
	}

	void TypeChar(FState& S, TCHAR Ch)
	{
		if (S.Focus == FName(TEXT("Address")))
		{
			if (AddressChar(Ch) && S.AddressEdit.Len() < 64) S.AddressEdit.AppendChar(Ch);
		}
		else if (S.Focus == FName(TEXT("Name")))
		{
			if (NameChar(Ch) && S.NameEdit.Len() < 20) S.NameEdit.AppendChar(Ch);
		}
	}

	/** Reserve when the viewport class is not ours (no InputChar): digits, '.', A–Z, space, '-'. 0 = no character. */
	TCHAR KeyToChar(const FKey& K, bool bShift)
	{
		static const FKey Digits[] = { EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
		static const FKey Pad[] = { EKeys::NumPadZero, EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour, EKeys::NumPadFive, EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight, EKeys::NumPadNine };
		for (int32 I = 0; I < 10; ++I)
		{
			if (K == Digits[I] || K == Pad[I]) return TCHAR(TEXT('0') + I);
		}
		if (K == EKeys::Period || K == EKeys::Decimal) return bShift ? TEXT(':') : TEXT('.');
		if (K == EKeys::Hyphen || K == EKeys::Subtract) return TEXT('-');
		if (K == EKeys::SpaceBar) return TEXT(' ');
		const FString Name = K.ToString();
		if (Name.Len() == 1 && Name[0] >= TEXT('A') && Name[0] <= TEXT('Z')) return bShift ? Name[0] : TCHAR(Name[0] + 32);
		return 0;
	}

	FVector2D MouseOnCanvas(const FState& S, FViewport* Viewport)
	{
		if (!Viewport) return FVector2D(-10000.f, -10000.f);
		FIntPoint P;
		Viewport->GetMousePos(P);
		FVector2D Out(P);
		const FIntPoint Size = Viewport->GetSizeXY();
		if (Size.X > 0 && Size.Y > 0 && S.CanvasSize.X > 0.f && S.CanvasSize.Y > 0.f)
		{
			Out.X *= S.CanvasSize.X / Size.X;
			Out.Y *= S.CanvasSize.Y / Size.Y;
		}
		return Out;
	}

	// ------------------------------------------------------------------ input hooks
	bool OnKey(FInputKeyEventArgs& Args, TWeakObjectPtr<UGameViewportClient> WeakVC)
	{
		UGameViewportClient* VC = WeakVC.Get();
		FState* SP = FindState(VC);
		if (!VC || !SP || !GEngine) return false;
		FState& S = *SP;
		const FKey Key = Args.Key;
		const EInputEvent Ev = Args.Event;
		if (Key == EKeys::LeftShift || Key == EKeys::RightShift) S.bShift = Ev != IE_Released;
		if (Key == EKeys::LeftControl || Key == EKeys::RightControl) S.bCtrl = Ev != IE_Released;

		UWorld* World = VC->GetWorld();
		ULocalPlayer* LP = GEngine->GetFirstGamePlayer(VC);
		AGSPlayerController* PC = LP ? Cast<AGSPlayerController>(LP->PlayerController) : nullptr;
		if (!PC || !World || S.World.Get() != World) return false;

		if (!IsModalState(S))
		{
			const bool bClick = Key == EKeys::LeftMouseButton && (Ev == IE_Pressed || Ev == IE_DoubleClick);
			if (Ev != IE_Pressed && !bClick) return false;
			// The combat team's Esc menu: the three buttons that leave a match go through our menu and its
			// confirm dialogs instead (until their own MenuClick calls GSMenu).
			if (PC->bMenuOpen && bClick && Args.Viewport)
			{
				FIntPoint P;
				Args.Viewport->GetMousePos(P);
				const AHUD* HUD = PC->GetHUD();
				const FHUDHitBox* HitBox = HUD ? HUD->GetHitBoxAtCoordinates(FVector2D(P), true) : nullptr;
				if (HitBox)
				{
					const FName Name = HitBox->GetName();
					if (Name == FName(TEXT("Host")) || Name == FName(TEXT("Join")))
					{
						PC->CloseMenu();
						OpenMainImpl(PC, S, TEXT("knapp i Kamp-menyen"));
						return true;
					}
					if (Name == FName(TEXT("Quit")))
					{
						PC->CloseMenu();
						S.Dialog = EDialog::Quit;
						SyncModal(PC, S);
						return true;
					}
				}
				return false;
			}
			// Esc / F10 open our menu (it has everything the combat team's Esc menu has). Their menu still closes with
			// Esc if it is open, and Esc still leaves a PC terminal first.
			// In the editor's Play Esc never gets here (it stops Play), so F10 and P are the keys there.
			if (Ev == IE_Pressed && (IsBackKey(Key) || Key == EKeys::P) && !PC->bMenuOpen && !PC->IsOnTerminal())
			{
				const TCHAR* Why = TEXT("P");
				if (Key == EKeys::Escape) Why = TEXT("Esc");
				else if (Key == EKeys::F10) Why = TEXT("F10");
				OpenMainImpl(PC, S, Why);
				if (IsModalState(S)) return true;
				// The cursor did not come on: give up at once, so the menu never holds the controls without it.
				UE_LOG(LogTemp, Warning, TEXT("GSMenu: musepekeren kom ikke på, menyen lukkes igjen"));
				CloseMain(PC, S, TEXT("ingen musepeker"));
				return false;
			}
			// Test kartet: the fly key (N) turns free flying on and off. Elsewhere N goes on to the game as before.
			if (Ev == IE_Pressed && Key == FlyKey() && LocalKind(World) == GSGameKind::Explore && !PC->bMenuOpen && !PC->IsOnTerminal())
			{
				GSExplore::ToggleFly(PC);
				return true;
			}
			// Team panel in the lobby
			if (Ev == IE_Pressed && Key == TeamKey() && !PC->bMenuOpen && !PC->IsOnTerminal())
			{
				const AGSGameState* GS = World->GetGameState<AGSGameState>();
				if (GS && (GS->Phase == EGSMatchPhase::Lobby || GS->Phase == EGSMatchPhase::Countdown))
				{
					S.bTeamOpen = true;
					SyncModal(PC, S);
					return true;
				}
			}
			return false;
		}

		// --- modal: nothing reaches the game ---
		if (Ev == IE_Released)
		{
			if (Key == EKeys::LeftMouseButton) EndDrag(S);
			return true;
		}
		const bool bPress = Ev == IE_Pressed || (Ev == IE_DoubleClick && Key == EKeys::LeftMouseButton);
		const bool bRepeatErase = Ev == IE_Repeat && Key == EKeys::BackSpace;
		if (!bPress && !bRepeatErase) return true;
		if (Key.IsGamepadKey() || Key.IsTouch()) return true;

		// The team panel opens by itself in the lobby. A new player who just starts walking should not be stuck
		// behind it: a walking key closes it and goes on to the game, so the walk starts at once (L opens it again).
		if (bPress && Key != EKeys::LeftMouseButton && S.bTeamOpen && !S.bMainOpen && S.Dialog == EDialog::None
			&& S.Focus == NAME_None && S.ListenRow == INDEX_NONE && IsActionKey(PC, TEXT("IA_Move"), Key))
		{
			UE_LOG(LogTemp, Log, TEXT("GSMenu: gåtast %s lukker lagpanelet, tasten går videre til spillet"), *Key.ToString());
			CloseTeam(S);
			SyncModal(PC, S);
			return false;
		}

		if (S.ListenRow != INDEX_NONE)
		{
			if (!bPress) return true;
			if (IsBackKey(Key))
			{
				S.ListenRow = INDEX_NONE;
				UE_LOG(LogTemp, Log, TEXT("GSMenu: %s avbryter ny tast"), *Key.ToString());
			}
			else AssignKey(PC, S, S.ListenRow, Key);
			return true;
		}

		if (Key == EKeys::LeftMouseButton)
		{
			const FVector2D Pos = MouseOnCanvas(S, Args.Viewport);
			const FIntPoint VSize = Args.Viewport ? Args.Viewport->GetSizeXY() : FIntPoint::ZeroValue;
			bool bFound = false;
			for (int32 I = S.Hits.Num() - 1; I >= 0; --I)
			{
				const FHit& H = S.Hits[I];
				if (Pos.X >= H.X0 && Pos.X <= H.X1 && Pos.Y >= H.Y0 && Pos.Y <= H.Y1)
				{
					const FHit Copy = H;   // the click may change the list
					UE_LOG(LogTemp, Log, TEXT("GSMenu: klikk (%.0f, %.0f) på lerret %.0fx%.0f, vindu %dx%d -> %s (%d)"),
						Pos.X, Pos.Y, S.CanvasSize.X, S.CanvasSize.Y, VSize.X, VSize.Y, *Copy.Id.ToString(), Copy.Arg);
					HandleClick(PC, World, S, Copy);
					bFound = true;
					break;
				}
			}
			if (!bFound)
			{
				UE_LOG(LogTemp, Log, TEXT("GSMenu: klikk (%.0f, %.0f) på lerret %.0fx%.0f, vindu %dx%d -> ingen knapp (%d knapper)"),
					Pos.X, Pos.Y, S.CanvasSize.X, S.CanvasSize.Y, VSize.X, VSize.Y, S.Hits.Num());
				SetFocus(PC, S, NAME_None);
			}
		}
		else if (IsBackKey(Key) || (Key == EKeys::P && S.Focus == NAME_None))
		{
			// Esc, F10 (the one that works in the editor's Play) or P (not while typing a name or an address).
			Back(PC, World, S, *Key.ToString());
		}
		else if (Key == EKeys::Enter)
		{
			if (S.Dialog != EDialog::None) DialogYes(PC, World, S);
			else if (S.Focus == FName(TEXT("Address"))) TryJoin(PC, World, S);
			else if (S.Focus == FName(TEXT("Name"))) SetFocus(PC, S, NAME_None);
			else if (S.bTeamOpen) PC->ToggleReady();
		}
		else if (S.bTeamOpen && !S.bMainOpen && S.Dialog == EDialog::None && S.Focus == NAME_None && Key == ActionKey(PC, TEXT("IA_Ready")))
		{
			PC->ToggleReady();   // the player's own Ready key (the panel names it), as well as Enter
		}
		else if (Key == EKeys::BackSpace)
		{
			if (S.Focus == FName(TEXT("Address"))) S.AddressEdit.LeftChopInline(1);
			else if (S.Focus == FName(TEXT("Name"))) S.NameEdit.LeftChopInline(1);
		}
		else if (Key == EKeys::V && S.bCtrl)
		{
			FString Clip;
			FPlatformApplicationMisc::ClipboardPaste(Clip);
			for (int32 I = 0; I < Clip.Len() && I < 256; ++I) TypeChar(S, Clip[I]);
		}
		else if (S.Focus != NAME_None && !S.bCtrl && !Cast<UGSGameViewportClient>(VC))
		{
			if (const TCHAR Ch = KeyToChar(Key, S.bShift)) TypeChar(S, Ch);
		}
		else if (S.Focus == NAME_None && S.bTeamOpen && S.Dialog == EDialog::None && Key == TeamKey())
		{
			CloseTeam(S);   // the same key closes the panel again
		}
		SyncModal(PC, S);
		return true;
	}

	bool OnAxis(FInputKeyEventArgs& Args, float& Delta, float& DeltaTime, int32& NumSamples, TWeakObjectPtr<UGameViewportClient> WeakVC)
	{
		const FState* S = FindState(WeakVC.Get());
		return S && IsModalState(*S);   // mouse look and the wheel do not reach the game
	}

	/** Puts our key and axis hooks on the window, in front of any hook already there (that one still runs after ours).
	 *  The editor's "Play in selected viewport" binds its own key hook before the game starts; checking only
	 *  IsBound() left ours out, so no click or Esc ever reached the menu while it swallowed mouse look. */
	void InstallHooks(UGameViewportClient* VC, FState& S)
	{
		const TWeakObjectPtr<UGameViewportClient> WeakVC(VC);
		FOverrideInputKeyHandler& KeyHook = VC->OnOverrideInputKey();
		if (!KeyHook.IsBound() || KeyHook.GetHandle() != S.KeyHook)
		{
			const bool bChained = KeyHook.IsBound();
			const FOverrideInputKeyHandler Old = KeyHook;
			KeyHook.BindLambda([WeakVC, Old](FInputKeyEventArgs& Args) -> bool
			{
				// If someone wraps ours and we wrap theirs again, ours still runs only once per key.
				static int32 Depth = 0;
				const bool bOurs = Depth == 0;
				TGuardValue<int32> Guard(Depth, Depth + 1);
				if (bOurs && OnKey(Args, WeakVC)) return true;
				return Old.IsBound() && Old.Execute(Args);
			});
			S.KeyHook = KeyHook.GetHandle();
			UE_LOG(LogTemp, Log, TEXT("GSMenu: tastekrok koblet til vinduet%s"), bChained ? TEXT(" (foran en krok som var der fra før, f.eks. editorens)") : TEXT(""));
		}
		FOverrideInputAxisHandler& AxisHook = VC->OnOverrideInputAxis();
		if (!AxisHook.IsBound() || AxisHook.GetHandle() != S.AxisHook)
		{
			const bool bChained = AxisHook.IsBound();
			const FOverrideInputAxisHandler Old = AxisHook;
			AxisHook.BindLambda([WeakVC, Old](FInputKeyEventArgs& Args, float& Delta, float& DeltaTime, int32& NumSamples) -> bool
			{
				static int32 Depth = 0;
				const bool bOurs = Depth == 0;
				TGuardValue<int32> Guard(Depth, Depth + 1);
				if (bOurs && OnAxis(Args, Delta, DeltaTime, NumSamples, WeakVC)) return true;
				return Old.IsBound() && Old.Execute(Args, Delta, DeltaTime, NumSamples);
			});
			S.AxisHook = AxisHook.GetHandle();
			UE_LOG(LogTemp, Log, TEXT("GSMenu: musekrok koblet til vinduet%s"), bChained ? TEXT(" (foran en krok som var der fra før)") : TEXT(""));
		}
		S.bHookLive = KeyHook.IsBound() && KeyHook.GetHandle() == S.KeyHook;
	}

	// ------------------------------------------------------------------ network failures
	FState* StateForFailure(UWorld* W)
	{
		if (FState* S = StateFor(W)) return S;
		return GEngine ? StateForVC(GEngine->GameViewport) : nullptr;
	}

	bool SkipFailure(const FState& S)
	{
		const double Now = FPlatformTime::Seconds();
		return Now < S.LeavingUntil || (S.bMsgPending && Now - S.MsgTime < 2.0);
	}

	void OnNetworkFailure(UWorld* W, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Err)
	{
		FState* SP = StateForFailure(W);
		if (!SP) return;
		FState& S = *SP;
		const bool bClient = W && W->GetNetMode() == NM_Client;
		const bool bListenFailure = Type == ENetworkFailure::NetDriverListenFailure;
		if (!bListenFailure && !bClient && !S.bConnecting) return;
		// Starting as host opens the "we left on purpose" window, and this failure comes inside it: never skip it.
		if (!bListenFailure && SkipFailure(S)) return;
		const FString Where = S.ConnectAddress.IsEmpty() ? FString(TEXT("adressen")) : S.ConnectAddress;
		switch (Type)
		{
		case ENetworkFailure::ConnectionLost:
		case ENetworkFailure::ConnectionTimeout:
			if (S.bConnecting) ShowMessage(S, TEXT("Kunne ikke koble til"), FString::Printf(TEXT("Fant ikke noe spill på %s."), *Where), Err);
			else ShowMessage(S, TEXT("Forbindelsen ble brutt"), TEXT("Verten kan ha avsluttet spillet, eller nettet falt ut."), Err);
			break;
		case ENetworkFailure::PendingConnectionFailure:
			ShowMessage(S, TEXT("Kunne ikke koble til"), FString::Printf(TEXT("Fant ikke noe spill på %s."), *Where), Err);
			break;
		case ENetworkFailure::FailureReceived:
			ShowMessage(S, TEXT("Verten avviste tilkoblingen"), TEXT("Kampen kan være full, eller verten avsluttet."), Err);
			break;
		case ENetworkFailure::OutdatedClient:
		case ENetworkFailure::OutdatedServer:
		case ENetworkFailure::NetChecksumMismatch:
		case ENetworkFailure::NetGuidMismatch:
			ShowMessage(S, TEXT("Ulike versjoner"), TEXT("Du og verten har ikke samme versjon av spillet."), Err);
			break;
		case ENetworkFailure::NetDriverListenFailure:
			ShowMessage(S, TEXT("Kunne ikke starte som vert"), TEXT("Spillet fikk ikke åpnet en nettverksport. Lukk andre kopier av spillet og prøv igjen."), Err);
			break;
		default:
			ShowMessage(S, TEXT("Noe gikk galt med forbindelsen"), FString(), Err);
			break;
		}
	}

	void OnTravelFailure(UWorld* W, ETravelFailure::Type Type, const FString& Err)
	{
		FState* SP = StateForFailure(W);
		if (!SP) return;
		FState& S = *SP;
		if (SkipFailure(S)) return;
		const FString Where = S.ConnectAddress.IsEmpty() ? FString(TEXT("adressen")) : S.ConnectAddress;
		switch (Type)
		{
		case ETravelFailure::PendingNetGameCreateFailure:
		case ETravelFailure::ClientTravelFailure:
		case ETravelFailure::TravelFailure:
			ShowMessage(S, TEXT("Kunne ikke koble til"), FString::Printf(TEXT("Fant ikke noe spill på %s."), *Where), Err);
			break;
		case ETravelFailure::InvalidURL:
			ShowMessage(S, TEXT("Ugyldig adresse"), TEXT("Sjekk at adressen er skrevet riktig."), Err);
			break;
		case ETravelFailure::LoadMapFailure:
		case ETravelFailure::PackageMissing:
		case ETravelFailure::PackageVersion:
		case ETravelFailure::NoLevel:
			ShowMessage(S, TEXT("Kartet kunne ikke lastes"), TEXT("Verten bruker et kart du ikke har."), Err);
			break;
		default:
			ShowMessage(S, TEXT("Noe gikk galt med forbindelsen"), FString(), Err);
			break;
		}
	}

	// ------------------------------------------------------------------ every frame
	void OnPostRender(AHUD* HUD, UCanvas* Canvas)
	{
		if (!HUD || !Canvas || Canvas->SizeX <= 0 || Canvas->SizeY <= 0 || !GEngine) return;
		AGSPlayerController* PC = Cast<AGSPlayerController>(HUD->GetOwningPlayerController());
		UWorld* World = HUD->GetWorld();
		if (!PC || !World || !PC->IsLocalController()) return;
		UGameViewportClient* VC = World->GetGameViewport();
		ULocalPlayer* LP = PC->GetLocalPlayer();
		if (!VC || !LP || GEngine->GetFirstGamePlayer(VC) != LP) return;   // one menu per window: the first local player
		FState* SP = StateForVC(VC);
		if (!SP) return;
		FState& S = *SP;

		// Hooks per window: both are called before the player controller sees the input.
		InstallHooks(VC, S);

		if (S.World.Get() != World) S.ResetWorld(World);
		S.CanvasSize = FVector2D(Canvas->SizeX, Canvas->SizeY);
		S.LastDrawFrame = GFrameCounter;
		const ENetMode NetMode = World->GetNetMode();
		const double Now = FPlatformTime::Seconds();

		// 1. Once per world
		if (!S.bWorldSetupDone)
		{
			S.bWorldSetupDone = true;
			ApplyVolume(World);
			bool bSavedRT = false;
			if (GConfig && GConfig->GetBool(Sec, TEXT("RayTracing"), bSavedRT, GGameUserSettingsIni) && bSavedRT != PC->IsRayTracingOn())
			{
				PC->Straaling(bSavedRT ? 1 : 0);
			}
			S.NameEdit = EnsureName();
			S.AddressEdit = PC->LastJoinAddress;
			// The input mode belongs to the window and survives a map change: start every world in game mode.
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
		}

		// 2. Saved keys, once per mapping context
		if (UInputMappingContext* C = PC->MappingContext.Get())
		{
			static TSet<TWeakObjectPtr<UInputMappingContext>> Applied;
			if (!Applied.Contains(C))
			{
				if (Applied.Num() > 32)
				{
					for (auto It = Applied.CreateIterator(); It; ++It)
					{
						if (!It->IsValid()) It.RemoveCurrent();
					}
				}
				Applied.Add(C);
				ApplySavedKeys(PC);
			}
		}

		// 3. Our name to the server
		if (!S.bNameSent && PC->PlayerState)
		{
			S.bNameSent = true;
			const FString Saved = EnsureName();
			if (PC->PlayerState->GetPlayerName() != Saved) PC->ServerChangeName(Saved);
		}

		// 4. Connecting
		if (S.bConnecting)
		{
			if (NetMode == NM_Client && S.ConnectWorld.Get() != World)
			{
				S.bConnecting = false;   // we are in
				S.bConnectFromMatch = false;
			}
			else if (Now - S.ConnectStart > ConnectTimeoutSeconds)
			{
				const FString Addr = S.ConnectAddress;
				CancelConnect(PC, World, S);
				ShowMessage(S, TEXT("Kunne ikke koble til"), FString::Printf(TEXT("Fant ikke noe spill på %s."), *Addr),
					TEXT("Sjekk adressen, og at verten har trykket Start spill og har åpnet for deg i brannmuren."));
			}
		}

		// 5. What opens by itself
		const AGSGameState* GS = World->GetGameState<AGSGameState>();
		const AGSPlayerState* PS = PC->GetPlayerState<AGSPlayerState>();
		// "Til hovedmenyen" from a bot match or the test: the start menu opens in the new world, also in the editor's Play.
		const uint8 Kind = LocalKind(World);
		if (S.bOpenMainAfterTravel && GS && NetMode == NM_Standalone && Kind == GSGameKind::Online)
		{
			S.bOpenMainAfterTravel = false;
			S.bMainDismissed = false;
			if (!S.bMainOpen) OpenMainImpl(PC, S, TEXT("tilbake til hovedmenyen"));
		}
		// The start menu opens by itself only in the real game, and only in the world without a game of its own (not a
		// bot match or the test). In the editor's Play you start playing at once, and F10/P opens the menu (you test the
		// game more often than the menu, and Play must never start locked).
		if (NetMode == NM_Standalone && GS && Kind == GSGameKind::Online && !S.bMainDismissed && !S.bMainOpen)
		{
			if (!World->IsPlayInEditor())
			{
				OpenMainImpl(PC, S, TEXT("start av spillet"));
			}
			else if (!S.bPIENoteLogged)
			{
				S.bPIENoteLogged = true;
				UE_LOG(LogTemp, Log, TEXT("GSMenu: Play i editoren: hovedmenyen åpnes ikke av seg selv. F10 eller P åpner den (Esc stopper Play)."));
			}
		}
		if (S.bMsgPending && S.Dialog == EDialog::None)
		{
			S.Dialog = EDialog::Message;
		}
		if (GS)
		{
			if (GS->Phase != S.LastPhase)
			{
				if (GS->Phase == EGSMatchPhase::Ended) S.bTeamShownThisLobby = false;
				if (GS->Phase == EGSMatchPhase::Playing) S.bTeamOpen = false;
				S.LastPhase = GS->Phase;
			}
			if (InMatch(World, S) && GS->Phase == EGSMatchPhase::Lobby && PS && PS->Team != GSTeam::None && !S.bTeamShownThisLobby
				&& !S.bMainOpen && !PC->bMenuOpen && !PC->IsOnTerminal())
			{
				S.bTeamOpen = true;
				S.bTeamShownThisLobby = true;
			}
		}
		if (S.bMainOpen) S.bTeamOpen = false;
		// Safety: without our key hook neither Esc nor a click can reach us, so nothing of ours may stay open.
		if (!S.bHookLive && (S.bMainOpen || S.bTeamOpen || S.Dialog != EDialog::None))
		{
			UE_LOG(LogTemp, Warning, TEXT("GSMenu: fikk ikke tastekroken på vinduet, menyen lukkes så spillet kan styres"));
			S.Dialog = EDialog::None;
			S.bMsgPending = false;
			S.bTeamOpen = false;
			CloseMain(PC, S, TEXT("ingen tastekrok"));
		}
		SyncModal(PC, S);

		// 6. Draw
		S.Hits.Reset();
		FVector2D Mouse = MouseOnCanvas(S, VC->Viewport);
		const bool bDialog = S.Dialog != EDialog::None;

		// A slider being dragged
		if (S.DragSlider != NAME_None && !bDialog)
		{
			ApplySlider(PC, World, S.DragSlider, (Mouse.X - S.DragX) / S.DragW);
		}

		if (S.bMainOpen)
		{
			if (!InMatch(World, S) && Kind == GSGameKind::Online && !S.bMainDismissed)
			{
				// Campus turns slowly behind the start menu (also while connecting from it). Not when Esc opened the
				// menu while practising: then the player's own view must stay where it was.
				const float Dt = FMath::Min(World->GetDeltaSeconds(), 0.1f);
				FRotator R = PC->GetControlRotation();
				R.Yaw += 2.f * Dt;
				R.Pitch = FMath::FInterpTo(FRotator::NormalizeAxis(R.Pitch), -4.f, Dt, 0.5f);
				R.Roll = 0.f;
				PC->SetControlRotation(R);
			}
			const FUi U{ S, Canvas, bDialog ? FVector2D(-10000.f, -10000.f) : Mouse };
			DrawMain(U, PC, World);
		}
		else if (S.bTeamOpen)
		{
			const FUi U{ S, Canvas, bDialog ? FVector2D(-10000.f, -10000.f) : Mouse };
			DrawTeam(U, PC, World);
		}
		if (bDialog)
		{
			S.Hits.Reset();   // a dialog swallows every click outside its buttons
			const FUi U{ S, Canvas, Mouse };
			DrawDialog(U, World);
		}
	}
}

// ---------------------------------------------------------------------- public

bool UGSGameViewportClient::InputChar(FViewport* InViewport, int32 ControllerId, TCHAR Character)
{
	// The console (§ in the editor) types first: its characters must not end up in our text fields.
	const bool bConsoleOpen = ViewportConsole && ViewportConsole->ConsoleActive();
	if (!bConsoleOpen && GSMenu::HandleChar(this, Character)) return true;
	return Super::InputChar(InViewport, ControllerId, Character);
}

namespace GSMenu
{
	using GSMenuImpl::FState;
	using GSMenuImpl::FKeyRow;
	using GSMenuImpl::EDialog;

	void EnsureHooked()
	{
		static bool bHooked = false;
		if (bHooked || !GEngine) return;
		bHooked = true;
		AHUD::OnHUDPostRender.AddStatic(&GSMenuImpl::OnPostRender);
		GEngine->OnNetworkFailure().AddStatic(&GSMenuImpl::OnNetworkFailure);
		GEngine->OnTravelFailure().AddStatic(&GSMenuImpl::OnTravelFailure);
	}

	void OpenMain(AGSPlayerController* PC)
	{
		UWorld* World = PC ? PC->GetWorld() : nullptr;
		FState* S = GSMenuImpl::StateFor(World);
		if (!S || S->World.Get() != World) return;
		GSMenuImpl::OpenMainImpl(PC, *S, TEXT("GSMenu::OpenMain"));
	}

	void AskQuit(AGSPlayerController* PC)
	{
		UWorld* World = PC ? PC->GetWorld() : nullptr;
		FState* S = GSMenuImpl::StateFor(World);
		if (!S || S->World.Get() != World) return;
		S->Dialog = EDialog::Quit;
		GSMenuImpl::SyncModal(PC, *S);
	}

	bool IsFullscreen(const APlayerController* PC)
	{
		const UWorld* World = PC ? PC->GetWorld() : nullptr;
		const FState* S = World ? GSMenuImpl::FindState(World->GetGameViewport()) : nullptr;
		return S && S->World.Get() == World && S->bMainOpen;
	}

	bool IsModal(const APlayerController* PC)
	{
		const UWorld* World = PC ? PC->GetWorld() : nullptr;
		const FState* S = World ? GSMenuImpl::FindState(World->GetGameViewport()) : nullptr;
		return S && S->World.Get() == World && GSMenuImpl::IsModalState(*S);
	}

	FKey KeyFor(const APlayerController* PC, FName Action)
	{
		if (Action == FName(TEXT("GS_Team"))) return GSMenuImpl::TeamKey();
		if (Action == FName(TEXT("GS_Fly"))) return GSMenuImpl::FlyKey();
		return GSMenuImpl::ActionKey(Cast<AGSPlayerController>(PC), Action);
	}

	FString KeyName(const APlayerController* PC, FName Action)
	{
		if (Action == FName(TEXT("GS_Team"))) return GSMenuImpl::KeyDisplay(GSMenuImpl::TeamKey());
		if (Action == FName(TEXT("GS_Fly"))) return GSMenuImpl::KeyDisplay(GSMenuImpl::FlyKey());
		const AGSPlayerController* GPC = Cast<AGSPlayerController>(PC);
		const UInputMappingContext* C = GPC ? GPC->MappingContext.Get() : nullptr;
		if (!C) return FString();
		for (const FEnhancedActionKeyMapping& M : C->GetMappings())
		{
			if (M.Action && M.Action->GetFName() == Action) return GSMenuImpl::KeyDisplay(M.Key);
		}
		return FString();
	}

	TArray<FGSControlRow> Controls(const APlayerController* PC)
	{
		TArray<FGSControlRow> Out;
		TArray<FKeyRow> Rows;
		GSMenuImpl::BuildRows(Cast<AGSPlayerController>(PC), Rows);
		for (const FKeyRow& Row : Rows)
		{
			FGSControlRow R;
			R.Label = GSMenuImpl::RowLabel(Row);
			R.Key = GSMenuImpl::KeyDisplay(Row.Current);
			R.bRebindable = true;
			Out.Add(R);
		}
		Out.Add(FGSControlRow{ TEXT("Se rundt"), TEXT("Mus"), false });
		Out.Add(FGSControlRow{ TEXT("Bla i våpen (i sekken med inventaret åpent)"), TEXT("Musehjul"), false });
		Out.Add(FGSControlRow{ TEXT("Meny"), GSMenuImpl::MenuKeysText(PC ? PC->GetWorld() : nullptr), false });
		Out.Add(FGSControlRow{ TEXT("Hjelp: taster og regler"), TEXT("H"), false });
		return Out;
	}

	bool HandleChar(UGameViewportClient* VC, TCHAR Ch)
	{
		FState* S = GSMenuImpl::FindState(VC);
		if (!S || !GSMenuImpl::IsModalState(*S)) return false;
		if (Ch < 32 || Ch == 127) return true;                 // control characters are handled as keys
		if (S->ListenRow != INDEX_NONE) return true;
		GSMenuImpl::TypeChar(*S, Ch);
		return true;
	}
}
