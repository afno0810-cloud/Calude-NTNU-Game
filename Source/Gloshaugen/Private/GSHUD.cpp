#include "GSHUD.h"
#include "GSCharacter.h"
#include "GSGameState.h"
#include "GSPlayerState.h"
#include "GSPlayerController.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"

void AGSHUD::Text(const FString& S, float X, float Y, const FLinearColor& Color, float Scale, bool bCenter, bool bRight)
{
	UFont* Font = GEngine->GetMediumFont();
	const float W = Font->GetStringSize(*S) * Scale;
	if (bCenter) X -= W * 0.5f;
	if (bRight) X -= W;
	// shadow for readability on bright snow
	Canvas->SetDrawColor(FColor(0, 0, 0, 160));
	Canvas->DrawText(Font, S, X + 1.5f, Y + 1.5f, Scale, Scale);
	Canvas->SetDrawColor(Color.ToFColor(true));
	Canvas->DrawText(Font, S, X, Y, Scale, Scale);
}

void AGSHUD::Box(float X, float Y, float W, float H, const FLinearColor& Color)
{
	FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(W, H), Color);
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);
}

void AGSHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;
	if (StartTime < 0.f) StartTime = GetWorld()->GetTimeSeconds();

	AGSCharacter* C = Cast<AGSCharacter>(GetOwningPawn());
	if (C && !C->bDead)
	{
		// Red flash when hit
		const float SinceHurt = GetWorld()->GetTimeSeconds() - C->GetLastDamageTakenTime();
		if (SinceHurt < 0.5f)
		{
			Box(0, 0, Canvas->SizeX, Canvas->SizeY, FLinearColor(0.6f, 0.f, 0.f, 0.35f * (1.f - SinceHurt / 0.5f)));
		}
		DrawCrosshair(C);
		DrawBars(C);
		DrawAmmo(C);
	}
	else
	{
		DrawDeathScreen();
	}

	DrawTopRight();
	DrawKillFeed();
	DrawHelp();

	if (const AGSPlayerController* PC = Cast<AGSPlayerController>(PlayerOwner))
	{
		if (PC->bShowScoreboard) DrawScoreboard();
	}
}

void AGSHUD::DrawCrosshair(AGSCharacter* C)
{
	const float CX = Canvas->SizeX * 0.5f, CY = Canvas->SizeY * 0.5f;
	const FLinearColor White(1.f, 1.f, 1.f, 0.85f);
	if (C->IsAiming())
	{
		Box(CX - 1.5f, CY - 1.5f, 3.f, 3.f, White);
	}
	else if (!C->bSprinting)
	{
		const float Gap = 6.f + C->GetCurrentSpread() * 9.f;
		const float Len = 9.f;
		Box(CX - Gap - Len, CY - 1.f, Len, 2.f, White);
		Box(CX + Gap, CY - 1.f, Len, 2.f, White);
		Box(CX - 1.f, CY - Gap - Len, 2.f, Len, White);
		Box(CX - 1.f, CY + Gap, 2.f, Len, White);
	}

	const float SinceHit = GetWorld()->GetTimeSeconds() - C->GetLastHitMarkerTime();
	if (SinceHit < 0.18f)
	{
		const FLinearColor Red(1.f, 0.2f, 0.2f, 1.f - SinceHit / 0.18f);
		for (int32 I = 0; I < 4; ++I)
		{
			const float SX = (I % 2 == 0) ? -1.f : 1.f;
			const float SY = (I < 2) ? -1.f : 1.f;
			Box(CX + SX * 10.f - 2.f, CY + SY * 10.f - 2.f, 5.f, 5.f, Red);
		}
	}
}

void AGSHUD::DrawBars(AGSCharacter* C)
{
	const float X = 40.f, Y = Canvas->SizeY - 90.f, W = 260.f;
	Box(X - 6.f, Y - 30.f, W + 12.f, 74.f, FLinearColor(0.f, 0.f, 0.f, 0.35f));
	Text(FString::Printf(TEXT("HELSE  %d"), FMath::RoundToInt(C->Health)), X, Y - 26.f, FLinearColor::White);
	Box(X, Y, W, 14.f, FLinearColor(0.1f, 0.1f, 0.1f, 0.7f));
	const float HP = FMath::Clamp(C->Health / C->MaxHealth, 0.f, 1.f);
	Box(X, Y, W * HP, 14.f, HP > 0.35f ? FLinearColor(0.2f, 0.85f, 0.3f, 0.9f) : FLinearColor(0.95f, 0.2f, 0.15f, 0.9f));
	Box(X, Y + 22.f, W, 8.f, FLinearColor(0.1f, 0.1f, 0.1f, 0.7f));
	Box(X, Y + 22.f, W * FMath::Clamp(C->Stamina / 100.f, 0.f, 1.f), 8.f, FLinearColor(0.95f, 0.8f, 0.2f, 0.9f));
}

void AGSHUD::DrawAmmo(AGSCharacter* C)
{
	const FGSWeaponSlot* W = C->GetCurrentWeapon();
	if (!W) return;
	const FGSWeaponStats& S = GSWeapons::Get(W->Id);
	const float X = Canvas->SizeX - 40.f, Y = Canvas->SizeY - 110.f;
	Box(X - 230.f, Y - 6.f, 236.f, 86.f, FLinearColor(0.f, 0.f, 0.f, 0.35f));
	Text(S.Name, X, Y, FLinearColor::White, 1.f, false, true);
	Text(FString::Printf(TEXT("%d / %d"), W->Ammo, W->Reserve), X, Y + 22.f,
		W->Ammo == 0 ? FLinearColor(1.f, 0.3f, 0.3f) : FLinearColor::White, 1.6f, false, true);
	if (C->bReloading)
	{
		Text(TEXT("LADER OM ..."), X, Y + 56.f, FLinearColor(1.f, 0.85f, 0.3f), 1.f, false, true);
	}
	else if (W->Ammo == 0)
	{
		Text(TEXT("Trykk R for å lade"), X, Y + 56.f, FLinearColor(1.f, 0.4f, 0.4f), 1.f, false, true);
	}

	// Slots
	for (int32 I = 0; I < C->Weapons.Num(); ++I)
	{
		const bool bSel = I == C->CurrentSlot;
		Text(FString::Printf(TEXT("%d  %s"), I + 1, GSWeapons::Get(C->Weapons[I].Id).Name),
			X, Y - 60.f + I * 20.f - (C->Weapons.Num() - 1) * 20.f,
			bSel ? FLinearColor(1.f, 0.9f, 0.4f) : FLinearColor(0.8f, 0.8f, 0.8f, 0.7f), 0.9f, false, true);
	}
}

void AGSHUD::DrawTopRight()
{
	const AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>();
	if (!GS) return;
	const float Hours = GS->GetTimeOfDayHours();
	const int32 H = FMath::FloorToInt(Hours);
	const int32 Min = FMath::FloorToInt((Hours - H) * 60.f);
	const FString Line = FString::Printf(TEXT("%02d:%02d   %s   Spillere: %d"), H, Min, GSNames::Weather(GS->Weather), GS->PlayerArray.Num());
	Text(Line, Canvas->SizeX - 30.f, 24.f, FLinearColor::White, 1.f, false, true);
	Text(TEXT("NTNU Gløshaugen"), Canvas->SizeX - 30.f, 46.f, FLinearColor(0.8f, 0.85f, 1.f, 0.8f), 0.85f, false, true);
}

void AGSHUD::DrawKillFeed()
{
	const AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>();
	if (!GS) return;
	float Y = 24.f;
	for (int32 I = GS->KillFeed.Num() - 1; I >= 0; --I)
	{
		const FGSKillFeedEntry& E = GS->KillFeed[I];
		const FString Line = FString::Printf(TEXT("%s  [%s%s]  %s"), *E.Killer, *E.Weapon, E.bHeadshot ? TEXT(", hode") : TEXT(""), *E.Victim);
		Text(Line, 30.f, Y, E.bHeadshot ? FLinearColor(1.f, 0.6f, 0.3f) : FLinearColor::White, 0.9f);
		Y += 20.f;
	}
}

void AGSHUD::DrawScoreboard()
{
	const AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>();
	if (!GS) return;

	TArray<AGSPlayerState*> Players;
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (AGSPlayerState* G = Cast<AGSPlayerState>(PS)) Players.Add(G);
	}
	Players.Sort([](const AGSPlayerState& A, const AGSPlayerState& B) { return A.Kills > B.Kills; });

	const float W = 560.f, RowH = 26.f;
	const float H = 70.f + Players.Num() * RowH;
	const float X = (Canvas->SizeX - W) * 0.5f, Y = Canvas->SizeY * 0.2f;
	Box(X, Y, W, H, FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Text(TEXT("POENGTAVLE"), X + W * 0.5f, Y + 10.f, FLinearColor::White, 1.2f, true);
	Text(TEXT("Spiller"), X + 20.f, Y + 40.f, FLinearColor(0.7f, 0.7f, 0.7f));
	Text(TEXT("Kills"), X + 360.f, Y + 40.f, FLinearColor(0.7f, 0.7f, 0.7f));
	Text(TEXT("Død"), X + 430.f, Y + 40.f, FLinearColor(0.7f, 0.7f, 0.7f));
	Text(TEXT("Ping"), X + 490.f, Y + 40.f, FLinearColor(0.7f, 0.7f, 0.7f));

	const APlayerState* Me = PlayerOwner ? PlayerOwner->PlayerState : nullptr;
	float RY = Y + 64.f;
	for (const AGSPlayerState* P : Players)
	{
		const FLinearColor Col = P == Me ? FLinearColor(1.f, 0.9f, 0.4f) : FLinearColor::White;
		Text(P->GetPlayerName().Left(28), X + 20.f, RY, Col);
		Text(FString::FromInt(P->Kills), X + 360.f, RY, Col);
		Text(FString::FromInt(P->Deaths), X + 430.f, RY, Col);
		Text(FString::FromInt(FMath::RoundToInt(P->GetPingInMilliseconds())), X + 490.f, RY, Col);
		RY += RowH;
	}
}

void AGSHUD::DrawDeathScreen()
{
	Box(0, 0, Canvas->SizeX, Canvas->SizeY, FLinearColor(0.15f, 0.f, 0.f, 0.35f));
	FString Killer;
	const AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>();
	const APlayerState* Me = PlayerOwner ? PlayerOwner->PlayerState : nullptr;
	if (GS && Me)
	{
		for (int32 I = GS->KillFeed.Num() - 1; I >= 0; --I)
		{
			if (GS->KillFeed[I].Victim == Me->GetPlayerName()) { Killer = GS->KillFeed[I].Killer; break; }
		}
	}
	const float CX = Canvas->SizeX * 0.5f, CY = Canvas->SizeY * 0.4f;
	Text(TEXT("DU ER DØD"), CX, CY, FLinearColor(1.f, 0.3f, 0.3f), 2.f, true);
	if (!Killer.IsEmpty())
	{
		Text(FString::Printf(TEXT("Drept av %s"), *Killer), CX, CY + 50.f, FLinearColor::White, 1.2f, true);
	}
	Text(TEXT("Du kommer tilbake om noen sekunder ..."), CX, CY + 85.f, FLinearColor(0.85f, 0.85f, 0.85f), 1.f, true);
}

void AGSHUD::DrawHelp()
{
	const float Since = GetWorld()->GetTimeSeconds() - StartTime;
	if (Since > 25.f) return;
	const float A = FMath::Clamp(25.f - Since, 0.f, 1.f);
	const float CX = Canvas->SizeX * 0.5f, Y = Canvas->SizeY - 70.f;
	Text(TEXT("WASD gå  ·  Shift løp  ·  Space hopp  ·  Ctrl/C huk  ·  Mus1 skyt  ·  Mus2 sikt  ·  R lad  ·  1/2 bytt våpen  ·  Tab poeng"),
		CX, Y, FLinearColor(1.f, 1.f, 1.f, A), 0.9f, true);
	Text(TEXT("Finn AG3-er (blå ring) inne i byggene. Lysgården er midt i Hovedbygningen."),
		CX, Y + 22.f, FLinearColor(0.8f, 0.9f, 1.f, A), 0.9f, true);
}
