#include "GSBreakable.h"
#include "GSMapState.h"
#include "GSDoor.h"
#include "GSPickup.h"
#include "GSEffects.h"
#include "GSUtils.h"
#include "GSCampusBuilder.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/BodySetup.h"

namespace GSBreakImpl
{
	constexpr int32 MarkCap = 400;
	constexpr int32 MarksPerKey = 12;
	constexpr float MarkCullCm = 3000.f;
	constexpr int32 LitterCap = 3000;
	constexpr float LitterCullCm = 2500.f;
	constexpr int32 MaxFlyingBits = 150;
	constexpr float MarksPerSecond = 30.f;
	constexpr float SettleAfter = 3.5f;

	UStaticMesh* Model(const TCHAR* Name)
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

	/** A model that can simulate (has simple collision), else null. */
	UStaticMesh* PhysModel(const TCHAR* Name)
	{
		UStaticMesh* M = Model(Name);
		if (!M) return nullptr;
		const UBodySetup* B = M->GetBodySetup();
		return (B && B->AggGeom.GetElementCount() > 0) ? M : nullptr;
	}

	UMaterialInterface* Mat(const TCHAR* Name)
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

	const TCHAR* ShardName(int32 I)
	{
		static const TCHAR* Names[] = { TEXT("SM_Int_Shard_A"), TEXT("SM_Int_Shard_B"), TEXT("SM_Int_Shard_C"), TEXT("SM_Int_Shard_D") };
		return Names[FMath::Clamp(I, 0, 3)];
	}

	bool IsVisualsMachine(const UWorld* W) { return W && W->GetNetMode() != NM_DedicatedServer; }

	float Thickness(EGSGlass T) { return T == EGSGlass::Railing ? 1.2f : 1.f; }

	enum class ESurf : uint8 { Plaster, Concrete, Wood, Metal, Glass };
}

// ---------------------------------------------------------------------------------------------------------------------
// UGSGlassSet

void UGSGlassSet::Setup(EGSGlass InType)
{
	Type = InType;
	UStaticMesh* Mesh = GSBreakImpl::Model(TEXT("SM_Mod_Helg_GlassPanel"));
	if (Mesh)
	{
		// The model's own size -> the unit cube the sizes are given for
		const FVector B = Mesh->GetBounds().BoxExtent * 2.f;
		MeshScaleFix = FVector(B.X > 0.1f ? 100.f / B.X : 1.f, B.Y > 0.1f ? 100.f / B.Y : 1.f, B.Z > 0.1f ? 100.f / B.Z : 1.f);
	}
	else
	{
		Mesh = GSUtils::Cube();
	}
	SetStaticMesh(Mesh);
	SetMobility(EComponentMobility::Movable);
	if (UMaterialInterface* M = GSBreakImpl::Mat(TEXT("Glass"))) SetMaterial(0, M);
	else GSUtils::SetColor(this, FLinearColor(0.6f, 0.75f, 0.8f));
	// Glass: blocks bodies, things and other glass; lets the Visibility, Camera and WorldStatic channels through.
	SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetCollisionObjectType(ECC_Destructible);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);
	bAffectDistanceFieldLighting = false;
	bVisibleInRayTracing = false;
	SetCanEverAffectNavigation(true);
	ComponentTags.Add(TEXT("Mat_Glass"));
	ComponentTags.Add(TEXT("GS_Int_Glass"));
	ComponentTags.Add(Type == EGSGlass::Railing ? FName(TEXT("Glass_Rekkverk")) : FName(TEXT("Glass_Vegg")));
}

int32 UGSGlassSet::AddPane(int32 Key, const FVector& CenterCm, float YawDeg, const FVector& SizeCm)
{
	const FTransform T(FRotator(0.f, YawDeg, 0.f), CenterCm, SizeCm / 100.f * MeshScaleFix);
	const int32 I = AddInstance(T, true);
	if (Keys.Num() <= I) Keys.SetNum(I + 1);
	Keys[I] = Key;
	IndexOf.Add(Key, I);
	if (Whole.Num() <= I) Whole.SetNum(I + 1);
	Whole[I] = T;
	if (Shown.Num() <= I) Shown.Add(true, I + 1 - Shown.Num());
	Shown[I] = true;
	return I;
}

int32 UGSGlassSet::InstanceOfKey(int32 Key) const
{
	const int32* I = IndexOf.Find(Key);
	return I ? *I : INDEX_NONE;
}

int32 UGSGlassSet::KeyOfInstance(int32 Index) const
{
	return Keys.IsValidIndex(Index) ? Keys[Index] : 0;
}

bool UGSGlassSet::IsPaneShown(int32 Key) const
{
	const int32 I = InstanceOfKey(Key);
	return I != INDEX_NONE && Shown[I];
}

void UGSGlassSet::SetPaneShown(int32 Key, bool bShown)
{
	const int32 I = InstanceOfKey(Key);
	if (I == INDEX_NONE || Shown[I] == bShown) return;
	Shown[I] = bShown;
	FTransform T = Whole[I];
	// Scale 0: the engine removes the body (and adds it again at full scale). Never removed, never moved away.
	if (!bShown) T.SetScale3D(FVector::ZeroVector);
	UpdateInstanceTransform(I, T, true, false, true);
}

bool UGSGlassSet::GetPaneTransform(int32 Key, FTransform& Out) const
{
	const int32 I = InstanceOfKey(Key);
	if (I == INDEX_NONE) return false;
	Out = Whole[I];
	Out.SetScale3D(Out.GetScale3D() / MeshScaleFix);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// UGSInstancePool

void UGSInstancePool::Configure(int32 InCap, int32 InPerKeyCap, float InCullCm)
{
	Cap = FMath::Max(1, InCap);
	PerKeyCap = FMath::Max(1, InPerKeyCap);
	CullCm = InCullCm;
}

int32 UGSInstancePool::AddVisual(UStaticMesh* Mesh, UMaterialInterface* InMat, const FLinearColor& Fallback)
{
	FVisual V;
	V.Mesh = Mesh ? Mesh : GSUtils::Cube();
	V.Mat = InMat;
	V.Color = Fallback;
	return Visuals.Add(V);
}

UInstancedStaticMeshComponent* UGSInstancePool::GetISM(int32 Visual, USceneComponent* Parent)
{
	if (!Visuals.IsValidIndex(Visual)) return nullptr;
	const TPair<TWeakObjectPtr<USceneComponent>, int32> MapKey(Parent, Visual);
	if (TWeakObjectPtr<UInstancedStaticMeshComponent>* Found = ISMByParent.Find(MapKey))
	{
		if (Found->IsValid()) return Found->Get();
	}
	// On a moving part (door leaf) the ISM belongs to that actor and is made at the first mark
	AActor* Owner = Parent ? Parent->GetOwner() : GetOwner();
	if (!Owner) return nullptr;
	const FVisual& V = Visuals[Visual];
	UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(Owner);
	ISM->SetStaticMesh(V.Mesh.Get() ? V.Mesh.Get() : GSUtils::Cube());
	ISM->SetMobility(EComponentMobility::Movable);
	ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ISM->SetCanEverAffectNavigation(false);
	ISM->SetGenerateOverlapEvents(false);
	ISM->SetCastShadow(false);
	ISM->bAffectDistanceFieldLighting = false;
	ISM->bVisibleInRayTracing = false;
	ISM->SetCullDistances(0, FMath::RoundToInt(CullCm));
	if (V.Mat.IsValid()) ISM->SetMaterial(0, V.Mat.Get());
	else GSUtils::SetColor(ISM, V.Color);
	if (Parent)
	{
		ISM->SetupAttachment(Parent);
		ISM->SetUsingAbsoluteScale(true);
	}
	else
	{
		ISM->SetupAttachment(this);
	}
	ISM->RegisterComponent();
	Owner->AddInstanceComponent(ISM);
	ISMByParent.Add(MapKey, ISM);
	AllISMs.Add(ISM);
	return ISM;
}

void UGSInstancePool::HideInst(const FInst& I)
{
	UInstancedStaticMeshComponent* ISM = I.ISM.Get();
	if (!ISM || I.Index == INDEX_NONE) return;
	FTransform T;
	ISM->GetInstanceTransform(I.Index, T, false);
	T.SetScale3D(FVector::ZeroVector);
	ISM->UpdateInstanceTransform(I.Index, T, false, false, true);
	FreeInsts.FindOrAdd(I.ISM).Add(I.Index);
	--ShownInstances;
}

void UGSInstancePool::Evict(int32 Slot)
{
	if (!Entries.IsValidIndex(Slot) || !Entries[Slot].bLive) return;
	FEntry& E = Entries[Slot];
	for (const FInst& I : E.Insts) HideInst(I);
	E.Insts.Reset();
	E.bLive = false;
	if (E.Key != 0)
	{
		int32& C = KeyCount.FindOrAdd(E.Key);
		if (--C <= 0) KeyCount.Remove(E.Key);
	}
	E.Key = 0;
	--LiveCount;
	FreeSlots.Add(Slot);
}

int32 UGSInstancePool::Add(const TArray<FPiece>& Pieces, int32 Key, USceneComponent* Parent)
{
	if (Pieces.Num() == 0) return INDEX_NONE;
	// Room: per key first (a door keeps its 12 newest), then the whole pool (the oldest go)
	if (Key != 0 && KeyCount.FindRef(Key) >= PerKeyCap)
	{
		for (int32 F = Head; F < Fifo.Num(); ++F)
		{
			const int32 S = Fifo[F].Key;
			if (Entries[S].bLive && Entries[S].Serial == Fifo[F].Value && Entries[S].Key == Key)
			{
				Evict(S);
				break;
			}
		}
	}
	while (LiveCount >= Cap && Head < Fifo.Num())
	{
		const TPair<int32, uint32> F = Fifo[Head++];
		if (Entries[F.Key].bLive && Entries[F.Key].Serial == F.Value) Evict(F.Key);
	}
	int32 Slot;
	if (FreeSlots.Num() > 0) Slot = FreeSlots.Pop(EAllowShrinking::No);
	else Slot = Entries.AddDefaulted();
	FEntry& E = Entries[Slot];
	E.bLive = true;
	E.Key = Key;
	E.Serial = NextSerial++;
	E.Insts.Reset();
	for (const FPiece& P : Pieces)
	{
		UInstancedStaticMeshComponent* ISM = GetISM(P.Visual, Parent);
		if (!ISM) continue;
		const FTransform Local = P.World.GetRelativeTransform(ISM->GetComponentTransform());
		FInst I;
		I.ISM = ISM;
		TArray<int32>* Free = FreeInsts.Find(ISM);
		if (Free && Free->Num() > 0)
		{
			I.Index = Free->Pop(EAllowShrinking::No);
			ISM->UpdateInstanceTransform(I.Index, Local, false, false, true);
		}
		else
		{
			I.Index = ISM->AddInstance(Local, false);
		}
		++ShownInstances;
		E.Insts.Add(I);
	}
	if (Key != 0) KeyCount.FindOrAdd(Key)++;
	++LiveCount;
	Fifo.Add(TPair<int32, uint32>(Slot, E.Serial));
	// Compact the queue now and then (evicted and removed slots stay in it until they reach the head)
	if (Head > 1024 && Head * 2 > Fifo.Num())
	{
		Fifo.RemoveAt(0, Head, EAllowShrinking::No);
		Head = 0;
	}
	return Slot;
}

void UGSInstancePool::RemoveKey(int32 Key)
{
	if (Key == 0 || !KeyCount.Contains(Key)) return;
	for (int32 S = 0; S < Entries.Num(); ++S)
	{
		if (Entries[S].bLive && Entries[S].Key == Key) Evict(S);
	}
	// Drop the stale copies from the queue now (a pane's shards can be many)
	TArray<TPair<int32, uint32>> Clean;
	Clean.Reserve(LiveCount);
	for (int32 F = Head; F < Fifo.Num(); ++F)
	{
		const TPair<int32, uint32>& Q = Fifo[F];
		if (Entries[Q.Key].bLive && Entries[Q.Key].Serial == Q.Value) Clean.Add(Q);
	}
	Fifo = MoveTemp(Clean);
	Head = 0;
}

void UGSInstancePool::ClearAll()
{
	for (TObjectPtr<UInstancedStaticMeshComponent>& ISM : AllISMs)
	{
		if (ISM) ISM->ClearInstances();
	}
	Entries.Reset();
	FreeSlots.Reset();
	Fifo.Reset();
	Head = 0;
	LiveCount = 0;
	ShownInstances = 0;
	KeyCount.Reset();
	FreeInsts.Reset();
}

// ---------------------------------------------------------------------------------------------------------------------
// AGSDebris

AGSDebris::AGSDebris()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetCanBeDamaged(false);
	USceneComponent* R = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	R->SetMobility(EComponentMobility::Movable);
	RootComponent = R;
}

void AGSDebris::Init(int32 InKey, const FTransform& Pane, const FVector& SizeCm, bool bTempered, const FVector& HitDir)
{
	Key = InKey;
	UWorld* W = GetWorld();
	Born = W->GetTimeSeconds();
	FRandomStream Rand(FMath::Rand());
	const FVector AxX = Pane.GetUnitAxis(EAxis::X), AxY = Pane.GetUnitAxis(EAxis::Y), AxZ = Pane.GetUnitAxis(EAxis::Z);
	const FVector Dir = HitDir.IsNearlyZero() ? AxY : HitDir.GetSafeNormal();
	const int32 N = bTempered ? Rand.RandRange(10, 14) : Rand.RandRange(4, 6);
	UMaterialInterface* GlassMat = GSBreakImpl::Mat(TEXT("Glass"));
	for (int32 I = 0; I < N; ++I)
	{
		const int32 Visual = bTempered ? GSLitterVis::Clump : GSLitterVis::ShardA + Rand.RandRange(0, 3);
		UStaticMesh* Mesh = GSBreakImpl::PhysModel(bTempered ? TEXT("SM_Int_GlassClump") : GSBreakImpl::ShardName(Visual - GSLitterVis::ShardA));
		FVector Scale;
		if (Mesh)
		{
			Scale = FVector(Rand.FRandRange(0.8f, 1.25f));
		}
		else
		{
			Mesh = GSUtils::Cube();
			Scale = bTempered ? FVector(Rand.FRandRange(3.f, 5.f), Rand.FRandRange(3.f, 5.f), 1.f) / 100.f
				: FVector(Rand.FRandRange(8.f, 16.f), 0.8f, Rand.FRandRange(10.f, 20.f)) / 100.f;
		}
		const FVector At = Pane.GetLocation() + AxX * (Rand.FRandRange(-0.45f, 0.45f) * SizeCm.X) + AxZ * (Rand.FRandRange(-0.45f, 0.45f) * SizeCm.Z);
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(Mesh);
		C->SetMobility(EComponentMobility::Movable);
		if (GlassMat) C->SetMaterial(0, GlassMat);
		else GSUtils::SetColor(C, FLinearColor(0.6f, 0.75f, 0.8f));
		C->SetCollisionObjectType(ECC_PhysicsBody);
		C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		C->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		C->SetGenerateOverlapEvents(false);
		C->SetCanEverAffectNavigation(false);
		C->SetCastShadow(false);
		C->bAffectDistanceFieldLighting = false;
		C->SetupAttachment(RootComponent);
		C->SetWorldTransform(FTransform(FRotator(Rand.FRandRange(-180.f, 180.f), Rand.FRandRange(-180.f, 180.f), Rand.FRandRange(-180.f, 180.f)), At, Scale));
		C->RegisterComponent();
		AddInstanceComponent(C);
		C->SetSimulatePhysics(true);
		const float Speed = Rand.FRandRange(200.f, 500.f) * (bTempered ? 1.f : 0.6f);
		C->SetPhysicsLinearVelocity(Dir * Speed + Rand.VRand() * 80.f + (bTempered ? FVector::ZeroVector : FVector(0.f, 0.f, -100.f)));
		C->SetPhysicsAngularVelocityInDegrees(Rand.VRand() * Rand.FRandRange(200.f, 900.f));
		FBit B;
		B.Comp = C;
		B.Visual = Visual;
		Bits.Add(B);
	}
	W->GetTimerManager().SetTimer(CheckTimer, this, &AGSDebris::Check, 0.5f, true, GSBreakImpl::SettleAfter);
}

int32 AGSDebris::NumLive() const
{
	int32 N = 0;
	for (const FBit& B : Bits) N += B.Comp ? 1 : 0;
	return N;
}

void AGSDebris::SettleBit(FBit& B)
{
	UStaticMeshComponent* C = B.Comp;
	if (!C) return;
	const FTransform T = C->GetComponentTransform();
	if (T.GetLocation().Z > -50000.f)
	{
		if (AGSMapState* MS = AGSMapState::Get(this))
		{
			if (UGSInstancePool* L = MS->GetLitter())
			{
				UGSInstancePool::FPiece P;
				P.Visual = B.Visual;
				P.World = T;
				L->Add({ P }, Key, nullptr);
			}
		}
	}
	C->DestroyComponent();
	B.Comp = nullptr;
}

int32 AGSDebris::SettleSome(int32 N)
{
	int32 Done = 0;
	for (FBit& B : Bits)
	{
		if (Done >= N) break;
		if (!B.Comp) continue;
		SettleBit(B);
		++Done;
	}
	if (NumLive() == 0) Destroy();
	return Done;
}

void AGSDebris::Check()
{
	const float Age = GetWorld()->GetTimeSeconds() - Born;
	for (FBit& B : Bits)
	{
		if (!B.Comp) continue;
		// Still falling (from a gallery): wait a little, never more than 8 s
		if (Age < 8.f && B.Comp->GetPhysicsLinearVelocity().SizeSquared() > FMath::Square(30.f)) continue;
		SettleBit(B);
	}
	if (NumLive() == 0) Destroy();
}

// ---------------------------------------------------------------------------------------------------------------------
// AGSMapState: glass (declared in GSMapState.h)

USceneComponent* AGSMapState::EnsureRoot()
{
	if (USceneComponent* R = GetRootComponent()) return R;
	USceneComponent* R = NewObject<USceneComponent>(this, TEXT("IntRoot"));
	R->SetMobility(EComponentMobility::Movable);
	SetRootComponent(R);
	R->RegisterComponent();
	AddInstanceComponent(R);
	R->SetWorldTransform(FTransform::Identity);
	return R;
}

UGSInstancePool* AGSMapState::GetMarks()
{
	if (Marks) return Marks;
	if (!GSBreakImpl::IsVisualsMachine(GetWorld())) return nullptr;
	Marks = NewObject<UGSInstancePool>(this, TEXT("BulletMarks"));
	Marks->Configure(GSBreakImpl::MarkCap, GSBreakImpl::MarksPerKey, GSBreakImpl::MarkCullCm);
	Marks->SetupAttachment(EnsureRoot());
	Marks->RegisterComponent();
	AddInstanceComponent(Marks);
	using namespace GSBreakImpl;
	// The order is GSMarkVis
	Marks->AddVisual(GSUtils::Cylinder(), Mat(TEXT("Gunmetal")), FLinearColor(0.03f, 0.03f, 0.03f));
	Marks->AddVisual(GSUtils::Cylinder(), Mat(TEXT("Plaster")), FLinearColor(0.7f, 0.7f, 0.68f));
	Marks->AddVisual(GSUtils::Cube(), Mat(TEXT("WoodLight")), FLinearColor(0.75f, 0.6f, 0.42f));
	Marks->AddVisual(GSUtils::Cylinder(), Mat(TEXT("Steel")), FLinearColor(0.7f, 0.7f, 0.72f));
	Marks->AddVisual(GSUtils::Cube(), Mat(TEXT("PlasticWhite")), FLinearColor(0.95f, 0.95f, 0.95f));
	return Marks;
}

UGSInstancePool* AGSMapState::GetLitter()
{
	if (Litter) return Litter;
	if (!GSBreakImpl::IsVisualsMachine(GetWorld())) return nullptr;
	Litter = NewObject<UGSInstancePool>(this, TEXT("GlassLitter"));
	Litter->Configure(GSBreakImpl::LitterCap, 1 << 30, GSBreakImpl::LitterCullCm);
	Litter->SetupAttachment(EnsureRoot());
	Litter->RegisterComponent();
	AddInstanceComponent(Litter);
	using namespace GSBreakImpl;
	UMaterialInterface* G = Mat(TEXT("Glass"));
	const FLinearColor GC(0.6f, 0.75f, 0.8f);
	// The order is GSLitterVis; the bits keep their own mesh and scale when they settle
	Litter->AddVisual(Model(TEXT("SM_Int_GlassGranules")), G, GC);
	Litter->AddVisual(PhysModel(TEXT("SM_Int_GlassClump")), G, GC);
	for (int32 I = 0; I < 4; ++I) Litter->AddVisual(PhysModel(ShardName(I)), G, GC);
	return Litter;
}

bool AGSMapState::TakeMarkToken()
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (MarkTokenTime < 0.f) MarkTokenTime = Now;
	MarkTokens = FMath::Min(GSBreakImpl::MarksPerSecond, MarkTokens + (Now - MarkTokenTime) * GSBreakImpl::MarksPerSecond);
	MarkTokenTime = Now;
	if (MarkTokens < 1.f) return false;
	MarkTokens -= 1.f;
	return true;
}

int32 AGSMapState::NumGlassBroken() const
{
	int32 N = 0;
	for (const auto& Pair : Glass) N += Pair.Value.State >= 2 ? 1 : 0;
	return N;
}

void AGSMapState::BuildGlass(const TArray<FGSGlassDef>& Defs)
{
	for (const FGSGlassDef& Def : Defs)
	{
		const int32 Key = GSPart::MakeKey(GSPart::Glass, Def.Id);
		if (Glass.Contains(Key)) continue;
		const bool bTest = Def.Id >= GSPart::TestIdBase;
		const int32 SetIndex = (bTest ? 4 : 0) + static_cast<int32>(Def.Type);
		UGSGlassSet* Set = nullptr;
		for (UGSGlassSet* S : GlassSets)
		{
			if (S && S->ComponentTags.Contains(FName(*FString::Printf(TEXT("GS_GlassSet_%d"), SetIndex))))
			{
				Set = S;
				break;
			}
		}
		if (!Set)
		{
			Set = NewObject<UGSGlassSet>(this);
			Set->Setup(Def.Type);
			Set->ComponentTags.Add(FName(*FString::Printf(TEXT("GS_GlassSet_%d"), SetIndex)));
			Set->SetupAttachment(EnsureRoot());
			Set->RegisterComponent();   // collision is set before this, so the instance bodies are made
			AddInstanceComponent(Set);
			GlassSets.Add(Set);
		}
		Set->AddPane(Key, Def.CenterCm, Def.YawDeg, FVector(Def.LengthCm, GSBreakImpl::Thickness(Def.Type), Def.HeightCm));
		FGlassPane P;
		P.Def = Def;
		P.Set = Set;
		Glass.Add(Key, P);
	}
}

int32 AGSMapState::HideBuilderGlass()
{
	// Grafikk's 4a panes are boxes tagged GS_Int_Glass, or Mat_Glass that ignore Visibility. Fixed glass (roof,
	// windows) is also Mat_Glass but blocks Visibility, so it stays. Never by Mat_Glass alone or by mesh name.
	int32 N = 0;
	for (TActorIterator<AGSCampusBuilder> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<UPrimitiveComponent*> Comps(*It);
		for (UPrimitiveComponent* C : Comps)
		{
			if (!C || C->IsA<UGSGlassSet>()) continue;
			const bool bOurs = C->ComponentHasTag(TEXT("GS_Int_Glass"))
				|| (C->ComponentHasTag(TEXT("Mat_Glass")) && C->GetCollisionEnabled() != ECollisionEnabled::NoCollision
					&& C->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore);
			if (!bOurs) continue;
			C->SetVisibility(false);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			++N;
		}
	}
	return N;
}

void AGSMapState::ServerDamageGlass(int32 Key, float Amount, uint8 Cause, const FVector& Dir)
{
	if (!HasAuthority()) return;
	const FGlassPane* P = Glass.Find(Key);
	if (!P) return;
	FGSPartState Cur;
	if (!GetPart(Key, Cur) || Cur.State >= 2) return;
	uint8 NewState = 2;
	int32 NewHealth = Cur.Health;
	if (P->Def.Type == EGSGlass::Railing)
	{
		// Laminated: the foil holds the pieces; it cracks where it is hit and gives way at 100
		float Add = Amount;
		if (Cause == GSCause::Melee) Add = 35.f;
		else if (Cause != GSCause::Blast) Add = Amount * 0.6f;
		if (Add < 1.f && Cause == GSCause::Blast) return;   // too far away to do anything
		NewHealth = FMath::Clamp(Cur.Health + FMath::Max(1, FMath::RoundToInt(Add)), 0, 250);
		NewState = NewHealth >= 100 ? 2 : 1;
	}
	const FVector D = Dir.GetSafeNormal();
	SetPart(Key, [NewState, NewHealth, Cause, D](FGSPartState& S)
	{
		S.State = NewState;
		S.Health = static_cast<uint8>(NewHealth);
		S.Cause = Cause;
		S.HitDir = D;
	});
	if (NewState == 2)
	{
		const FVector Ext(P->Def.LengthCm * 0.5f + 100.f, P->Def.LengthCm * 0.5f + 100.f, P->Def.HeightCm * 0.5f + 100.f);
		GSMapState::OnWalkableChanged().Broadcast(GetWorld(), FBox(P->Def.CenterCm - Ext, P->Def.CenterCm + Ext));
		GSMapState::OnNoise().Broadcast(GetWorld(), P->Def.CenterCm, 40.f);
	}
}

void AGSMapState::ApplyGlassLocal(const FGSPartState& Item, bool bFresh)
{
	FGlassPane* P = Glass.Find(Item.Key);
	if (!P) return;
	UGSGlassSet* Set = P->Set.Get();
	const uint8 OldState = P->State;
	const uint8 OldHealth = P->Health;
	P->State = Item.State;
	P->Health = Item.Health;
	if (Item.State == 0)
	{
		ResetGlassLocal(Item.Key);
		return;
	}
	if (Item.State >= 2)
	{
		FTransform T;
		const bool bHave = Set && Set->GetPaneTransform(Item.Key, T);
		if (Set) Set->SetPaneShown(Item.Key, false);
		if (Marks) Marks->RemoveKey(Item.Key);   // the cracks go with the pane
		if (bHave)
		{
			const FVector Size = T.GetScale3D() * 100.f;
			T.SetScale3D(FVector(1.f));
			GlassBreakFX(Item.Key, T, Size, P->Def.Type, Item.HitDir, bFresh && OldState < 2);
		}
		return;
	}
	// Cracked laminated glass
	if (Set) Set->SetPaneShown(Item.Key, true);
	if (bFresh && Item.Health > OldHealth)
	{
		AGSMapState::PlayPartSound(this, TEXT("Glass_Crack"), P->Def.CenterCm, 0.6f, 25.f);
	}
	EnsureCrackMarks(Item.Key, bFresh ? FMath::Clamp(1 + Item.Health / 34, 1, 3) : 3);
}

void AGSMapState::ResetGlassLocal(int32 Key)
{
	FGlassPane* P = Glass.Find(Key);
	if (!P) return;
	P->State = 0;
	P->Health = 0;
	if (UGSGlassSet* Set = P->Set.Get()) Set->SetPaneShown(Key, true);
	if (Marks) Marks->RemoveKey(Key);
	if (Litter) Litter->RemoveKey(Key);
	LitteredKeys.Remove(Key);
}

void AGSMapState::ResetGlassAllLocal()
{
	for (auto& Pair : Glass)
	{
		Pair.Value.State = 0;
		Pair.Value.Health = 0;
		if (UGSGlassSet* Set = Pair.Value.Set.Get()) Set->SetPaneShown(Pair.Key, true);
	}
	for (TWeakObjectPtr<AGSDebris>& D : Debris)
	{
		if (AGSDebris* A = D.Get()) A->Destroy();
	}
	Debris.Reset();
	if (Marks) Marks->ClearAll();
	if (Litter) Litter->ClearAll();
	LitteredKeys.Reset();
}

void AGSMapState::GlassBreakFX(int32 Key, const FTransform& Pane, const FVector& SizeCm, EGSGlass Type, const FVector& HitDir, bool bFresh)
{
	UWorld* W = GetWorld();
	if (!GSBreakImpl::IsVisualsMachine(W)) return;
	const bool bTempered = Type != EGSGlass::Railing;
	const FVector AxX = Pane.GetUnitAxis(EAxis::X), AxY = Pane.GetUnitAxis(EAxis::Y);
	const FVector Dir = HitDir.IsNearlyZero() ? AxY : HitDir.GetSafeNormal();
	if (bFresh)
	{
		const FVector At = Pane.GetLocation();
		PlayPartSound(this, bTempered ? TEXT("Glass_Shatter") : TEXT("Glass_Collapse"), At, bTempered ? 1.f : 0.9f, bTempered ? 45.f : 40.f);
		FTimerHandle H;
		TWeakObjectPtr<AGSMapState> WeakThis(this);
		W->GetTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([WeakThis, At]()
		{
			if (AGSMapState* MS = WeakThis.Get()) AGSMapState::PlayPartSound(MS, TEXT("Glass_Tinkle"), At, 0.5f, 20.f);
		}), 0.4f, false);
		MakeRoomForBits(bTempered ? 14 : 6);
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SP.ObjectFlags |= RF_Transient;
		if (AGSDebris* D = W->SpawnActor<AGSDebris>(Pane.GetLocation(), FRotator::ZeroRotator, SP))
		{
			D->Init(Key, Pane, SizeCm, bTempered, Dir);
			TrackDebris(D);
		}
	}
	// Shards on the floor, once per break, the same spots on every machine (seeded with the key)
	if (LitteredKeys.Contains(Key)) return;
	UGSInstancePool* L = GetLitter();
	if (!L) return;
	LitteredKeys.Add(Key);
	FRandomStream Rand(Key * 2654435761u);
	const FVector Foot = Pane.GetLocation() - FVector(0.f, 0.f, SizeCm.Z * 0.5f);
	const float Side = FVector::DotProduct(Dir, AxY) >= 0.f ? 1.f : -1.f;
	const bool bModel = GSBreakImpl::Model(TEXT("SM_Int_GlassGranules")) != nullptr;
	const int32 Total = Rand.RandRange(30, 50);
	FCollisionQueryParams QP(SCENE_QUERY_STAT(GSGlassLitter), false);
	for (int32 G = 0; G < 5; ++G)
	{
		const float S = Rand.FRand() < 0.75f ? Side : -Side;
		const float Dist = Rand.FRandRange(20.f, 160.f);
		const FVector C = Foot + AxY * (S * Dist) + AxX * (Rand.FRandRange(-0.5f, 0.5f) * SizeCm.X);
		FHitResult Floor;
		const bool bFloor = W->LineTraceSingleByObjectType(Floor, C + FVector(0.f, 0.f, 60.f), C - FVector(0.f, 0.f, 800.f), FCollisionObjectQueryParams(ECC_WorldStatic), QP);
		const int32 InGroup = Total / 5 + (G < Total % 5 ? 1 : 0);
		// No floor found within 8 m (or the scene is not ready): the pane's own floor
		if (!bFloor) Floor.ImpactPoint = FVector(C.X, C.Y, Foot.Z);
		const float Spread = 12.f + Dist * 0.25f;
		for (int32 I = 0; I < InGroup; ++I)
		{
			const FVector2D Off = FVector2D(Rand.FRandRange(-1.f, 1.f), Rand.FRandRange(-1.f, 1.f)) * Spread;
			const FVector At = FVector(Floor.ImpactPoint) + AxX * Off.X + AxY * Off.Y + FVector(0.f, 0.f, 0.2f);
			const FVector Scale = bModel ? FVector(Rand.FRandRange(0.7f, 1.3f))
				: FVector(Rand.FRandRange(0.6f, 1.6f), Rand.FRandRange(0.6f, 1.6f), 0.4f) / 100.f;
			UGSInstancePool::FPiece P;
			P.Visual = GSLitterVis::Granules;
			P.World = FTransform(FRotator(0.f, Rand.FRandRange(0.f, 360.f), 0.f), At, Scale);
			L->Add({ P }, Key, nullptr);
		}
	}
}

void AGSMapState::GlassCrackAt(int32 Key, const FVector& At, bool bSound)
{
	const FGlassPane* P = Glass.Find(Key);
	UGSInstancePool* M = GetMarks();
	if (!P || !M) return;
	const FRotator R(0.f, P->Def.YawDeg, 0.f);
	const FVector AxX = FRotationMatrix(R).GetUnitAxis(EAxis::X), AxY = FRotationMatrix(R).GetUnitAxis(EAxis::Y);
	// On the pane's middle plane, through the glass so it shows from both sides
	const FVector Center = At - AxY * FVector::DotProduct(At - P->Def.CenterCm, AxY);
	const float Thick = GSBreakImpl::Thickness(P->Def.Type) + 0.2f;
	FRandomStream Rand(Key ^ FMath::RoundToInt(At.X * 7.f + At.Y * 13.f + At.Z * 17.f));
	TArray<UGSInstancePool::FPiece> Pieces;
	const int32 N = Rand.RandRange(5, 7);
	for (int32 I = 0; I < N; ++I)
	{
		const float A = (I + Rand.FRandRange(-0.3f, 0.3f)) * 2.f * PI / N;
		const FVector D = AxX * FMath::Cos(A) + FVector::UpVector * FMath::Sin(A);
		const float Len = Rand.FRandRange(3.f, 10.f);
		UGSInstancePool::FPiece Pc;
		Pc.Visual = GSMarkVis::GlassStreak;
		Pc.World = FTransform(FRotationMatrix::MakeFromXY(D, AxY).ToQuat(), Center + D * (Len * 0.5f), FVector(Len, Thick, 0.12f) / 100.f);
		Pieces.Add(Pc);
	}
	M->Add(Pieces, Key, nullptr);
	if (bSound) PlayPartSound(this, TEXT("Glass_Crack"), At, 0.6f, 25.f);
}

void AGSMapState::EnsureCrackMarks(int32 Key, int32 Want)
{
	const FGlassPane* P = Glass.Find(Key);
	UGSInstancePool* M = GetMarks();
	if (!P || !M) return;
	const FVector AxX = FRotator(0.f, P->Def.YawDeg, 0.f).Vector();
	for (int32 Have = M->CountKey(Key), I = Have; I < Want; ++I)
	{
		FRandomStream Rand(Key * 7919 + I * 104729);
		const FVector At = P->Def.CenterCm + AxX * (Rand.FRandRange(-0.35f, 0.35f) * P->Def.LengthCm)
			+ FVector::UpVector * (Rand.FRandRange(-0.35f, 0.35f) * P->Def.HeightCm);
		GlassCrackAt(Key, At, false);
	}
}

void AGSMapState::TrackDebris(AGSDebris* D)
{
	Debris.RemoveAll([](const TWeakObjectPtr<AGSDebris>& W) { return !W.IsValid(); });
	Debris.Add(D);
}

int32 AGSMapState::NumFlyingBits() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<AGSDebris>& D : Debris)
	{
		if (const AGSDebris* A = D.Get()) N += A->NumLive();
	}
	return N;
}

void AGSMapState::MakeRoomForBits(int32 NewBits)
{
	int32 Excess = NumFlyingBits() + NewBits - GSBreakImpl::MaxFlyingBits;
	for (int32 I = 0; I < Debris.Num() && Excess > 0; ++I)
	{
		if (AGSDebris* A = Debris[I].Get()) Excess -= A->SettleSome(Excess);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// GSBreakable

namespace GSBreakable
{
	int32 GlassKeyOfHit(const FHitResult& Hit)
	{
		const UPrimitiveComponent* C = Hit.GetComponent();
		if (!C) return 0;
		if (const UGSGlassSet* S = Cast<UGSGlassSet>(C)) return S->KeyOfInstance(Hit.Item);
		if (const AGSDoor* D = Cast<AGSDoor>(Hit.GetActor()))
		{
			if (D->IsPane(C) && !D->IsPaneBroken()) return D->GetInteractKey();
		}
		return 0;
	}

	void DamageGlass(UWorld* World, int32 Key, float Amount, uint8 Cause, const FVector& Dir)
	{
		if (!World || World->GetNetMode() == NM_Client) return;
		AGSMapState* MS = AGSMapState::Get(World);
		if (!MS) return;
		if (GSPart::CatOf(Key) == GSPart::Door)
		{
			if (AGSDoor* D = MS->FindDoor(Key)) D->ServerPaneHit(Dir, Cause);
			return;
		}
		MS->ServerDamageGlass(Key, Amount, Cause, Dir);
	}

	bool DamageGlassHit(UWorld* World, const FHitResult& Hit, float Amount, uint8 Cause, const FVector& Dir)
	{
		const int32 Key = GlassKeyOfHit(Hit);
		if (Key == 0) return false;
		DamageGlass(World, Key, Amount, Cause, Dir);
		return true;
	}

	bool AddMark(UWorld* World, const FHitResult& Hit, bool bForce)
	{
		using namespace GSBreakImpl;
		if (!IsVisualsMachine(World)) return false;
		UPrimitiveComponent* Comp = Hit.GetComponent();
		AActor* Actor = Hit.GetActor();
		if (!Comp || !Comp->IsVisible()) return false;
		// Never on figures, things on the floor, flying bits or anything that simulates
		if (Cast<APawn>(Actor) || Cast<AGSPickup>(Actor) || Cast<AGSDebris>(Actor) || Comp->IsSimulatingPhysics()) return false;
		if (Comp->IsA<UGSGlassSet>()) return false;   // our glass cracks by its own rules
		AGSDoor* Door = Cast<AGSDoor>(Actor);
		if (Door && Door->IsPane(Comp)) return false;
		AGSMapState* MS = AGSMapState::Get(World);
		UGSInstancePool* Pool = MS ? MS->GetMarks() : nullptr;
		if (!Pool) return false;
		if (!bForce && !MS->TakeMarkToken()) return false;

		// The surface: tags, then the physical material, then the door, then the material's name
		ESurf Surf = ESurf::Plaster;
		bool bFound = true;
		if (Comp->ComponentHasTag(TEXT("Mat_Glass"))) Surf = ESurf::Glass;
		else if (Comp->ComponentHasTag(TEXT("Mat_Wood"))) Surf = ESurf::Wood;
		else if (Comp->ComponentHasTag(TEXT("Mat_Metal"))) Surf = ESurf::Metal;
		else if (Comp->ComponentHasTag(TEXT("Mat_Concrete"))) Surf = ESurf::Concrete;
		else if (Comp->ComponentHasTag(TEXT("Mat_Plaster"))) Surf = ESurf::Plaster;
		else bFound = false;
		if (!bFound && Hit.PhysMaterial.IsValid())
		{
			const FString N = Hit.PhysMaterial->GetName();
			bFound = true;
			if (N.Contains(TEXT("Glass"))) Surf = ESurf::Glass;
			else if (N.Contains(TEXT("Wood"))) Surf = ESurf::Wood;
			else if (N.Contains(TEXT("Metal")) || N.Contains(TEXT("Steel"))) Surf = ESurf::Metal;
			else if (N.Contains(TEXT("Concrete"))) Surf = ESurf::Concrete;
			else if (N.Contains(TEXT("Plaster"))) Surf = ESurf::Plaster;
			else bFound = false;
		}
		if (!bFound && Door)
		{
			bFound = true;
			switch (Door->GetDef().Art)
			{
			case EGSDoorArt::Wood: Surf = ESurf::Wood; break;
			case EGSDoorArt::Double: Surf = ESurf::Plaster; break;
			default: Surf = ESurf::Metal; break;
			}
		}
		if (!bFound)
		{
			const UMaterialInterface* M = Comp->GetMaterial(0);
			const FString N = M ? M->GetName() : FString();
			if (N.Contains(TEXT("Wood"))) Surf = ESurf::Wood;
			else if (N.Contains(TEXT("Metal")) || N.Contains(TEXT("Steel")) || N.Contains(TEXT("Chrome")) || N.Contains(TEXT("Gunmetal"))) Surf = ESurf::Metal;
			else if (N.Contains(TEXT("Concrete")) || N.Contains(TEXT("Terrazzo")) || N.Contains(TEXT("Asphalt"))) Surf = ESurf::Concrete;
			else if (N.Contains(TEXT("Glass"))) Surf = ESurf::Glass;
		}

		const FVector N = Hit.ImpactNormal.IsNearlyZero() ? -FVector(Hit.TraceEnd - Hit.TraceStart).GetSafeNormal() : FVector(Hit.ImpactNormal);
		const FVector P = Hit.ImpactPoint;
		FRandomStream Rand(FMath::Rand());
		const FQuat Q = FQuat(N, Rand.FRandRange(0.f, 2.f * PI)) * FRotationMatrix::MakeFromZ(N).ToQuat();
		const FVector T1 = Q.GetAxisX(), T2 = Q.GetAxisY();
		TArray<UGSInstancePool::FPiece> Pieces;
		auto Disc = [&](int32 Vis, float Diam, float H, float Lift, const FVector& Off)
		{
			UGSInstancePool::FPiece Pc;
			Pc.Visual = Vis;
			Pc.World = FTransform(Q, P + Off + N * (Lift + H * 0.5f), FVector(Diam, Diam, H) / 100.f);
			Pieces.Add(Pc);
		};
		auto Streak = [&](int32 Vis, float Ang, float Len, float W, float H, float Lift, float From)
		{
			const FVector D = T1 * FMath::Cos(Ang) + T2 * FMath::Sin(Ang);
			UGSInstancePool::FPiece Pc;
			Pc.Visual = Vis;
			Pc.World = FTransform(FRotationMatrix::MakeFromXZ(D, N).ToQuat(), P + D * (From + Len * 0.5f) + N * (Lift + H * 0.5f), FVector(Len, W, H) / 100.f);
			Pieces.Add(Pc);
		};
		switch (Surf)
		{
		case ESurf::Wood:
			// Torn fibres round a dark hole
			for (int32 I = 0, K = Rand.RandRange(4, 6); I < K; ++I)
			{
				Streak(GSMarkVis::Splinter, (I + Rand.FRandRange(-0.3f, 0.3f)) * 2.f * PI / K, Rand.FRandRange(1.f, 2.5f), Rand.FRandRange(0.2f, 0.35f), 0.08f, 0.03f, 0.2f);
			}
			Disc(GSMarkVis::Core, 0.8f, 0.08f, 0.1f, FVector::ZeroVector);
			break;
		case ESurf::Metal:
			// Bright ring where the paint is gone, dark dent in the middle
			Disc(GSMarkVis::MetalRing, Rand.FRandRange(1.6f, 2.f), 0.05f, 0.02f, FVector::ZeroVector);
			Disc(GSMarkVis::Core, 0.8f, 0.06f, 0.08f, FVector::ZeroVector);
			break;
		case ESurf::Glass:
			for (int32 I = 0, K = Rand.RandRange(5, 7); I < K; ++I)
			{
				Streak(GSMarkVis::GlassStreak, (I + Rand.FRandRange(-0.3f, 0.3f)) * 2.f * PI / K, Rand.FRandRange(3.f, 10.f), 0.12f, 0.1f, 0.02f, 0.f);
			}
			Disc(GSMarkVis::MetalRing, 0.7f, 0.04f, 0.05f, FVector::ZeroVector);
			break;
		default:
		{
			// Plaster / concrete: a crater of broken surface round a dark core, a little off-centre chip
			const float Crater = Rand.FRandRange(2.5f, 4.f) * (Surf == ESurf::Concrete ? 0.8f : 1.f);
			Disc(GSMarkVis::Crater, Crater, 0.06f, 0.02f, FVector::ZeroVector);
			Disc(GSMarkVis::Crater, Crater * Rand.FRandRange(0.4f, 0.6f), 0.06f, 0.03f, (T1 * Rand.FRandRange(-1.f, 1.f) + T2 * Rand.FRandRange(-1.f, 1.f)) * Crater * 0.35f);
			Disc(GSMarkVis::Core, 0.9f, 0.08f, 0.1f, FVector::ZeroVector);
			break;
		}
		}
		// On something that moves (a door leaf) the hole moves with it; doors keep their 12 newest
		USceneComponent* Parent = Comp->Mobility == EComponentMobility::Movable ? Comp : nullptr;
		const int32 Key = Door ? Door->GetInteractKey() : 0;
		return Pool->Add(Pieces, Key, Parent) != INDEX_NONE;
	}
}
