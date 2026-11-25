#include "GSPlayerController.h"
#include "HAL/IConsoleManager.h"
#include "GSHUD.h"
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
	NextWeaponAction = MakeAction(this, TEXT("IA_NextWeapon"), EInputActionValueType::Boolean);
	ScoreboardAction = MakeAction(this, TEXT("IA_Scoreboard"), EInputActionValueType::Boolean);
	InspectAction = MakeAction(this, TEXT("IA_Inspect"), EInputActionValueType::Boolean);

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
	C->MapKey(NextWeaponAction, EKeys::MouseScrollDown);
	C->MapKey(NextWeaponAction, EKeys::MouseScrollUp);
	C->MapKey(ScoreboardAction, EKeys::Tab);
	C->MapKey(InspectAction, EKeys::T);
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
	}
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
	ClientTravel(Address, ETravelType::TRAVEL_Absolute);
}

void AGSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController() || GIsEditor) return;

	// First time the game runs: settings that look good and stay smooth on a laptop RTX 3050 (6 GB).
	UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!S) return;
	bool bDone = false;
	GConfig->GetBool(TEXT("Gloshaugen"), TEXT("GraphicsDefaultsApplied"), bDone, GGameUserSettingsIni);
	if (bDone) return;
	S->SetOverallScalabilityLevel(2);          // High
	S->SetResolutionScaleValueEx(72.f);       // percent; TSR upscales to full resolution
	S->SetFrameRateLimit(0.f);
	S->ApplySettings(false);
	S->SaveSettings();
	GConfig->SetBool(TEXT("Gloshaugen"), TEXT("GraphicsDefaultsApplied"), true, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void AGSPlayerController::Grafikk(int32 Level)
{
	if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		S->SetOverallScalabilityLevel(FMath::Clamp(Level, 0, 4));
		S->ApplySettings(false);
		S->SaveSettings();
		ClientMessage(FString::Printf(TEXT("Grafikk satt til %d (0 = lav, 4 = film)"), FMath::Clamp(Level, 0, 4)));
	}
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

void AGSPlayerController::Straaling(int32 On)
{
	const bool bOn = On != 0;
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
		SetViewTargetWithBlend(GetPawn(), 0.15f);
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
	AGSTerminal* T = ActiveTerminal.Get();
	if (!T && ActiveTerminal.IsStale()) { ActiveTerminal = nullptr; TerminalMode = 0; SetViewTargetWithBlend(GetPawn(), 0.f); return; }
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
	SetViewTargetWithBlend(Cam ? static_cast<AActor*>(Cam) : static_cast<AActor*>(GetPawn()), 0.f);
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
