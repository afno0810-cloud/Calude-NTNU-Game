#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSTerminal.generated.h"

class UStaticMeshComponent;
class UCameraComponent;
class UPointLightComponent;

UENUM()
enum class EGSTerminalKind : uint8
{
	Computer,   // security cameras + player tracking
	Printer     // 3D printer: turns filament into items
};

/** One thing a 3D printer can make. */
struct FGSPrintRecipe
{
	FName ItemId;
	int32 Filament;   // spools of filament it costs
	float Seconds;    // print time
};

/** A PC or a 3D printer you can walk up to and use with E. */
UCLASS()
class GLOSHAUGEN_API AGSTerminal : public AActor
{
	GENERATED_BODY()

public:
	AGSTerminal();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(ReplicatedUsing = OnRep_Kind) EGSTerminalKind Kind = EGSTerminalKind::Computer;
	/** Name shown on screen, like "PC i Lysgaarden". */
	UPROPERTY(Replicated) FString Label;

	// --- Printer state ---
	UPROPERTY(Replicated) FName PrintingId;
	UPROPERTY(Replicated) float PrintStart = 0.f;
	UPROPERTY(Replicated) float PrintEnd = 0.f;
	bool IsBusy() const { return !PrintingId.IsNone(); }

	/** Server: start printing recipe Index for Who. Returns an error text, or empty on success. */
	FString StartPrint(class AGSCharacter* Who, int32 Index);
	/** Server, match start: stop a print that is running (nothing comes out). */
	void CancelPrint();

	static const TArray<FGSPrintRecipe>& Recipes();

	/** Where the user stands / where printed items appear. */
	FVector FrontPoint(float Distance = 90.f) const;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Screen;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Glow;

	UFUNCTION() void OnRep_Kind();
	void ApplyLook();
	void FinishPrint();
	FTimerHandle PrintTimer;
};

/** A security camera on a wall. PCs can look through it. Always relevant so every client knows the list. */
UCLASS()
class GLOSHAUGEN_API AGSSecurityCamera : public AActor
{
	GENERATED_BODY()

public:
	AGSSecurityCamera();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(Replicated) FString Label;
	/** Degrees it sweeps left/right of its base direction. */
	UPROPERTY(Replicated) float SweepDeg = 35.f;

	UCameraComponent* GetCamera() const { return Camera; }

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> RecLight;
	float Phase = 0.f;
};
