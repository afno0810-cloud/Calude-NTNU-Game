#include "GSPlayerState.h"
#include "GSGameMode.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

void AGSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSPlayerState, Kills);
	DOREPLIFETIME(AGSPlayerState, Deaths);
	DOREPLIFETIME(AGSPlayerState, bReady);
	DOREPLIFETIME(AGSPlayerState, Team);
	DOREPLIFETIME(AGSPlayerState, CarriedFlagTeam);
	DOREPLIFETIME(AGSPlayerState, Captures);
	DOREPLIFETIME(AGSPlayerState, Returns);
}

void AGSPlayerState::ServerRequestTeam_Implementation(uint8 NewTeam)
{
	if (AGSGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AGSGameMode>() : nullptr)
	{
		GM->RequestTeam(this, NewTeam);
	}
}
