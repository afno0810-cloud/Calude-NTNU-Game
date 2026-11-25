#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GSPlayerState.generated.h"

UCLASS()
class GLOSHAUGEN_API AGSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadOnly) int32 Kills = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 Deaths = 0;
};
