#include "GSInteract.h"
#include "GSMapState.h"
#include "GSCharacter.h"
#include "GSPlayerController.h"
#include "GSPickup.h"
#include "GSTerminal.h"
#include "GSMenu.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "GameFramework/HUD.h"
#include "GameFramework/GameStateBase.h"

namespace GSInteractImpl
{
	constexpr float HoldSeconds = 0.3f;
	constexpr float FocusRange = 220.f;      // cm from the eye to the use point
	constexpr float ServerRange = 250.f;
	constexpr float UseCooldown = 0.15f;     // server, per player (a tap needs a release and a new press)

	const FName InteractName(TEXT("IA_Interact"));

	FString KeyText(const APlayerController* PC)
	{
		FString K = GSMenu::KeyName(PC, InteractName);
		return K.IsEmpty() ? FString(TEXT("E")) : K;
	}

	void DrawCentered(UCanvas* Canvas, const FString& Text, float CX, float Y, const FLinearColor& Color, float Scale)
	{
		if (Text.IsEmpty()) return;
		FCanvasTextItem Item(FVector2D(CX, Y), FText::FromString(Text), GEngine->GetMediumFont(), Color);
		Item.Scale = FVector2D(Scale, Scale);
		Item.bCentreX = true;
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f), FVector2D(1.f, 1.f));
		Canvas->DrawItem(Item);
	}

	void Rect(UCanvas* Canvas, float X, float Y, float W, float H, const FLinearColor& Color)
	{
		FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(W, H), Color);
		Tile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Tile);
	}

	void OnPostRender(AHUD* HUD, UCanvas* Canvas)
	{
		if (!HUD || !Canvas || !HUD->PlayerOwner) return;
		for (TActorIterator<AGSInteractor> It(HUD->GetWorld()); It; ++It)
		{
			if (It->GetOwner() == HUD->PlayerOwner)
			{
				It->DrawHUD(HUD, Canvas);
				return;
			}
		}
	}

	IGSInteract* FindUsable(const UObject* Ctx, int32 Key, AActor** OutActor = nullptr)
	{
		const AGSMapState* MS = AGSMapState::Get(Ctx);
		AActor* A = MS ? MS->FindUsable(Key) : nullptr;
		if (OutActor) *OutActor = A;
		return Cast<IGSInteract>(A);
	}
}


// ---------------------------------------------------------------------------------------------------------------------

AGSInteractor::AGSInteractor()
{
	bReplicates = true;
	bOnlyRelevantToOwner = true;
	bAlwaysRelevant = false;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(10.f);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetCanBeDamaged(false);
}

void AGSInteractor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGSInteractor, Job, COND_OwnerOnly);
}

AGSPlayerController* AGSInteractor::GetPC() const
{
	return Cast<AGSPlayerController>(GetOwner());
}

AGSCharacter* AGSInteractor::GetCharacter() const
{
	const AGSPlayerController* PC = GetPC();
	return PC ? Cast<AGSCharacter>(PC->GetPawn()) : nullptr;
}

void AGSInteractor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (AGSPlayerController* PC = BoundPC.Get())
	{
		if (InputComp) PC->PopInputComponent(InputComp);
	}
	BoundPC.Reset();
	Super::EndPlay(Reason);
}

void AGSInteractor::TryBindInput()
{
	AGSPlayerController* PC = GetPC();
	if (!PC) return;
	UInputAction* Action = PC->InteractAction;
	if (!Action) Action = FindObject<UInputAction>(PC, TEXT("IA_Interact"));
	if (!Action)
	{
		NextBindTry = GetWorld()->GetTimeSeconds() + 0.5f;
		return;
	}
	InputComp = NewObject<UEnhancedInputComponent>(this, TEXT("GSInteractInput"));
	InputComp->RegisterComponent();
	InputComp->BindAction(Action, ETriggerEvent::Started, this, &AGSInteractor::OnInteractStarted);
	PC->PushInputComponent(InputComp);
	BoundPC = PC;
	UE_LOG(LogTemp, Log, TEXT("Interaktiv: bruk-tasten (%s) bundet for %s"), *GSInteractImpl::KeyText(PC), *PC->GetName());
}

bool AGSInteractor::IsBlocked() const
{
	const AGSPlayerController* PC = GetPC();
	const AGSCharacter* C = GetCharacter();
	return !PC || PC->IsGameInputBlocked() || PC->bShowInventory || PC->IsOnTerminal()
		|| !C || C->bDead || !C->UseItemId.IsNone();
}

float AGSInteractor::KampFocusScore(const AGSCharacter* C, const FVector& Eye, const FVector& Fwd) const
{
	// Kamp's own formula (AGSCharacter::UpdateFocusPickup), so we only win where Kamp would lose
	float Best = -BIG_NUMBER;
	for (TActorIterator<AGSTerminal> It(GetWorld()); It; ++It)
	{
		const FVector To = It->GetActorLocation() + FVector(0.f, 0.f, 40.f) - Eye;
		const float Dist = To.Size();
		if (Dist > 240.f) continue;
		const float Dot = FVector::DotProduct(To / FMath::Max(Dist, 1.f), Fwd);
		if (Dot > 0.8f) Best = FMath::Max(Best, Dot - Dist / 1000.f);
	}
	for (TActorIterator<AGSPickup> It(GetWorld()); It; ++It)
	{
		if (!It->bAvailable) continue;
		const FVector To = It->GetActorLocation() - Eye;
		const float Dist = To.Size();
		if (Dist > 280.f) continue;
		const float Dot = FVector::DotProduct(To / FMath::Max(Dist, 1.f), Fwd);
		if (Dot > 0.75f || Dist < 110.f) Best = FMath::Max(Best, Dot - Dist / 1000.f);
	}
	return Best;
}

void AGSInteractor::UpdateFocus()
{
	FocusKey = 0;
	AGSPlayerController* PC = GetPC();
	AGSCharacter* C = GetCharacter();
	if (!PC || !C || IsBlocked()) return;
	const AGSMapState* MS = AGSMapState::Get(this);
	if (!MS) return;
	FVector Eye;
	FRotator Rot;
	PC->GetPlayerViewPoint(Eye, Rot);
	const FVector Fwd = Rot.Vector();
	float Best = -BIG_NUMBER;
	AActor* BestA = nullptr;
	for (const TWeakObjectPtr<AActor>& W : MS->GetUsables())
	{
		AActor* A = W.Get();
		const IGSInteract* I = Cast<IGSInteract>(A);
		if (!I) continue;
		if (FVector::DistSquared(A->GetActorLocation(), Eye) > FMath::Square(450.f)) continue;
		const FVector P = I->GetUsePoint(Eye);
		const FVector To = P - Eye;
		const float Dist = To.Size();
		if (Dist > GSInteractImpl::FocusRange) continue;
		const float Dot = FVector::DotProduct(To / FMath::Max(Dist, 1.f), Fwd);
		if (!(Dot > 0.8f || (Dist < 90.f && Dot > 0.5f))) continue;
		const float Score = Dot - Dist / 1000.f;
		if (Score <= Best) continue;
		if (!GSInteract::HasClearLine(GetWorld(), Eye, P, A, C)) continue;
		Best = Score;
		BestA = A;
	}
	if (!BestA) return;
	// Kamp's pickups and PCs: without the hook (K4) we always step aside; with it, the one nearest the middle of the view wins.
	if (!bCharacterHooked)
	{
		if (C->GetFocusPickup() || C->GetFocusTerminal()) return;
	}
	else if (KampFocusScore(C, Eye, Fwd) >= Best)
	{
		return;
	}
	FGSUsePrompt Prompt;
	const IGSInteract* I = Cast<IGSInteract>(BestA);
	if (!I->GetUsePrompt(C, Prompt)) return;
	const FString K = GSInteractImpl::KeyText(PC);
	Prompt.Main.ReplaceInline(TEXT("%K%"), *K);
	Prompt.Sub.ReplaceInline(TEXT("%K%"), *K);
	FocusPrompt = Prompt;
	FocusKey = I->GetInteractKey();
}

void AGSInteractor::OnInteractStarted()
{
	if (bPressActive) return;
	const AGSCharacter* C = GetCharacter();
	UpdateFocus();   // what you look at right now, not up to 50 ms ago
	const bool bKamp = C && (C->GetFocusPickup() || C->GetFocusTerminal());
	bPressActive = true;
	bHoldSent = false;
	PressTime = GetWorld()->GetTimeSeconds();
	PressKey = FocusKey;
	// Decided now, for the whole press: if E is not ours when it goes down, the release does nothing either.
	bPressMine = FocusKey != 0 && !IsBlocked() && (bCharacterHooked || !bKamp);
}

void AGSInteractor::SendVerb(EGSUseVerb Verb)
{
	if (PressKey == 0) return;
	ServerUse(PressKey, static_cast<uint8>(Verb));
	if (Verb == EGSUseVerb::Tap && !Job.IsActive())
	{
		if (IGSInteract* I = GSInteractImpl::FindUsable(this, PressKey))
		{
			if (AGSCharacter* C = GetCharacter()) I->PredictUse(C);
		}
	}
}

void AGSInteractor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* W = GetWorld();
	const float Now = W->GetTimeSeconds();
	AGSPlayerController* PC = GetPC();

	// --- The player's own machine: focus, key, hint ---
	if (PC && PC->IsLocalController())
	{
		GSInteract::EnsureHooked();
		if (!InputComp && Now >= NextBindTry) TryBindInput();
		if (Now >= NextFocus)
		{
			NextFocus = Now + 0.05f;
			UpdateFocus();
		}
		if (bPressActive)
		{
			const bool bDown = GSMenu::IsActionDown(PC, GSInteractImpl::InteractName);
			if (bPressMine && IsBlocked())
			{
				// A menu opened (or the body died) in the middle of a hold: let go of it
				if (bHoldSent) SendVerb(EGSUseVerb::HoldEnd);
				bPressMine = false;
			}
			if (!bDown)
			{
				if (bPressMine) SendVerb(bHoldSent ? EGSUseVerb::HoldEnd : EGSUseVerb::Tap);
				bPressActive = false;
				bPressMine = false;
			}
			else if (bPressMine && !bHoldSent && Now - PressTime >= GSInteractImpl::HoldSeconds)
			{
				bHoldSent = true;
				SendVerb(EGSUseVerb::HoldStart);
			}
		}
	}

	// --- Server: the job runs on the server's clock ---
	if (HasAuthority() && Job.IsActive())
	{
		AGSCharacter* C = JobUser.Get();
		AActor* A = nullptr;
		IGSInteract* I = GSInteractImpl::FindUsable(this, Job.Key, &A);
		bool bOk = C && I && C->GetController() == GetOwner() && !C->bDead && C->UseItemId.IsNone();
		if (bOk) bOk = FVector::Dist(C->GetPawnViewLocation(), I->GetUsePoint(C->GetPawnViewLocation())) <= GSInteractImpl::ServerRange;
		if (bOk) bOk = I->ServerJobTick(C, Job, Now);
		if (!bOk) EndJob(false);
		else if (Now >= Job.End) EndJob(true);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Server

void AGSInteractor::ServerUse_Implementation(int32 Key, uint8 Verb)
{
	AGSCharacter* C = GetCharacter();
	if (!C) return;
	const EGSUseVerb V = static_cast<EGSUseVerb>(FMath::Min<uint8>(Verb, static_cast<uint8>(EGSUseVerb::HoldEnd)));
	if (V == EGSUseVerb::HoldEnd)
	{
		if (Job.IsActive() && Job.Key == Key) EndJob(false);
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastUseTime < GSInteractImpl::UseCooldown) return;
	LastUseTime = Now;
	HandleUse(C, Key, V);
}

void AGSInteractor::HandleUse(AGSCharacter* C, int32 Key, EGSUseVerb Verb)
{
	AActor* A = nullptr;
	IGSInteract* I = GSInteractImpl::FindUsable(this, Key, &A);
	if (!I || !GSInteract::CanUse(C, A, GSInteractImpl::ServerRange)) return;
	if (Job.IsActive()) EndJob(false);
	FGSUseResult R;
	I->ServerUse(C, Verb, R);
	if (R.Job.IsActive())
	{
		Job = R.Job;
		JobUser = C;
		I->ServerJobStarted(C, Job);
		ForceNetUpdate();
	}
	if (!R.Feedback.IsEmpty() || !R.FeedbackSound.IsEmpty())
	{
		ClientFeedback(R.Feedback, R.FeedbackSound, I->GetUsePoint(C->GetPawnViewLocation()));
	}
}

void AGSInteractor::EndJob(bool bCompleted)
{
	if (!Job.IsActive()) return;
	const FGSUseJob Old = Job;
	Job = FGSUseJob();
	ForceNetUpdate();
	AGSCharacter* C = JobUser.Get();
	IGSInteract* I = GSInteractImpl::FindUsable(this, Old.Key);
	FGSUseJob Next;
	if (I && I->ServerJobDone(C, Old, bCompleted, Next) && bCompleted && C && Next.IsActive())
	{
		Job = Next;
	}
}

void AGSInteractor::ClientFeedback_Implementation(const FString& Text, const FString& Sound, FVector_NetQuantize Where)
{
	FeedbackText = Text;
	FeedbackTime = GetWorld()->GetTimeSeconds();
	if (!Sound.IsEmpty()) AGSMapState::PlayPartSound(this, *Sound, Where, 0.7f, 10.f);
}

// ---------------------------------------------------------------------------------------------------------------------
// Hint and progress bar, in the middle under the crosshair (where Kamp's DrawPrompt stands; never both at once)

void AGSInteractor::DrawHUD(AHUD* HUD, UCanvas* Canvas)
{
	const AGSPlayerController* PC = GetPC();
	if (!PC || HUD->PlayerOwner != PC || !PC->IsLocalController()) return;
	if (IsBlocked()) return;   // menus, inventory, PC screen, using an item (Kamp's bar is in the middle then)
	const float Now = GetWorld()->GetTimeSeconds();
	const float CX = Canvas->SizeX * 0.5f;
	const float Y = Canvas->SizeY * 0.5f + 60.f;
	if (Now - FeedbackTime < 2.f && !FeedbackText.IsEmpty())
	{
		const float A = FMath::Clamp((2.f - (Now - FeedbackTime)) * 2.f, 0.f, 1.f);
		GSInteractImpl::DrawCentered(Canvas, FeedbackText, CX, Y, FLinearColor(1.f, 0.85f, 0.4f, A), 1.1f);
	}
	else if (FocusKey != 0 && !Job.IsActive())
	{
		GSInteractImpl::DrawCentered(Canvas, FocusPrompt.Main, CX, Y, FocusPrompt.Color, 1.1f);
		GSInteractImpl::DrawCentered(Canvas, FocusPrompt.Sub, CX, Y + 24.f, FLinearColor(0.75f, 0.75f, 0.75f), 0.8f);
	}
	if (Job.IsActive())
	{
		const AGameStateBase* GS = GetWorld()->GetGameState();
		const float ServerNow = GS ? GS->GetServerWorldTimeSeconds() : Now;
		const float Frac = FMath::Clamp((ServerNow - Job.Start) / FMath::Max(0.01f, Job.End - Job.Start), 0.f, 1.f);
		const float BarW = 240.f, BarY = Canvas->SizeY * 0.5f + 110.f, X = CX - BarW * 0.5f;
		FString Label;
		if (IGSInteract* I = GSInteractImpl::FindUsable(this, Job.Key)) Label = I->JobLabel(Job.Kind);
		GSInteractImpl::DrawCentered(Canvas, Label, CX, BarY - 22.f, FLinearColor(0.95f, 0.95f, 0.95f), 0.85f);
		GSInteractImpl::Rect(Canvas, X - 1.f, BarY - 1.f, BarW + 2.f, 10.f, FLinearColor(0.f, 0.f, 0.f, 0.55f));
		GSInteractImpl::Rect(Canvas, X, BarY, BarW * Frac, 8.f, FLinearColor(1.f, 0.85f, 0.4f, 0.95f));
	}
}

// ---------------------------------------------------------------------------------------------------------------------

namespace GSInteract
{
	bool HasFocus(const AGSCharacter* C)
	{
		if (!C || !C->GetController()) return false;
		for (TActorIterator<AGSInteractor> It(C->GetWorld()); It; ++It)
		{
			if (It->GetOwner() == C->GetController())
			{
				It->bCharacterHooked = true;
				return It->HasFocus();
			}
		}
		return false;
	}

	bool HasClearLine(const UWorld* World, const FVector& Eye, const FVector& Point, const AActor* Part, const AActor* User)
	{
		if (!World) return false;
		FCollisionObjectQueryParams OP;
		OP.AddObjectTypesToQuery(ECC_WorldStatic);
		OP.AddObjectTypesToQuery(ECC_WorldDynamic);
		OP.AddObjectTypesToQuery(ECC_Destructible);   // glass: you cannot open a door on the other side of a glass wall
		FCollisionQueryParams QP(SCENE_QUERY_STAT(GSInteractLine), false);
		if (Part) QP.AddIgnoredActor(Part);
		if (User) QP.AddIgnoredActor(User);
		FHitResult H;
		return !World->LineTraceSingleByObjectType(H, Eye, Point, OP, QP);
	}

	bool CanUse(const AGSCharacter* C, AActor* Part, float MaxDistCm)
	{
		const IGSInteract* I = Cast<IGSInteract>(Part);
		if (!C || !I || C->bDead || !C->UseItemId.IsNone()) return false;
		const FVector Eye = C->GetPawnViewLocation();
		const FVector P = I->GetUsePoint(Eye);
		if (FVector::Dist(Eye, P) > MaxDistCm) return false;
		return HasClearLine(C->GetWorld(), Eye, P, Part, C);
	}

	bool BotUse(AGSCharacter* Bot, int32 Key, EGSUseVerb Verb)
	{
		if (!Bot || !Bot->HasAuthority() || Verb == EGSUseVerb::HoldEnd) return false;
		AActor* A = nullptr;
		IGSInteract* I = GSInteractImpl::FindUsable(Bot, Key, &A);
		if (!I || !CanUse(Bot, A, GSInteractImpl::ServerRange)) return false;
		FGSUseResult R;
		I->ServerUse(Bot, Verb, R);   // jobs (picking, kicking in) are not run for bots yet
		return true;
	}

	void EnsureHooked()
	{
		static bool bHooked = false;
		if (bHooked) return;
		bHooked = true;
		AHUD::OnHUDPostRender.AddStatic(&GSInteractImpl::OnPostRender);
	}
}
