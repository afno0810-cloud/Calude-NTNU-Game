#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSMapState.h"
#include "GSInteract.h"
#include "GSDoor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UMaterialInstanceDynamic;

/**
 * A door in the map (or the test room). A local actor: every machine builds the same doors from the map data with the
 * same key, and AGSMapState tells them how they stand. Opens away from you (E), can be kicked open (hold E), kicked in
 * when locked (wood), opened with a card, picked or broken open (glass doors with a code lock), and every locked door
 * opens from its escape side (handle / push bar), like real escape doors.
 */
UCLASS(NotPlaceable)
class GLOSHAUGEN_API AGSDoor : public AActor, public IGSInteract
{
	GENERATED_BODY()

public:
	AGSDoor();
	virtual void Tick(float DeltaSeconds) override;

	/** Builds the parts. Call once right after spawning. */
	void Init(const FGSDoorDef& InDef, int32 InKey);

	// --- IGSInteract ---
	virtual int32 GetInteractKey() const override { return Key; }
	virtual FVector GetUsePoint(const FVector& UserEye) const override;
	virtual bool GetUsePrompt(const AGSCharacter* User, FGSUsePrompt& Out) const override;
	virtual void ServerUse(AGSCharacter* User, EGSUseVerb Verb, FGSUseResult& Out) override;
	virtual void PredictUse(AGSCharacter* User) override;
	virtual void ServerJobStarted(AGSCharacter* User, FGSUseJob& InJob) override;
	virtual bool ServerJobTick(AGSCharacter* User, FGSUseJob& InJob, float Now) override;
	virtual bool ServerJobDone(AGSCharacter* User, const FGSUseJob& InJob, bool bCompleted, FGSUseJob& Next) override;
	virtual FString JobLabel(uint8 Kind) const override;

	// --- State (every machine) ---
	/** The replicated state arrived (or the server changed it). bFresh: it just happened (sound, fast swing). */
	void ApplyState(const FGSPartState& S, bool bFresh);
	/** Back to the start of a match, at once and silent. */
	void ResetLocal();
	/** How this door stands at the start of a match. */
	FGSPartState MakeDefault() const;

	uint8 GetLock() const { return Lock; }
	bool IsLocked() const { return Lock == GSLock::Locked; }
	/** Bots: the door stands open (or is swinging open). */
	bool IsOpen() const { return TargetOpen != 0; }
	bool IsClosedNow() const { return TargetOpen == 0 && FMath::Abs(CurOpen) < 1.f; }
	/** +1 in front (outside), -1 behind. */
	int32 SideOf(const FVector& P) const;
	/** The side a locked door still opens from: behind (inside the room) for most, in front for fire doors (escape route
	 *  into the stairwell; from the stairwell they stay shut). */
	int32 EscapeSide() const { return Def.Art == EGSDoorArt::Fire ? 1 : -1; }
	FVector GetNormal() const { return GetActorForwardVector(); }
	FVector GetBase() const { return GetActorLocation(); }
	FBox GetOpeningBox() const;
	const FGSDoorDef& GetDef() const { return Def; }
	FGSDoorInfo GetInfo() const;
	FString DebugText() const;

	// --- Server ---
	/** Opens the door away from a point (blast, kick, bot). */
	void ServerOpenAway(const FVector& From, uint8 Cause, bool bBreakLock, bool bAddHealth);
	void ServerBulletHit(const FHitResult& Hit, float Damage);
	void ServerMeleeHit(const FHitResult& Hit, AActor* User);
	bool IsKeypad(const UPrimitiveComponent* C) const;

protected:
	struct FLeaf
	{
		USceneComponent* Pivot = nullptr;
		float HingeSide = 1.f;   // +1 = hinge on the actor's +Y (left seen from the front)
		float Width = 100.f;
	};

	UStaticMeshComponent* AddBox(USceneComponent* Parent, const FVector& Center, const FVector& SizeCm, const TCHAR* Mat, bool bCollide);
	UBoxComponent* AddCollider(USceneComponent* Parent, const FVector& Center, const FVector& SizeCm);
	void BuildLeaf(FLeaf& L, float LeafHeight);
	void PlaceKeypad();
	void UpdatePose();
	void SetMovingCollision(bool bMoving);
	void UpdateLed();
	void UpdateTick();
	FVector LeafCenter(int32 Index) const;
	void SparksAt(const FVector& Loc);
	FVector HandleWorld() const;

	FGSDoorDef Def;
	int32 Key = 0;
	TArray<FLeaf> Leaves;
	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TArray<TObjectPtr<UPrimitiveComponent>> Blockers;   // leaf boxes and the pane: ignore pawns while swinging
	UPROPERTY() TObjectPtr<UBoxComponent> KeypadBox;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> KeypadLed;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> LedMID;

	// Local copy of the replicated state
	uint8 Lock = 0;
	int8 TargetOpen = 0;
	uint8 Health = 0;
	uint8 Flags = 0;
	uint8 StateBits = 0;
	// Motion
	float CurOpen = 0.f;          // -100..100
	float SpeedDegS = 170.f;
	bool bMovingCollision = false;
	bool bSoundOnClose = false;   // play the close/slam when it reaches 0 (fresh changes only)
	bool bSlamOnClose = false;
	// Prediction on the owner's machine
	int8 ServerOpen = 0;          // the last replicated target
	float PredictUntil = -1.f;
	// Server timers
	float AutoCloseAt = -1.f;
	// Local sound beats
	float NextPickSound = 0.f;
	float NextForceSound = 0.f;
};
