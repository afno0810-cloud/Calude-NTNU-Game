#include "GSHUD.h"
#include "GSCharacter.h"
#include "GSGameState.h"
#include "GSItems.h"
#include "GSPickup.h"
#include "GSPlayerState.h"
#include "GSPlayerController.h"
#include "GSGameMode.h"
#include "GSTerminal.h"
#include "GSCampus.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void AGSHUD::Text(const FString& S, float X, float Y, const FLinearColor& Color, float Scale, bool bCenter, bool bRight)
{
	UFont* Font = GEngine->GetMediumFont();
	const float W = Font->GetStringSize(*S) * Scale;
	if (bCenter) X -= W * 0.5f;
	if (bRight) X -= W;
	// shadow for readability on bright snow
	Canvas->SetDrawColor(FColor(0, 0, 0, uint8(160 * Color.A)));
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

void AGSHUD::Frame(float X, float Y, float W, float H, const FLinearColor& Color, float T)
{
	Box(X, Y, W, T, Color);
	Box(X, Y + H - T, W, T, Color);
	Box(X, Y, T, H, Color);
	Box(X + W - T, Y, T, H, Color);
}

void AGSHUD::Bar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Color)
{
	Box(X, Y, W, H, FLinearColor(0.1f, 0.1f, 0.1f, 0.7f));
	Box(X, Y, W * FMath::Clamp(Fraction, 0.f, 1.f), H, Color);
}

void AGSHUD::AddShotMarker(const FVector& Where)
{
	Shots.Add(TPair<FVector, float>(Where, GetWorld()->GetTimeSeconds()));
	if (Shots.Num() > 40) Shots.RemoveAt(0);
}

void AGSHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;
	if (StartTime < 0.f) StartTime = GetWorld()->GetTimeSeconds();

	AGSCharacter* C = Cast<AGSCharacter>(GetOwningPawn());
	const AGSPlayerController* PC = Cast<AGSPlayerController>(PlayerOwner);
	if (C && !C->bDead && PC && PC->IsOnTerminal())
	{
		DrawTerminal(C, const_cast<AGSPlayerController*>(PC));
		DrawToast(C);
		return;
	}
	if (C && !C->bDead)
	{
		const FGSEffectMods Mods = C->GetMods();
		if (Mods.bWallhack)
		{
			// The drone only sees 40 m, the mushroom sees everything.
			bool bLong = false;
			for (const FGSActiveEffect& E : C->Effects)
			{
				if (E.Id == FName(TEXT("Oyet")) && C->EffectTimeLeft(E) > 0.f) bLong = true;
			}
			DrawWallhack(C, bLong ? 20000.f : 4000.f);
		}
		if (C->IsUsingBinoculars()) DrawBinoculars();

		// Red flash when hit
		const float SinceHurt = GetWorld()->GetTimeSeconds() - C->GetLastDamageTakenTime();
		if (SinceHurt < 0.5f)
		{
			Box(0, 0, Canvas->SizeX, Canvas->SizeY, FLinearColor(0.6f, 0.f, 0.f, 0.35f * (1.f - SinceHurt / 0.5f)));
		}
		if (C->IsScopedIn() && C->GetAimBlend() > 0.85f) DrawScope(C);
		else DrawCrosshair(C);
		DrawDamageDirection(C);
		DrawBars(C);
		DrawAmmo(C);
		DrawQuickbar(C);
		DrawEffects(C);
		DrawPrompt(C);
		DrawUseProgress(C);
		DrawToast(C);
		DrawMinimap(C);
		if (C->HasItemKind(FName(TEXT("radio")))) DrawRadar(C);
		if (C->HasItemKind(FName(TEXT("compass")))) DrawCompass(C);
		if (PC && PC->bShowInventory) DrawInventory(C);

		// Flashbang: white screen that fades
		for (const FGSActiveEffect& E : C->Effects)
		{
			if (E.Id == FName(TEXT("Blendet")))
			{
				const float Left = C->EffectTimeLeft(E);
				if (Left > 0.f) Box(0, 0, Canvas->SizeX, Canvas->SizeY, FLinearColor(1.f, 1.f, 1.f, FMath::Clamp(Left / 3.f, 0.f, 0.97f)));
			}
		}
	}
	else
	{
		DrawDeathScreen();
	}

	DrawTopRight();
	DrawMatch();
	DrawKillFeed();
	DrawHelp();

	if (PC && PC->bShowScoreboard) DrawScoreboard();
	if (PC && PC->bMenuOpen) DrawMenu(const_cast<AGSPlayerController*>(PC));
}

void AGSHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);
	if (AGSPlayerController* PC = Cast<AGSPlayerController>(PlayerOwner))
	{
		if (PC->bMenuOpen) PC->MenuClick(BoxName);
	}
}

bool AGSHUD::MenuButton(FName Id, const FString& Label, float X, float Y, float W, float H, const FVector2D& Mouse, bool bAccent)
{
	const bool bHover = Mouse.X >= X && Mouse.X <= X + W && Mouse.Y >= Y && Mouse.Y <= Y + H;
	Box(X, Y, W, H, bHover ? FLinearColor(0.95f, 0.75f, 0.2f, 0.9f) : (bAccent ? FLinearColor(0.2f, 0.35f, 0.6f, 0.85f) : FLinearColor(0.08f, 0.09f, 0.11f, 0.85f)));
	Frame(X, Y, W, H, FLinearColor(1.f, 1.f, 1.f, bHover ? 0.9f : 0.25f), 1.5f);
	Text(Label, X + W * 0.5f, Y + H * 0.5f - 11.f, bHover ? FLinearColor(0.05f, 0.05f, 0.05f) : FLinearColor::White, 1.1f, true);
	AddHitBox(FVector2D(X, Y), FVector2D(W, H), Id, true);
	return bHover;
}

void AGSHUD::DrawMenu(AGSPlayerController* PC)
{
	const float SX = Canvas->SizeX, SY = Canvas->SizeY;
	Box(0, 0, SX, SY, FLinearColor(0.f, 0.f, 0.02f, 0.62f));
	float MX = 0.f, MY = 0.f;
	PC->GetMousePosition(MX, MY);
	const FVector2D Mouse(MX, MY);
	const float W = 420.f, H = 44.f, Gap = 10.f;
	const float X = SX * 0.5f - W * 0.5f;
	float Y = SY * 0.5f - 290.f;
	Text(TEXT("GL\u00D8SHAUGEN"), SX * 0.5f, Y, FLinearColor(0.95f, 0.75f, 0.2f), 2.6f, true);
	Text(TEXT("NTNU-campus, Trondheim"), SX * 0.5f, Y + 62.f, FLinearColor(1.f, 1.f, 1.f, 0.7f), 1.0f, true);
	Y += 110.f;
	MenuButton(TEXT("Resume"), TEXT("Fortsett"), X, Y, W, H, Mouse, true); Y += H + Gap;
	MenuButton(TEXT("Host"), TEXT("Start server (venner kan bli med)"), X, Y, W, H, Mouse); Y += H + Gap;
	MenuButton(TEXT("Join"), FString::Printf(TEXT("Bli med: %s"), *PC->LastJoinAddress), X, Y, W, H, Mouse); Y += H + Gap;
	static const TCHAR* Levels[] = { TEXT("Lav"), TEXT("Middels"), TEXT("H\u00F8y"), TEXT("Episk"), TEXT("Film") };
	MenuButton(TEXT("Graphics"), FString::Printf(TEXT("Grafikk: %s"), Levels[PC->GetGraphicsLevel()]), X, Y, W, H, Mouse); Y += H + Gap;
	MenuButton(TEXT("Resolution"), FString::Printf(TEXT("Oppl\u00F8sning: %d %%"), PC->GetResolutionPercent()), X, Y, W, H, Mouse); Y += H + Gap;
	MenuButton(TEXT("RayTracing"), FString::Printf(TEXT("Str\u00E5lesporing (RTX): %s"), PC->IsRayTracingOn() ? TEXT("P\u00C5") : TEXT("AV")), X, Y, W, H, Mouse); Y += H + Gap;
	// sensitivity row: [-] value [+]
	MenuButton(TEXT("SensDown"), TEXT("-"), X, Y, H, H, Mouse);
	Box(X + H + 6.f, Y, W - 2.f * H - 12.f, H, FLinearColor(0.08f, 0.09f, 0.11f, 0.85f));
	Text(FString::Printf(TEXT("Musf\u00F8lsomhet: %.1f"), PC->MouseSensitivity), SX * 0.5f, Y + H * 0.5f - 11.f, FLinearColor::White, 1.1f, true);
	MenuButton(TEXT("SensUp"), TEXT("+"), X + W - H, Y, H, H, Mouse); Y += H + Gap;
	MenuButton(TEXT("Quit"), TEXT("Avslutt spillet"), X, Y, W, H, Mouse); Y += H + Gap * 2.f;
	Text(TEXT("Annen adresse: trykk \u00A7 for konsollen og skriv  Join 192.168.1.20"), SX * 0.5f, Y, FLinearColor(1.f, 1.f, 1.f, 0.6f), 0.9f, true);
	Text(TEXT("Esc lukker menyen"), SX * 0.5f, Y + 24.f, FLinearColor(1.f, 1.f, 1.f, 0.6f), 0.9f, true);
}

void AGSHUD::DrawCrosshair(AGSCharacter* C)
{
	if (C->IsUsingBinoculars()) return;
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

	DrawHitMarker(C, CX, CY);
}

void AGSHUD::DrawHitMarker(AGSCharacter* C, float CX, float CY)
{
	const float Since = GetWorld()->GetTimeSeconds() - C->GetLastHitMarkerTime();
	const uint8 Kind = C->GetLastHitKind();
	float Inner = 7.f, Outer = 14.f, Thick = 2.f, Duration = 0.2f;
	FLinearColor Color(1.f, 1.f, 1.f);
	if (Kind == GSHitKind::Armor) Color = FLinearColor(0.6f, 0.8f, 1.f);
	else if (Kind == GSHitKind::Head) { Color = FLinearColor(1.f, 0.82f, 0.2f); Inner = 8.f; Outer = 17.f; Thick = 2.5f; Duration = 0.3f; }
	else if (Kind == GSHitKind::Kill) { Color = FLinearColor(1.f, 0.15f, 0.1f); Inner = 9.f; Outer = 20.f; Thick = 3.f; Duration = 0.5f; }
	if (Since < 0.f || Since >= Duration) return;

	float Scale = 1.f, Alpha;
	if (Kind == GSHitKind::Kill)
	{
		// Kill: starts big and snaps in, holds, then fades out over the last 0.2 s
		if (Since < 0.12f) Scale = FMath::Lerp(1.35f, 1.f, Since / 0.12f);
		Alpha = FMath::Clamp((Duration - Since) / 0.2f, 0.f, 1.f);
	}
	else
	{
		Alpha = 1.f - Since / Duration;
	}
	Color.A = Alpha;
	const FLinearColor Shadow(0.f, 0.f, 0.f, 0.5f * Alpha);
	const FVector2D Mid(CX, CY);
	const FVector2D ShadowOff(1.f, 1.f);
	for (const FVector2D& D : { FVector2D(1.f, 1.f), FVector2D(-1.f, 1.f), FVector2D(1.f, -1.f), FVector2D(-1.f, -1.f) })
	{
		const FVector2D Dir = D * UE_INV_SQRT_2;
		const FVector2D A = Mid + Dir * Inner * Scale, B = Mid + Dir * Outer * Scale;
		Canvas->K2_DrawLine(A + ShadowOff, B + ShadowOff, Thick, Shadow);   // shadow so it shows on snow and sky
		Canvas->K2_DrawLine(A, B, Thick, Color);
	}
	if (Kind == GSHitKind::Kill && C->WasLastKillHeadshot())
	{
		Text(TEXT("HODESKUDD"), CX, CY + 34.f, FLinearColor(1.f, 0.82f, 0.2f, Alpha), 0.8f, true);
	}
}

void AGSHUD::DrawBars(AGSCharacter* C)
{
	const float X = 40.f, W = 260.f;
	float Y = Canvas->SizeY - 150.f;
	Box(X - 8.f, Y - 30.f, W + 16.f, 150.f, FLinearColor(0.f, 0.f, 0.f, 0.35f));
	Text(FString::Printf(TEXT("HELSE  %d"), FMath::RoundToInt(C->Health)), X, Y - 26.f, FLinearColor::White);
	const float HP = C->Health / C->MaxHealth;
	Bar(X, Y, W, 14.f, HP, HP > 0.35f ? FLinearColor(0.2f, 0.85f, 0.3f, 0.9f) : FLinearColor(0.95f, 0.2f, 0.15f, 0.9f));
	Y += 20.f;
	Bar(X, Y, W, 7.f, C->Stamina / 100.f, FLinearColor(0.95f, 0.8f, 0.2f, 0.9f));
	Y += 14.f;

	// Armor
	const FGSItemDef* Vest = GSItems::Find(C->BodyArmor);
	const FGSItemDef* Hat = GSItems::Find(C->Helmet);
	Text(Vest ? FString::Printf(TEXT("Vest: %s"), *Vest->Name) : FString(TEXT("Ingen vest")), X, Y, FLinearColor(0.7f, 0.85f, 1.f), 0.75f);
	Text(Hat ? FString::Printf(TEXT("Hjelm: %s"), *Hat->Name) : FString(TEXT("Ingen hjelm")), X + W, Y, FLinearColor(0.7f, 0.85f, 1.f), 0.75f, false, true);
	Y += 18.f;
	Bar(X, Y, W * 0.62f, 7.f, Vest ? C->ArmorPoints / Vest->Armor : 0.f, FLinearColor(0.3f, 0.65f, 1.f, 0.9f));
	Bar(X + W * 0.66f, Y, W * 0.34f, 7.f, Hat ? C->HelmetPoints / Hat->Armor : 0.f, FLinearColor(0.3f, 0.65f, 1.f, 0.9f));
	Y += 14.f;

	// Hunger and thirst
	const bool bHungry = C->Hunger < 20.f, bThirsty = C->Thirst < 20.f;
	const float Blink = (FMath::Frac(GetWorld()->GetTimeSeconds()) < 0.5f) ? 1.f : 0.4f;
	Text(TEXT("Sult"), X, Y, FLinearColor(1.f, 0.7f, 0.35f, bHungry ? Blink : 1.f), 0.75f);
	Text(TEXT("T\u00F8rst"), X + W * 0.5f + 4.f, Y, FLinearColor(0.45f, 0.75f, 1.f, bThirsty ? Blink : 1.f), 0.75f);
	Y += 18.f;
	Bar(X, Y, W * 0.5f - 4.f, 7.f, C->Hunger / 100.f, FLinearColor(1.f, 0.6f, 0.2f, 0.9f));
	Bar(X + W * 0.5f + 4.f, Y, W * 0.5f - 4.f, 7.f, C->Thirst / 100.f, FLinearColor(0.3f, 0.6f, 1.f, 0.9f));
}

void AGSHUD::DrawAmmo(AGSCharacter* C)
{
	const FGSWeaponSlot* W = C->GetCurrentWeapon();
	const float X = Canvas->SizeX - 40.f, Y = Canvas->SizeY - 110.f;
	if (W)
	{
		const FGSItemDef* Def = GSItems::Find(W->Id);
		const FGSWeaponStats S = C->GetStats(*W);
		Box(X - 260.f, Y - 6.f, 266.f, 86.f, FLinearColor(0.f, 0.f, 0.f, 0.35f));
		Text(GSItems::Name(W->Id), X, Y, Def ? GSItems::RarityColor(Def->Rarity) : FLinearColor::White, 1.f, false, true);
		if (S.UsesAmmo())
		{
			Text(FString::Printf(TEXT("%d / %d"), W->Ammo, W->Reserve), X, Y + 22.f,
				W->Ammo == 0 ? FLinearColor(1.f, 0.3f, 0.3f) : FLinearColor::White, 1.6f, false, true);
		}
		else
		{
			Text(TEXT("N\u00C6RKAMP"), X, Y + 26.f, FLinearColor::White, 1.2f, false, true);
		}
		if (C->bReloading)
		{
			Text(TEXT("LADER OM ..."), X, Y + 56.f, FLinearColor(1.f, 0.85f, 0.3f), 1.f, false, true);
		}
		else if (S.UsesAmmo() && W->Ammo == 0)
		{
			Text(TEXT("Trykk R for \u00E5 lade"), X, Y + 56.f, FLinearColor(1.f, 0.4f, 0.4f), 1.f, false, true);
		}
		FString Att;
		static const TPair<uint16, const TCHAR*> AttNames[] = {
			{ GSAttach::Suppressor, TEXT("Demper") }, { GSAttach::Compensator, TEXT("Kompensator") },
			{ GSAttach::RedDot, TEXT("R\u00F8dpunkt") }, { GSAttach::Holo, TEXT("Holo") }, { GSAttach::Scope, TEXT("4x") },
			{ GSAttach::Scope8x, TEXT("8x") }, { GSAttach::ExtMag, TEXT("Stort mag") }, { GSAttach::Drum, TEXT("Trommel") },
			{ GSAttach::Laser, TEXT("Laser") }, { GSAttach::Grip, TEXT("H\u00E5ndtak") } };
		for (const TPair<uint16, const TCHAR*>& A : AttNames)
		{
			if (W->Attachments & A.Key) { Att += A.Value; Att += TEXT("  "); }
		}
		static const TCHAR* AmmoNames[] = { TEXT(""), TEXT("PANSERBRYTENDE"), TEXT("HULSPISS"), TEXT("BRANN"), TEXT("LYSSPOR"), TEXT("SJOKK") };
		if (W->AmmoType > 0 && W->AmmoType < UE_ARRAY_COUNT(AmmoNames))
		{
			Text(AmmoNames[W->AmmoType], X, Y + 78.f, FLinearColor(1.f, 0.6f, 0.2f), 0.8f, false, true);
		}
		// Rounds in the sack for this gun
		const FGSWeaponStats Stats = C->GetStats(*W);
		if (!Stats.Caliber.IsNone())
		{
			const int32 Boxes = C->CountItem(FName(*(FString(TEXT("Ammo_")) + Stats.Caliber.ToString())));
			if (Boxes > 0) Text(FString::Printf(TEXT("+%d esker i sekken"), Boxes), X, Y + 96.f, FLinearColor(0.7f, 0.8f, 0.9f), 0.75f, false, true);
		}
		if (!Att.IsEmpty()) Text(Att, X - 270.f, Y + 56.f, FLinearColor(0.7f, 0.9f, 0.7f), 0.75f, false, true);
	}

	// Weapon slots 1-4
	for (int32 I = 0; I < C->Weapons.Num(); ++I)
	{
		const bool bSel = I == C->CurrentSlot;
		const FGSItemDef* D = GSItems::Find(C->Weapons[I].Id);
		const FLinearColor Col = D ? GSItems::RarityColor(D->Rarity) : FLinearColor::White;
		Text(FString::Printf(TEXT("%s%d  %s"), bSel ? TEXT("> ") : TEXT(""), I + 1, *GSItems::Name(C->Weapons[I].Id)),
			X, Y - 30.f - (C->Weapons.Num() - I) * 20.f, bSel ? Col : Col * FLinearColor(1.f, 1.f, 1.f, 0.6f), 0.9f, false, true);
	}
}

void AGSHUD::DrawQuickbar(AGSCharacter* C)
{
	const int32 Slots = C->GetBackpackSlots();
	const float S = 58.f, Gap = 6.f;
	const float W = Slots * (S + Gap) - Gap;
	const float X0 = (Canvas->SizeX - W) * 0.5f, Y = Canvas->SizeY - S - 30.f;
	for (int32 I = 0; I < Slots; ++I)
	{
		const float X = X0 + I * (S + Gap);
		Box(X, Y, S, S, FLinearColor(0.f, 0.f, 0.f, 0.45f));
		if (!C->Backpack.IsValidIndex(I)) continue;
		const FGSInvStack& St = C->Backpack[I];
		const FGSItemDef* D = GSItems::Find(St.Id);
		const FLinearColor Col = D ? GSItems::RarityColor(D->Rarity) : FLinearColor::White;
		Box(X, Y + S - 5.f, S, 5.f, Col);
		if (I == C->SelectedItem) Frame(X - 2.f, Y - 2.f, S + 4.f, S + 4.f, FLinearColor(1.f, 0.95f, 0.5f), 3.f);
		const FString Short = D ? D->Name.Left(7) : FString(TEXT("?"));
		Text(Short, X + S * 0.5f, Y + 12.f, FLinearColor::White, 0.65f, true);
		if (St.Count > 1) Text(FString::Printf(TEXT("x%d"), St.Count), X + S - 4.f, Y + S - 24.f, FLinearColor::White, 0.7f, false, true);
	}
	if (C->Backpack.IsValidIndex(C->SelectedItem))
	{
		const FGSItemDef* D = GSItems::Find(C->Backpack[C->SelectedItem].Id);
		if (D)
		{
			Text(FString::Printf(TEXT("%s  [%s]   F bruk  \u00B7  Q neste  \u00B7  G slipp"), *D->Name, GSItems::RarityName(D->Rarity)),
				Canvas->SizeX * 0.5f, Y - 24.f, GSItems::RarityColor(D->Rarity), 0.85f, true);
		}
	}
}

void AGSHUD::DrawEffects(AGSCharacter* C)
{
	float Y = Canvas->SizeY * 0.35f;
	for (const FGSActiveEffect& E : C->Effects)
	{
		const float Left = C->EffectTimeLeft(E);
		const FGSEffectDef* Def = GSItems::FindEffect(E.Id);
		if (Left <= 0.f || !Def) continue;
		const bool bBad = Def->Mods.Drain > 0.f || Def->Mods.Spread > 1.01f || Def->Mods.Speed < 0.9f || Def->Mods.StaminaRegen < 0.9f;
		Box(30.f, Y, 210.f, 24.f, FLinearColor(0.f, 0.f, 0.f, 0.4f));
		Box(30.f, Y + 21.f, 210.f * FMath::Clamp(Left / Def->Duration, 0.f, 1.f), 3.f, bBad ? FLinearColor(1.f, 0.3f, 0.3f) : FLinearColor(0.4f, 1.f, 0.5f));
		Text(FString::Printf(TEXT("%s  %ds"), *Def->Name, FMath::CeilToInt(Left)), 38.f, Y + 2.f, bBad ? FLinearColor(1.f, 0.6f, 0.6f) : FLinearColor(0.7f, 1.f, 0.75f), 0.8f);
		Y += 28.f;
	}
}

void AGSHUD::DrawPrompt(AGSCharacter* C)
{
	if (const AGSTerminal* T = C->GetFocusTerminal())
	{
		const float CX = Canvas->SizeX * 0.5f, Y = Canvas->SizeY * 0.5f + 60.f;
		const bool bPC = T->Kind == EGSTerminalKind::Computer;
		Text(bPC ? TEXT("[E]  Logg inn p\u00E5 PC-en") : TEXT("[E]  Bruk 3D-printeren"), CX, Y, FLinearColor(0.5f, 0.8f, 1.f), 1.1f, true);
		Text(bPC ? TEXT("Overv\u00E5kningskameraer og sporing av spillere") : TEXT("Lag ting av filament-spoler"), CX, Y + 24.f, FLinearColor(0.85f, 0.85f, 0.85f), 0.8f, true);
		if (!T->Label.IsEmpty()) Text(T->Label, CX, Y + 44.f, FLinearColor(0.7f, 0.7f, 0.7f), 0.75f, true);
		return;
	}
	AGSPickup* P = C->GetFocusPickup();
	if (!P) return;
	const FGSItemDef* D = GSItems::Find(P->ItemId);
	const float CX = Canvas->SizeX * 0.5f, Y = Canvas->SizeY * 0.5f + 60.f;
	if (P->bLocked)
	{
		Text(TEXT("[E]  L\u00E5st kasse  (adgangskort eller dirkesett)"), CX, Y, FLinearColor(1.f, 0.85f, 0.4f), 1.f, true);
		return;
	}
	if (!D) return;
	const FString CountStr = P->Count > 1 ? FString::Printf(TEXT(" x%d"), P->Count) : FString();
	Text(FString::Printf(TEXT("[E]  %s%s"), *D->Name, *CountStr), CX, Y, GSItems::RarityColor(D->Rarity), 1.1f, true);
	Text(FString::Printf(TEXT("%s  \u00B7  %s"), GSItems::RarityName(D->Rarity), GSItems::CategoryName(D->Cat)), CX, Y + 24.f, FLinearColor(0.85f, 0.85f, 0.85f), 0.8f, true);
	if (!D->Desc.IsEmpty()) Text(D->Desc, CX, Y + 44.f, FLinearColor(0.8f, 0.8f, 0.8f, 0.9f), 0.75f, true);
	if (D->IsWeapon() && C->Weapons.Num() >= AGSCharacter::MaxWeapons)
	{
		Text(TEXT("(bytter med v\u00E5penet du holder)"), CX, Y + 64.f, FLinearColor(1.f, 0.7f, 0.5f), 0.75f, true);
	}
}

void AGSHUD::DrawUseProgress(AGSCharacter* C)
{
	if (C->UseItemId.IsNone()) return;
	const float Now = C->GetServerTime();
	const float Dur = FMath::Max(0.01f, C->UseEndTime - C->UseStartTime);
	const float Frac = (Now - C->UseStartTime) / Dur;
	const float W = 240.f, X = (Canvas->SizeX - W) * 0.5f, Y = Canvas->SizeY * 0.5f + 40.f;
	Text(FString::Printf(TEXT("Bruker %s ..."), *GSItems::Name(C->UseItemId)), Canvas->SizeX * 0.5f, Y - 22.f, FLinearColor::White, 0.85f, true);
	Bar(X, Y, W, 8.f, Frac, FLinearColor(0.4f, 0.9f, 1.f));
}

void AGSHUD::DrawToast(AGSCharacter* C)
{
	const float Age = GetWorld()->GetTimeSeconds() - C->ToastTime;
	if (Age > 3.5f || C->ToastText.IsEmpty()) return;
	const float A = FMath::Clamp(3.5f - Age, 0.f, 1.f);
	Text(C->ToastText, Canvas->SizeX * 0.5f, Canvas->SizeY * 0.28f, FLinearColor(1.f, 0.95f, 0.7f, A), 1.1f, true);
}

void AGSHUD::DrawWallhack(AGSCharacter* C, float Range)
{
	for (TActorIterator<AGSCharacter> It(GetWorld()); It; ++It)
	{
		if (*It == C || It->bDead) continue;
		const float Dist = FVector::Dist(It->GetActorLocation(), C->GetActorLocation());
		if (Dist > Range) continue;
		FVector2D Top, Bottom;
		if (!PlayerOwner->ProjectWorldLocationToScreen(It->GetActorLocation() + FVector(0.f, 0.f, 95.f), Top)) continue;
		if (!PlayerOwner->ProjectWorldLocationToScreen(It->GetActorLocation() - FVector(0.f, 0.f, 92.f), Bottom)) continue;
		const float H = FMath::Max(8.f, float(Bottom.Y - Top.Y));
		const float W = H * 0.45f;
		Frame(Top.X - W * 0.5f, Top.Y, W, H, FLinearColor(1.f, 0.15f, 0.15f, 0.9f));
		Text(FString::Printf(TEXT("%dm"), FMath::RoundToInt(Dist / 100.f)), Top.X, Top.Y - 18.f, FLinearColor(1.f, 0.4f, 0.4f), 0.7f, true);
	}
}

void AGSHUD::DrawRadar(AGSCharacter* C)
{
	const float R = 70.f, CX = 40.f + R, CY = Canvas->SizeY - 190.f - R;
	Box(CX - R, CY - R, R * 2.f, R * 2.f, FLinearColor(0.f, 0.1f, 0.f, 0.35f));
	Frame(CX - R, CY - R, R * 2.f, R * 2.f, FLinearColor(0.3f, 1.f, 0.3f, 0.5f), 1.f);
	Box(CX - 2.f, CY - 2.f, 4.f, 4.f, FLinearColor::White);
	Text(TEXT("Walkie-talkie"), CX, CY - R - 18.f, FLinearColor(0.5f, 1.f, 0.5f), 0.7f, true);
	const float Now = GetWorld()->GetTimeSeconds();
	const float Yaw = C->GetControlRotation().Yaw;
	for (const TPair<FVector, float>& S : Shots)
	{
		const float Age = Now - S.Value;
		if (Age > 6.f) continue;
		FVector D = S.Key - C->GetActorLocation();
		if (D.Size2D() > 15000.f) continue;
		D = D.RotateAngleAxis(-Yaw - 90.f, FVector::UpVector);
		const FVector2D P(D.X / 15000.f * R, D.Y / 15000.f * R);
		Box(CX + P.X - 3.f, CY + P.Y - 3.f, 6.f, 6.f, FLinearColor(1.f, 0.3f, 0.2f, 1.f - Age / 6.f));
	}
}

void AGSHUD::FillClipped(TArray<FVector2D> Poly, const FBox2D& Clip, const FLinearColor& Color)
{
	// Sutherland-Hodgman against the four edges, then a triangle fan.
	auto ClipEdge = [](const TArray<FVector2D>& In, int32 Axis, float Value, bool bKeepGreater)
	{
		TArray<FVector2D> Out;
		for (int32 i = 0; i < In.Num(); ++i)
		{
			const FVector2D A = In[i], B = In[(i + 1) % In.Num()];
			const bool bA = bKeepGreater ? A[Axis] >= Value : A[Axis] <= Value;
			const bool bB = bKeepGreater ? B[Axis] >= Value : B[Axis] <= Value;
			if (bA) Out.Add(A);
			if (bA != bB)
			{
				const float T = (Value - A[Axis]) / (B[Axis] - A[Axis]);
				Out.Add(A + (B - A) * T);
			}
		}
		return Out;
	};
	Poly = ClipEdge(Poly, 0, Clip.Min.X, true);
	if (Poly.Num() >= 3) Poly = ClipEdge(Poly, 0, Clip.Max.X, false);
	if (Poly.Num() >= 3) Poly = ClipEdge(Poly, 1, Clip.Min.Y, true);
	if (Poly.Num() >= 3) Poly = ClipEdge(Poly, 1, Clip.Max.Y, false);
	if (Poly.Num() < 3) return;
	TArray<FCanvasUVTri> Tris;
	for (int32 i = 1; i + 1 < Poly.Num(); ++i)
	{
		FCanvasUVTri T;
		T.V0_Pos = Poly[0]; T.V1_Pos = Poly[i]; T.V2_Pos = Poly[i + 1];
		T.V0_Color = T.V1_Color = T.V2_Color = Color;
		Tris.Add(T);
	}
	Canvas->K2_DrawTriangle(nullptr, Tris);
}

void AGSHUD::DrawMinimap(AGSCharacter* C)
{
	const float Size = FMath::Clamp(Canvas->SizeY * 0.22f, 160.f, 260.f);
	const float X0 = Canvas->SizeX - Size - 30.f, Y0 = 70.f;
	const FBox2D Clip(FVector2D(X0, Y0), FVector2D(X0 + Size, Y0 + Size));
	const FVector2D Mid(X0 + Size * 0.5f, Y0 + Size * 0.5f);
	const float RangeM = 140.f;                       // meters across the map
	const float PxPerM = Size / RangeM;
	const FVector Me = C->GetActorLocation() / 100.f; // meters
	const float Yaw = C->GetControlRotation().Yaw;
	// world (m) -> screen, rotated so that where you look is up
	const float Rad = FMath::DegreesToRadians(-Yaw - 90.f);
	const float Cs = FMath::Cos(Rad), Sn = FMath::Sin(Rad);
	auto ToScreen = [&](float WX, float WY)
	{
		const float DX = WX - Me.X, DY = WY - Me.Y;
		return Mid + FVector2D(DX * Cs - DY * Sn, DX * Sn + DY * Cs) * PxPerM;
	};

	Box(X0 - 3.f, Y0 - 3.f, Size + 6.f, Size + 6.f, FLinearColor(0.f, 0.f, 0.f, 0.55f));
	FillClipped({ Clip.Min, FVector2D(Clip.Max.X, Clip.Min.Y), Clip.Max, FVector2D(Clip.Min.X, Clip.Max.Y) }, Clip, FLinearColor(0.12f, 0.2f, 0.1f, 0.85f));
	for (const GSCampus::FRoad& R : GSCampus::Roads())
	{
		const FVector2D D = (R.B - R.A).GetSafeNormal();
		const FVector2D N(-D.Y, D.X);
		const float HW = R.Width * 0.5f;
		const FVector2D A = R.A - D * HW, B = R.B + D * HW;
		FillClipped({ ToScreen(A.X + N.X * HW, A.Y + N.Y * HW), ToScreen(B.X + N.X * HW, B.Y + N.Y * HW),
			ToScreen(B.X - N.X * HW, B.Y - N.Y * HW), ToScreen(A.X - N.X * HW, A.Y - N.Y * HW) }, Clip,
			R.bPath ? FLinearColor(0.5f, 0.48f, 0.44f, 0.9f) : FLinearColor(0.22f, 0.22f, 0.24f, 0.95f));
	}
	const GSCampus::FBuilding* Here = nullptr;
	float HereDist = 1e9f;
	for (const GSCampus::FBuilding& B : GSCampus::Buildings())
	{
		const float BR = FMath::DegreesToRadians(B.Yaw);
		const float BC = FMath::Cos(BR), BS = FMath::Sin(BR);
		TArray<FVector2D> Q;
		for (const FVector2D& Corner : { FVector2D(-1.f, -1.f), FVector2D(1.f, -1.f), FVector2D(1.f, 1.f), FVector2D(-1.f, 1.f) })
		{
			const float LX = Corner.X * B.SizeX * 0.5f, LY = Corner.Y * B.SizeY * 0.5f;
			Q.Add(ToScreen(B.X + LX * BC - LY * BS, B.Y + LX * BS + LY * BC));
		}
		FillClipped(Q, Clip, B.bEnterable ? FLinearColor(0.62f, 0.6f, 0.55f, 0.95f) : FLinearColor(0.45f, 0.45f, 0.47f, 0.95f));
		const float Dist = FVector2D::Distance(FVector2D(B.X, B.Y), FVector2D(Me.X, Me.Y)) - FMath::Max(B.SizeX, B.SizeY) * 0.5f;
		if (Dist < HereDist) { HereDist = Dist; Here = &B; }
		const FVector2D Centre = ToScreen(B.X, B.Y);
		if (Clip.ExpandBy(-14.f).IsInside(Centre) && B.bEnterable)
		{
			Text(FString(B.Name).Replace(TEXT("_"), TEXT(" ")), Centre.X, Centre.Y - 6.f, FLinearColor(1.f, 1.f, 1.f, 0.85f), 0.55f, true);
		}
	}
	// loot with a beam (epic and better) shows as a coloured dot
	for (TActorIterator<AGSPickup> It(GetWorld()); It; ++It)
	{
		const AGSPickup* P = *It;
		const FGSItemDef* Def = GSItems::Find(P->ItemId);
		if (!Def || !P->bAvailable || Def->Rarity < EGSRarity::Epic) continue;
		const FVector L = P->GetActorLocation() / 100.f;
		const FVector2D S = ToScreen(L.X, L.Y);
		if (!Clip.ExpandBy(-3.f).IsInside(S)) continue;
		Box(S.X - 3.f, S.Y - 3.f, 6.f, 6.f, GSItems::RarityColor(Def->Rarity));
	}
	// you: an arrow pointing up
	{
		FCanvasUVTri T;
		T.V0_Pos = Mid + FVector2D(0.f, -9.f); T.V1_Pos = Mid + FVector2D(6.f, 7.f); T.V2_Pos = Mid + FVector2D(-6.f, 7.f);
		T.V0_Color = T.V1_Color = T.V2_Color = FLinearColor(0.3f, 0.75f, 1.f);
		Canvas->K2_DrawTriangle(nullptr, TArray<FCanvasUVTri>{ T });
	}
	// north on the rim (world north is -Y)
	const FVector2D North = ToScreen(Me.X, Me.Y - 1000.f) - Mid;
	const FVector2D NDir = North.GetSafeNormal();
	const float Edge = Size * 0.5f - 10.f;
	const float K = Edge / FMath::Max(FMath::Abs(NDir.X), FMath::Abs(NDir.Y));
	Text(TEXT("N"), Mid.X + NDir.X * K, Mid.Y + NDir.Y * K - 8.f, FLinearColor(1.f, 0.85f, 0.3f), 0.8f, true);
	Frame(X0 - 3.f, Y0 - 3.f, Size + 6.f, Size + 6.f, FLinearColor(1.f, 1.f, 1.f, 0.35f), 1.f);
	if (Here)
	{
		const FString Where = HereDist < 2.f ? FString::Printf(TEXT("I %s"), Here->Name) : FString::Printf(TEXT("Ved %s"), Here->Name);
		Text(Where.Replace(TEXT("_"), TEXT(" ")), X0 + Size * 0.5f, Y0 + Size + 8.f, FLinearColor(0.9f, 0.95f, 1.f, 0.9f), 0.75f, true);
	}
}

void AGSHUD::DrawScope(AGSCharacter* C)
{
	const float CX = Canvas->SizeX * 0.5f, CY = Canvas->SizeY * 0.5f;
	const float R = Canvas->SizeY * 0.46f, Far = FMath::Max(Canvas->SizeX, Canvas->SizeY) * 2.f;
	const int32 Seg = 64;
	TArray<FCanvasUVTri> Tris;
	const FLinearColor Black(0.f, 0.f, 0.f, 1.f);
	for (int32 i = 0; i < Seg; ++i)
	{
		const float A0 = 2.f * PI * i / Seg, A1 = 2.f * PI * (i + 1) / Seg;
		const FVector2D In0(CX + FMath::Cos(A0) * R, CY + FMath::Sin(A0) * R), In1(CX + FMath::Cos(A1) * R, CY + FMath::Sin(A1) * R);
		const FVector2D Out0(CX + FMath::Cos(A0) * Far, CY + FMath::Sin(A0) * Far), Out1(CX + FMath::Cos(A1) * Far, CY + FMath::Sin(A1) * Far);
		FCanvasUVTri T1; T1.V0_Pos = In0; T1.V1_Pos = Out0; T1.V2_Pos = Out1; T1.V0_Color = T1.V1_Color = T1.V2_Color = Black;
		FCanvasUVTri T2; T2.V0_Pos = In0; T2.V1_Pos = Out1; T2.V2_Pos = In1; T2.V0_Color = T2.V1_Color = T2.V2_Color = Black;
		Tris.Add(T1); Tris.Add(T2);
	}
	Canvas->K2_DrawTriangle(nullptr, Tris);
	// reticle: thick outer posts, thin centre cross, mil dots
	const FLinearColor Ink(0.f, 0.f, 0.f, 0.95f);
	Box(CX - R, CY - 2.f, R * 0.55f, 4.f, Ink);
	Box(CX + R * 0.45f, CY - 2.f, R * 0.55f, 4.f, Ink);
	Box(CX - 2.f, CY + R * 0.45f, 4.f, R * 0.55f, Ink);
	Box(CX - R * 0.45f, CY - 0.5f, R * 0.9f, 1.f, Ink);
	Box(CX - 0.5f, CY - R * 0.45f, 1.f, R * 0.9f, Ink);
	for (int32 i = 1; i <= 4; ++i)
	{
		const float D = R * 0.1f * i;
		for (const FVector2D& P : { FVector2D(CX + D, CY), FVector2D(CX - D, CY), FVector2D(CX, CY + D), FVector2D(CX, CY - D) })
		{
			Box(P.X - 2.f, P.Y - 2.f, 4.f, 4.f, Ink);
		}
	}
	Box(CX - 1.5f, CY - 1.5f, 3.f, 3.f, FLinearColor(1.f, 0.1f, 0.1f, 0.9f));
	// breath: hint, and a bar that empties while you hold it
	const float Held = C->GetBreathHeld();
	const float BY = CY + R * 0.8f;
	if (Held <= 0.01f)
	{
		Text(TEXT("Shift: hold pusten"), CX, BY, FLinearColor(0.8f, 0.8f, 0.8f, 0.7f), 0.9f, true);
	}
	else
	{
		const float Left = FMath::Clamp(1.f - Held / 4.f, 0.f, 1.f);
		Box(CX - 80.f, BY, 160.f, 6.f, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		Box(CX - 80.f, BY, 160.f * Left, 6.f, Left > 0.25f ? FLinearColor(0.8f, 0.9f, 1.f, 0.85f) : FLinearColor(1.f, 0.3f, 0.2f, 0.9f));
	}
	// hit marker still shows when scoped
	DrawHitMarker(C, CX, CY);
}

void AGSHUD::DrawDamageDirection(AGSCharacter* C)
{
	const float Age = GetWorld()->GetTimeSeconds() - C->LastDamageFromTime;
	if (Age > 1.6f) return;
	const FVector D = C->LastDamageFrom - C->GetActorLocation();
	const float Rel = FMath::DegreesToRadians(FRotator::NormalizeAxis(D.Rotation().Yaw - C->GetControlRotation().Yaw));
	const float CX = Canvas->SizeX * 0.5f, CY = Canvas->SizeY * 0.5f, Rad = Canvas->SizeY * 0.17f;
	const FVector2D Dir(FMath::Sin(Rel), -FMath::Cos(Rel));
	const FVector2D Side(-Dir.Y, Dir.X);
	const FVector2D Tip = FVector2D(CX, CY) + Dir * (Rad + 26.f);
	const FVector2D Base = FVector2D(CX, CY) + Dir * Rad;
	const FLinearColor Col(1.f, 0.1f, 0.05f, 0.85f * (1.f - Age / 1.6f));
	FCanvasUVTri T;
	T.V0_Pos = Tip; T.V1_Pos = Base + Side * 34.f; T.V2_Pos = Base - Side * 34.f;
	T.V0_Color = T.V1_Color = T.V2_Color = Col;
	Canvas->K2_DrawTriangle(nullptr, TArray<FCanvasUVTri>{ T });
}

void AGSHUD::DrawMatch()
{
	const AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>();
	if (!GS) return;
	const float CX = Canvas->SizeX * 0.5f;
	const float Left = GS->PhaseSecondsLeft();
	const int32 Sec = FMath::CeilToInt(Left);
	switch (GS->Phase)
	{
	case EGSMatchPhase::Lobby:
	case EGSMatchPhase::Countdown:
	{
		const int32 N = GS->PlayerArray.Num();
		const float W = 420.f, H = 90.f + N * 24.f;
		const float X = CX - W * 0.5f, Y = 60.f;
		Box(X, Y, W, H, FLinearColor(0.02f, 0.03f, 0.06f, 0.7f));
		Frame(X, Y, W, H, FLinearColor(0.4f, 0.6f, 1.f, 0.6f), 2.f);
		Text(TEXT("LOBBY  -  oppvarming, ingen skade"), CX, Y + 10.f, FLinearColor(0.7f, 0.85f, 1.f), 1.1f, true);
		float RY = Y + 44.f;
		const AGSPlayerState* Mine = PlayerOwner ? PlayerOwner->GetPlayerState<AGSPlayerState>() : nullptr;
		for (APlayerState* P : GS->PlayerArray)
		{
			const AGSPlayerState* PS = Cast<AGSPlayerState>(P);
			if (!PS) continue;
			const bool bMe = PS == Mine;
			Text(PS->GetPlayerName() + (bMe ? TEXT("  (deg)") : TEXT("")), X + 24.f, RY, bMe ? FLinearColor(0.5f, 0.85f, 1.f) : FLinearColor::White, 0.9f);
			Text(PS->bReady ? TEXT("KLAR") : TEXT("venter"), X + W - 24.f, RY, PS->bReady ? FLinearColor(0.3f, 1.f, 0.4f) : FLinearColor(1.f, 0.7f, 0.3f), 0.9f, false, true);
			RY += 24.f;
		}
		if (GS->Phase == EGSMatchPhase::Countdown)
		{
			Text(FString::Printf(TEXT("Kampen starter om %d"), Sec), CX, Y + H + 14.f, FLinearColor(1.f, 0.85f, 0.3f), 2.2f, true);
		}
		else
		{
			const bool bReady = Mine && Mine->bReady;
			Text(bReady ? TEXT("Du er klar. Trykk Enter for \u00E5 angre.") : TEXT("Trykk Enter n\u00E5r du er klar"),
				CX, Y + H - 26.f, FLinearColor(1.f, 1.f, 1.f, 0.85f), 0.85f, true);
			Text(FString::Printf(TEXT("F\u00F8rst til %d drap eller %d minutter"), AGSGameState::KillLimit, int32(AGSGameState::MatchMinutes)),
				CX, Y + H + 8.f, FLinearColor(0.8f, 0.8f, 0.8f, 0.8f), 0.75f, true);
		}
		break;
	}
	case EGSMatchPhase::Playing:
	{
		const FString Clock = FString::Printf(TEXT("%d:%02d"), Sec / 60, Sec % 60);
		Box(CX - 70.f, 46.f, 140.f, 30.f, FLinearColor(0.f, 0.f, 0.f, 0.45f));
		Text(Clock, CX, 50.f, Sec < 60 ? FLinearColor(1.f, 0.4f, 0.3f) : FLinearColor::White, 1.2f, true);
		break;
	}
	case EGSMatchPhase::Ended:
	{
		Box(0.f, Canvas->SizeY * 0.3f, Canvas->SizeX, 130.f, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		Text(TEXT("KAMPEN ER OVER"), CX, Canvas->SizeY * 0.3f + 14.f, FLinearColor(1.f, 0.85f, 0.3f), 2.2f, true);
		Text(GS->Winner.IsEmpty() ? FString(TEXT("Ingen vinner")) : FString::Printf(TEXT("Vinner: %s"), *GS->Winner),
			CX, Canvas->SizeY * 0.3f + 66.f, FLinearColor::White, 1.4f, true);
		Text(FString::Printf(TEXT("Tilbake til lobbyen om %d"), Sec), CX, Canvas->SizeY * 0.3f + 100.f, FLinearColor(0.8f, 0.8f, 0.8f), 0.8f, true);
		DrawScoreboard();
		break;
	}
	}
}

void AGSHUD::DrawCompass(AGSCharacter* C)
{
	const float Yaw = FRotator::NormalizeAxis(C->GetControlRotation().Yaw);
	const float CX = Canvas->SizeX * 0.5f, Y = 18.f, W = 400.f;
	Box(CX - W * 0.5f, Y, W, 22.f, FLinearColor(0.f, 0.f, 0.f, 0.35f));
	// World +X is east, +Y is south
	struct FMark { float Angle; const TCHAR* Label; };
	static const FMark Marks[] = { { 0.f, TEXT("\u00D8") }, { 90.f, TEXT("S") }, { 180.f, TEXT("V") }, { -90.f, TEXT("N") },
		{ 45.f, TEXT("S\u00D8") }, { 135.f, TEXT("SV") }, { -45.f, TEXT("N\u00D8") }, { -135.f, TEXT("NV") } };
	for (const FMark& M : Marks)
	{
		const float Delta = FRotator::NormalizeAxis(M.Angle - Yaw);
		if (FMath::Abs(Delta) > 60.f) continue;
		Text(M.Label, CX + Delta / 60.f * W * 0.5f, Y + 2.f, FLinearColor::White, 0.8f, true);
	}
	Box(CX - 1.f, Y + 18.f, 2.f, 8.f, FLinearColor(1.f, 0.8f, 0.3f));
}

void AGSHUD::DrawInventory(AGSCharacter* C)
{
	const float W = 620.f, X = (Canvas->SizeX - W) * 0.5f, Y = Canvas->SizeY * 0.12f;
	const int32 Rows = 6 + C->Weapons.Num() + C->Backpack.Num();
	Box(X, Y, W, 60.f + Rows * 22.f, FLinearColor(0.f, 0.f, 0.f, 0.72f));
	Text(TEXT("INVENTAR  (I for \u00E5 lukke)"), X + W * 0.5f, Y + 10.f, FLinearColor::White, 1.1f, true);
	float RY = Y + 44.f;
	auto Row = [&](const FString& L, const FString& R, const FLinearColor& Col)
	{
		Text(L, X + 20.f, RY, Col, 0.85f);
		Text(R, X + W - 20.f, RY, FLinearColor(0.8f, 0.8f, 0.8f), 0.75f, false, true);
		RY += 22.f;
	};
	Text(TEXT("V\u00E5pen"), X + 20.f, RY, FLinearColor(0.6f, 0.6f, 0.6f), 0.8f); RY += 22.f;
	for (int32 I = 0; I < C->Weapons.Num(); ++I)
	{
		const FGSItemDef* D = GSItems::Find(C->Weapons[I].Id);
		if (!D) continue;
		Row(FString::Printf(TEXT("%d  %s"), I + 1, *D->Name), FString::Printf(TEXT("%s \u00B7 %d skade \u00B7 %d/%d"), GSItems::RarityName(D->Rarity),
			FMath::RoundToInt(D->W.Damage), C->Weapons[I].Ammo, C->Weapons[I].Reserve), GSItems::RarityColor(D->Rarity));
	}
	Text(TEXT("Rustning"), X + 20.f, RY, FLinearColor(0.6f, 0.6f, 0.6f), 0.8f); RY += 22.f;
	if (const FGSItemDef* V = GSItems::Find(C->BodyArmor)) Row(V->Name, FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(C->ArmorPoints), FMath::RoundToInt(V->Armor)), GSItems::RarityColor(V->Rarity));
	else Row(TEXT("(ingen vest)"), TEXT(""), FLinearColor(0.5f, 0.5f, 0.5f));
	if (const FGSItemDef* H = GSItems::Find(C->Helmet)) Row(H->Name, FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(C->HelmetPoints), FMath::RoundToInt(H->Armor)), GSItems::RarityColor(H->Rarity));
	else Row(TEXT("(ingen hjelm)"), TEXT(""), FLinearColor(0.5f, 0.5f, 0.5f));
	Text(FString::Printf(TEXT("Sekk (%d/%d)"), C->Backpack.Num(), C->GetBackpackSlots()), X + 20.f, RY, FLinearColor(0.6f, 0.6f, 0.6f), 0.8f); RY += 22.f;
	for (int32 I = 0; I < C->Backpack.Num(); ++I)
	{
		const FGSItemDef* D = GSItems::Find(C->Backpack[I].Id);
		if (!D) continue;
		Row(FString::Printf(TEXT("%s%s x%d"), I == C->SelectedItem ? TEXT("> ") : TEXT(""), *D->Name, C->Backpack[I].Count),
			D->Desc.Left(60), GSItems::RarityColor(D->Rarity));
	}
}

void AGSHUD::DrawBinoculars()
{
	const float CX = Canvas->SizeX * 0.5f, CY = Canvas->SizeY * 0.5f, R = Canvas->SizeY * 0.42f;
	// Dark outside two circles (rough: dark bars around a box)
	Box(0, 0, Canvas->SizeX, CY - R, FLinearColor(0, 0, 0, 0.95f));
	Box(0, CY + R, Canvas->SizeX, CY - R + 1.f, FLinearColor(0, 0, 0, 0.95f));
	Box(0, CY - R, CX - R * 1.7f, R * 2.f, FLinearColor(0, 0, 0, 0.95f));
	Box(CX + R * 1.7f, CY - R, CX - R * 1.7f + 1.f, R * 2.f, FLinearColor(0, 0, 0, 0.95f));
	Box(CX - 40.f, CY, 80.f, 1.f, FLinearColor(0, 0, 0, 0.6f));
	Box(CX, CY - 40.f, 1.f, 80.f, FLinearColor(0, 0, 0, 0.6f));
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
	Text(TEXT("NTNU Gl\u00F8shaugen"), Canvas->SizeX - 30.f, 46.f, FLinearColor(0.8f, 0.85f, 1.f, 0.8f), 0.85f, false, true);
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
	Text(TEXT("D\u00F8d"), X + 430.f, Y + 40.f, FLinearColor(0.7f, 0.7f, 0.7f));
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
	const AGSPlayerController* PC = Cast<AGSPlayerController>(PlayerOwner);
	if (!PC || !PC->bHasDeathInfo) return;   // not dead yet (lobby, first spawn)
	const FGSDeathInfo& Info = PC->DeathInfo;
	const float SX = Canvas->SizeX, SY = Canvas->SizeY, CX = SX * 0.5f;
	const float Since = GetWorld()->GetTimeSeconds() - PC->LocalDeathTime;
	const float In = FMath::Clamp(Since / 0.6f, 0.f, 1.f);
	const float FadeIn = FMath::Clamp(Since / 0.3f, 0.f, 1.f);
	UFont* Font = GEngine->GetMediumFont();
	auto TextWidth = [Font](const FString& S, float Scale) { return Font->GetStringSize(*S) * Scale; };

	// Red tint and a dark vignette that fade in
	Box(0, 0, SX, SY, FLinearColor(0.35f, 0.f, 0.f, 0.25f * In));
	for (int32 Band = 0; Band < 6; ++Band)
	{
		const float D = 120.f - 20.f * Band;   // 120 px at the edge, softer toward the middle
		const FLinearColor Dark(0.f, 0.f, 0.f, 0.35f / 6.f * In);
		Box(0, 0, SX, D, Dark);
		Box(0, SY - D, SX, D, Dark);
		Box(0, 0, D, SY, Dark);
		Box(SX - D, 0, D, SY, Dark);
	}

	Text(TEXT("DU ER D\u00D8D"), CX, SY * 0.18f, FLinearColor(1.f, 0.3f, 0.3f, FadeIn), 2.f, true);

	// Killer card
	const bool bKiller = !Info.Killer.IsEmpty();
	const float W = 460.f, H = bKiller ? 118.f : 80.f, Pad = 18.f;
	const float X = CX - W * 0.5f, Y = SY * 0.68f;
	Box(X, Y, W, H, FLinearColor(0.05f, 0.05f, 0.06f, 0.78f * FadeIn));
	Frame(X, Y, W, H, FLinearColor(1.f, 1.f, 1.f, 0.25f * FadeIn), 2.f);
	const FLinearColor White(1.f, 1.f, 1.f, FadeIn), Grey(0.8f, 0.8f, 0.82f, FadeIn), Gold(1.f, 0.82f, 0.2f, FadeIn);
	if (bKiller)
	{
		Text(FString::Printf(TEXT("DREPT AV %s"), *Info.Killer.Left(24)), X + Pad, Y + 12.f, White, 1.4f);
		Text(FString::Printf(TEXT("%s  \u00B7  %d m"), *Info.Weapon, FMath::RoundToInt(Info.DistanceM)), X + Pad, Y + 50.f, Grey, 1.f);
		if (Info.bHeadshot) Text(TEXT("HODESKUDD"), X + W - Pad, Y + 50.f, Gold, 1.f, false, true);

		// How hurt the killer was: health (red -> green) and armor (blue, vest + helmet, max 200)
		const float RowY = Y + 84.f, BarH = 8.f, Small = 0.85f;
		const float HealthFrac = FMath::Clamp(Info.KillerHealth / FMath::Max(Info.KillerMaxHealth, 1.f), 0.f, 1.f);
		const FLinearColor HealthCol(FMath::Lerp(1.f, 0.25f, HealthFrac), FMath::Lerp(0.2f, 0.85f, HealthFrac), 0.2f, FadeIn);
		const FLinearColor ArmorCol(0.35f, 0.6f, 1.f, FadeIn);
		float LX = X + Pad;
		Text(TEXT("Liv"), LX, RowY, Grey, Small);
		LX += TextWidth(TEXT("Liv"), Small) + 10.f;
		Box(LX, RowY + 6.f, 120.f, BarH, FLinearColor(0.15f, 0.15f, 0.15f, 0.8f * FadeIn));
		Box(LX, RowY + 6.f, 120.f * HealthFrac, BarH, HealthCol);
		Text(FString::FromInt(FMath::CeilToInt(Info.KillerHealth)), LX + 128.f, RowY, White, Small);
		const FString ArmorLabel = TEXT("Rustning");
		float RX = X + W * 0.5f + 6.f;
		Text(ArmorLabel, RX, RowY, Grey, Small);
		RX += TextWidth(ArmorLabel, Small) + 10.f;
		const float ArmorBarW = X + W - Pad - 38.f - RX;
		Box(RX, RowY + 6.f, ArmorBarW, BarH, FLinearColor(0.15f, 0.15f, 0.15f, 0.8f * FadeIn));
		Box(RX, RowY + 6.f, ArmorBarW * FMath::Clamp(Info.KillerArmor / 200.f, 0.f, 1.f), BarH, ArmorCol);
		Text(FString::FromInt(FMath::CeilToInt(Info.KillerArmor)), X + W - Pad, RowY, White, Small, false, true);
	}
	else
	{
		Text(TEXT("DU D\u00D8DE"), X + Pad, Y + 12.f, White, 1.4f);
		Text(FString::Printf(TEXT("\u00C5rsak: %s"), *Info.Weapon), X + Pad, Y + 48.f, Grey, 1.f);
	}

	// Countdown to respawn with a thin bar
	const float Delay = AGSGameMode::RespawnDelay;
	const float Left = Delay - Since;
	const float CY = Y + H + 14.f;
	Text(Left > 0.f ? FString::Printf(TEXT("Tilbake om %d s"), FMath::CeilToInt(Left)) : FString(TEXT("Kommer tilbake ...")),
		CX, CY, White, 1.f, true);
	Box(X, CY + 28.f, W, 4.f, FLinearColor(1.f, 1.f, 1.f, 0.15f * FadeIn));
	Box(X, CY + 28.f, W * FMath::Clamp(Since / Delay, 0.f, 1.f), 4.f, FLinearColor(0.95f, 0.75f, 0.2f, 0.9f * FadeIn));
	Text(TEXT("Tingene dine ligger der du d\u00F8de."), CX, CY + 42.f, FLinearColor(0.7f, 0.7f, 0.7f, 0.8f * FadeIn), 0.85f, true);
}

void AGSHUD::DrawHelp()
{
	const float Since = GetWorld()->GetTimeSeconds() - StartTime;
	if (Since > 30.f) return;
	const float A = FMath::Clamp(30.f - Since, 0.f, 1.f);
	const float CX = Canvas->SizeX * 0.5f, Y = Canvas->SizeY * 0.5f + 150.f;
	Text(TEXT("WASD g\u00E5 \u00B7 Shift l\u00F8p \u00B7 Space hopp/klatre \u00B7 C huk (l\u00F8p+C = skli) \u00B7 Mus1 skyt \u00B7 Mus2 sikt \u00B7 R lad \u00B7 V slag"),
		CX, Y, FLinearColor(1.f, 1.f, 1.f, A), 0.9f, true);
	Text(TEXT("1-4 v\u00E5pen \u00B7 E plukk opp \u00B7 F bruk \u00B7 Q neste ting \u00B7 G slipp \u00B7 I inventar \u00B7 T se p\u00E5 v\u00E5penet \u00B7 Tab poeng \u00B7 Esc/F10 meny"),
		CX, Y + 22.f, FLinearColor(1.f, 1.f, 1.f, A), 0.9f, true);
	Text(TEXT("Alt du finner har en farge: gr\u00E5 vanlig, gr\u00F8nn uvanlig, bl\u00E5 sjelden, lilla episk, gull legendarisk, r\u00F8d mytisk."),
		CX, Y + 44.f, FLinearColor(0.8f, 0.9f, 1.f, A), 0.85f, true);
	Text(TEXT("Spis og drikk, ellers mister du helse. L\u00E5ste kasser trenger adgangskort eller dirkesett. Mytisk kasse i Lysg\u00E5rden!"),
		CX, Y + 66.f, FLinearColor(0.8f, 0.9f, 1.f, A), 0.85f, true);
}

// ---------------------------------------------------------------- PCs and 3D printers

void AGSHUD::DrawTerminal(AGSCharacter* C, AGSPlayerController* PC)
{
	AGSTerminal* T = PC->GetActiveTerminal();
	if (!T) return;
	if (T->Kind == EGSTerminalKind::Printer) DrawPrinter(C, PC, T);
	else DrawComputer(C, PC, T);
}

void AGSHUD::DrawPrinter(AGSCharacter* C, AGSPlayerController* PC, AGSTerminal* T)
{
	const float W = 560.f, X = Canvas->SizeX * 0.5f - W * 0.5f;
	const TArray<FGSPrintRecipe>& R = AGSTerminal::Recipes();
	const float H = 150.f + R.Num() * 26.f;
	const float Y = Canvas->SizeY * 0.5f - H * 0.5f;
	const FLinearColor Panel(0.03f, 0.04f, 0.05f, 0.88f), Accent(1.f, 0.55f, 0.15f);
	Box(X, Y, W, H, Panel);
	Frame(X, Y, W, H, Accent);
	Text(TEXT("3D-PRINTER"), X + 18.f, Y + 12.f, Accent, 1.3f);
	if (!T->Label.IsEmpty()) Text(T->Label, X + W - 18.f, Y + 18.f, FLinearColor(0.7f, 0.7f, 0.7f), 0.8f, false, true);
	static const FName Filament(TEXT("Filament"));
	const int32 Have = C->CountItem(Filament);
	Text(FString::Printf(TEXT("Filament i sekken: %d spoler"), Have), X + 18.f, Y + 46.f, FLinearColor(0.9f, 0.9f, 0.9f), 0.9f);

	float RY = Y + 80.f;
	for (int32 I = 0; I < R.Num(); ++I)
	{
		const FGSItemDef* D = GSItems::Find(R[I].ItemId);
		const bool bSel = I == PC->RecipeIndex;
		const bool bAfford = Have >= R[I].Filament;
		if (bSel) Box(X + 10.f, RY - 2.f, W - 20.f, 24.f, FLinearColor(1.f, 0.55f, 0.15f, 0.25f));
		const FLinearColor Col = D ? GSItems::RarityColor(D->Rarity) : FLinearColor::White;
		Text(FString::Printf(TEXT("%s %s"), bSel ? TEXT(">") : TEXT(" "), D ? *D->Name : *R[I].ItemId.ToString()), X + 20.f, RY, bAfford ? Col : Col * 0.5f, 0.9f);
		Text(FString::Printf(TEXT("%d spoler  \u00B7  %.0f sek"), R[I].Filament, R[I].Seconds), X + W - 20.f, RY, bAfford ? FLinearColor(0.9f, 0.9f, 0.9f) : FLinearColor(1.f, 0.4f, 0.35f), 0.85f, false, true);
		RY += 26.f;
	}
	if (T->IsBusy())
	{
		const float Now = C->GetServerTime();
		const float F = FMath::Clamp((Now - T->PrintStart) / FMath::Max(T->PrintEnd - T->PrintStart, 0.1f), 0.f, 1.f);
		Text(FString::Printf(TEXT("Printer: %s  %d %%"), *GSItems::Name(T->PrintingId), FMath::RoundToInt(F * 100.f)), X + 18.f, RY + 8.f, Accent, 0.9f);
		Bar(X + 18.f, RY + 32.f, W - 36.f, 10.f, F, Accent);
	}
	else
	{
		Text(TEXT("Printeren er ledig"), X + 18.f, RY + 8.f, FLinearColor(0.6f, 0.9f, 0.6f), 0.9f);
	}
	Text(TEXT("[Q] / [1] [2] velg     [F] eller venstre mus: print     [E] g\u00E5 vekk"), X + W * 0.5f, Y + H - 26.f, FLinearColor(0.8f, 0.8f, 0.8f), 0.8f, true);
}

void AGSHUD::DrawComputer(AGSCharacter* C, AGSPlayerController* PC, AGSTerminal* T)
{
	const float Now = GetWorld()->GetTimeSeconds();
	const float SX = Canvas->SizeX, SY = Canvas->SizeY;
	const FLinearColor Blue(0.35f, 0.65f, 1.f), Dim(0.75f, 0.8f, 0.85f);

	if (PC->IsLoggingIn())
	{
		Box(0, 0, SX, SY, FLinearColor(0.02f, 0.05f, 0.12f, 0.95f));
		Text(TEXT("NTNU  \u00B7  Sikkerhetssentralen"), SX * 0.5f, SY * 0.4f, Blue, 1.6f, true);
		Text(TEXT("Logger inn ..."), SX * 0.5f, SY * 0.4f + 50.f, Dim, 1.f, true);
		Bar(SX * 0.5f - 200.f, SY * 0.4f + 90.f, 400.f, 8.f, FMath::Clamp((Now - PC->TerminalOpenTime) / 1.4f, 0.f, 1.f), Blue);
		return;
	}

	if (PC->TerminalMode == 1)
	{
		// Looking through a security camera: grainy monitor look
		const TArray<AGSSecurityCamera*> Cams = PC->GetCameras();
		Box(0, 0, SX, SY, FLinearColor(0.05f, 0.12f, 0.08f, 0.18f));
		for (float LY = 0.f; LY < SY; LY += 5.f) Box(0, LY, SX, 1.f, FLinearColor(0.f, 0.f, 0.f, 0.22f));
		const float Band = FMath::Fmod(Now * 140.f, SY + 80.f) - 40.f;
		Box(0, Band, SX, 30.f, FLinearColor(1.f, 1.f, 1.f, 0.04f));
		const float V = 120.f;
		Box(0, 0, SX, V * 0.5f, FLinearColor(0.f, 0.f, 0.f, 0.35f));
		Box(0, SY - V * 0.5f, SX, V * 0.5f, FLinearColor(0.f, 0.f, 0.f, 0.35f));
		const AGSSecurityCamera* Cam = Cams.IsValidIndex(PC->CameraIndex) ? Cams[PC->CameraIndex] : nullptr;
		Text(FString::Printf(TEXT("CAM %02d / %02d   %s"), PC->CameraIndex + 1, Cams.Num(), Cam ? *Cam->Label : TEXT("")), 40.f, 22.f, FLinearColor::White, 1.2f);
		if (FMath::Fmod(Now, 1.2f) < 0.6f) Text(TEXT("\u25CF REC"), SX - 40.f, 22.f, FLinearColor(1.f, 0.15f, 0.1f), 1.2f, false, true);
		if (const AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>())
		{
			const float Hours = GS->GetTimeOfDayHours();
			const int32 HH = FMath::FloorToInt(Hours), MM = FMath::FloorToInt((Hours - HH) * 60.f);
			Text(FString::Printf(TEXT("%02d:%02d:%02d"), HH, MM, FMath::FloorToInt(FMath::Fmod(Now, 60.f))), SX - 40.f, 52.f, FLinearColor::White, 0.9f, false, true);
		}
		// Corner brackets
		const float L = 40.f, M = 30.f;
		for (int32 I = 0; I < 4; ++I)
		{
			const float CX = (I & 1) ? SX - M : M, CY = (I & 2) ? SY - M : M;
			const float DX = (I & 1) ? -L : 0.f, DY = (I & 2) ? -L : 0.f;
			Box(CX + DX, CY + ((I & 2) ? -2.f : 0.f), L, 2.f, FLinearColor(1.f, 1.f, 1.f, 0.7f));
			Box(CX + ((I & 1) ? -2.f : 0.f), CY + DY, 2.f, L, FLinearColor(1.f, 1.f, 1.f, 0.7f));
		}
		Text(TEXT("[Q] neste kamera    [F] forrige    [2] spor spillere    [3] meny    [E] logg ut"), SX * 0.5f, SY - 40.f, FLinearColor::White, 0.85f, true);
		return;
	}

	// Menu and tracker: a desktop window
	const float W = FMath::Min(SX - 80.f, 900.f), H = FMath::Min(SY - 80.f, 620.f);
	const float X = SX * 0.5f - W * 0.5f, Y = SY * 0.5f - H * 0.5f;
	Box(X, Y, W, H, FLinearColor(0.02f, 0.04f, 0.09f, 0.94f));
	Box(X, Y, W, 34.f, FLinearColor(0.08f, 0.2f, 0.45f, 1.f));
	Text(TEXT("NTNU Sikkerhetssentral"), X + 14.f, Y + 7.f, FLinearColor::White, 0.95f);
	if (!T->Label.IsEmpty()) Text(T->Label, X + W - 14.f, Y + 7.f, Dim, 0.85f, false, true);
	Frame(X, Y, W, H, Blue, 1.f);

	if (PC->TerminalMode == 2)
	{
		DrawTrackerMap(PC, X + 14.f, Y + 48.f, W - 28.f, H - 100.f);
		const float Left = PC->TrackUntil - Now;
		const FString Status = PC->Tracked.Num() == 0 ? FString(TEXT("Henter posisjoner ..."))
			: Left > 0.f ? FString::Printf(TEXT("Sporer %d spillere  \u00B7  %d sek igjen  \u00B7  oppdatert for %.0f sek siden"), PC->Tracked.Num(), FMath::CeilToInt(Left), Now - PC->TrackedTime)
			: FString(TEXT("Sporingen er ferdig. Siste kjente posisjoner vises."));
		Text(Status, X + 14.f, Y + H - 44.f, Dim, 0.85f);
		Text(TEXT("[1] kameraer    [2] spor p\u00E5 nytt    [3] meny    [E] logg ut"), X + W - 14.f, Y + H - 44.f, Dim, 0.85f, false, true);
		return;
	}

	const int32 NumCams = PC->GetCameras().Num();
	float RY = Y + 70.f;
	Text(TEXT("Velkommen. Hva vil du gj\u00F8re?"), X + 30.f, RY, FLinearColor::White, 1.1f); RY += 50.f;
	Text(FString::Printf(TEXT("[1]  Overv\u00E5kningskameraer  (%d kameraer)"), NumCams), X + 40.f, RY, Blue, 1.f); RY += 34.f;
	Text(TEXT("[2]  Spor alle spillere p\u00E5 campus i 20 sekunder"), X + 40.f, RY, Blue, 1.f); RY += 34.f;
	Text(TEXT("[E]  Logg ut"), X + 40.f, RY, Blue, 1.f); RY += 60.f;
	Text(TEXT("Pass p\u00E5: mens du sitter ved PC-en ser du ikke hva som skjer bak deg."), X + 30.f, RY, FLinearColor(1.f, 0.75f, 0.4f), 0.85f);
}

void AGSHUD::DrawTrackerMap(AGSPlayerController* PC, float X, float Y, float W, float H)
{
	// Campus in meters: X east (-280..430), Y south (-110..520). Keep the aspect ratio.
	const float MinX = -290.f, MaxX = 440.f, MinY = -120.f, MaxY = 530.f;
	const float S = FMath::Min(W / (MaxX - MinX), H / (MaxY - MinY));
	const float OX = X + (W - (MaxX - MinX) * S) * 0.5f, OY = Y + (H - (MaxY - MinY) * S) * 0.5f;
	auto ToScreen = [&](float MX, float MY) { return FVector2D(OX + (MX - MinX) * S, OY + (MY - MinY) * S); };
	Box(OX, OY, (MaxX - MinX) * S, (MaxY - MinY) * S, FLinearColor(0.05f, 0.1f, 0.08f, 1.f));
	for (const GSCampus::FRoad& R : GSCampus::Roads())
	{
		const FVector2D A = ToScreen(R.A.X, R.A.Y), B = ToScreen(R.B.X, R.B.Y);
		Canvas->K2_DrawLine(A, B, FMath::Max(1.f, R.Width * S), FLinearColor(0.25f, 0.27f, 0.3f));
	}
	for (const GSCampus::FBuilding& B : GSCampus::Buildings())
	{
		const FVector2D P = ToScreen(B.X - B.SizeX * 0.5f, B.Y - B.SizeY * 0.5f);
		Box(P.X, P.Y, B.SizeX * S, B.SizeY * S, FLinearColor(0.35f, 0.4f, 0.48f, 0.9f));
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Now * 6.f);
	for (const FGSTrackedPlayer& P : PC->Tracked)
	{
		const FVector2D D = ToScreen(P.Location.X / 100.f, P.Location.Y / 100.f);
		const FLinearColor Col = P.bSelf ? FLinearColor(0.3f, 0.7f, 1.f) : FLinearColor(1.f, 0.2f, 0.15f);
		const float R = 5.f + (P.bSelf ? 0.f : 3.f * Pulse);
		Box(D.X - R, D.Y - R, R * 2.f, R * 2.f, Col);
		Text(P.bSelf ? FString(TEXT("Deg")) : P.Name, D.X + 10.f, D.Y - 8.f, Col, 0.7f);
	}
}
