#include "GSHelp.h"
#include "GSFlag.h"
#include "GSGameState.h"
#include "GSPlayerController.h"
#include "GSPlayerState.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "InputCoreTypes.h"

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

	uint64 LastTeamKeyFrame = 0;

	void DrawImpl(AHUD* HUD, UCanvas* Canvas)
	{
		if (!HUD || !Canvas || Canvas->SizeX <= 0) return;
		APlayerController* PC = HUD->GetOwningPlayerController();
		UWorld* World = HUD->GetWorld();
		if (!PC || !PC->IsLocalController() || !World) return;
		const AGSGameState* GS = World->GetGameState<AGSGameState>();
		AGSPlayerState* PS = PC->GetPlayerState<AGSPlayerState>();
		if (!GS) return;
		if (const AGSPlayerController* GPC = Cast<AGSPlayerController>(PC))
		{
			if (GPC->bMenuOpen || GPC->IsOnTerminal()) return;
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
					}
				}
				else if (GS->LastFlagEvent.Event != 0 && World->GetTimeSeconds() - GS->LastFlagEvent.LocalTime < 6.f)
				{
					const AGSGameState::FLastFlagEvent& E = GS->LastFlagEvent;
					Col = TeamText(E.FlagTeam);
					if (E.Who.IsEmpty())
					{
						Line = E.Event == GSFlagEvent::Returned
							? FString::Printf(TEXT("%s er tilbake på basen"), *FlagNameCap(E.FlagTeam))
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
				if (!Line.IsEmpty()) Text(Canvas, Line, CX, OwnY, Col, 0.8f, 1);
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
		if (bLobby && PS)
		{
			const float X = 28.f, W = 400.f, S = 0.8f, LH = LineHeight(S) + 4.f;
			float Y = Canvas->SizeY * 0.30f;
			TArray<TPair<FString, FLinearColor>> Lines;
			Lines.Emplace(TEXT("L bytter lag (når det andre laget har færre)."), DimColor);
			Lines.Emplace(TEXT("Ta motstanderens flagg ved å gå bort til det,"), TextColor);
			Lines.Emplace(TEXT("og bær det til din egen base."), TextColor);
			Lines.Emplace(TEXT("Du får bare poeng når ditt eget flagg står hjemme."), TextColor);
			Lines.Emplace(TEXT("Dør du, blir flagget liggende. Eget flagg hentes"), TextColor);
			Lines.Emplace(TEXT("hjem ved å gå bort til det."), TextColor);
			Lines.Emplace(FString::Printf(TEXT("Første lag til %d erobringer vinner, ellers flest"), AGSGameState::CaptureLimit), TextColor);
			Lines.Emplace(FString::Printf(TEXT("når %d minutter er gått."), int32(AGSGameState::MatchMinutes)), TextColor);
			Lines.Emplace(TEXT("Oppvarming nå, ingen skade. Trykk Enter når du er klar."), DimColor);
			if (GS->Phase == EGSMatchPhase::Lobby && GS->PhaseEndsAt > 0.f)
			{
				Lines.Emplace(FString::Printf(TEXT("Starter av seg selv om %d s."), FMath::CeilToInt(GS->PhaseSecondsLeft())), DimColor);
			}
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

		// --- 5. L = switch team, lobby only ---
		if (GS->Phase == EGSMatchPhase::Lobby && PS && GFrameCounter != LastTeamKeyFrame && PC->WasInputKeyJustPressed(EKeys::L))
		{
			LastTeamKeyFrame = GFrameCounter;
			PS->ServerRequestTeam(GSTeam::Other(PS->Team));
		}
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
}
