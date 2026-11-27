#include "GSDoor.h"
#include "GSCharacter.h"
#include "GSEffects.h"
#include "GSUtils.h"
#include "GSAudio.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace GSDoorImpl
{
	constexpr float OpenDeg = 95.f;
	constexpr float NormalSpeed = 170.f;   // deg/s
	constexpr float FastSpeed = 650.f;     // kicked or blown open
	constexpr float FireCloseSpeed = 60.f;
	constexpr float HandleZ = 105.f;
	constexpr int32 KicksToBreak = 3;
	constexpr int32 BlowsToBreakKeypad = 2;
	constexpr float PickSeconds = 6.f;
	constexpr float ForceSeconds = 4.f;
	constexpr float KickSeconds = 1.f;
	constexpr float FireCloseDelay = 6.f;

	UMaterialInterface* DoorMat(const TCHAR* Name)
	{
		static TMap<FString, TWeakObjectPtr<UMaterialInterface>> Cache;
		static TSet<FString> Missing;
		const FString Key(Name);
		if (TWeakObjectPtr<UMaterialInterface>* Found = Cache.Find(Key))
		{
			if (Found->IsValid()) return Found->Get();
		}
		if (Missing.Contains(Key)) return nullptr;
		const FString Path = FString::Printf(TEXT("/Game/GS/Materials/MI_GS_%s.MI_GS_%s"), Name, Name);
		UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!M)
		{
			Missing.Add(Key);
			return nullptr;
		}
		Cache.Add(Key, M);
		return M;
	}

	UMaterialInterface* GlowMat()
	{
		static TWeakObjectPtr<UMaterialInterface> M;
		static bool bTried = false;
		if (!M.IsValid() && !bTried)
		{
			bTried = true;
			M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/GS/Materials/M_GS_FXAdd.M_GS_FXAdd"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
		return M.Get();
	}

	bool IsServer(const UWorld* W) { return W && W->GetNetMode() != NM_Client; }

	/** Sizes the Blender models are made at (cm): leaf width, height, thickness. */
	constexpr float ModelLeafW = 100.f;
	constexpr float ModelLeafH = 209.f;
}

UStaticMesh* AGSDoor::IntModel(const TCHAR* Name)
{
	static TMap<FString, TWeakObjectPtr<UStaticMesh>> Cache;
	static TSet<FString> Missing;
	const FString Key(Name);
	if (TWeakObjectPtr<UStaticMesh>* Found = Cache.Find(Key))
	{
		if (Found->IsValid()) return Found->Get();
	}
	if (Missing.Contains(Key)) return nullptr;
	const FString Path = FString::Printf(TEXT("/Game/Models/%s.%s"), Name, Name);
	UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!M)
	{
		Missing.Add(Key);
		return nullptr;
	}
	Cache.Add(Key, M);
	return M;
}

void AGSDoor::Sound(const TCHAR* Name, const FVector& At, float Volume, float RangeM, float Pitch) const
{
	// Kamp's names per kind of door (bestillinger.md line 6), tried first
	const TCHAR* Specific = nullptr;
	const bool bOpen = FCString::Strcmp(Name, TEXT("Door_Open")) == 0;
	const bool bSlam = FCString::Strcmp(Name, TEXT("Door_Slam")) == 0;
	const bool bClose = FCString::Strcmp(Name, TEXT("Door_Close")) == 0 || (bSlam && Def.Art != EGSDoorArt::Wood && Def.Art != EGSDoorArt::Double);
	switch (Def.Art)
	{
	case EGSDoorArt::Wood:
	case EGSDoorArt::Double:
		if (bOpen) Specific = TEXT("Door_Wood_Open");
		else if (bClose) Specific = TEXT("Door_Wood_Close");
		else if (bSlam || FCString::Strcmp(Name, TEXT("Door_Kick")) == 0) Specific = Lock == GSLock::Broken ? TEXT("Door_Wood_Break") : TEXT("Door_Wood_Kick");
		break;
	case EGSDoorArt::GlassCode:
		if (bOpen) Specific = TEXT("Door_Glass_Open");
		else if (bClose) Specific = TEXT("Door_Glass_Close");
		break;
	case EGSDoorArt::Fire:
		if (bOpen) Specific = TEXT("Door_Pushbar");
		else if (bClose) Specific = TEXT("Door_Metal_Stair");
		break;
	}
	if (FCString::Strcmp(Name, TEXT("Keypad_Beep")) == 0) Specific = TEXT("Codelock_Beep");
	if (Specific && GSAudio::Get(Specific))
	{
		GSAudio::PlayAt(this, Specific, At, Volume, RangeM, Pitch);
		return;
	}
	AGSMapState::PlayPartSound(this, Name, At, Volume, RangeM, Pitch);
}

UStaticMeshComponent* AGSDoor::AddModel(USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
{
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetStaticMesh(Mesh);
	M->SetMobility(EComponentMobility::Movable);
	M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	M->SetCanEverAffectNavigation(false);
	M->SetGenerateOverlapEvents(false);
	M->SetupAttachment(Parent);
	M->SetRelativeLocationAndRotation(Loc, Rot);
	M->SetRelativeScale3D(Scale);
	M->RegisterComponent();
	AddInstanceComponent(M);
	return M;
}


AGSDoor::AGSDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = false;
	SetCanBeDamaged(false);
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
}

UStaticMeshComponent* AGSDoor::AddBox(USceneComponent* Parent, const FVector& Center, const FVector& SizeCm, const TCHAR* MatName, bool bCollide)
{
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetStaticMesh(GSUtils::Cube());
	M->SetMobility(EComponentMobility::Movable);
	M->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	M->SetCanEverAffectNavigation(false);
	M->SetGenerateOverlapEvents(false);
	M->SetupAttachment(Parent);
	M->SetRelativeLocation(Center);
	M->SetRelativeScale3D(SizeCm / 100.f);
	if (UMaterialInterface* Mi = MatName ? GSDoorImpl::DoorMat(MatName) : nullptr) M->SetMaterial(0, Mi);
	else GSUtils::SetColor(M, FLinearColor(0.5f, 0.5f, 0.5f));
	M->RegisterComponent();
	AddInstanceComponent(M);
	return M;
}

UBoxComponent* AGSDoor::AddCollider(USceneComponent* Parent, const FVector& Center, const FVector& SizeCm)
{
	UBoxComponent* B = NewObject<UBoxComponent>(this);
	B->SetMobility(EComponentMobility::Movable);
	B->SetBoxExtent(SizeCm * 0.5f, false);
	B->SetCollisionObjectType(ECC_WorldDynamic);
	B->SetCollisionResponseToAllChannels(ECR_Block);
	B->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	B->SetCanEverAffectNavigation(false);
	B->SetGenerateOverlapEvents(false);
	B->SetupAttachment(Parent);
	B->SetRelativeLocation(Center);
	B->RegisterComponent();
	AddInstanceComponent(B);
	Blockers.Add(B);
	return B;
}

void AGSDoor::Init(const FGSDoorDef& InDef, int32 InKey)
{
	Def = InDef;
	Key = InKey;
	SetActorLocationAndRotation(Def.BaseCm, FRotator(0.f, Def.YawDeg, 0.f));
	const float W = Def.WidthCm;
	const float LeafH = Def.HeightCm - 2.f;
	Leaves.Reset();
	if (Def.Art == EGSDoorArt::Double)
	{
		Leaves.Add({ nullptr, 1.f, W * 0.5f - 1.5f });
		Leaves.Add({ nullptr, -1.f, W * 0.5f - 1.5f });
	}
	else
	{
		Leaves.Add({ nullptr, Def.bHingeLeft ? 1.f : -1.f, W - 2.f });
	}
	for (FLeaf& L : Leaves)
	{
		USceneComponent* P = NewObject<USceneComponent>(this);
		P->SetMobility(EComponentMobility::Movable);
		P->SetupAttachment(Root);
		P->SetRelativeLocation(FVector(0.f, L.HingeSide * W * 0.5f, 0.f));
		P->RegisterComponent();
		AddInstanceComponent(P);
		L.Pivot = P;
		BuildLeaf(L, LeafH);
	}
	if (Def.Art == EGSDoorArt::GlassCode) PlaceKeypad();
	ResetLocal();
}

void AGSDoor::BuildLeaf(FLeaf& L, float LeafH)
{
	// Along the leaf: a = 0 at the hinge .. Width at the free edge; y(a) = -HingeSide * (a + 0.5)
	auto Y = [&L](float A) { return -L.HingeSide * (A + 0.5f); };
	const float Mid = L.Width * 0.5f;
	const float Z0 = 1.f;
	// The Blender models (SM_Int_*) replace the boxes when they have been imported; the boxes are the fallback.
	UStaticMesh* HandleModel = IntModel(TEXT("SM_Int_Handle"));
	auto Handle = [&](float Side, float FaceX, float A)
	{
		// Model: origin on the door face at the rose, sticks out along +X, the lever points along +Y (made for the
		// front). The lever points towards the hinge (+HingeSide in local Y) on both faces.
		const FVector At(Side * FaceX, Y(A), Z0 + GSDoorImpl::HandleZ);
		if (HandleModel)
		{
			AddModel(L.Pivot, HandleModel, At, FRotator(0.f, Side > 0.f ? 0.f : 180.f, 0.f), FVector(1.f, Side > 0.f ? L.HingeSide : -L.HingeSide, 1.f));
			return;
		}
		AddBox(L.Pivot, At + FVector(Side * 3.f, 0.f, 0.f), FVector(2.f, 14.f, 2.f), TEXT("M_Chrome_In"), false);
		AddBox(L.Pivot, At + FVector(Side * 0.4f, 0.f, 0.f), FVector(0.8f, 6.f, 14.f), TEXT("M_Chrome_In"), false);
	};
	switch (Def.Art)
	{
	case EGSDoorArt::GlassCode:
	{
		const float T = 5.f, Stile = 9.f, Bottom = 20.f, Top = 10.f;
		const TCHAR* Frame = TEXT("Metal_In");
		const FVector StileSize(T, Stile, LeafH);
		const FVector RailB(T, L.Width - 2.f * Stile, Bottom), RailT(T, L.Width - 2.f * Stile, Top);
		const FVector S1(0.f, Y(Stile * 0.5f), Z0 + LeafH * 0.5f), S2(0.f, Y(L.Width - Stile * 0.5f), Z0 + LeafH * 0.5f);
		const FVector B(0.f, Y(Mid), Z0 + Bottom * 0.5f), Tp(0.f, Y(Mid), Z0 + LeafH - Top * 0.5f);
		if (UStaticMesh* FrameModel = IntModel(TEXT("SM_Int_Door_GlassFrame")))
		{
			AddModel(L.Pivot, FrameModel, FVector(0.f, Y(Mid), Z0), FRotator::ZeroRotator,
				FVector(1.f, L.Width / GSDoorImpl::ModelLeafW, LeafH / GSDoorImpl::ModelLeafH));
		}
		else
		{
			AddBox(L.Pivot, S1, StileSize, Frame, false);
			AddBox(L.Pivot, S2, StileSize, Frame, false);
			AddBox(L.Pivot, B, RailB, Frame, false);
			AddBox(L.Pivot, Tp, RailT, Frame, false);
		}
		FrameColliders.Add(AddCollider(L.Pivot, S1, StileSize));
		FrameColliders.Add(AddCollider(L.Pivot, S2, StileSize));
		FrameColliders.Add(AddCollider(L.Pivot, B, RailB));
		FrameColliders.Add(AddCollider(L.Pivot, Tp, RailT));
		// The pane: real glass collision (Destructible: blocks bodies, lets the Visibility channel through). Tempered:
		// any hit breaks it (1b), and then the frame lets you step through.
		const float PaneH = LeafH - Bottom - Top;
		UStaticMeshComponent* Pane = AddBox(L.Pivot, FVector(0.f, Y(Mid), Z0 + Bottom + PaneH * 0.5f), FVector(1.2f, L.Width - 2.f * Stile, PaneH), TEXT("Glass"), true);
		Pane->SetCollisionObjectType(ECC_Destructible);
		Pane->SetCollisionResponseToAllChannels(ECR_Ignore);
		Pane->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Pane->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
		Pane->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		Pane->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);
		Pane->SetCastShadow(false);
		Pane->ComponentTags.Add(TEXT("Mat_Glass"));
		Pane->ComponentTags.Add(TEXT("GS_Int_Glass"));
		Blockers.Add(Pane);
		Panes.Add(Pane);
		// Lever handles on both sides
		for (const float S : { 1.f, -1.f })
		{
			Handle(S, T * 0.5f, L.Width - Stile * 0.5f - 6.f);
		}
		break;
	}
	case EGSDoorArt::Fire:
	{
		const float T = 5.f;
		const FVector C(0.f, Y(Mid), Z0 + LeafH * 0.5f), S(T, L.Width, LeafH);
		if (UStaticMesh* FireModel = IntModel(TEXT("SM_Int_Door_Fire")))
		{
			// Leaf, push bar (front = +X) and the wired-glass window in one model
			AddModel(L.Pivot, FireModel, FVector(0.f, Y(Mid), Z0), FRotator::ZeroRotator,
				FVector(1.f, L.Width / GSDoorImpl::ModelLeafW, LeafH / GSDoorImpl::ModelLeafH));
		}
		else
		{
			AddBox(L.Pivot, C, S, TEXT("MetalDark_In"), false);
			// Push bar on the escape side (front)
			AddBox(L.Pivot, FVector(T * 0.5f + 5.f, Y(Mid), Z0 + 100.f), FVector(4.f, L.Width - 24.f, 5.f), TEXT("M_Chrome_In"), false);
			// A small window (wired glass), real fire doors have one
			AddBox(L.Pivot, FVector(0.f, Y(Mid), Z0 + 150.f), FVector(T + 0.4f, 22.f, 50.f), TEXT("GlassDark"), false);
		}
		AddCollider(L.Pivot, C, S);
		// A lever on the other side
		Handle(-1.f, T * 0.5f, L.Width - 9.f);
		break;
	}
	default:
	{
		const float T = 4.5f;
		const FVector C(0.f, Y(Mid), Z0 + LeafH * 0.5f), S(T, L.Width, LeafH);
		UStaticMesh* WoodModel = Def.Art == EGSDoorArt::Wood ? IntModel(TEXT("SM_Int_Door_Wood")) : nullptr;
		if (WoodModel)
		{
			AddModel(L.Pivot, WoodModel, FVector(0.f, Y(Mid), Z0), FRotator::ZeroRotator,
				FVector(T / 4.f, L.Width / GSDoorImpl::ModelLeafW, LeafH / GSDoorImpl::ModelLeafH));
		}
		else
		{
			AddBox(L.Pivot, C, S, Def.Art == EGSDoorArt::Double ? TEXT("PlasticWhite_In") : TEXT("WoodLight_In"), false);
		}
		AddCollider(L.Pivot, C, S);
		for (const float Sd : { 1.f, -1.f })
		{
			Handle(Sd, T * 0.5f, L.Width - 9.f);
		}
		break;
	}
	}
}

void AGSDoor::PlaceKeypad()
{
	// On the wall by the handle, on the outside (front), 1.1 m up. Find the wall's face so it sits on it.
	const float HingeSide = Leaves.Num() > 0 ? Leaves[0].HingeSide : 1.f;
	const float LocalY = -HingeSide * (Def.WidthCm * 0.5f + 14.f);
	const FVector N = GetActorForwardVector(), R = GetActorRightVector();
	const FVector Probe = Def.BaseCm + R * LocalY + FVector(0.f, 0.f, 110.f);
	float Face = 7.f;
	FHitResult H;
	FCollisionQueryParams QP(SCENE_QUERY_STAT(GSKeypadWall), false, this);
	if (GetWorld()->LineTraceSingleByObjectType(H, Probe + N * 60.f, Probe - N * 20.f, FCollisionObjectQueryParams(ECC_WorldStatic), QP))
	{
		Face = FVector::DotProduct(H.ImpactPoint - Def.BaseCm, N);
	}
	const FVector Local(Face + 1.5f, LocalY, 110.f);
	KeypadBox = NewObject<UBoxComponent>(this);
	KeypadBox->SetMobility(EComponentMobility::Movable);
	KeypadBox->SetBoxExtent(FVector(1.5f, 4.f, 7.f), false);
	KeypadBox->SetCollisionObjectType(ECC_WorldDynamic);
	KeypadBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	KeypadBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);   // bullets and Interaktiv.Knus hit it
	KeypadBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	KeypadBox->SetCanEverAffectNavigation(false);
	KeypadBox->SetGenerateOverlapEvents(false);
	KeypadBox->ComponentTags.Add(TEXT("GS_Int_Keypad"));
	KeypadBox->SetupAttachment(Root);
	KeypadBox->SetRelativeLocation(Local);
	KeypadBox->RegisterComponent();
	AddInstanceComponent(KeypadBox);
	if (UStaticMesh* KeypadModel = IntModel(TEXT("SM_Int_Keypad")))
	{
		// Model: origin at the back (on the wall), front +X, 8 x 14 x 3 cm, the LED spot at the top
		AddModel(Root, KeypadModel, Local - FVector(1.5f, 0.f, 0.f), FRotator::ZeroRotator, FVector(1.f));
	}
	else
	{
		AddBox(Root, Local, FVector(3.f, 8.f, 14.f), TEXT("MetalDark_In"), false);
		AddBox(Root, Local + FVector(1.55f, 0.f, -1.f), FVector(0.2f, 6.f, 7.f), TEXT("Plastic_In"), false);   // the key field
	}
	KeypadLed = AddBox(Root, Local + FVector(1.6f, 0.f, 5.f), FVector(0.4f, 1.2f, 1.2f), nullptr, false);
	KeypadLed->SetCastShadow(false);
	if (UMaterialInterface* G = GSDoorImpl::GlowMat())
	{
		LedMID = UMaterialInstanceDynamic::Create(G, this);
		LedMID->SetScalarParameterValue(TEXT("Soft"), 0.01f);
		KeypadLed->SetMaterial(0, LedMID);
	}
}

void AGSDoor::UpdateLed()
{
	if (!KeypadLed) return;
	const FLinearColor C = Lock == GSLock::Locked ? FLinearColor(1.f, 0.08f, 0.05f)
		: (Lock == GSLock::Broken ? FLinearColor(0.02f, 0.02f, 0.02f) : FLinearColor(0.1f, 1.f, 0.2f));
	if (LedMID)
	{
		LedMID->SetVectorParameterValue(TEXT("Color"), C);
		LedMID->SetScalarParameterValue(TEXT("Glow"), Lock == GSLock::Broken ? 0.f : 8.f);
	}
	else
	{
		GSUtils::SetColor(KeypadLed, C);
	}
}

FVector AGSDoor::LeafCenter(int32 Index) const
{
	if (!Leaves.IsValidIndex(Index) || !Leaves[Index].Pivot) return GetActorLocation();
	const FLeaf& L = Leaves[Index];
	return L.Pivot->GetComponentTransform().TransformPosition(FVector(0.f, -L.HingeSide * L.Width * 0.5f, GSDoorImpl::HandleZ));
}

FVector AGSDoor::HandleWorld() const
{
	if (!Leaves.IsValidIndex(0) || !Leaves[0].Pivot) return GetActorLocation() + FVector(0.f, 0.f, GSDoorImpl::HandleZ);
	const FLeaf& L = Leaves[0];
	return L.Pivot->GetComponentTransform().TransformPosition(FVector(0.f, -L.HingeSide * (L.Width - 9.f), GSDoorImpl::HandleZ));
}

bool AGSDoor::IsKeypad(const UPrimitiveComponent* C) const
{
	return C && KeypadBox && C == static_cast<const UPrimitiveComponent*>(KeypadBox.Get());
}

bool AGSDoor::IsPane(const UPrimitiveComponent* C) const
{
	if (!C) return false;
	for (const TObjectPtr<UStaticMeshComponent>& P : Panes)
	{
		if (C == static_cast<const UPrimitiveComponent*>(P.Get())) return true;
	}
	return false;
}

bool AGSDoor::GetPaneWorld(FTransform& OutPane, FVector& OutSizeCm) const
{
	if (Panes.Num() == 0 || !Panes[0]) return false;
	OutPane = Panes[0]->GetComponentTransform();
	// The pane is a unit cube scaled to (thickness, width, height): turn it so X runs along the width, like a glass set pane
	const FVector S = OutPane.GetScale3D() * 100.f;
	OutSizeCm = FVector(S.Y, S.X, S.Z);
	OutPane = FTransform(FRotator(0.f, 90.f, 0.f)) * FTransform(OutPane.GetRotation(), OutPane.GetLocation());
	OutPane.SetScale3D(OutSizeCm / 100.f);
	return true;
}

void AGSDoor::SetPaneBroken(bool bBroken)
{
	for (UStaticMeshComponent* P : Panes)
	{
		if (!P) continue;
		P->SetVisibility(!bBroken);
		P->SetCollisionEnabled(bBroken ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}
	// Forces the pawn responses to be set again (the frame lets pawns through while the pane is gone)
	bMovingCollision = !bMovingCollision;
	SetMovingCollision(!bMovingCollision);
}

void AGSDoor::ServerPaneHit(const FVector& Dir, uint8 Cause)
{
	if (Panes.Num() == 0 || IsPaneBroken()) return;
	AGSMapState* MS = AGSMapState::Get(this);
	if (!MS) return;
	const FVector D = Dir.GetSafeNormal();
	MS->SetPart(Key, [D, Cause](FGSPartState& P) { P.State |= GSLock::PaneBroken; P.Cause = Cause; P.HitDir = D; });
	GSMapState::OnWalkableChanged().Broadcast(GetWorld(), GetOpeningBox());
	GSMapState::OnNoise().Broadcast(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 100.f), 40.f);
	UE_LOG(LogTemp, Log, TEXT("Interaktiv: glassruta i dør %d knust"), GSPart::IdOf(Key));
}

int32 AGSDoor::SideOf(const FVector& P) const
{
	return FVector::DotProduct(P - GetActorLocation(), GetActorForwardVector()) >= 0.f ? 1 : -1;
}

FBox AGSDoor::GetOpeningBox() const
{
	const float E = Def.WidthCm * 0.5f + 30.f;
	const FVector B = GetActorLocation();
	return FBox(B - FVector(E, E, 0.f), B + FVector(E, E, Def.HeightCm));
}

FGSDoorInfo AGSDoor::GetInfo() const
{
	FGSDoorInfo I;
	I.Key = Key;
	I.LocationCm = GetActorLocation();
	I.Normal = GetActorForwardVector();
	I.WidthCm = Def.WidthCm;
	I.Art = Def.Art;
	I.Lock = Lock;
	I.bOpen = TargetOpen != 0;
	I.Door = const_cast<AGSDoor*>(this);
	return I;
}

FString AGSDoor::DebugText() const
{
	static const TCHAR* Arts[] = { TEXT("tre"), TEXT("glass/kodelås"), TEXT("dobbel"), TEXT("brann") };
	static const TCHAR* Locks[] = { TEXT("ulåst"), TEXT("låst"), TEXT("ødelagt"), TEXT("?") };
	return FString::Printf(TEXT("Dør %d (%s%s) %s, åpen %d %%, spark %d%s"), GSPart::IdOf(Key), Arts[static_cast<int32>(Def.Art)],
		Def.Room.IsEmpty() ? TEXT("") : *(TEXT(", ") + Def.Room), Locks[Lock & 3], static_cast<int32>(TargetOpen), static_cast<int32>(Health),
		Flags ? TEXT(", noen dirker/bryter") : TEXT(""));
}

FGSPartState AGSDoor::MakeDefault() const
{
	FGSPartState S;
	S.Key = Key;
	S.State = (Def.bLocked || Def.Art == EGSDoorArt::GlassCode) ? GSLock::Locked : GSLock::Open;
	S.Open = (Def.bOpen && S.State == GSLock::Open) ? -100 : 0;
	return S;
}

void AGSDoor::ResetLocal()
{
	PredictUntil = -1.f;
	AutoCloseAt = -1.f;
	ApplyState(MakeDefault(), false);
}

// ---------------------------------------------------------------------------------------------------------------------
// State and motion

void AGSDoor::ApplyState(const FGSPartState& S, bool bFresh)
{
	const uint8 OldLock = Lock;
	const int8 OldOpen = ServerOpen;
	const uint8 OldHealth = Health;
	const bool bPredicted = PredictUntil > 0.f && TargetOpen == S.Open;
	const uint8 OldBits = StateBits;
	Lock = S.State & GSLock::Mask;
	StateBits = S.State;
	// The glass door's pane (1b): gone or back. Old news (late client, catching up) only leaves the shards on the floor.
	auto PaneChange = [this, OldBits, &S, bFresh]()
	{
		const bool bWas = (OldBits & GSLock::PaneBroken) != 0;
		const bool bNow = (S.State & GSLock::PaneBroken) != 0;
		if (bWas == bNow || Panes.Num() == 0) return;
		FTransform PaneT;
		FVector Size;
		const bool bHave = GetPaneWorld(PaneT, Size);
		SetPaneBroken(bNow);
		AGSMapState* MS = AGSMapState::Get(this);
		if (bNow && bHave && MS) MS->GlassBreakFX(Key, PaneT, Size, EGSGlass::DoorPane, S.HitDir, bFresh);
	};
	Health = S.Health;
	Flags = S.Flags;
	ServerOpen = S.Open;
	PredictUntil = -1.f;
	TargetOpen = S.Open;

	UWorld* W = GetWorld();
	if (!bFresh)
	{
		AutoCloseAt = -1.f;   // a door standing open at the start stays open
		CurOpen = TargetOpen;
		SpeedDegS = GSDoorImpl::NormalSpeed;
		bSoundOnClose = false;
		UpdatePose();
		SetMovingCollision(false);
		PaneChange();
		UpdateLed();
		UpdateTick();
		return;
	}
	PaneChange();

	// Server: a fire door that was opened closes by itself after a while
	if (GSDoorImpl::IsServer(W) && Def.Art == EGSDoorArt::Fire)
	{
		if (S.Open != 0 && (OldOpen == 0 || AutoCloseAt < 0.f)) AutoCloseAt = W->GetTimeSeconds() + GSDoorImpl::FireCloseDelay;
		if (S.Open == 0) AutoCloseAt = -1.f;
	}

	const bool bFast = S.Cause == GSCause::Kick || S.Cause == GSCause::Blast || S.Cause == GSCause::Melee;
	SpeedDegS = bFast ? GSDoorImpl::FastSpeed : (S.Cause == GSCause::AutoClose ? GSDoorImpl::FireCloseSpeed : GSDoorImpl::NormalSpeed);
	const FVector At = HandleWorld();
	if (OldOpen == 0 && S.Open != 0 && !bPredicted)
	{
		if (bFast) Sound(TEXT("Door_Slam"), At, 1.f, 40.f);
		else Sound(TEXT("Door_Open"), At, 0.8f, 18.f);
	}
	if (S.Open == 0 && OldOpen != 0 && !bPredicted)
	{
		bSoundOnClose = true;
		bSlamOnClose = S.Cause == GSCause::AutoClose;
	}
	if (Health > OldHealth && Lock == GSLock::Locked && S.Open == 0)
	{
		if (S.Cause == GSCause::Melee && Def.Art == EGSDoorArt::GlassCode && KeypadBox)
		{
			Sound(TEXT("Keypad_Break"), KeypadBox->GetComponentLocation(), 0.6f, 25.f);
		}
		else
		{
			Sound(TEXT("Door_Kick"), At, 1.f, 40.f);
		}
	}
	if (OldLock == GSLock::Locked && Lock == GSLock::Open)
	{
		Sound(TEXT("Keypad_Beep"), KeypadBox ? KeypadBox->GetComponentLocation() : At, 0.7f, 12.f);
	}
	if (OldLock != GSLock::Broken && Lock == GSLock::Broken)
	{
		if (KeypadBox)
		{
			Sound(TEXT("Keypad_Break"), KeypadBox->GetComponentLocation(), 1.f, 30.f);
			SparksAt(KeypadBox->GetComponentLocation());
		}
		else if (S.Open == 0)
		{
			Sound(TEXT("Door_Kick"), At, 1.f, 40.f, 0.85f);
		}
	}
	UpdateLed();
	UpdateTick();
}

void AGSDoor::UpdatePose()
{
	for (const FLeaf& L : Leaves)
	{
		if (L.Pivot) L.Pivot->SetRelativeRotation(FRotator(0.f, CurOpen / 100.f * GSDoorImpl::OpenDeg * L.HingeSide, 0.f));
	}
}

void AGSDoor::SetMovingCollision(bool bMoving)
{
	if (bMoving == bMovingCollision) return;
	bMovingCollision = bMoving;
	const bool bPaneGone = IsPaneBroken();
	for (UPrimitiveComponent* P : Blockers)
	{
		if (!P) continue;
		// A glass door without its pane: you step through the frame (the bottom rail is a step, the stiles are thin)
		const bool bThrough = bPaneGone && FrameColliders.Contains(P);
		P->SetCollisionResponseToChannel(ECC_Pawn, (bMoving || bThrough) ? ECR_Ignore : ECR_Block);
	}
}

void AGSDoor::UpdateTick()
{
	const bool bServerTimer = GSDoorImpl::IsServer(GetWorld()) && AutoCloseAt > 0.f;
	SetActorTickEnabled(!FMath::IsNearlyEqual(CurOpen, static_cast<float>(TargetOpen)) || PredictUntil > 0.f || Flags != 0 || bServerTimer);
}

void AGSDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* W = GetWorld();
	const float Now = W->GetTimeSeconds();

	const float Target = TargetOpen;
	if (!FMath::IsNearlyEqual(CurOpen, Target))
	{
		SetMovingCollision(true);
		CurOpen = FMath::FInterpConstantTo(CurOpen, Target, DeltaSeconds, SpeedDegS / GSDoorImpl::OpenDeg * 100.f);
		UpdatePose();
		if (FMath::IsNearlyEqual(CurOpen, Target))
		{
			CurOpen = Target;
			SetMovingCollision(false);
			if (Target == 0.f && bSoundOnClose)
			{
				Sound(bSlamOnClose ? TEXT("Door_Slam") : TEXT("Door_Close"), HandleWorld(), bSlamOnClose ? 0.8f : 0.7f, bSlamOnClose ? 30.f : 18.f);
			}
			bSoundOnClose = false;
			bSlamOnClose = false;
		}
	}

	// The owner's guess ran out without word from the server: back to what the server says
	if (PredictUntil > 0.f && Now > PredictUntil)
	{
		PredictUntil = -1.f;
		TargetOpen = ServerOpen;
		SpeedDegS = GSDoorImpl::NormalSpeed;
	}

	// Server: fire doors close by themselves, but never onto someone standing in the doorway
	if (AutoCloseAt > 0.f && Now >= AutoCloseAt && GSDoorImpl::IsServer(W))
	{
		AutoCloseAt = -1.f;
		if (ServerOpen != 0)
		{
			const FVector Center = GetActorLocation() + FVector(0.f, 0.f, Def.HeightCm * 0.5f);
			const FVector Extent(Def.WidthCm, Def.WidthCm * 0.5f, Def.HeightCm * 0.5f);
			const bool bBlocked = W->OverlapAnyTestByObjectType(Center, GetActorQuat(), FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeBox(Extent));
			if (bBlocked)
			{
				AutoCloseAt = Now + 1.f;
			}
			else if (AGSMapState* MS = AGSMapState::Get(this))
			{
				MS->SetPart(Key, [](FGSPartState& S) { S.Open = 0; S.Cause = GSCause::AutoClose; });
			}
		}
	}

	// Someone picking or breaking the code lock: everyone near hears it
	if ((Flags & GSDoorFlag::Picking) && Now >= NextPickSound)
	{
		NextPickSound = Now + FMath::FRandRange(0.8f, 1.4f);
		Sound(TEXT("Lock_Pick"), KeypadBox ? KeypadBox->GetComponentLocation() : HandleWorld(), 0.8f, 12.f);
	}
	if ((Flags & GSDoorFlag::Forcing) && Now >= NextForceSound)
	{
		NextForceSound = Now + 1.f;
		const FVector At = KeypadBox ? KeypadBox->GetComponentLocation() : HandleWorld();
		Sound(TEXT("Keypad_Break"), At, 0.9f, 30.f, FMath::FRandRange(0.9f, 1.1f));
		SparksAt(At);
	}
	UpdateTick();
}

void AGSDoor::SparksAt(const FVector& Loc)
{
	UWorld* W = GetWorld();
	if (!W || W->GetNetMode() == NM_DedicatedServer) return;
	const FVector N = GetActorForwardVector();
	for (int32 I = 0; I < 7; ++I)
	{
		const FVector V = (N * 0.8f + FMath::VRand() * 0.7f).GetSafeNormal() * FMath::FRandRange(150.f, 400.f);
		AGSShortFX::SpawnSprite(W, Loc + N * 2.f, 2.5f, 0.6f, FLinearColor(1.f, 0.55f, 0.15f), 1.f, FMath::FRandRange(0.15f, 0.35f), V, true, 18.f, -300.f, 1.f);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Use

FVector AGSDoor::GetUsePoint(const FVector& UserEye) const
{
	const int32 Side = SideOf(UserEye);
	if (TargetOpen != 0 || FMath::Abs(CurOpen) > 10.f)
	{
		// Open: the leaf (the one nearest you on a double door)
		FVector Best = LeafCenter(0);
		if (Leaves.Num() > 1)
		{
			const FVector Other = LeafCenter(1);
			if (FVector::DistSquared(Other, UserEye) < FVector::DistSquared(Best, UserEye)) Best = Other;
		}
		return Best;
	}
	return GetActorLocation() + GetActorForwardVector() * (Side * 4.f) + FVector(0.f, 0.f, GSDoorImpl::HandleZ);
}

bool AGSDoor::GetUsePrompt(const AGSCharacter* User, FGSUsePrompt& Out) const
{
	if (!User) return false;
	const int32 Side = SideOf(User->GetPawnViewLocation());
	const FLinearColor White(0.92f, 0.92f, 0.92f), Yellow(1.f, 0.85f, 0.4f);
	Out = FGSUsePrompt();
	Out.Color = White;
	if (TargetOpen != 0)
	{
		Out.Main = TEXT("[%K%]  Lukk døra");
		return true;
	}
	if (Lock != GSLock::Locked)
	{
		Out.Main = TEXT("[%K%]  Åpne døra");
		Out.Sub = TEXT("Hold [%K%]: spark opp");
		Out.bHasHold = true;
		return true;
	}
	if (Side == EscapeSide())
	{
		Out.Main = TEXT("[%K%]  Åpne døra");
		Out.Sub = Def.Art == EGSDoorArt::Fire ? TEXT("Panikkstang (låst fra den andre siden)") : TEXT("Dørvrider innenfra");
		return true;
	}
	if (IsPaneBroken())
	{
		Out.Main = TEXT("[%K%]  Åpne døra");
		Out.Sub = TEXT("Ruta er knust: grip inn til dørvrideren, eller gå gjennom");
		return true;
	}
	Out.Color = Yellow;
	switch (Def.Art)
	{
	case EGSDoorArt::GlassCode:
	{
		const bool bCard = User->HasItemKind(TEXT("key"));
		const bool bPick = User->HasItemKind(TEXT("lockpick"));
		const FString Room = Def.Room.IsEmpty() ? FString(TEXT("Grupperom")) : Def.Room;
		Out.Main = FString::Printf(TEXT("%s – låst med kodelås"), *Room);
		const FString Hold = bPick ? TEXT("Hold [%K%]: dirk opp") : TEXT("Hold [%K%]: bryt opp kodelåsen (høylytt)");
		Out.Sub = bCard ? FString(TEXT("[%K%] adgangskort  ·  ")) + Hold : Hold;
		Out.bHasHold = true;
		return true;
	}
	case EGSDoorArt::Fire:
		Out.Main = TEXT("Alarmdør – låst");
		Out.Sub = TEXT("Kan ikke åpnes fra denne siden");
		return true;
	default:
		Out.Main = TEXT("Låst dør");
		Out.Sub = TEXT("Hold [%K%]: spark inn");
		Out.bHasHold = true;
		return true;
	}
}

void AGSDoor::ServerUse(AGSCharacter* User, EGSUseVerb Verb, FGSUseResult& Out)
{
	AGSMapState* MS = AGSMapState::Get(this);
	if (!MS || !User) return;
	FGSPartState S;
	if (!MS->GetPart(Key, S)) return;
	const uint8 L = S.State & GSLock::Mask;
	const FVector Eye = User->GetPawnViewLocation();
	const int32 Side = SideOf(Eye);
	const int8 Away = static_cast<int8>(-Side * 100);
	const float Now = MS->ServerNow();
	const bool bOpen = S.Open != 0;
	UWorld* W = GetWorld();

	auto StartJob = [&](uint8 Kind, float Seconds)
	{
		Out.Job.Key = Key;
		Out.Job.Kind = Kind;
		Out.Job.Start = Now;
		Out.Job.End = Now + Seconds;
		Out.Job.SnapLock = L;
		Out.Job.SnapOpen = S.Open;
		Out.Job.NextBeat = Now + 1.f;
	};

	if (Verb == EGSUseVerb::Tap)
	{
		if (bOpen)
		{
			MS->SetPart(Key, [](FGSPartState& P) { P.Open = 0; P.Cause = GSCause::Use; });
			return;
		}
		if (L != GSLock::Locked)
		{
			MS->SetPart(Key, [Away](FGSPartState& P) { P.Open = Away; P.Cause = GSCause::Use; });
			return;
		}
		if (Side == EscapeSide() || (S.State & GSLock::PaneBroken))
		{
			// Handle or push bar from the inside (or reaching in through the broken pane): it opens, the lock stays locked
			MS->SetPart(Key, [Away](FGSPartState& P) { P.Open = Away; P.Cause = GSCause::Escape; });
			return;
		}
		switch (Def.Art)
		{
		case EGSDoorArt::GlassCode:
			if (User->HasItemKind(TEXT("key")))
			{
				MS->SetPart(Key, [Away](FGSPartState& P) { P.State = (P.State & ~GSLock::Mask) | GSLock::Open; P.Open = Away; P.Cause = GSCause::Card; });
				GSMapState::OnWalkableChanged().Broadcast(W, GetOpeningBox());
			}
			else
			{
				Out.Feedback = TEXT("Låst. Kodelås.");
				Out.FeedbackSound = TEXT("Keypad_Error");
			}
			return;
		case EGSDoorArt::Fire:
			Out.Feedback = TEXT("Alarmdør. Kan ikke åpnes herfra.");
			return;
		default:
			Out.Feedback = TEXT("Låst.");
			Out.FeedbackSound = TEXT("Door_Kick");
			return;
		}
	}

	if (Verb == EGSUseVerb::HoldStart)
	{
		if (bOpen) return;
		if (L != GSLock::Locked)
		{
			ServerOpenAway(Eye, GSCause::Kick, false, true);
			return;
		}
		if (Side == EscapeSide())
		{
			MS->SetPart(Key, [Away](FGSPartState& P) { P.Open = Away; P.Cause = GSCause::Escape; });
			return;
		}
		switch (Def.Art)
		{
		case EGSDoorArt::GlassCode:
			if (User->HasItemKind(TEXT("lockpick"))) StartJob(GSJob::Pick, GSDoorImpl::PickSeconds);
			else StartJob(GSJob::Force, GSDoorImpl::ForceSeconds);
			return;
		case EGSDoorArt::Fire:
			Out.Feedback = TEXT("Alarmdør. Kan ikke åpnes herfra.");
			return;
		default:
			StartJob(GSJob::Kick, GSDoorImpl::KickSeconds);
			return;
		}
	}
}

void AGSDoor::ServerJobStarted(AGSCharacter* User, FGSUseJob& InJob)
{
	AGSMapState* MS = AGSMapState::Get(this);
	if (!MS) return;
	const uint8 F = InJob.Kind == GSJob::Pick ? GSDoorFlag::Picking : (InJob.Kind == GSJob::Force ? GSDoorFlag::Forcing : 0);
	if (F) MS->SetPart(Key, [F](FGSPartState& P) { P.Flags |= F; P.Cause = GSCause::Flags; });
	if (InJob.Kind == GSJob::Force) GSMapState::OnNoise().Broadcast(GetWorld(), HandleWorld(), 30.f);
}

bool AGSDoor::ServerJobTick(AGSCharacter* User, FGSUseJob& InJob, float Now)
{
	// Someone else changed the door (opened it from the inside, blew it open ...): the job is over
	if (Lock != InJob.SnapLock || ServerOpen != InJob.SnapOpen) return false;
	if (InJob.Kind == GSJob::Force && Now >= InJob.NextBeat)
	{
		InJob.NextBeat += 1.f;
		GSMapState::OnNoise().Broadcast(GetWorld(), HandleWorld(), 30.f);
	}
	return true;
}

bool AGSDoor::ServerJobDone(AGSCharacter* User, const FGSUseJob& InJob, bool bCompleted, FGSUseJob& Next)
{
	AGSMapState* MS = AGSMapState::Get(this);
	if (!MS) return false;
	UWorld* W = GetWorld();
	const uint8 Clear = GSDoorFlag::Picking | GSDoorFlag::Forcing;
	if (!bCompleted)
	{
		if (Flags & Clear) MS->SetPart(Key, [Clear](FGSPartState& P) { P.Flags &= ~Clear; P.Cause = GSCause::Flags; });
		return false;
	}
	switch (InJob.Kind)
	{
	case GSJob::Pick:
		MS->SetPart(Key, [Clear](FGSPartState& P) { P.State = (P.State & ~GSLock::Mask) | GSLock::Open; P.Flags &= ~Clear; P.Cause = GSCause::Pick; });
		GSMapState::OnWalkableChanged().Broadcast(W, GetOpeningBox());
		UE_LOG(LogTemp, Log, TEXT("Interaktiv: kodelås dirket opp (dør %d)"), GSPart::IdOf(Key));
		return false;
	case GSJob::Force:
		MS->SetPart(Key, [Clear](FGSPartState& P) { P.State = (P.State & ~GSLock::Mask) | GSLock::Broken; P.Flags &= ~Clear; P.Cause = GSCause::Force; });
		GSMapState::OnWalkableChanged().Broadcast(W, GetOpeningBox());
		GSMapState::OnNoise().Broadcast(W, HandleWorld(), 30.f);
		UE_LOG(LogTemp, Log, TEXT("Interaktiv: kodelås brutt opp (dør %d)"), GSPart::IdOf(Key));
		return false;
	case GSJob::Kick:
	{
		const FVector From = User ? User->GetActorLocation() : GetActorLocation() + GetActorForwardVector() * 100.f;
		const bool bBreaks = Health + 1 >= GSDoorImpl::KicksToBreak;
		if (bBreaks)
		{
			ServerOpenAway(From, GSCause::Kick, true, true);
		}
		else
		{
			MS->SetPart(Key, [](FGSPartState& P) { P.Health = static_cast<uint8>(FMath::Min(250, P.Health + 1)); P.Cause = GSCause::Kick; });
		}
		GSMapState::OnNoise().Broadcast(W, HandleWorld(), 40.f);
		if (bBreaks)
		{
			UE_LOG(LogTemp, Log, TEXT("Interaktiv: låst dør sparket inn (dør %d)"), GSPart::IdOf(Key));
			return false;
		}
		// Keep kicking while the key is held
		Next = InJob;
		Next.Start = MS->ServerNow();
		Next.End = Next.Start + GSDoorImpl::KickSeconds;
		Next.SnapLock = Lock;
		Next.SnapOpen = ServerOpen;
		return true;
	}
	default:
		return false;
	}
}

FString AGSDoor::JobLabel(uint8 Kind) const
{
	switch (Kind)
	{
	case GSJob::Pick: return TEXT("Dirker opp kodelåsen ...");
	case GSJob::Force: return TEXT("Bryter opp kodelåsen ...");
	case GSJob::Kick: return FString::Printf(TEXT("Sparker inn døra ... (%d/%d)"), FMath::Min<int32>(Health + 1, GSDoorImpl::KicksToBreak), GSDoorImpl::KicksToBreak);
	default: return FString();
	}
}

void AGSDoor::PredictUse(AGSCharacter* User)
{
	if (!User || PredictUntil > 0.f) return;
	const int32 Side = SideOf(User->GetPawnViewLocation());
	int8 Pred;
	if (ServerOpen != 0) Pred = 0;
	else if (Lock != GSLock::Locked || Side == EscapeSide() || IsPaneBroken()) Pred = static_cast<int8>(-Side * 100);
	else return;   // locked: only the server knows (card, feedback)
	TargetOpen = Pred;
	SpeedDegS = GSDoorImpl::NormalSpeed;
	PredictUntil = GetWorld()->GetTimeSeconds() + 1.f;
	if (Pred != 0)
	{
		Sound(TEXT("Door_Open"), HandleWorld(), 0.8f, 18.f);
	}
	else
	{
		bSoundOnClose = true;
		bSlamOnClose = false;
	}
	UpdateTick();
}

// ---------------------------------------------------------------------------------------------------------------------
// Server: kicks, bullets, blows, blasts

void AGSDoor::ServerOpenAway(const FVector& From, uint8 Cause, bool bBreakLock, bool bAddHealth)
{
	AGSMapState* MS = AGSMapState::Get(this);
	if (!MS) return;
	const int8 Away = static_cast<int8>(-SideOf(From) * 100);
	const bool bWasLocked = Lock == GSLock::Locked;
	const FVector Dir = (GetActorLocation() - From).GetSafeNormal();
	MS->SetPart(Key, [=](FGSPartState& P)
	{
		P.Open = Away;
		P.Cause = Cause;
		P.HitDir = Dir;
		if (bBreakLock) P.State = (P.State & ~GSLock::Mask) | GSLock::Broken;
		if (bAddHealth) P.Health = static_cast<uint8>(FMath::Min(250, P.Health + 1));
	});
	if (bBreakLock && bWasLocked) GSMapState::OnWalkableChanged().Broadcast(GetWorld(), GetOpeningBox());
	if (Cause == GSCause::Kick) GSMapState::OnNoise().Broadcast(GetWorld(), HandleWorld(), 35.f);
}

void AGSDoor::ServerBulletHit(const FHitResult& Hit, float Damage)
{
	if (Lock != GSLock::Locked) return;
	AGSMapState* MS = AGSMapState::Get(this);
	if (!MS) return;
	bool bBreak = false;
	if (IsKeypad(Hit.GetComponent()))
	{
		bBreak = true;
	}
	else if ((Def.Art == EGSDoorArt::Wood || Def.Art == EGSDoorArt::Double) && FVector::Dist(Hit.ImpactPoint, HandleWorld()) <= 12.f)
	{
		bBreak = true;   // shot through the lock
	}
	if (!bBreak) return;
	const FVector Dir = -Hit.ImpactNormal;
	MS->SetPart(Key, [Dir](FGSPartState& P) { P.State = (P.State & ~GSLock::Mask) | GSLock::Broken; P.Cause = GSCause::Bullet; P.HitDir = Dir; });
	GSMapState::OnWalkableChanged().Broadcast(GetWorld(), GetOpeningBox());
	UE_LOG(LogTemp, Log, TEXT("Interaktiv: låsen skutt i stykker (dør %d)"), GSPart::IdOf(Key));
}

void AGSDoor::ServerMeleeHit(const FHitResult& Hit, AActor* User)
{
	AGSMapState* MS = AGSMapState::Get(this);
	if (!MS) return;
	const FVector From = User ? User->GetActorLocation() : FVector(Hit.TraceStart);
	if (IsKeypad(Hit.GetComponent()))
	{
		if (Lock != GSLock::Locked) return;
		const bool bBreaks = Health + 1 >= GSDoorImpl::BlowsToBreakKeypad;
		MS->SetPart(Key, [bBreaks](FGSPartState& P)
		{
			P.Health = static_cast<uint8>(FMath::Min(250, P.Health + 1));
			if (bBreaks) P.State = (P.State & ~GSLock::Mask) | GSLock::Broken;
			P.Cause = GSCause::Melee;
		});
		if (bBreaks) GSMapState::OnWalkableChanged().Broadcast(GetWorld(), GetOpeningBox());
		GSMapState::OnNoise().Broadcast(GetWorld(), HandleWorld(), 25.f);
		return;
	}
	if (ServerOpen != 0) return;
	if (Lock != GSLock::Locked)
	{
		ServerOpenAway(From, GSCause::Kick, false, true);
		return;
	}
	if (Def.Art == EGSDoorArt::Wood || Def.Art == EGSDoorArt::Double)
	{
		if (Health + 1 >= GSDoorImpl::KicksToBreak) ServerOpenAway(From, GSCause::Kick, true, true);
		else MS->SetPart(Key, [](FGSPartState& P) { P.Health = static_cast<uint8>(FMath::Min(250, P.Health + 1)); P.Cause = GSCause::Kick; });
		GSMapState::OnNoise().Broadcast(GetWorld(), HandleWorld(), 40.f);
	}
}
