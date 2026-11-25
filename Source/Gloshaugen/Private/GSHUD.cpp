#include "GSHUD.h"
#include "GSCharacter.h"
#include "GSGameState.h"
#include "GSItems.h"
#include "GSPickup.h"
#include "GSPlayerState.h"
#include "GSPlayerController.h"
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
		DrawCrosshair(C);
		DrawBars(C);
		DrawAmmo(C);
		DrawQuickbar(C);
		DrawEffects(C);
		DrawPrompt(C);
		DrawUseProgress(C);
		DrawToast(C);
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
	DrawKillFeed();
	DrawHelp();

	if (PC && PC->bShowScoreboard) DrawScoreboard();
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
		if (W->Attachments & GSAttach::Suppressor) Att += TEXT("Demper ");
		if (W->Attachments & GSAttach::RedDot) Att += TEXT("R\u00F8dpunkt ");
		if (W->Attachments & GSAttach::Scope) Att += TEXT("Kikkert ");
		if (W->Attachments & GSAttach::ExtMag) Att += TEXT("Stort mag ");
		if (W->Attachments & GSAttach::Laser) Att += TEXT("Laser");
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
	Box(0, 0, Canvas->SizeX, Canvas->SizeY, FLinearColor(0.15f, 0.f, 0.f, 0.35f));
	FString Killer, Weapon;
	const AGSGameState* GS = GetWorld()->GetGameState<AGSGameState>();
	const APlayerState* Me = PlayerOwner ? PlayerOwner->PlayerState : nullptr;
	if (GS && Me)
	{
		for (int32 I = GS->KillFeed.Num() - 1; I >= 0; --I)
		{
			if (GS->KillFeed[I].Victim == Me->GetPlayerName()) { Killer = GS->KillFeed[I].Killer; Weapon = GS->KillFeed[I].Weapon; break; }
		}
	}
	const float CX = Canvas->SizeX * 0.5f, CY = Canvas->SizeY * 0.4f;
	Text(TEXT("DU ER D\u00D8D"), CX, CY, FLinearColor(1.f, 0.3f, 0.3f), 2.f, true);
	if (!Killer.IsEmpty())
	{
		Text(FString::Printf(TEXT("Drept av %s (%s)"), *Killer, *Weapon), CX, CY + 50.f, FLinearColor::White, 1.2f, true);
	}
	Text(TEXT("Tingene dine ligger der du d\u00F8de. Du kommer tilbake om noen sekunder ..."), CX, CY + 85.f, FLinearColor(0.85f, 0.85f, 0.85f), 1.f, true);
}

void AGSHUD::DrawHelp()
{
	const float Since = GetWorld()->GetTimeSeconds() - StartTime;
	if (Since > 30.f) return;
	const float A = FMath::Clamp(30.f - Since, 0.f, 1.f);
	const float CX = Canvas->SizeX * 0.5f, Y = Canvas->SizeY * 0.5f + 150.f;
	Text(TEXT("WASD g\u00E5 \u00B7 Shift l\u00F8p \u00B7 Space hopp/klatre \u00B7 C huk (l\u00F8p+C = skli) \u00B7 Mus1 skyt \u00B7 Mus2 sikt \u00B7 R lad \u00B7 V slag"),
		CX, Y, FLinearColor(1.f, 1.f, 1.f, A), 0.9f, true);
	Text(TEXT("1-4 v\u00E5pen \u00B7 E plukk opp \u00B7 F bruk \u00B7 Q neste ting \u00B7 G slipp \u00B7 I inventar \u00B7 Tab poeng"),
		CX, Y + 22.f, FLinearColor(1.f, 1.f, 1.f, A), 0.9f, true);
	Text(TEXT("Alt du finner har en farge: gr\u00E5 vanlig, gr\u00F8nn uvanlig, bl\u00E5 sjelden, lilla episk, gull legendarisk, r\u00F8d mytisk."),
		CX, Y + 44.f, FLinearColor(0.8f, 0.9f, 1.f, A), 0.85f, true);
	Text(TEXT("Spis og drikk, ellers mister du helse. L\u00E5ste kasser trenger adgangskort eller dirkesett. Mytisk kasse i Lysg\u00E5rden!"),
		CX, Y + 66.f, FLinearColor(0.8f, 0.9f, 1.f, A), 0.85f, true);
}
