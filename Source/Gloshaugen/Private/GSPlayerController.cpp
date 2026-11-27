#include "GSPlayerController.h"
#include "HAL/IConsoleManager.h"
#include "GSHUD.h"
#include "GSMenu.h"
#include "GSGameMode.h"
#include "GSPlayerState.h"
#include "GSCharacter.h"
#include "GSTerminal.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "GameFramework/PlayerState.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/Engine.h"

AGSPlayerController::AGSPlayerController()
{
}

static UInputAction* MakeAction(UObject* Outer, const TCHAR* Name, EInputActionValueType Type)
{
	UInputAction* A = NewObject<UInputAction>(Outer, FName(Name));
	A->ValueType = Type;
	return A;
}

void AGSPlayerController::EnsureInputCreated()
{
	if (MappingContext)
	{
		return;
	}

	MoveAction = MakeAction(this, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(this, TEXT("IA_Look"), EInputActionValueType::Axis2D);
	JumpAction = MakeAction(this, TEXT("IA_Jump"), EInputActionValueType::Boolean);
	SprintAction = MakeAction(this, TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	CrouchAction = MakeAction(this, TEXT("IA_Crouch"), EInputActionValueType::Boolean);
	FireAction = MakeAction(this, TEXT("IA_Fire"), EInputActionValueType::Boolean);
	AimAction = MakeAction(this, TEXT("IA_Aim"), EInputActionValueType::Boolean);
	ReloadAction = MakeAction(this, TEXT("IA_Reload"), EInputActionValueType::Boolean);
	Slot1Action = MakeAction(this, TEXT("IA_Slot1"), EInputActionValueType::Boolean);
	Slot2Action = MakeAction(this, TEXT("IA_Slot2"), EInputActionValueType::Boolean);
	Slot3Action = MakeAction(this, TEXT("IA_Slot3"), EInputActionValueType::Boolean);
	Slot4Action = MakeAction(this, TEXT("IA_Slot4"), EInputActionValueType::Boolean);
	InteractAction = MakeAction(this, TEXT("IA_Interact"), EInputActionValueType::Boolean);
	UseItemAction = MakeAction(this, TEXT("IA_UseItem"), EInputActionValueType::Boolean);
	NextItemAction = MakeAction(this, TEXT("IA_NextItem"), EInputActionValueType::Boolean);
	DropItemAction = MakeAction(this, TEXT("IA_DropItem"), EInputActionValueType::Boolean);
	MeleeAction = MakeAction(this, TEXT("IA_Melee"), EInputActionValueType::Boolean);
	InventoryAction = MakeAction(this, TEXT("IA_Inventory"), EInputActionValueType::Boolean);
	ScrollAction = MakeAction(this, TEXT("IA_Scroll"), EInputActionValueType::Axis1D);
	ScoreboardAction = MakeAction(this, TEXT("IA_Scoreboard"), EInputActionValueType::Boolean);
	InspectAction = MakeAction(this, TEXT("IA_Inspect"), EInputActionValueType::Boolean);
	ReadyAction = MakeAction(this, TEXT("IA_Ready"), EInputActionValueType::Boolean);
	MenuAction = MakeAction(this, TEXT("IA_Menu"), EInputActionValueType::Boolean);

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Gloshaugen"));
	UInputMappingContext* C = MappingContext;

	// WASD -> 2D axis (X = right, Y = forward)
	{
		FEnhancedActionKeyMapping& W = C->MapKey(MoveAction, EKeys::W);
		UInputModifierSwizzleAxis* Swz = NewObject<UInputModifierSwizzleAxis>(C);
		Swz->Order = EInputAxisSwizzle::YXZ;
		W.Modifiers.Add(Swz);

		FEnhancedActionKeyMapping& S = C->MapKey(MoveAction, EKeys::S);
		UInputModifierSwizzleAxis* Swz2 = NewObject<UInputModifierSwizzleAxis>(C);
		Swz2->Order = EInputAxisSwizzle::YXZ;
		S.Modifiers.Add(Swz2);
		S.Modifiers.Add(NewObject<UInputModifierNegate>(C));

		C->MapKey(MoveAction, EKeys::D);

		FEnhancedActionKeyMapping& A = C->MapKey(MoveAction, EKeys::A);
		A.Modifiers.Add(NewObject<UInputModifierNegate>(C));
	}

	C->MapKey(LookAction, EKeys::Mouse2D);
	C->MapKey(JumpAction, EKeys::SpaceBar);
	C->MapKey(SprintAction, EKeys::LeftShift);
	C->MapKey(CrouchAction, EKeys::LeftControl);
	C->MapKey(CrouchAction, EKeys::C);
	C->MapKey(FireAction, EKeys::LeftMouseButton);
	C->MapKey(AimAction, EKeys::RightMouseButton);
	C->MapKey(ReloadAction, EKeys::R);
	C->MapKey(Slot1Action, EKeys::One);
	C->MapKey(Slot2Action, EKeys::Two);
	C->MapKey(Slot3Action, EKeys::Three);
	C->MapKey(Slot4Action, EKeys::Four);
	C->MapKey(InteractAction, EKeys::E);
	C->MapKey(UseItemAction, EKeys::F);
	C->MapKey(NextItemAction, EKeys::Q);
	C->MapKey(DropItemAction, EKeys::G);
	C->MapKey(MeleeAction, EKeys::V);
	C->MapKey(InventoryAction, EKeys::I);
	// The wheel as an axis, not as the two "buttons" MouseScrollUp/Down: those are pressed and released in the same
	// frame, which Enhanced Input reports as already held, so the Started event bound to them hardly ever came. Both
	// were also on the same action, so up and down did the same thing (and nothing at all with only one gun).
	C->MapKey(ScrollAction, EKeys::MouseWheelAxis);
	C->MapKey(ScoreboardAction, EKeys::Tab);
	C->MapKey(InspectAction, EKeys::T);
	C->MapKey(ReadyAction, EKeys::Enter);
	C->MapKey(MenuAction, EKeys::Escape);
	C->MapKey(MenuAction, EKeys::F10);
	// P as well: in the editor's Play Esc stops Play, and on many laptops F10 needs the Fn key. Kept last, so the
	// mapping indexes the menu's key rebinding has saved do not move.
	C->MapKey(MenuAction, EKeys::P);
}

void AGSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	EnsureInputCreated();

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->ClearAllMappings();
			Sub->AddMappingContext(MappingContext, 0);
		}
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(ScoreboardAction, ETriggerEvent::Started, this, &AGSPlayerController::ShowScoreboard);
		EIC->BindAction(ScoreboardAction, ETriggerEvent::Completed, this, &AGSPlayerController::HideScoreboard);
		EIC->BindAction(InventoryAction, ETriggerEvent::Started, this, &AGSPlayerController::ToggleInventory);
		EIC->BindAction(ReadyAction, ETriggerEvent::Started, this, &AGSPlayerController::ToggleReady);
		EIC->BindAction(MenuAction, ETriggerEvent::Started, this, &AGSPlayerController::ToggleMenu);
	}
}

void AGSPlayerController::ToggleMenu()
{
	if (bMenuOpen) { CloseMenu(); return; }
	if (IsOnTerminal()) { CloseTerminal(); return; }
	bMenuOpen = true;
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	SetIgnoreLookInput(true);
	SetIgnoreMoveInput(true);
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	UE_LOG(LogTemp, Log, TEXT("GSInput: Kamp-menyen åpnet, all spillinput sperret"));
}

bool AGSPlayerController::IsGameInputBlocked() const
{
	return bMenuOpen || GSMenu::IsModal(this);
}

bool AGSPlayerController::IsEditorPlay() const
{
	const UWorld* W = GetWorld();
	return W && W->WorldType == EWorldType::PIE;
}

void AGSPlayerController::CloseMenu()
{
	if (bMenuOpen) UE_LOG(LogTemp, Log, TEXT("GSInput: Kamp-menyen lukket, spillinput sluppet"));
	bMenuOpen = false;
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	ResetIgnoreLookInput();
	ResetIgnoreMoveInput();
	SetInputMode(FInputModeGameOnly());
}

int32 AGSPlayerController::GetGraphicsLevel() const
{
	// The shadow level, as the main menu reads it: the overall level is -1 ("custom") as soon as the render
	// resolution is not the one the level comes with, which showed "Lav" whatever was chosen.
	const UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	return S ? FMath::Clamp(S->GetShadowQuality(), 0, 4) : 2;
}

void AGSPlayerController::ApplyGraphicsLevel(int32 Level)
{
	UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!S) return;
	// The level also sets the render resolution: put yours back afterwards.
	const int32 Res = GetResolutionPercent();
	S->SetOverallScalabilityLevel(FMath::Clamp(Level, 0, 4));
	S->SetResolutionScaleValueEx(static_cast<float>(FMath::Clamp(Res, 50, 100)));
	S->ApplySettings(false);
	S->SaveSettings();
}

void AGSPlayerController::SetFrameLimit(int32 Fps)
{
	FrameLimit = FMath::Clamp(Fps, 30, 240);
	ApplyFrameLimit(true, true);
}

int32 AGSPlayerController::GetResolutionPercent() const
{
	UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!S) return 100;
	float Norm = 1.f, Cur = 100.f, Min = 50.f, Max = 100.f;
	S->GetResolutionScaleInformationEx(Norm, Cur, Min, Max);
	return FMath::RoundToInt(Cur);
}

bool AGSPlayerController::IsRayTracingOn() const
{
	const IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing"));
	return V && V->GetInt() != 0;
}

void AGSPlayerController::MenuClick(FName Button)
{
	const FString B = Button.ToString();
	if (B == TEXT("Resume")) { CloseMenu(); }
	// "Hovedmeny" (the old Host/Join ids): hosting and joining live in the main menu. GSMenu may already have caught
	// the click itself; opening it twice is harmless.
	else if (B == TEXT("Host") || B == TEXT("Join")) { CloseMenu(); GSMenu::OpenMain(this); }
	else if (B == TEXT("Graphics")) { ApplyGraphicsLevel((GetGraphicsLevel() + 1) % 5); }
	else if (B == TEXT("Resolution"))
	{
		static const int32 Steps[] = { 50, 60, 72, 85, 100 };
		const int32 Cur = GetResolutionPercent();
		int32 Next = Steps[0];
		for (int32 I = 0; I < 5; ++I) { if (Steps[I] > Cur) { Next = Steps[I]; break; } }
		Opplosning(Next);
	}
	else if (B == TEXT("RayTracing")) { Straaling(IsRayTracingOn() ? 0 : 1); }
	else if (B == TEXT("SensDown") || B == TEXT("SensUp"))
	{
		MouseSensitivity = FMath::Clamp(MouseSensitivity + (B == TEXT("SensUp") ? 0.1f : -0.1f), 0.2f, 3.f);
		GConfig->SetFloat(TEXT("Gloshaugen"), TEXT("MouseSensitivity"), MouseSensitivity, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
	else if (B == TEXT("Quit")) { CloseMenu(); GSMenu::AskQuit(this); }   // asks first ("Avslutte spillet?")
}

void AGSPlayerController::ToggleReady()
{
	const AGSPlayerState* PS = GetPlayerState<AGSPlayerState>();
	ServerSetReady(!(PS && PS->bReady));
}

void AGSPlayerController::ServerSetReady_Implementation(bool bReady)
{
	if (AGSGameMode* GM = GetWorld()->GetAuthGameMode<AGSGameMode>()) GM->SetReady(this, bReady);
}

void AGSPlayerController::Host()
{
	if (UWorld* World = GetWorld())
	{
		const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
		UGameplayStatics::OpenLevel(this, FName(*Map), true, TEXT("listen"));
	}
}

void AGSPlayerController::Join(const FString& Address)
{
	LastJoinAddress = Address;
	GConfig->SetString(TEXT("Gloshaugen"), TEXT("LastJoinAddress"), *Address, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
	ClientTravel(Address, ETravelType::TRAVEL_Absolute);
}

void AGSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return;
	GConfig->GetFloat(TEXT("Gloshaugen"), TEXT("MouseSensitivity"), MouseSensitivity, GGameUserSettingsIni);
	MouseSensitivity = FMath::Clamp(MouseSensitivity, 0.2f, 3.f);
	GConfig->GetString(TEXT("Gloshaugen"), TEXT("LastJoinAddress"), LastJoinAddress, GGameUserSettingsIni);
	// Frame rate cap (default 60): without one a laptop renders as fast as it can, runs hot and throttles.
	GConfig->GetInt(TEXT("Gloshaugen"), TEXT("FrameLimit"), FrameLimit, GGameUserSettingsIni);
	FrameLimit = FMath::Clamp(FrameLimit, 30, 240);
	ApplyFrameLimit(false, false);
	if (GIsEditor) return;

	// First time the game runs: settings that look good and stay smooth on a laptop RTX 3050 (6 GB).
	UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!S) return;
	bool bDone = false;
	GConfig->GetBool(TEXT("Gloshaugen"), TEXT("GraphicsDefaultsApplied"), bDone, GGameUserSettingsIni);
	if (bDone) return;
	S->SetOverallScalabilityLevel(2);          // High
	S->SetResolutionScaleValueEx(72.f);       // percent; TSR upscales to full resolution
	S->SetFrameRateLimit(static_cast<float>(FrameLimit));
	S->ApplySettings(false);
	S->SaveSettings();
	GConfig->SetBool(TEXT("Gloshaugen"), TEXT("GraphicsDefaultsApplied"), true, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

// ---------------------------------------------------------------- Death cam

void AGSPlayerController::ClientDeathCam_Implementation(AGSCharacter* Body, APawn* KillerPawn, const FGSDeathInfo& Info)
{
	DeathInfo = Info;
	bHasDeathInfo = true;
	LocalDeathTime = GetWorld()->GetTimeSeconds();
	if (!Body) return;
	DeathCamBody = Body;
	Body->DeathCamKiller = KillerPawn;
	// The killer's pawn is null here when it is not relevant to us (too far away): turn toward where he stood instead.
	Body->DeathCamKillerLoc = Info.KillerLocation;
	Body->bDeathCamHasKillerLoc = Info.bHasKillerLocation;
	// The camera manager follows a viewed pawn's player state to that player's next pawn. The server has already
	// cleared it on the body; do the same here at once, or the view jumps away when that update arrives.
	if (Body->GetPlayerState()) Body->SetPlayerState(nullptr);
	SetViewTarget(Body);
}

void AGSPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
	// Losing the pawn points the view at the controller; while dead it should stay on the body.
	if (InPawn) DeathCamBody = nullptr;
	else if (AGSCharacter* Body = DeathCamBody.Get()) SetViewTarget(Body);
}

void AGSPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);
	if (P)
	{
		bHasDeathInfo = false;
		DeathCamBody = nullptr;
	}
	// ClientRestart (respawn, match start) calls ResetIgnoreInputFlags just before this: with the menu still open the
	// new body could otherwise walk and look around behind it.
	if (bMenuOpen)
	{
		SetIgnoreLookInput(true);
		SetIgnoreMoveInput(true);
		UE_LOG(LogTemp, Log, TEXT("GSInput: ny figur mens menyen er åpen, gange og blikk sperret igjen"));
	}
}

void AGSPlayerController::PawnLeavingGame()
{
	// Called by APlayerController::Destroyed on the server when the connection is gone, before GameMode::Logout.
	// In a match the body does not vanish: it dies where it stands, the gear and the flag fall, and whoever hurt
	// him lately gets the kill (no quitting to deny the kill or to take the flag off the map).
	AGSCharacter* C = Cast<AGSCharacter>(GetPawn());
	UWorld* World = GetWorld();
	const AGSGameMode* GM = World ? World->GetAuthGameMode<AGSGameMode>() : nullptr;
	if (C && !C->bDead && GM && GM->DamageEnabled() && !World->bIsTearingDown)
	{
		UE_LOG(LogTemp, Log, TEXT("GSNet: %s forlot kampen, kroppen blir liggende"), *GetNameSafe(C));
		C->DieLeavingGame();   // lets go of the pawn itself (DetachFromControllerPendingDestroy)
		return;                // not Super: that would destroy the body
	}
	Super::PawnLeavingGame();
}

AActor* AGSPlayerController::GetHomeViewTarget() const
{
	if (APawn* P = GetPawn()) return P;
	return DeathCamBody.Get();
}

void AGSPlayerController::Grafikk(int32 Level)
{
	ApplyGraphicsLevel(Level);
	ClientMessage(FString::Printf(TEXT("Grafikk satt til %d (0 = lav, 4 = film)"), FMath::Clamp(Level, 0, 4)));
}

void AGSPlayerController::Opplosning(int32 Percent)
{
	if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		S->SetResolutionScaleValueEx(static_cast<float>(FMath::Clamp(Percent, 50, 100)));
		S->ApplySettings(false);
		S->SaveSettings();
		ClientMessage(FString::Printf(TEXT("Oppl\u00F8sning %d %%"), FMath::Clamp(Percent, 50, 100)));
	}
}

void AGSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// In the editor the cap would otherwise stay on the editor window after Play is stopped.
	if (GIsEditor && bFrameLimitSet)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS")))
		{
			V->Set(0.f, ECVF_SetByConsole);   // the highest priority we ever set it with, so this always takes
		}
		bFrameLimitSet = false;
	}
	Super::EndPlay(EndPlayReason);
}

void AGSPlayerController::ApplyFrameLimit(bool bSave, bool bFromConsole)
{
	if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS")))
	{
		// Typed by the player: console priority, so it also wins over a "t.MaxFPS 120" typed earlier.
		// In the editor always: EndPlay takes the cap off with console priority, and a weaker Set in the next
		// Play would then be dropped without a word.
		V->Set(static_cast<float>(FrameLimit), bFromConsole || GIsEditor ? ECVF_SetByConsole : ECVF_SetByGameSetting);
		bFrameLimitSet = true;
	}
	if (!GIsEditor)
	{
		if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			if (!FMath::IsNearlyEqual(S->GetFrameRateLimit(), static_cast<float>(FrameLimit)))
			{
				S->SetFrameRateLimit(static_cast<float>(FrameLimit));
				S->ApplySettings(false);
				S->SaveSettings();
			}
		}
	}
	if (bSave)
	{
		GConfig->SetInt(TEXT("Gloshaugen"), TEXT("FrameLimit"), FrameLimit, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void AGSPlayerController::Bildefrekvens(int32 Fps)
{
	SetFrameLimit(Fps);
	ClientMessage(FString::Printf(TEXT("Bildefrekvens: h\u00F8yst %d bilder i sekundet"), FrameLimit));
}

void AGSPlayerController::Straaling(int32 On)
{
	const bool bOn = On != 0;
	// The whole ray tracing pipeline, not only Lumen's use of it: off really frees the GPU time and memory.
	if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.Enable")))
	{
		V->Set(bOn ? 1 : 0, ECVF_SetByConsole);
	}
	if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing")))
	{
		V->Set(bOn ? 1 : 0, ECVF_SetByConsole);
	}
	if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.Reflections.HardwareRayTracing")))
	{
		V->Set(bOn ? 1 : 0, ECVF_SetByConsole);
	}
	ClientMessage(bOn ? TEXT("Ray tracing P\u00C5 (Lumen med RTX)") : TEXT("Ray tracing AV (programvare-Lumen, raskere)"));
}

// ---------------------------------------------------------------- PCs (security cameras, tracker) and 3D printers

static constexpr float LoginSeconds = 1.4f;
static constexpr float TrackCooldown = 45.f;
static constexpr int32 TrackPulses = 10;      // one every 2 s = 20 s of tracking

void AGSPlayerController::OpenTerminal(AGSTerminal* T)
{
	if (!T) return;
	ActiveTerminal = T;
	TerminalMode = 0;
	TerminalOpenTime = GetWorld()->GetTimeSeconds();
	RecipeIndex = FMath::Clamp(RecipeIndex, 0, AGSTerminal::Recipes().Num() - 1);
}

void AGSPlayerController::CloseTerminal()
{
	AGSTerminal* T = ActiveTerminal.Get();
	if (TerminalMode == 1)
	{
		SetViewTargetWithBlend(GetHomeViewTarget(), 0.15f);
		ServerViewCamera(T, nullptr);
	}
	ActiveTerminal = nullptr;
	TerminalMode = 0;
}

bool AGSPlayerController::IsLoggingIn() const
{
	const AGSTerminal* T = ActiveTerminal.Get();
	return T && T->Kind == EGSTerminalKind::Computer && GetWorld()->GetTimeSeconds() - TerminalOpenTime < LoginSeconds;
}

TArray<AGSSecurityCamera*> AGSPlayerController::GetCameras() const
{
	TArray<AGSSecurityCamera*> Out;
	for (TActorIterator<AGSSecurityCamera> It(GetWorld()); It; ++It) Out.Add(*It);
	Out.Sort([](const AGSSecurityCamera& A, const AGSSecurityCamera& B) { return A.Label < B.Label; });
	return Out;
}

void AGSPlayerController::TerminalInput(const FString& Key)
{
	AGSTerminal* T = ActiveTerminal.Get();
	if (!T) return;
	if (Key == TEXT("E")) { CloseTerminal(); return; }
	if (IsLoggingIn()) return;

	if (T->Kind == EGSTerminalKind::Printer)
	{
		const int32 N = AGSTerminal::Recipes().Num();
		if (Key == TEXT("Q") || Key == TEXT("2")) RecipeIndex = (RecipeIndex + 1) % N;
		else if (Key == TEXT("1")) RecipeIndex = (RecipeIndex + N - 1) % N;
		else if (Key == TEXT("F") || Key == TEXT("Fire")) ServerPrint(T, RecipeIndex);
		return;
	}

	// Computer
	const TArray<AGSSecurityCamera*> Cams = GetCameras();
	auto ShowCamera = [&](int32 Index)
	{
		if (Cams.Num() == 0) { ClientTerminalMessage(TEXT("Ingen kameraer er koblet til")); return; }
		CameraIndex = (Index % Cams.Num() + Cams.Num()) % Cams.Num();
		TerminalMode = 1;
		SetViewTargetWithBlend(Cams[CameraIndex], 0.f);
		ServerViewCamera(T, Cams[CameraIndex]);
	};
	if (Key == TEXT("1")) ShowCamera(TerminalMode == 1 ? CameraIndex : 0);
	else if (Key == TEXT("2"))
	{
		if (TerminalMode == 1) { SetViewTargetWithBlend(GetPawn(), 0.f); ServerViewCamera(T, nullptr); }
		TerminalMode = 2;
		ServerTrack(T);
	}
	else if (Key == TEXT("3") && TerminalMode != 0)
	{
		if (TerminalMode == 1) { SetViewTargetWithBlend(GetPawn(), 0.f); ServerViewCamera(T, nullptr); }
		TerminalMode = 0;
	}
	else if ((Key == TEXT("Q") || Key == TEXT("Fire")) && TerminalMode == 1) ShowCamera(CameraIndex + 1);
	else if (Key == TEXT("F") && TerminalMode == 1) ShowCamera(CameraIndex - 1);
}

void AGSPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	// Dead: keep watching the body if something else moved the view (the engine resets it when the pawn is let go).
	if (!GetPawn() && DeathCamBody.IsValid() && !ActiveTerminal.IsValid() && GetViewTarget() != DeathCamBody.Get())
	{
		SetViewTarget(DeathCamBody.Get());
	}
	AGSTerminal* T = ActiveTerminal.Get();
	if (!T && ActiveTerminal.IsStale()) { ActiveTerminal = nullptr; TerminalMode = 0; SetViewTargetWithBlend(GetHomeViewTarget(), 0.f); return; }
	if (!T) return;
	// Leaving, dying or getting shot kicks you off the terminal.
	const AGSCharacter* C = Cast<AGSCharacter>(GetPawn());
	if (!C || C->bDead || FVector::Dist(C->GetActorLocation(), T->GetActorLocation()) > 300.f || C->GetLastDamageTakenTime() > TerminalOpenTime)
	{
		CloseTerminal();
	}
}

bool AGSPlayerController::ServerCanUse(const AGSTerminal* T) const
{
	const AGSCharacter* C = Cast<AGSCharacter>(GetPawn());
	return T && C && !C->bDead && FVector::Dist(C->GetActorLocation(), T->GetActorLocation()) < 350.f;
}

void AGSPlayerController::ServerViewCamera_Implementation(AGSTerminal* T, AGSSecurityCamera* Cam)
{
	if (Cam && (!ServerCanUse(T) || T->Kind != EGSTerminalKind::Computer)) Cam = nullptr;
	SetViewTargetWithBlend(Cam ? static_cast<AActor*>(Cam) : GetHomeViewTarget(), 0.f);
}

void AGSPlayerController::ServerTrack_Implementation(AGSTerminal* T)
{
	if (!ServerCanUse(T) || T->Kind != EGSTerminalKind::Computer) return;
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < ServerNextTrackTime)
	{
		ClientTerminalMessage(FString::Printf(TEXT("Sporingen lader opp, vent %d sek"), FMath::CeilToInt(ServerNextTrackTime - Now)));
		return;
	}
	ServerNextTrackTime = Now + TrackCooldown;
	TrackPulsesLeft = TrackPulses;
	SendTrackPulse();
	GetWorldTimerManager().SetTimer(TrackTimer, this, &AGSPlayerController::SendTrackPulse, 2.f, true);
}

void AGSPlayerController::SendTrackPulse()
{
	if (TrackPulsesLeft-- <= 0) { GetWorldTimerManager().ClearTimer(TrackTimer); return; }
	TArray<FGSTrackedPlayer> Data;
	for (TActorIterator<AGSCharacter> It(GetWorld()); It; ++It)
	{
		const AGSCharacter* C = *It;
		if (C->bDead) continue;
		FGSTrackedPlayer P;
		P.Location = C->GetActorLocation();
		P.Name = C->GetPlayerState() ? C->GetPlayerState()->GetPlayerName() : TEXT("?");
		P.bSelf = C == GetPawn();
		Data.Add(P);
	}
	ClientTrackData(Data, TrackPulsesLeft * 2.f + 2.f);
}

void AGSPlayerController::ClientTrackData_Implementation(const TArray<FGSTrackedPlayer>& Data, float SecondsLeft)
{
	Tracked = Data;
	TrackedTime = GetWorld()->GetTimeSeconds();
	TrackUntil = TrackedTime + SecondsLeft;
}

void AGSPlayerController::ServerPrint_Implementation(AGSTerminal* T, int32 Recipe)
{
	if (!ServerCanUse(T)) return;
	const FString Error = T->StartPrint(Cast<AGSCharacter>(GetPawn()), Recipe);
	ClientTerminalMessage(Error.IsEmpty() ? FString(TEXT("Printer ...")) : Error);
}

void AGSPlayerController::ClientTerminalMessage_Implementation(const FString& Message)
{
	if (AGSCharacter* C = Cast<AGSCharacter>(GetPawn()))
	{
		C->ToastText = Message;
		C->ToastTime = GetWorld()->GetTimeSeconds();
	}
}
