#include "GSExplore.h"
#include "GSCharacter.h"
#include "GSGameState.h"
#include "GSMenu.h"
#include "GSGameMode.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

namespace GSExploreImpl
{
	constexpr float FlySpeed = 700.f;          // cm/s, a brisk walk through the air
	constexpr float FlySprintSpeed = 1800.f;   // with the sprint key
	constexpr float FloorSearchCm = 3000.f;    // at most 30 m down to a floor

	AGSExplore* Find(const UWorld* World)
	{
		if (!World) return nullptr;
		for (TActorIterator<AGSExplore> It(const_cast<UWorld*>(World)); It; ++It) return *It;
		return nullptr;
	}

	AGSCharacter* Body(const APlayerController* PC)
	{
		AGSCharacter* C = PC ? Cast<AGSCharacter>(PC->GetPawn()) : nullptr;
		return (C && !C->bDead) ? C : nullptr;
	}

	FCollisionShape BodyShape(const AGSCharacter* C)
	{
		const UCapsuleComponent* Cap = C->GetCapsuleComponent();
		return FCollisionShape::MakeCapsule(Cap->GetScaledCapsuleRadius(), Cap->GetScaledCapsuleHalfHeight());
	}

	bool IsFree(const UWorld* World, const AGSCharacter* C, const FVector& At)
	{
		FCollisionQueryParams Q(SCENE_QUERY_STAT(GSExploreFree), false, C);
		return !World->OverlapBlockingTestByChannel(At, FQuat::Identity, ECC_Pawn, BodyShape(C), Q);
	}

	/** The floor under At (object trace: only the world blocks, not other bodies), as the capsule centre standing on it. */
	bool FloorUnder(const UWorld* World, const AGSCharacter* C, const FVector& At, float MaxDown, FVector& Out)
	{
		FHitResult Hit;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(GSExploreFloor), false, C);
		if (!World->LineTraceSingleByObjectType(Hit, At, At - FVector(0.f, 0.f, MaxDown), FCollisionObjectQueryParams(ECC_WorldStatic), Q)) return false;
		Out = Hit.ImpactPoint + FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 3.f);
		return true;
	}

	/** A free place to stand near From: From itself, then 8 directions x 0.5-3 m, then 0.5-3 m up. */
	bool SafeSpotNear(const UWorld* World, const AGSCharacter* C, const FVector& From, FVector& Out)
	{
		auto TryAt = [&](const FVector& P) -> bool
		{
			if (!IsFree(World, C, P)) return false;
			FVector Ground;
			if (!FloorUnder(World, C, P, FloorSearchCm, Ground)) return false;
			if (!IsFree(World, C, Ground)) return false;
			Out = Ground;
			return true;
		};
		if (TryAt(From)) return true;
		static const float Steps[] = { 50.f, 100.f, 150.f, 200.f, 300.f };
		for (const float D : Steps)
		{
			for (int32 I = 0; I < 8; ++I)
			{
				const FVector Dir = FVector(1.f, 0.f, 0.f).RotateAngleAxis(45.f * I, FVector::UpVector);
				if (TryAt(From + Dir * D)) return true;
			}
		}
		for (const float D : Steps)
		{
			if (TryAt(From + FVector(0.f, 0.f, D))) return true;
		}
		return false;
	}

	/** All the action's keys ("C / V. Ctrl"): in the air every one of them works. */
	FString KeyText(const APlayerController* PC, const TCHAR* Action, const TCHAR* Default)
	{
		const FString K = GSMenu::KeyNames(PC, FName(Action));
		return K.IsEmpty() ? FString(Default) : K;
	}

	/** Above the game's own context (priority 0 in AGSPlayerController). */
	constexpr int32 FlyBlockPriority = 100;
}

void AGSExplore::UpdateKeyBlock(APlayerController* PC, bool bOn)
{
	ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* Sub = LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Sub) return;
	if (!bOn)
	{
		if (FlyBlockContext && Sub->HasMappingContext(FlyBlockContext)) Sub->RemoveMappingContext(FlyBlockContext);
		return;
	}
	if (!FlyBlockAction)
	{
		FlyBlockAction = NewObject<UInputAction>(this, TEXT("IA_GSFlyBlock"));
		FlyBlockAction->ValueType = EInputActionValueType::Boolean;
		FlyBlockAction->bConsumeInput = true;   // the point: lower contexts never see these keys
	}
	if (!FlyBlockContext) FlyBlockContext = NewObject<UInputMappingContext>(this, TEXT("IMC_GSFlyBlock"));

	TArray<FKey> Keys = GSMenu::KeysFor(PC, TEXT("IA_Jump"));
	for (const FKey& K : GSMenu::KeysFor(PC, TEXT("IA_Crouch"))) Keys.AddUnique(K);
	const bool bAdded = Sub->HasMappingContext(FlyBlockContext);
	if (Keys != BlockedKeys)
	{
		BlockedKeys = Keys;
		FlyBlockContext->UnmapAll();
		for (const FKey& K : Keys) FlyBlockContext->MapKey(FlyBlockAction, K);
		if (bAdded) Sub->RequestRebuildControlMappings();
	}
	// Re-added if the game cleared all contexts (AGSPlayerController::SetupInputComponent does on a new body).
	if (!bAdded) Sub->AddMappingContext(FlyBlockContext, GSExploreImpl::FlyBlockPriority);
}

void AGSExplore::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld()) UpdateKeyBlock(World->GetFirstPlayerController(), false);
	Super::EndPlay(EndPlayReason);
}

AGSExplore::AGSExplore()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetTickableWhenPaused(false);   // pause means standing still
	bReplicates = false;            // only ever on the one machine that plays
}

void AGSExplore::StartFlying(APlayerController* PC, AGSCharacter* C)
{
	if (!C || bFlying) return;
	UCharacterMovementComponent* Move = C->GetCharacterMovement();
	if (!Move) return;
	if (Move->IsMovingOnGround())
	{
		LastSafe = C->GetActorLocation();
		bHasLastSafe = true;
	}
	bFlying = true;
	C->SetActorEnableCollision(false);   // through walls
	Move->bCheatFlying = true;
	Move->SetMovementMode(MOVE_Flying);
	Move->MaxFlySpeed = GSExploreImpl::FlySpeed;
	Move->BrakingDecelerationFlying = 3000.f;
	Move->Velocity = FVector::ZeroVector;
	NextModeFixAt = 0.f;
	if (C->bIsCrouched) C->UnCrouch();   // the crouch key is taken by the flight from now on
	UpdateKeyBlock(PC, true);
	UE_LOG(LogTemp, Log, TEXT("GSExplore: fri flyging på"));
	C->ClientToast(FString::Printf(TEXT("Fri flyging på – %s opp, %s ned, %s fort, %s av"),
		*GSExploreImpl::KeyText(PC, TEXT("IA_Jump"), TEXT("Mellomrom")), *GSExploreImpl::KeyText(PC, TEXT("IA_Crouch"), TEXT("C")),
		*GSExploreImpl::KeyText(PC, TEXT("IA_Sprint"), TEXT("Shift")), *GSExploreImpl::KeyText(PC, TEXT("GS_Fly"), TEXT("N"))));
}

void AGSExplore::StopFlying(APlayerController* PC, AGSCharacter* C, bool bMoveToFloor)
{
	if (!bFlying) return;
	bFlying = false;
	UpdateKeyBlock(PC, false);
	if (!C) return;
	UCharacterMovementComponent* Move = C->GetCharacterMovement();
	UWorld* World = GetWorld();
	FString Where = TEXT("der du var");
	if (bMoveToFloor && World)
	{
		FVector Spot;
		if (GSExploreImpl::SafeSpotNear(World, C, C->GetActorLocation(), Spot))
		{
			C->SetActorLocation(Spot, false, nullptr, ETeleportType::TeleportPhysics);
			Where = FString::Printf(TEXT("ved %s"), *Spot.ToCompactString());
		}
		else if (bHasLastSafe)
		{
			C->SetActorLocation(LastSafe, false, nullptr, ETeleportType::TeleportPhysics);
			Where = TEXT("på siste trygge sted (ingen ledig plass her)");
		}
		else if (const AGSGameMode* GM = World->GetAuthGameMode<AGSGameMode>())
		{
			// Never stood on the ground and nothing free here: the own base (a free spawn place there), never in a wall.
			const FTransform Home = GM->PickSpawnTransform(PC);
			C->SetActorLocation(Home.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
			if (PC) PC->SetControlRotation(Home.Rotator());
			Where = TEXT("ved egen base (ingen ledig plass her og ikke stått på bakken før)");
		}
	}
	C->SetActorEnableCollision(true);
	if (Move)
	{
		Move->bCheatFlying = false;
		Move->Velocity = FVector::ZeroVector;
		Move->SetMovementMode(MOVE_Falling);
	}
	UE_LOG(LogTemp, Log, TEXT("GSExplore: fri flyging av (satt ned %s)"), *Where);
	C->ClientToast(TEXT("Fri flyging av"));
}

void AGSExplore::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	AGSCharacter* C = GSExploreImpl::Body(PC);
	if (!C) return;
	const float Now = World->GetTimeSeconds();

	// No hunger or thirst in the test (they do not go through DamageEnabled).
	if (Now >= NextFoodAt)
	{
		NextFoodAt = Now + 5.f;
		C->Hunger = 100.f;
		C->Thirst = 100.f;
	}

	UCharacterMovementComponent* Move = C->GetCharacterMovement();
	if (!Move) return;
	if (!bFlying)
	{
		if (Now >= NextSafeAt && Move->IsMovingOnGround())
		{
			NextSafeAt = Now + 0.5f;
			LastSafe = C->GetActorLocation();
			bHasLastSafe = true;
		}
		return;
	}

	// Climbing, sliding or a jump can set another movement mode: back to flying, at most five times a second.
	if (Move->MovementMode != MOVE_Flying && Now >= NextModeFixAt)
	{
		NextModeFixAt = Now + 0.2f;
		if (!bModeFixLogged)
		{
			bModeFixLogged = true;
			UE_LOG(LogTemp, Log, TEXT("GSExplore: bevegelsen ble satt til modus %d under flyging, settes tilbake"), int32(Move->MovementMode));
		}
		Move->bCheatFlying = true;
		Move->SetMovementMode(MOVE_Flying);
	}
	if (C->GetActorEnableCollision()) C->SetActorEnableCollision(false);
	UpdateKeyBlock(PC, true);   // cheap; follows rebinding and a cleared input setup

	if (GSMenu::IsModal(PC)) return;   // a menu has the keys (the game is paused then anyway)
	// Every key on the action counts (crouch is both C and Ctrl), not only the first one.
	const bool bUp = GSMenu::IsActionDown(PC, TEXT("IA_Jump"));
	const bool bDown = GSMenu::IsActionDown(PC, TEXT("IA_Crouch"));
	const bool bFast = GSMenu::IsActionDown(PC, TEXT("IA_Sprint"));
	Move->MaxFlySpeed = bFast ? GSExploreImpl::FlySprintSpeed : GSExploreImpl::FlySpeed;
	if (bUp != bDown) C->AddMovementInput(FVector::UpVector, bUp ? 1.f : -1.f);
	// Forward, back and sideways come from the normal walking keys (AGSCharacter::Move).
}

// ---------------------------------------------------------------------- public

namespace GSExplore
{
	void ToggleFly(APlayerController* PC)
	{
		AGSExplore* E = GSExploreImpl::Find(PC ? PC->GetWorld() : nullptr);
		AGSCharacter* C = GSExploreImpl::Body(PC);
		if (!E || !C) return;
		if (E->bFlying) E->StopFlying(PC, C, true);
		else E->StartFlying(PC, C);
	}

	bool IsFlying(const APlayerController* PC)
	{
		const AGSExplore* E = GSExploreImpl::Find(PC ? PC->GetWorld() : nullptr);
		return E && E->bFlying;
	}

	void Teleport(APlayerController* PC, const FVector& Target, float Yaw)
	{
		AGSExplore* E = GSExploreImpl::Find(PC ? PC->GetWorld() : nullptr);
		AGSCharacter* C = GSExploreImpl::Body(PC);
		if (!E || !C) return;
		UWorld* World = PC->GetWorld();
		const FVector Before = C->GetActorLocation();
		const float BeforeYaw = PC->GetControlRotation().Yaw;
		const bool bWasFlying = E->bFlying;
		if (bWasFlying) E->StopFlying(PC, C, false);   // collision back on before the move
		// A floor near the target (from 1.5 m above it, at most 5 m down), else 1 m above the point itself.
		FVector Dest = Target + FVector(0.f, 0.f, 100.f);
		FVector Ground;
		if (GSExploreImpl::FloorUnder(World, C, Target + FVector(0.f, 0.f, 150.f), 650.f, Ground)) Dest = Ground;
		const FRotator Rot(0.f, Yaw, 0.f);
		if (C->TeleportTo(Dest, Rot))
		{
			E->PrevSpot = Before;
			E->PrevYaw = BeforeYaw;
			E->bHasPrevSpot = true;
			PC->SetControlRotation(Rot);
			if (UCharacterMovementComponent* Move = C->GetCharacterMovement()) Move->Velocity = FVector::ZeroVector;
			UE_LOG(LogTemp, Log, TEXT("GSExplore: flyttet til %s"), *Dest.ToCompactString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("GSExplore: fikk ikke flyttet til %s, blir stående"), *Dest.ToCompactString());
			if (bWasFlying)
			{
				// Back on the ground where we were (the flight may have ended inside a wall).
				E->bFlying = true;
				E->StopFlying(PC, C, true);
			}
			C->ClientToast(TEXT("Det er ikke plass der akkurat nå."));
		}
	}

	bool HasPreviousSpot(const APlayerController* PC)
	{
		const AGSExplore* E = GSExploreImpl::Find(PC ? PC->GetWorld() : nullptr);
		return E && E->bHasPrevSpot;
	}

	void TeleportBack(APlayerController* PC)
	{
		const AGSExplore* E = GSExploreImpl::Find(PC ? PC->GetWorld() : nullptr);
		if (!E || !E->bHasPrevSpot) return;
		const FVector Spot = E->PrevSpot;
		const float Yaw = E->PrevYaw;
		// The spot is the capsule centre: aim the floor search at the feet.
		Teleport(PC, Spot - FVector(0.f, 0.f, 90.f), Yaw);
	}

	void SetClock(APlayerController* PC, float Hours)
	{
		AGSGameState* GS = PC && PC->GetWorld() ? PC->GetWorld()->GetGameState<AGSGameState>() : nullptr;
		if (!GS || !GS->HasAuthority() || GS->GameKind != GSGameKind::Explore) return;
		GS->StartHour = FMath::Fmod(FMath::Max(0.f, Hours), 24.f);
		GS->ClockStartedAt = GS->GetServerWorldTimeSeconds();
		UE_LOG(LogTemp, Log, TEXT("GSExplore: klokke %02d, vær %s"), FMath::FloorToInt(GS->StartHour), GSNames::Weather(GS->Weather));
	}

	void SetWeather(APlayerController* PC, EGSWeather Weather)
	{
		AGSGameState* GS = PC && PC->GetWorld() ? PC->GetWorld()->GetGameState<AGSGameState>() : nullptr;
		if (!GS || !GS->HasAuthority() || GS->GameKind != GSGameKind::Explore || Weather >= EGSWeather::MAX) return;
		GS->PreviousWeather = Weather;
		GS->Weather = Weather;
		GS->WeatherChangedAt = GS->GetServerWorldTimeSeconds() - AGSGameState::WeatherBlendSeconds;   // fully there at once
		GS->HoldWeather(24.f * 3600.f);
		UE_LOG(LogTemp, Log, TEXT("GSExplore: klokke %02d, vær %s"), FMath::FloorToInt(GS->GetTimeOfDayHours()), GSNames::Weather(Weather));
	}
}
