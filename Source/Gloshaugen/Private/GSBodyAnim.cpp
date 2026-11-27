#include "GSBodyAnim.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"
#include "BonePose.h"
#include "TwoBoneIK.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/Paths.h"

namespace GSBodyAnimImpl
{
	const TCHAR* const Root = TEXT("/Game/Characters/Mannequins/Anims/");
	/** Same order as the walk/jog slots: Fwd, Fwd_Right, Right, Bwd_Right, Bwd, Bwd_Left, Left, Fwd_Left (45 degree steps
	 *  clockwise seen from above, like atan2(right, forward)). */
	const TCHAR* const Dirs[8] = { TEXT("Fwd"), TEXT("Fwd_Right"), TEXT("Right"), TEXT("Bwd_Right"), TEXT("Bwd"),
		TEXT("Bwd_Left"), TEXT("Left"), TEXT("Fwd_Left") };
	struct FSetPaths { const TCHAR* Idle; const TCHAR* Walk; const TCHAR* Jog; const TCHAR* Fall; };
	const FSetPaths Sets[GSBodySlots::Sets] = {
		{ TEXT("Unarmed/MM_Idle"),         TEXT("Unarmed/Walk/MF_Unarmed_Walk_"), TEXT("Unarmed/Jog/MF_Unarmed_Jog_"), TEXT("Unarmed/Jump/MM_Fall_Loop") },
		{ TEXT("Rifle/MF_Rifle_Idle_ADS"),  TEXT("Rifle/Walk/MF_Rifle_Walk_"),     TEXT("Rifle/Jog/MF_Rifle_Jog_"),     TEXT("Rifle/Jump/MM_Rifle_Jump_Fall_Loop") },
		{ TEXT("Pistol/MF_Pistol_Idle_ADS"), TEXT("Pistol/Walk/MF_Pistol_Walk_"),  TEXT("Pistol/Jog/MF_Pistol_Jog_"),   TEXT("Pistol/Jump/MM_Pistol_Jump_Fall_Loop") },
	};

	UAnimSequence* Load(const FString& Rel)
	{
		const FString Name = FPaths::GetBaseFilename(Rel);
		const FString Path = FString(Root) + Rel + TEXT(".") + Name;
		return LoadObject<UAnimSequence>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	/** Forward speed (cm/s) of a looping clip from its root motion over one cycle. 0 when it has none. */
	float MeasureSpeed(const UAnimSequence* Seq)
	{
		if (!Seq) return 0.f;
		const float Len = Seq->GetPlayLength();
		if (Len <= KINDA_SMALL_NUMBER) return 0.f;
		const FTransform Motion = Seq->ExtractRootMotionFromRange(0.0, Len, FAnimExtractContext());
		return static_cast<float>(Motion.GetTranslation().Size2D()) / Len;
	}

	/** Rotation of a bone in component space, built from the local pose up the parent chain. */
	FQuat CSRotation(const FCompactPose& Pose, FCompactPoseBoneIndex Bone)
	{
		FQuat Q = FQuat::Identity;
		while (Bone != INDEX_NONE)
		{
			Q = Pose[Bone].GetRotation() * Q;
			Bone = Pose.GetParentBoneIndex(Bone);
		}
		return Q;
	}

	/** Turns a bone (local pose) by Degrees around an axis given in component space. */
	void RotateBone(FCompactPose& Pose, FCompactPoseBoneIndex Bone, const FVector& AxisCS, float Degrees)
	{
		if (Bone == INDEX_NONE || FMath::IsNearlyZero(Degrees)) return;
		const FCompactPoseBoneIndex Parent = Pose.GetParentBoneIndex(Bone);
		const FQuat ParentCS = Parent != INDEX_NONE ? CSRotation(Pose, Parent) : FQuat::Identity;
		const FVector Axis = ParentCS.Inverse().RotateVector(AxisCS).GetSafeNormal();
		if (Axis.IsNearlyZero()) return;
		FTransform& T = Pose[Bone];
		T.SetRotation((FQuat(Axis, FMath::DegreesToRadians(Degrees)) * T.GetRotation()).GetNormalized());
	}

	bool HasNaN(const FTransform& A, const FTransform& B, const FTransform& C)
	{
		return A.ContainsNaN() || B.ContainsNaN() || C.ContainsNaN();
	}
}

// ---------------------------------------------------------------------------------------------------------------------

struct FGSBodyAnimProxy : public FAnimInstanceProxy
{
	FGSBodyAnimProxy() = default;
	explicit FGSBodyAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

protected:
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override
	{
		FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
		const UGSBodyAnim* Anim = CastChecked<UGSBodyAnim>(InAnimInstance);
		In = Anim->Input;
		bSeqsMissing = Anim->bMissingBones || Anim->Seqs.Num() != GSBodySlots::Num;
		for (int32 I = 0; I < GSBodySlots::Num; ++I)
		{
			Seqs[I] = Anim->Seqs.IsValidIndex(I) ? Anim->Seqs[I].Get() : nullptr;
		}
		WalkSpd = Anim->WalkSpeed;
		JogSpd = Anim->JogSpeed;
	}

	virtual void Update(float Dt) override
	{
		Dt = FMath::Clamp(Dt, 0.f, 0.1f);
		// Smooth the velocity a little, so a stop or a turn does not snap the legs to another clip.
		SmoothVel = FMath::VInterpTo(SmoothVel, In.LocalVel, Dt, 10.f);
		FallAlpha = FMath::FInterpConstantTo(FallAlpha, In.bFalling ? 1.f : 0.f, Dt, 1.f / 0.15f);
		HandWR = FMath::FInterpConstantTo(HandWR, In.bHandIK_R && !In.bFirstPerson ? 1.f : 0.f, Dt, 10.f);
		HandWL = FMath::FInterpConstantTo(HandWL, In.bHandIK_L && !In.bFirstPerson ? 1.f : 0.f, Dt, 10.f);

		const FBand B = GetBand();
		const UAnimSequence* WalkFwd = Seq(GSBodySlots::Walk);
		const UAnimSequence* JogFwd = Seq(GSBodySlots::Jog);
		const float WalkLen = WalkFwd ? FMath::Max(WalkFwd->GetPlayLength(), 0.1f) : 1.f;
		const float JogLen = JogFwd ? FMath::Max(JogFwd->GetPlayLength(), 0.1f) : 1.f;
		const float Spd = SmoothVel.Size2D();
		const float WalkRate = FMath::Clamp(Spd / FMath::Max(WalkSpd, 1.f), 0.6f, 1.5f);
		// The rate follows the measured clip speed, so a sprint already plays the jog faster (no extra factor:
		// that would make the feet slip on the floor).
		const float JogRate = FMath::Clamp(Spd / FMath::Max(JogSpd, 1.f), 0.6f, 1.5f);
		// Walk and jog share one phase (0..1 of a cycle), so the feet stay in step through the blend.
		const float JogW = B.bJog ? B.T : 0.f;
		const float Rate = FMath::Lerp(WalkRate, JogRate, JogW);
		const float Len = FMath::Lerp(WalkLen, JogLen, JogW);
		if (Spd > 5.f) Phase = FMath::Fmod(Phase + Dt * Rate / Len, 1.f);
		IdleTime += Dt;
		FallTime = In.bFalling ? FallTime + Dt : 0.f;
	}

	virtual bool Evaluate(FPoseContext& Output) override
	{
		using namespace GSBodyAnimImpl;
		if (In.bDead) return true;   // the ragdoll has the body
		if (bSeqsMissing || !Seq(GSBodySlots::Idle) || !Seq(GSBodySlots::Walk))
		{
			bMissing = true;
			Output.ResetToRefPose();
			return true;
		}
		CacheBoneIndices(Output.Pose.GetBoneContainer());
		if (bBonesMissing)
		{
			bMissing = true;
			Output.ResetToRefPose();
			return true;
		}

		// --- 1. Movement, in local space: at most four samples ---
		const FBand Band = GetBand();
		const float DirDeg = FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(SmoothVel.Y, SmoothVel.X)) + 360.f, 360.f);
		const float DirF = DirDeg / 45.f;
		const int32 DirA = FMath::FloorToInt32(DirF) % 8;
		const int32 DirB = (DirA + 1) % 8;
		const float Frac = DirF - FMath::FloorToFloat(DirF);
		const bool bReduce = FallAlpha > 0.001f;   // fading into the fall loop: one direction only (2 + 1 samples)

		FAnimationPoseData OutData(Output);
		FPoseContext TmpA(Output);
		FAnimationPoseData DataA(TmpA);

		if (FallAlpha >= 0.999f)
		{
			Sample(GSBodySlots::Fall, FallTime, OutData);
		}
		else
		{
			if (bReduce)
			{
				const int32 D = Frac < 0.5f ? DirA : DirB;
				if (!Band.bJog)
				{
					Sample(GSBodySlots::Idle, IdleTime, OutData);
					if (Band.T > 0.001f) { SampleMove(GSBodySlots::Walk + D, DataA); FAnimationRuntime::BlendTwoPosesTogetherInPlace(OutData, DataA, 1.f - Band.T); }
				}
				else
				{
					SampleMove(GSBodySlots::Walk + D, OutData);
					if (Band.T > 0.001f) { SampleMove(GSBodySlots::Jog + D, DataA); FAnimationRuntime::BlendTwoPosesTogetherInPlace(OutData, DataA, 1.f - Band.T); }
				}
				Sample(GSBodySlots::Fall, FallTime, DataA);
				FAnimationRuntime::BlendTwoPosesTogetherInPlace(OutData, DataA, 1.f - FallAlpha);
			}
			else if (!Band.bJog)
			{
				if (Band.T <= 0.001f)
				{
					Sample(GSBodySlots::Idle, IdleTime, OutData);
				}
				else
				{
					SampleDirs(GSBodySlots::Walk, DirA, DirB, Frac, OutData, DataA);
					if (Band.T < 0.999f)
					{
						Sample(GSBodySlots::Idle, IdleTime, DataA);
						FAnimationRuntime::BlendTwoPosesTogetherInPlace(OutData, DataA, Band.T);
					}
				}
			}
			else
			{
				SampleDirs(GSBodySlots::Walk, DirA, DirB, Frac, OutData, DataA);
				if (Band.T > 0.001f)
				{
					FPoseContext TmpB(Output);
					FAnimationPoseData DataB(TmpB);
					SampleDirs(GSBodySlots::Jog, DirA, DirB, Frac, DataA, DataB);
					FAnimationRuntime::BlendTwoPosesTogetherInPlace(OutData, DataA, 1.f - Band.T);
				}
			}
		}

		FCompactPose& Pose = Output.Pose;
		const float Crouch = FMath::Clamp(In.CrouchAlpha, 0.f, 1.f);
		const bool bCrouchIK = Crouch > 0.001f;
		const bool bHands = !In.bFirstPerson && (HandWR > 0.001f || HandWL > 0.001f);

		// Where the feet are in the animation before the hips go down (component space).
		FCSPose<FCompactPose> CS;
		FTransform FootCS[2];
		if (bCrouchIK)
		{
			CS.InitPose(Pose);
			FootCS[0] = CS.GetComponentSpaceTransform(Foot[0]);
			FootCS[1] = CS.GetComponentSpaceTransform(Foot[1]);
		}

		// --- 2. Local space: hips and back ---
		if (bCrouchIK)
		{
			// Hips down and tipped 12 degrees forward; the back straightens 4 of them again, so the chest leans
			// about 8 degrees. Tuned against the crouched head hit box (120 cm over the floor, 6 cm forward): the
			// head bone sits about 10 cm under the middle of the head, as it does standing.
			const FCompactPoseBoneIndex PelvisParent = Pose.GetParentBoneIndex(Pelvis);
			const FQuat ParentCS = PelvisParent != INDEX_NONE ? CSRotation(Pose, PelvisParent) : FQuat::Identity;
			Pose[Pelvis].AddToTranslation(ParentCS.Inverse().RotateVector(In.CompUp * (-CrouchDrop * Crouch)));
			RotateBone(Pose, Pelvis, In.CompRight, 12.f * Crouch);
			for (int32 I = 0; I < 3; ++I) RotateBone(Pose, Spine[I], In.CompRight, -4.f / 3.f * Crouch);
		}
		if (!In.bFirstPerson)
		{
			// Bend toward the aim: at most 10 degrees in all (the plan allows 15; 10 keeps the head within about
			// 7 cm of where it is standing, inside the +-9.5 cm head hit box). The arms and the gun take the rest.
			const float Bend = -FMath::Clamp(In.AimPitch, -10.f, 10.f);
			for (int32 I = 0; I < 5; ++I) RotateBone(Pose, Spine[I], In.CompRight, 0.2f * Bend);
		}
		if (In.Set == 1)
		{
			// Rifle and shotgun: shooter's stance, the chest turned with the left shoulder forward; the neck turns
			// back so the face still looks where the body goes.
			for (int32 I = 2; I < 5; ++I) RotateBone(Pose, Spine[I], In.CompUp, 5.f);
			RotateBone(Pose, Neck, In.CompUp, -15.f);
		}

		// --- 3. Component space: knees and hands ---
		if (bCrouchIK || bHands)
		{
			CS.InitPose(Pose);
			if (bCrouchIK)
			{
				for (int32 S = 0; S < 2; ++S)
				{
					FTransform A = CS.GetComponentSpaceTransform(Thigh[S]);
					FTransform B = CS.GetComponentSpaceTransform(Calf[S]);
					FTransform C = CS.GetComponentSpaceTransform(Foot[S]);
					C.SetRotation(FootCS[S].GetRotation());
					const FVector Hint = B.GetLocation() + In.CompFwd * 30.f;
					AnimationCore::SolveTwoBoneIK(A, B, C, Hint, FootCS[S].GetLocation(), false, 1.0, 1.0);
					if (HasNaN(A, B, C)) continue;
					const FBoneTransform Chain[3] = { FBoneTransform(Thigh[S], A), FBoneTransform(Calf[S], B), FBoneTransform(Foot[S], C) };
					CS.SafeSetCSBoneTransforms(Chain);
				}
			}
			if (bHands)
			{
				for (int32 S = 0; S < 2; ++S)
				{
					const float W = S == 0 ? HandWR : HandWL;
					if (W <= 0.001f) continue;
					const FTransform& Target = S == 0 ? In.HandR : In.HandL;
					const FVector& HintOff = S == 0 ? In.ElbowHintR : In.ElbowHintL;
					FTransform A = CS.GetComponentSpaceTransform(UpperArm[S]);
					FTransform B = CS.GetComponentSpaceTransform(LowerArm[S]);
					FTransform C = CS.GetComponentSpaceTransform(Hand[S]);
					C.SetRotation((Target.GetRotation() * C.GetRotation()).GetNormalized());
					AnimationCore::SolveTwoBoneIK(A, B, C, A.GetLocation() + HintOff, Target.GetLocation(), false, 1.0, 1.0);
					if (HasNaN(A, B, C)) continue;
					const FBoneTransform Chain[3] = { FBoneTransform(UpperArm[S], A), FBoneTransform(LowerArm[S], B), FBoneTransform(Hand[S], C) };
					CS.LocalBlendCSBoneTransforms(Chain, W);
				}
			}
			FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(CS), Pose);
		}
		return true;
	}

	virtual void PostEvaluate(UAnimInstance* InAnimInstance) override
	{
		FAnimInstanceProxy::PostEvaluate(InAnimInstance);
		if (bMissing) CastChecked<UGSBodyAnim>(InAnimInstance)->bMissingBones = true;
	}

private:
	struct FBand { bool bJog = false; float T = 0.f; };   // idle->walk (T) or walk->jog (T)

	FBand GetBand() const
	{
		FBand B;
		const float Spd = SmoothVel.Size2D();
		const float W = FMath::Max(WalkSpd, 1.f);
		// Idle fades into the walk up to the slowest walk rate (0.6); from there the walk alone, played at the
		// speed you move, so the feet do not slide.
		if (Spd <= W) { B.T = FMath::Clamp(Spd / (0.6f * W), 0.f, 1.f); return B; }
		B.bJog = true;
		B.T = FMath::Clamp((Spd - W) / FMath::Max(JogSpd - W, 50.f), 0.f, 1.f);
		return B;
	}

	const UAnimSequence* Seq(int32 Slot) const
	{
		const int32 I = FMath::Clamp<int32>(In.Set, 0, GSBodySlots::Sets - 1) * GSBodySlots::PerSet + Slot;
		return Seqs[I] ? Seqs[I] : Seqs[Slot];   // the unarmed set fills gaps
	}

	void Sample(int32 Slot, float Time, FAnimationPoseData& Out) const
	{
		const UAnimSequence* S = Seq(Slot);
		if (!S) { Out.GetPose().ResetToRefPose(); return; }
		const float Len = FMath::Max(S->GetPlayLength(), 0.01f);
		// bExtractRootMotion locks the root (the clips have root motion on): the body walks on the spot and the
		// capsule does the moving, so it never slides ahead and snaps back.
		const FAnimExtractContext Ctx(static_cast<double>(FMath::Fmod(Time, Len)), true, FDeltaTimeRecord(), true);
		S->GetAnimationPose(Out, Ctx);
	}

	void SampleMove(int32 Slot, FAnimationPoseData& Out) const
	{
		const UAnimSequence* S = Seq(Slot);
		Sample(Slot, S ? Phase * S->GetPlayLength() : 0.f, Out);
	}

	/** Blends the two nearest of the eight directions of one gait into Out (Scratch is used and left dirty). */
	void SampleDirs(int32 First, int32 DirA, int32 DirB, float Frac, FAnimationPoseData& Out, FAnimationPoseData& Scratch) const
	{
		SampleMove(First + DirA, Out);
		if (Frac > 0.01f)
		{
			SampleMove(First + DirB, Scratch);
			FAnimationRuntime::BlendTwoPosesTogetherInPlace(Out, Scratch, 1.f - Frac);
		}
	}

	void CacheBoneIndices(const FBoneContainer& BC)
	{
		if (CachedSerial == BC.GetSerialNumber() && bBonesCached) return;
		CachedSerial = BC.GetSerialNumber();
		bBonesCached = true;
		auto Find = [&BC](const TCHAR* Name)
		{
			const int32 MeshIdx = BC.GetPoseBoneIndexForBoneName(FName(Name));
			return MeshIdx == INDEX_NONE ? FCompactPoseBoneIndex(INDEX_NONE) : BC.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIdx));
		};
		Pelvis = Find(TEXT("pelvis"));
		Spine[0] = Find(TEXT("spine_01")); Spine[1] = Find(TEXT("spine_02")); Spine[2] = Find(TEXT("spine_03"));
		Spine[3] = Find(TEXT("spine_04")); Spine[4] = Find(TEXT("spine_05"));
		Neck = Find(TEXT("neck_01"));
		Thigh[0] = Find(TEXT("thigh_l")); Calf[0] = Find(TEXT("calf_l")); Foot[0] = Find(TEXT("foot_l"));
		Thigh[1] = Find(TEXT("thigh_r")); Calf[1] = Find(TEXT("calf_r")); Foot[1] = Find(TEXT("foot_r"));
		UpperArm[0] = Find(TEXT("upperarm_r")); LowerArm[0] = Find(TEXT("lowerarm_r")); Hand[0] = Find(TEXT("hand_r"));
		UpperArm[1] = Find(TEXT("upperarm_l")); LowerArm[1] = Find(TEXT("lowerarm_l")); Hand[1] = Find(TEXT("hand_l"));
		bBonesMissing = Pelvis == INDEX_NONE || Spine[2] == INDEX_NONE;
		for (int32 S = 0; S < 2; ++S)
		{
			bBonesMissing |= Thigh[S] == INDEX_NONE || Calf[S] == INDEX_NONE || Foot[S] == INDEX_NONE || Hand[S] == INDEX_NONE
				|| UpperArm[S] == INDEX_NONE || LowerArm[S] == INDEX_NONE;
		}
	}

	static constexpr float CrouchDrop = 45.f;   // cm the hips go down when crouched (the plan's 38 left the head ~7 cm high)

	FGSBodyAnimInput In;
	const UAnimSequence* Seqs[GSBodySlots::Num] = {};
	bool bSeqsMissing = true;
	bool bMissing = false;     // written on an animation thread, handed over in PostEvaluate
	float WalkSpd = 165.f, JogSpd = 380.f;
	FVector SmoothVel = FVector::ZeroVector;
	float Phase = 0.f, IdleTime = 0.f, FallTime = 0.f, FallAlpha = 0.f;
	float HandWR = 0.f, HandWL = 0.f;

	uint16 CachedSerial = 0;
	bool bBonesCached = false, bBonesMissing = false;
	FCompactPoseBoneIndex Pelvis = FCompactPoseBoneIndex(INDEX_NONE), Neck = FCompactPoseBoneIndex(INDEX_NONE);
	FCompactPoseBoneIndex Spine[5] = { FCompactPoseBoneIndex(INDEX_NONE), FCompactPoseBoneIndex(INDEX_NONE), FCompactPoseBoneIndex(INDEX_NONE),
		FCompactPoseBoneIndex(INDEX_NONE), FCompactPoseBoneIndex(INDEX_NONE) };
	// [0] left, [1] right for the legs; [0] right, [1] left for the arms (the right hand is the gun hand)
	FCompactPoseBoneIndex Thigh[2] = { FCompactPoseBoneIndex(INDEX_NONE), FCompactPoseBoneIndex(INDEX_NONE) };
	FCompactPoseBoneIndex Calf[2] = { FCompactPoseBoneIndex(INDEX_NONE), FCompactPoseBoneIndex(INDEX_NONE) };
	FCompactPoseBoneIndex Foot[2] = { FCompactPoseBoneIndex(INDEX_NONE), FCompactPoseBoneIndex(INDEX_NONE) };
	FCompactPoseBoneIndex UpperArm[2] = { FCompactPoseBoneIndex(INDEX_NONE), FCompactPoseBoneIndex(INDEX_NONE) };
	FCompactPoseBoneIndex LowerArm[2] = { FCompactPoseBoneIndex(INDEX_NONE), FCompactPoseBoneIndex(INDEX_NONE) };
	FCompactPoseBoneIndex Hand[2] = { FCompactPoseBoneIndex(INDEX_NONE), FCompactPoseBoneIndex(INDEX_NONE) };
};

// ---------------------------------------------------------------------------------------------------------------------

UGSBodyAnim::UGSBodyAnim()
{
	// The clips' root motion is never used: the capsule moves the body.
	RootMotionMode = ERootMotionMode::IgnoreRootMotion;
}

bool UGSBodyAnim::HasSequences()
{
	using namespace GSBodyAnimImpl;
	return Load(Sets[0].Idle) != nullptr && Load(FString(Sets[1].Walk) + Dirs[0]) != nullptr;
}

void UGSBodyAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	LoadSequences();
}

void UGSBodyAnim::LoadSequences()
{
	using namespace GSBodyAnimImpl;
	check(IsInGameThread());
	if (Seqs.Num() == GSBodySlots::Num) return;   // already loaded (the mesh was set again)
	Seqs.SetNum(GSBodySlots::Num);
	for (int32 Set = 0; Set < GSBodySlots::Sets; ++Set)
	{
		const int32 Base = Set * GSBodySlots::PerSet;
		Seqs[Base + GSBodySlots::Idle] = Load(Sets[Set].Idle);
		for (int32 D = 0; D < 8; ++D)
		{
			Seqs[Base + GSBodySlots::Walk + D] = Load(FString(Sets[Set].Walk) + Dirs[D]);
			Seqs[Base + GSBodySlots::Jog + D] = Load(FString(Sets[Set].Jog) + Dirs[D]);
		}
		Seqs[Base + GSBodySlots::Fall] = Load(Sets[Set].Fall);
	}
	// Gaps take the unarmed set's clip.
	for (int32 I = GSBodySlots::PerSet; I < GSBodySlots::Num; ++I)
	{
		if (!Seqs[I]) Seqs[I] = Seqs[I % GSBodySlots::PerSet];
	}

	// The body needs the unarmed idle and the rifle walk, and clips made for this mesh's skeleton.
	const USkeletalMeshComponent* Comp = GetSkelMeshComponent();
	const USkeletalMesh* Mesh = Comp ? Comp->GetSkeletalMeshAsset() : nullptr;
	const USkeleton* Skel = Mesh ? Mesh->GetSkeleton() : nullptr;
	bMissingBones = !Seqs[GSBodySlots::Idle] || !Seqs[GSBodySlots::PerSet + GSBodySlots::Walk] || !Skel;
	for (const TObjectPtr<UAnimSequence>& S : Seqs)
	{
		if (S && S->GetSkeleton() != Skel) { bMissingBones = true; break; }
	}

	// Real speeds of the clips (their root motion over one cycle), used for the play rate and the blend bands.
	const float W = MeasureSpeed(Seqs[GSBodySlots::PerSet + GSBodySlots::Walk]);
	const float J = MeasureSpeed(Seqs[GSBodySlots::PerSet + GSBodySlots::Jog]);
	if (W > 50.f) WalkSpeed = FMath::Clamp(W, 100.f, 400.f);
	if (J > 50.f) JogSpeed = FMath::Clamp(J, FMath::Max(250.f, WalkSpeed + 80.f), 800.f);
	static bool bLogged = false;
	if (!bLogged)
	{
		bLogged = true;
		UE_LOG(LogTemp, Log, TEXT("GSBodyAnim: rifle walk %.0f cm/s (measured %.0f), jog %.0f cm/s (measured %.0f)%s"),
			WalkSpeed, W, JogSpeed, J, bMissingBones ? TEXT(", MISSING clips or skeleton: old body is used") : TEXT(""));
		for (int32 Set = 0; Set < GSBodySlots::Sets; ++Set)
		{
			const UAnimSequence* WS = Seqs[Set * GSBodySlots::PerSet + GSBodySlots::Walk];
			const UAnimSequence* JS = Seqs[Set * GSBodySlots::PerSet + GSBodySlots::Jog];
			UE_LOG(LogTemp, Log, TEXT("GSBodyAnim: set %d walk fwd %.0f cm/s (%.2f s), jog fwd %.0f cm/s (%.2f s)"), Set,
				MeasureSpeed(WS), WS ? WS->GetPlayLength() : 0.f, MeasureSpeed(JS), JS ? JS->GetPlayLength() : 0.f);
		}
	}
}

FAnimInstanceProxy* UGSBodyAnim::CreateAnimInstanceProxy()
{
	return new FGSBodyAnimProxy(this);
}

void UGSBodyAnim::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FGSBodyAnimProxy*>(InProxy);
}
