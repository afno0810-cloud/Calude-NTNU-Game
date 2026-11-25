#include "GSPlayerController.h"
#include "GSHUD.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

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
	NextWeaponAction = MakeAction(this, TEXT("IA_NextWeapon"), EInputActionValueType::Boolean);
	ScoreboardAction = MakeAction(this, TEXT("IA_Scoreboard"), EInputActionValueType::Boolean);

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
	C->MapKey(NextWeaponAction, EKeys::MouseScrollDown);
	C->MapKey(NextWeaponAction, EKeys::MouseScrollUp);
	C->MapKey(ScoreboardAction, EKeys::Tab);
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
