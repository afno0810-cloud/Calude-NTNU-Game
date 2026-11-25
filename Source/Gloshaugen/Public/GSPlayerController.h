#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GSPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

/** Input actions are created in code so the project needs no input assets. */
UCLASS()
class GLOSHAUGEN_API AGSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGSPlayerController();

	virtual void SetupInputComponent() override;

	void EnsureInputCreated();

	UPROPERTY() TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> LookAction;
	UPROPERTY() TObjectPtr<UInputAction> JumpAction;
	UPROPERTY() TObjectPtr<UInputAction> SprintAction;
	UPROPERTY() TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY() TObjectPtr<UInputAction> FireAction;
	UPROPERTY() TObjectPtr<UInputAction> AimAction;
	UPROPERTY() TObjectPtr<UInputAction> ReloadAction;
	UPROPERTY() TObjectPtr<UInputAction> Slot1Action;
	UPROPERTY() TObjectPtr<UInputAction> Slot2Action;
	UPROPERTY() TObjectPtr<UInputAction> NextWeaponAction;
	UPROPERTY() TObjectPtr<UInputAction> ScoreboardAction;

	bool bShowScoreboard = false;

	/** Console: "Host" starts a listen server on this map. */
	UFUNCTION(Exec) void Host();
	/** Console: "Join 192.168.1.20" joins a friend's game. */
	UFUNCTION(Exec) void Join(const FString& Address);

protected:
	void ShowScoreboard() { bShowScoreboard = true; }
	void HideScoreboard() { bShowScoreboard = false; }
};
