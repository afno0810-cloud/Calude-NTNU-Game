#include "GSPlayerState.h"
#include "Net/UnrealNetwork.h"

void AGSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGSPlayerState, Kills);
	DOREPLIFETIME(AGSPlayerState, Deaths);
	DOREPLIFETIME(AGSPlayerState, bReady);
}
