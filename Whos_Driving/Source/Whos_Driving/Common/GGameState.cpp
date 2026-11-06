// Fill out your copyright notice in the Description page of Project Settings.


#include "Common/GGameState.h"
#include "Net/UnrealNetwork.h"

void AGGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGGameState, repairParts);
	DOREPLIFETIME(AGGameState, money);
}

int32 AGGameState::GetRepairParts() const
{
	return repairParts;
}

void AGGameState::PlusRepairParts(int32 amount)
{
	repairParts += amount;
	if (repairParts < 0)
	{
		repairParts = 0;
	}
	OnResourcesInfoUpdate.Broadcast(repairParts, money);
}

void AGGameState::HandleEnemyDeath(int32 scrapPrice)
{
	PlusRepairParts(scrapPrice);
}
