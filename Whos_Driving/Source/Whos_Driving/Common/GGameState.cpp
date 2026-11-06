// Fill out your copyright notice in the Description page of Project Settings.


#include "Common/GGameState.h"
#include "Net/UnrealNetwork.h"

void AGGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGGameState, repairParts);
}
