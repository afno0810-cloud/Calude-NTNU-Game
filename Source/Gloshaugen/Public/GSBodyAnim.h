#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GSBodyAnim.generated.h"

class UAnimSequence;
struct FGSBodyAnimProxy;

/** What the body animation needs from the game, copied from the character (game thread) to the proxy every frame. */
struct FGSBodyAnimInput
{
	FVector LocalVel = FVector::ZeroVector;    // velocity in the actor's space (cm/s), X forward, Y right
	float AimPitch = 0.f;                       // degrees, normalised (up is positive)
	float CrouchAlpha = 0.f;                    // 0 standing, 1 crouched or sliding (smoothed)
	float SprintAlpha = 0.f;
	bool bFalling = false;
	bool bDead = false;                         // the pose is left alone: the ragdoll has the body
	uint8 Set = 0;                              // 0 unarmed, 1 rifle, 2 pistol
	bool bHandIK_R = false, bHandIK_L = false;
	/** Hand targets in COMPONENT space: the location is where the hand bone goes; the rotation is applied on top of
	 *  the hand's animated rotation (how far the gun is turned from straight ahead), so the fingers keep the grip
	 *  the rifle/pistol animation gives them. */
	FTransform HandR = FTransform::Identity, HandL = FTransform::Identity;
	/** Elbow hints, component space, as offsets from each upper arm. */
	FVector ElbowHintR = FVector::ZeroVector, ElbowHintL = FVector::ZeroVector;
	bool bFirstPerson = false;                  // the owner's own body: no aim bend and no hand IK (round 14a)
	/** The actor's axes in component space (the mesh is turned -90 yaw, so forward is +Y there). */
	FVector CompFwd = FVector(0.f, 1.f, 0.f), CompRight = FVector(-1.f, 0.f, 0.f), CompUp = FVector(0.f, 0.f, 1.f);
};

namespace GSBodySlots
{
	/** Per set: 0 idle, 1..8 walk, 9..16 jog (Fwd, Fwd_Right, Right, Bwd_Right, Bwd, Bwd_Left, Left, Fwd_Left), 17 fall. */
	constexpr int32 Idle = 0, Walk = 1, Jog = 9, Fall = 17, PerSet = 18, Sets = 3, Num = PerSet * Sets;
}

/**
 * The human body in plain C++, without an animation blueprint: walk and jog in eight directions and the fall loop
 * from the mannequin sequences in the project (unarmed, rifle, pistol), crouch with bent knees, a little back bend
 * toward the aim, and two-bone IK that puts the hands on the gun. The proxy does the work on the animation threads.
 */
UCLASS(Transient)
class GLOSHAUGEN_API UGSBodyAnim : public UAnimInstance
{
	GENERATED_BODY()

public:
	UGSBodyAnim();

	/** Filled by the character every frame (game thread). */
	FGSBodyAnimInput Input;
	/** Sequences or bones are missing: the character goes back to the old body (read on the game thread). */
	bool bMissingBones = false;
	/** Real speed of the forward walk and jog (cm/s), measured from their root motion when loaded. */
	float WalkSpeed = 165.f;
	float JogSpeed = 380.f;

	/** The sequences without which this body cannot be used (unarmed idle, rifle walk forward). */
	static bool HasSequences();

	/** Kept here so the garbage collector keeps them; the proxy only gets raw pointers. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimSequence>> Seqs;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:
	/** Game thread only: loads the 54 sequence slots (empty slots take the unarmed set's). */
	void LoadSequences();
	friend struct FGSBodyAnimProxy;
};
