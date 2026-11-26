#include "GSMenu.h"
#include "GSGameMode.h"
#include "GSGameState.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"
#include "GSTypes.h"
#include "AudioDevice.h"
#include "AudioDeviceHandle.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
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
	constexpr double ConnectTimeoutSeconds = 20.0;

	enum class EPage : uint8 { Root, Host, Join, Connecting, Settings, Controls };
	enum class EDialog : uint8 { None, Host, Join, Leave, Quit, Message };

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
		TArray<FHit> Hits;                 // rebuilt every frame, read by the next click
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
			Hits.Reset();
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
		// Modal needs us to be drawing: if the canvas goes away, no input is swallowed.
		return (S.bMainOpen || S.bTeamOpen || S.Dialog != EDialog::None) && GFrameCounter - S.LastDrawFrame <= 30;
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

	FString CleanName(const FString& In)
	{
		FString Out;
		for (int32 I = 0; I < In.Len(); ++I)
		{
			const TCHAR Ch = In[I];
			if (Ch < 32 || Ch == 127 || Ch == TEXT('"') || Ch == TEXT('?') || Ch == TEXT('=') || Ch == TEXT('#') || Ch == TEXT('\\')) continue;
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
			if (K.IsValid()) return K;
		}
		return EKeys::L;
	}

	/** Every key that can be changed, in the combat team's mapping order, the team panel last. Reserved = keys that are taken for good. */
	void BuildRows(const AGSPlayerController* PC, TArray<FKeyRow>& Out, TArray<FKey>* Reserved = nullptr)
	{
		Out.Reset();
		if (Reserved)
		{
			Reserved->Reset();
			Reserved->Add(EKeys::Escape);
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
			{ TEXT("IA_Ready"), TEXT("Klar (lobby)") }, { TEXT("GS_Team"), TEXT("Velg lag (lobby)") } };
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
		BuildRows(PC, Rows);
		for (const FKeyRow& Row : Rows)
		{
			if (Row.Map == INDEX_NONE || Row.Map >= C->GetMappings().Num()) continue;
			FKey K = Row.Original;
			const FString Str = CfgString(KeySec, *Row.Id);
			if (!Str.IsEmpty())
			{
				const FKey Saved(*Str);
				if (Saved.IsValid()) K = Saved;
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
			S.KeyNote = TEXT("Esc, F10, musa og hjulet er satt av og kan ikke brukes.");
			return;
		}
		const FKeyRow& Row = Rows[RowIndex];
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
	bool InMatch(const UWorld* W)
	{
		const ENetMode Mode = W ? W->GetNetMode() : NM_Standalone;
		return Mode == NM_ListenServer || Mode == NM_Client;
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
			if (!S.bModalApplied) PC->FlushPressedKeys();
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
		S.KeyNote.Empty();
		S.JoinNote.Empty();
		if (Page == EPage::Join)
		{
			if (S.AddressEdit.IsEmpty() && PC) S.AddressEdit = PC->LastJoinAddress;
			S.Focus = FName(TEXT("Address"));
		}
	}

	void OpenMainImpl(AGSPlayerController* PC, FState& S)
	{
		S.bTeamOpen = false;
		S.bMainOpen = true;
		S.Page = S.bConnecting ? EPage::Connecting : EPage::Root;
		S.Focus = NAME_None;
		S.ListenRow = INDEX_NONE;
		SyncModal(PC, S);
	}

	void CloseMain(AGSPlayerController* PC, FState& S)
	{
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
		if (S.Page == EPage::Connecting) S.Page = EPage::Join;
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
		S.bConnecting = true;
		S.bConnectFromMatch = InMatch(World);
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
		S.JoinNote.Empty();
		if (InMatch(World))
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
	static const int32 FpsSteps[] = { 30, 60, 90, 120, 144, 0 };

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
		if (!Gus) return;
		if (Id == TEXT("Set.Graphics"))
		{
			// The overall level also resets the resolution scale: keep the player's own.
			const int32 Res = PC->GetResolutionPercent();
			Gus->SetOverallScalabilityLevel(FMath::Clamp(PC->GetGraphicsLevel() + Dir, 0, 4));
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
		else if (Id == TEXT("Set.Fps"))
		{
			const int32 Cur = FMath::RoundToInt(Gus->GetFrameRateLimit());
			int32 I = Cur <= 0 ? int32(UE_ARRAY_COUNT(FpsSteps)) - 1 : NearestIndex(FpsSteps, int32(UE_ARRAY_COUNT(FpsSteps)) - 1, Cur);
			I = FMath::Clamp(I + Dir, 0, int32(UE_ARRAY_COUNT(FpsSteps)) - 1);
			Gus->SetFrameRateLimit(float(FpsSteps[I]));
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
		const float RowH = FMath::Clamp((H - 110.f) / 10.f, 30.f, 40.f);
		const float LX = X + 24.f;
		const float CX = X + W * 0.45f;
		const float CW = FMath::Min(W * 0.55f - 24.f, 340.f);
		const float S = 0.85f;
		auto Label = [&](const TCHAR* Str) { Text(U.C, Str, LX, Y + (28.f - LineHeight(S)) * 0.5f, TextColor, S); };

		static const TCHAR* Levels[] = { TEXT("Lav"), TEXT("Middels"), TEXT("Høy"), TEXT("Episk"), TEXT("Film") };
		Label(TEXT("Grafikk"));
		Choice(U, TEXT("Set.Graphics"), Levels[FMath::Clamp(PC->GetGraphicsLevel(), 0, 4)], CX, Y, CW);
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
		{
			const int32 Fps = Gus ? FMath::RoundToInt(Gus->GetFrameRateLimit()) : 0;
			Choice(U, TEXT("Set.Fps"), Fps > 0 ? FString::Printf(TEXT("%d"), Fps) : FString(TEXT("Ubegrenset")), CX, Y, CW);
		}
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

		Label(TEXT("Navn"));
		Field(U, TEXT("Name"), U.S.NameEdit, TEXT("Navnet ditt"), CX, Y - 2.f, CW, 32.f);
		Y += RowH;

		Label(TEXT("Taster"));
		Button(U, TEXT("Nav.Controls"), TEXT("Endre taster ..."), CX, Y, CW, 30.f);
		Y += RowH + 6.f;

		Text(U.C, TEXT("Alt lagres med en gang."), LX, Y, DimColor, 0.75f);
	}

	void DrawControls(const FUi& U, AGSPlayerController* PC, float X, float Y, float W, float H)
	{
		TArray<FKeyRow> Rows;
		BuildRows(PC, Rows);
		const int32 PerCol = FMath::Max(1, (Rows.Num() + 1) / 2);
		const float RowH = FMath::Clamp((H - 250.f) / PerCol, 18.f, 28.f);
		const float S = FMath::Clamp(0.85f * RowH / 26.f, 0.62f, 0.85f);
		const float ColW = (W - 48.f - 20.f) * 0.5f;
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
		const float LX = X + 24.f, MaxW = W - 48.f;
		Y = Para(U, TEXT("Klikk en linje og trykk den nye tasten (Esc avbryter). Faste: mus = se rundt · musehjul = bla i våpnene, eller i sekken når inventaret er åpent · Esc / F10 = meny."),
			LX, Y, MaxW, DimColor, 0.72f);
		Text(U.C, U.S.KeyNote, LX, Y + 2.f, AccentColor, 0.8f);
		Y += LineHeight(0.8f) + 8.f;
		Button(U, TEXT("Key.Reset"), TEXT("Tilbakestill alle taster"), LX, Y, FMath::Min(300.f, MaxW), 32.f);
		Y += 44.f;
		Text(U.C, TEXT("Capture the Flag"), LX, Y, FLinearColor::White, 0.9f);
		Y += LineHeight(0.9f) + 4.f;
		Para(U, FString::Printf(TEXT("Ta motstanderens flagg og bær det til din egen base. Du får bare poeng når ditt eget flagg står hjemme. Første lag til %d erobringer vinner."),
			AGSGameState::CaptureLimit), LX, Y, MaxW, TextColor, 0.78f);
	}

	void DrawMain(const FUi& U, AGSPlayerController* PC, UWorld* World)
	{
		FState& S = U.S;
		const float SX = U.C->SizeX, SY = U.C->SizeY;
		const bool bMatch = InMatch(World);
		Box(U.C, 0.f, 0.f, SX, SY, FLinearColor(0.f, 0.f, 0.f, 0.30f));

		// --- left panel ---
		const float PW = FMath::Min(FMath::Max(460.f, SX * 0.30f), SX * 0.48f);
		Box(U.C, 0.f, 0.f, PW, SY, PanelColor);
		const float LX = 48.f, BW = PW - 96.f;
		float Y = SY * 0.12f;
		Text(U.C, TEXT("GLØSHAUGEN STRIKE"), LX, Y, FLinearColor::White, FMath::Min(1.9f, BW / FMath::Max(1.f, TextWidth(TEXT("GLØSHAUGEN STRIKE"), 1.f))));
		Y += LineHeight(1.9f) + 4.f;
		Text(U.C, TEXT("Capture the Flag · NTNU Gløshaugen"), LX, Y, DimColor, 0.8f);
		Y += LineHeight(0.8f) + 18.f;
		Text(U.C, TEXT("Navn"), LX, Y, DimColor, 0.8f);
		Y += LineHeight(0.8f) + 4.f;
		Field(U, TEXT("Name"), S.NameEdit, TEXT("Navnet ditt"), LX, Y, BW, 36.f);
		Y += 36.f + 22.f;
		Y = FMath::Max(Y, SY * 0.32f);

		struct FNav { const TCHAR* Id; const TCHAR* Label; bool bSelected; };
		TArray<FNav> Nav;
		if (bMatch)
		{
			Nav.Add(FNav{ TEXT("Nav.Resume"), TEXT("Tilbake til kampen"), false });
			Nav.Add(FNav{ TEXT("Nav.Host"), TEXT("Ny kamp som vert"), S.Page == EPage::Host });
			Nav.Add(FNav{ TEXT("Nav.Join"), TEXT("Bli med i en annen"), S.Page == EPage::Join || S.Page == EPage::Connecting });
			Nav.Add(FNav{ TEXT("Nav.Settings"), TEXT("Innstillinger"), S.Page == EPage::Settings });
			Nav.Add(FNav{ TEXT("Nav.Controls"), TEXT("Kontroller"), S.Page == EPage::Controls });
			Nav.Add(FNav{ TEXT("Nav.Leave"), TEXT("Forlat kampen"), false });
			Nav.Add(FNav{ TEXT("Nav.Quit"), TEXT("Avslutt"), false });
		}
		else
		{
			Nav.Add(FNav{ TEXT("Nav.Host"), TEXT("Start spill"), S.Page == EPage::Host });
			Nav.Add(FNav{ TEXT("Nav.Join"), TEXT("Bli med"), S.Page == EPage::Join || S.Page == EPage::Connecting });
			Nav.Add(FNav{ TEXT("Nav.Solo"), TEXT("Øv alene"), false });
			Nav.Add(FNav{ TEXT("Nav.Settings"), TEXT("Innstillinger"), S.Page == EPage::Settings });
			Nav.Add(FNav{ TEXT("Nav.Controls"), TEXT("Kontroller"), S.Page == EPage::Controls });
			Nav.Add(FNav{ TEXT("Nav.Quit"), TEXT("Avslutt"), false });
		}
		const float BH = FMath::Clamp((SY - Y - 50.f) / Nav.Num() - 6.f, 30.f, 44.f);
		for (const FNav& N : Nav)
		{
			// While connecting, only Cancel (on the right) and Quit make sense.
			const bool bEnabled = !S.bConnecting || FCString::Strcmp(N.Id, TEXT("Nav.Quit")) == 0;
			Button(U, N.Id, N.Label, LX, Y, BW, BH, bEnabled, N.bSelected);
			Y += BH + 6.f;
		}
		Text(U.C, TEXT("Esc: tilbake"), LX, SY - 36.f, DimColor, 0.75f);

		if (S.Page == EPage::Root) return;

		// --- right panel ---
		const float X = PW + 40.f, PY = SY * 0.12f;
		const float W = FMath::Min(SX - X - 48.f, 900.f), H = SY * 0.76f;
		if (W < 200.f) return;
		Box(U.C, X, PY, W, H, PanelColor);
		const float IX = X + 24.f, MaxW = W - 48.f, TS = 0.85f;
		Y = PY + 18.f;
		const TCHAR* Title = S.Page == EPage::Host ? (bMatch ? TEXT("Ny kamp som vert") : TEXT("Start spill"))
			: S.Page == EPage::Join ? TEXT("Bli med")
			: S.Page == EPage::Connecting ? TEXT("Kobler til")
			: S.Page == EPage::Settings ? TEXT("Innstillinger") : TEXT("Kontroller");
		Text(U.C, Title, IX, Y, FLinearColor::White, 1.2f);
		Y += LineHeight(1.2f) + 14.f;

		switch (S.Page)
		{
		case EPage::Host:
		{
			Y = Para(U, TEXT("Du blir vert for en ny kamp. Vennene dine kobler seg til deg."), IX, Y, MaxW, TextColor, TS) + 10.f;
			const TArray<FString>& Addrs = LocalAddresses();
			if (Addrs.Num() > 0)
			{
				Text(U.C, TEXT("Venner på samme nett skriver inn:"), IX, Y, TextColor, TS);
				Y += LineHeight(TS) + 6.f;
				for (const FString& A : Addrs)
				{
					Text(U.C, A, IX + 12.f, Y, FLinearColor::White, 1.1f);
					Y += LineHeight(1.1f) + 4.f;
				}
			}
			else
			{
				Text(U.C, TEXT("Fant ingen nettverksadresse."), IX, Y, DimColor, TS);
				Y += LineHeight(TS) + 4.f;
			}
			Y += 10.f;
			Y = Para(U, TEXT("Over internett må port 7777 (UDP) åpnes i ruteren din."), IX, Y, MaxW, DimColor, TS);
			Y = Para(U, TEXT("Første gang spør Windows om brannmuren. Trykk Tillat."), IX, Y, MaxW, DimColor, TS) + 8.f;
			if (bMatch) Y = Para(U, TEXT("Kampen du er i nå avsluttes."), IX, Y, MaxW, AccentColor, TS) + 8.f;
			Button(U, TEXT("Host.Go"), TEXT("Start som vert"), IX, Y + 6.f, FMath::Min(300.f, MaxW), 44.f, true, false, 0, true);
			break;
		}
		case EPage::Join:
		{
			Text(U.C, TEXT("Adresse"), IX, Y, DimColor, 0.8f);
			Y += LineHeight(0.8f) + 4.f;
			const float FW = FMath::Min(420.f, MaxW);
			Field(U, TEXT("Address"), S.AddressEdit, TEXT("192.168.1.20"), IX, Y, FW, 36.f);
			Y += 36.f + 10.f;
			Button(U, TEXT("Join.Go"), TEXT("Koble til"), IX, Y, FMath::Min(200.f, FW), 44.f, true, false, 0, true);
			if (PC && !PC->LastJoinAddress.IsEmpty() && PC->LastJoinAddress != S.AddressEdit)
			{
				Button(U, TEXT("Join.Last"), FString::Printf(TEXT("Siste: %s"), *PC->LastJoinAddress), IX + FMath::Min(200.f, FW) + 10.f, Y, FMath::Min(300.f, MaxW - 210.f), 44.f);
			}
			Y += 44.f + 12.f;
			Text(U.C, S.JoinNote, IX, Y, AccentColor, TS);
			Y += LineHeight(TS) + 8.f;
			Para(U, TEXT("Skriv adressen verten ser under Start spill, for eksempel 192.168.1.20. Ctrl+V limer inn."), IX, Y, MaxW, DimColor, TS);
			break;
		}
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
			const FString Block = TeamJoinBlock(GS, PS, T);
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
			Text(U.C, FString::Printf(TEXT("Venner blir med på: %s"), *LocalAddresses()[0]), X + 24.f, Y, TextColor, 0.85f);
			Y += 26.f;
		}
		Text(U.C, FString::Printf(TEXT("%s klar · Esc lukk · %s åpner igjen"), *GSMenu::KeyName(PC, TEXT("IA_Ready")), *GSMenu::KeyName(PC, TEXT("GS_Team"))),
			X + 24.f, Y, DimColor, 0.75f);
	}

	/** "Du er vert. Kampen avsluttes for alle 3 andre." / "Du forlater kampen du er i." / empty when alone. */
	FString LeaveText(UWorld* World)
	{
		if (!InMatch(World)) return FString();
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
		case EDialog::Host: Title = TEXT("Starte en ny kamp?"); Body = LeaveText(World); Yes = TEXT("Ja, start ny"); break;
		case EDialog::Join: Title = TEXT("Bli med i en annen kamp?"); Body = LeaveText(World); Yes = TEXT("Ja, koble til"); break;
		case EDialog::Leave: Title = TEXT("Forlate kampen?"); Body = LeaveText(World); Yes = TEXT("Ja, forlat"); break;
		case EDialog::Quit: Title = TEXT("Avslutte spillet?"); Body = LeaveText(World); Yes = TEXT("Avslutt"); break;
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
			S.LeavingUntil = FPlatformTime::Seconds() + 10.0;
			PC->Host();
			break;
		case EDialog::Join:
			if (!S.bMainOpen) OpenMainImpl(PC, S);
			StartJoin(PC, World, S, S.PendingJoin);
			break;
		case EDialog::Leave:
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

		if (Id == TEXT("Nav.Host")) SetPage(PC, S, EPage::Host);
		else if (Id == TEXT("Nav.Join")) SetPage(PC, S, EPage::Join);
		else if (Id == TEXT("Nav.Settings")) SetPage(PC, S, EPage::Settings);
		else if (Id == TEXT("Nav.Controls")) SetPage(PC, S, EPage::Controls);
		else if (Id == TEXT("Nav.Solo") || Id == TEXT("Nav.Resume")) CloseMain(PC, S);
		else if (Id == TEXT("Nav.Leave")) S.Dialog = EDialog::Leave;
		else if (Id == TEXT("Nav.Quit")) S.Dialog = EDialog::Quit;
		else if (Id == TEXT("Host.Go"))
		{
			if (InMatch(World)) S.Dialog = EDialog::Host;
			else { S.LeavingUntil = FPlatformTime::Seconds() + 10.0; PC->Host(); }
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
		else if (Id == TEXT("Team.Join"))
		{
			if (AGSPlayerState* PS = PC->GetPlayerState<AGSPlayerState>()) PS->ServerRequestTeam(uint8(Hit.Arg));   // the server decides
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

	void Back(AGSPlayerController* PC, UWorld* World, FState& S)
	{
		if (S.Dialog != EDialog::None) { DialogNo(S); return; }
		if (S.Focus != NAME_None) { SetFocus(PC, S, NAME_None); return; }
		if (S.bMainOpen)
		{
			if (S.Page == EPage::Connecting) CancelConnect(PC, World, S);
			else if (S.Page != EPage::Root) SetPage(PC, S, EPage::Root);
			else if (InMatch(World)) CloseMain(PC, S);
			return;   // alone at the root: nothing
		}
		if (S.bTeamOpen) CloseTeam(S);
	}

	bool AddressChar(TCHAR Ch)
	{
		return (Ch >= TEXT('0') && Ch <= TEXT('9')) || (Ch >= TEXT('a') && Ch <= TEXT('z')) || (Ch >= TEXT('A') && Ch <= TEXT('Z'))
			|| Ch == TEXT('.') || Ch == TEXT(':') || Ch == TEXT('-');
	}

	bool NameChar(TCHAR Ch)
	{
		return Ch >= 32 && Ch != 127 && Ch != TEXT('"') && Ch != TEXT('?') && Ch != TEXT('=') && Ch != TEXT('#') && Ch != TEXT('\\');
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
						OpenMainImpl(PC, S);
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

		if (S.ListenRow != INDEX_NONE)
		{
			if (!bPress) return true;
			if (Key == EKeys::Escape) S.ListenRow = INDEX_NONE;
			else AssignKey(PC, S, S.ListenRow, Key);
			return true;
		}

		if (Key == EKeys::LeftMouseButton)
		{
			const FVector2D Pos = MouseOnCanvas(S, Args.Viewport);
			bool bFound = false;
			for (int32 I = S.Hits.Num() - 1; I >= 0; --I)
			{
				const FHit& H = S.Hits[I];
				if (Pos.X >= H.X0 && Pos.X <= H.X1 && Pos.Y >= H.Y0 && Pos.Y <= H.Y1)
				{
					const FHit Copy = H;   // the click may change the list
					HandleClick(PC, World, S, Copy);
					bFound = true;
					break;
				}
			}
			if (!bFound) SetFocus(PC, S, NAME_None);
		}
		else if (Key == EKeys::Escape)
		{
			Back(PC, World, S);
		}
		else if (Key == EKeys::Enter)
		{
			if (S.Dialog != EDialog::None) DialogYes(PC, World, S);
			else if (S.Focus == FName(TEXT("Address"))) TryJoin(PC, World, S);
			else if (S.Focus == FName(TEXT("Name"))) SetFocus(PC, S, NAME_None);
			else if (S.bTeamOpen) PC->ToggleReady();
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
		if (Type != ENetworkFailure::NetDriverListenFailure && !bClient && !S.bConnecting) return;
		if (SkipFailure(S)) return;
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
			ShowMessage(S, TEXT("Kunne ikke starte som vert"), TEXT("Port 7777 er i bruk. Lukk andre kopier av spillet og prøv igjen."), Err);
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
		if (!VC->OnOverrideInputKey().IsBound())
		{
			VC->OnOverrideInputKey().BindStatic(&OnKey, TWeakObjectPtr<UGameViewportClient>(VC));
		}
		if (!VC->OnOverrideInputAxis().IsBound())
		{
			VC->OnOverrideInputAxis().BindStatic(&OnAxis, TWeakObjectPtr<UGameViewportClient>(VC));
		}

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
		if (NetMode == NM_Standalone && !S.bMainDismissed && !S.bMainOpen)
		{
			OpenMainImpl(PC, S);
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
			if (InMatch(World) && GS->Phase == EGSMatchPhase::Lobby && PS && PS->Team != GSTeam::None && !S.bTeamShownThisLobby
				&& !S.bMainOpen && !PC->bMenuOpen && !PC->IsOnTerminal())
			{
				S.bTeamOpen = true;
				S.bTeamShownThisLobby = true;
			}
		}
		if (S.bMainOpen) S.bTeamOpen = false;
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
			if (NetMode == NM_Standalone)
			{
				// Campus turns slowly behind the menu.
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
	if (GSMenu::HandleChar(this, Character)) return true;
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
		GSMenuImpl::OpenMainImpl(PC, *S);
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

	FString KeyName(const APlayerController* PC, FName Action)
	{
		if (Action == FName(TEXT("GS_Team"))) return GSMenuImpl::KeyDisplay(GSMenuImpl::TeamKey());
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
		Out.Add(FGSControlRow{ TEXT("Bla i våpnene, eller i sekken når inventaret er åpent"), TEXT("Musehjul"), false });
		Out.Add(FGSControlRow{ TEXT("Meny"), TEXT("Esc / F10"), false });
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
