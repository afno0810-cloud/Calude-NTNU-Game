#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/Interface.h"
#include "Engine/NetSerialization.h"
#include "GSInteract.generated.h"

class AGSCharacter;
class AGSPlayerController;
class UEnhancedInputComponent;
class UInputAction;
class AHUD;
class UCanvas;

/** What the use key did: a short press, the start of a hold (0.3 s) or the release after a hold. */
UENUM()
enum class EGSUseVerb : uint8 { Tap, HoldStart, HoldEnd };

/** The hint shown in the middle of the screen. "%K%" in the texts is replaced by the use key's name. */
struct FGSUsePrompt
{
	FString Main;
	FString Sub;
	FLinearColor Color = FLinearColor(0.92f, 0.92f, 0.92f);
	bool bHasHold = false;
};

/** Something that takes time while the key is held (picking a lock, breaking it open, kicking a door in). */
namespace GSJob
{
	constexpr uint8 None = 0;
	constexpr uint8 Pick = 1;
	constexpr uint8 Force = 2;
	constexpr uint8 Kick = 3;
}

USTRUCT()
struct FGSUseJob
{
	GENERATED_BODY()

	UPROPERTY() int32 Key = 0;
	UPROPERTY() uint8 Kind = 0;      // GSJob
	UPROPERTY() float Start = 0.f;   // server world time
	UPROPERTY() float End = 0.f;
	/** Server only: the part's lock and opening when the job started (a change from someone else cancels it). */
	uint8 SnapLock = 0;
	int8 SnapOpen = 0;
	float NextBeat = 0.f;            // server: next noise/sound beat
	bool IsActive() const { return Kind != GSJob::None; }
};

/** What a use did on the server: start a job, or tell the user something (only he sees/hears it). */
struct FGSUseResult
{
	FGSUseJob Job;
	FString Feedback;
	FString FeedbackSound;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UGSInteract : public UInterface
{
	GENERATED_BODY()
};

/** Something in the map that can be used with the use key (E): doors now; switches, lockers, lifts later. */
class GLOSHAUGEN_API IGSInteract
{
	GENERATED_BODY()

public:
	virtual int32 GetInteractKey() const { return 0; }
	/** Where the user aims to use it. Follows the user's side (inside/outside). */
	virtual FVector GetUsePoint(const FVector& UserEye) const { return FVector::ZeroVector; }
	/** The hint for this user. False = nothing to do here. */
	virtual bool GetUsePrompt(const AGSCharacter* User, FGSUsePrompt& Out) const { return false; }
	/** Server: the use key did Verb (checked: alive, close, free line). */
	virtual void ServerUse(AGSCharacter* User, EGSUseVerb Verb, FGSUseResult& Out) {}
	/** The owner's machine, right when a tap is sent: show the result at once (the server corrects it). */
	virtual void PredictUse(AGSCharacter* User) {}
	/** Server: a player's job was accepted and starts now (sounds for everyone near, flags). */
	virtual void ServerJobStarted(AGSCharacter* User, FGSUseJob& Job) {}
	/** Server, every frame while a job runs. False = cancel it. */
	virtual bool ServerJobTick(AGSCharacter* User, FGSUseJob& Job, float Now) { return true; }
	/** Server: a job ended. bCompleted false = cancelled. Return true and fill Next to start another one right away. */
	virtual bool ServerJobDone(AGSCharacter* User, const FGSUseJob& Job, bool bCompleted, FGSUseJob& Next) { return false; }
	/** Text over the progress bar. */
	virtual FString JobLabel(uint8 Kind) const { return FString(); }
};

/**
 * One per human player (made by AGSMapState on the server, owned by the player controller, relevant only to its owner).
 * On the owner's machine it finds what you look at, catches the use key, measures tap/hold and draws the hint and the
 * progress bar. On the server it checks every use and runs the jobs on the server clock.
 */
UCLASS(NotPlaceable)
class GLOSHAUGEN_API AGSInteractor : public AActor
{
	GENERATED_BODY()

public:
	AGSInteractor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UFUNCTION(Server, Reliable) void ServerUse(int32 Key, uint8 Verb);
	/** Server -> owner: a short message (and a sound at the part) only the user gets. */
	UFUNCTION(Client, Reliable) void ClientFeedback(const FString& Text, const FString& Sound, FVector_NetQuantize Where);

	/** Replicated to the owner: the job running now (progress bar). */
	UPROPERTY(Replicated) FGSUseJob Job;

	/** Server: stop the job (bCompleted false = cancelled). */
	void EndJob(bool bCompleted);
	/** Server: run a use (players through ServerUse, bots through GSInteract::BotUse). */
	void HandleUse(AGSCharacter* C, int32 Key, EGSUseVerb Verb);

	AGSPlayerController* GetPC() const;
	AGSCharacter* GetCharacter() const;
	bool HasFocus() const { return FocusKey != 0; }
	void DrawHUD(AHUD* HUD, UCanvas* Canvas);

	/** Kamp's character asks (K4): from then on Kamp steps aside when we have the focus. */
	bool bCharacterHooked = false;

protected:
	// Owner machine
	void TryBindInput();
	void OnInteractStarted();
	void UpdateFocus();
	bool IsBlocked() const;
	float KampFocusScore(const AGSCharacter* C, const FVector& Eye, const FVector& Fwd) const;
	void SendVerb(EGSUseVerb Verb);

	UPROPERTY() TObjectPtr<UEnhancedInputComponent> InputComp;
	TWeakObjectPtr<AGSPlayerController> BoundPC;
	float NextBindTry = 0.f;
	float NextFocus = 0.f;
	int32 FocusKey = 0;
	FGSUsePrompt FocusPrompt;
	// The press that is going on: decided when the key went down
	bool bPressActive = false;
	bool bPressMine = false;
	bool bHoldSent = false;
	int32 PressKey = 0;
	float PressTime = 0.f;
	FString FeedbackText;
	float FeedbackTime = -100.f;

	// Server
	float LastUseTime = -100.f;
	TWeakObjectPtr<AGSCharacter> JobUser;
};

namespace GSInteract
{
	/** Kamp (K4): true when the use key belongs to us right now for this character's player (Kamp then does nothing). */
	GLOSHAUGEN_API bool HasFocus(const AGSCharacter* C);
	/** Bots: use a part like a player would (same checks, no RPC). Jobs (picking, kicking in) are not run for bots yet. */
	GLOSHAUGEN_API bool BotUse(AGSCharacter* Bot, int32 Key, EGSUseVerb Verb);
	/** Server: may this character use that part now (alive, not using an item, close, free line)? */
	GLOSHAUGEN_API bool CanUse(const AGSCharacter* C, AActor* Part, float MaxDistCm = 250.f);
	/** Free line from Eye to Point through the world (objects: WorldStatic, WorldDynamic, Destructible), Part and User ignored. */
	GLOSHAUGEN_API bool HasClearLine(const UWorld* World, const FVector& Eye, const FVector& Point, const AActor* Part, const AActor* User);
	/** Starts drawing the hints (AHUD::OnHUDPostRender). Safe to call many times. */
	void EnsureHooked();
}
