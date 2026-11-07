// Fill out your copyright notice in the Description page of Project Settings.


#include "Common/GGameState.h"
#include "Net/UnrealNetwork.h"
#include "GGIWhosDriving.h"

void AGGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGGameState, repairParts);
	DOREPLIFETIME(AGGameState, enemyKills);
	DOREPLIFETIME(AGGameState, timeBonusThreshold);
}

int32 AGGameState::GetRepairParts() const
{
	return repairParts;
}

int32 AGGameState::GetEnemyKills() const
{
	return enemyKills;
}


void AGGameState::HandleEnemyDeath(int32 scrapPrice)
{
	PlusRepairParts(scrapPrice);
	IncrementEnemiesKills();
}

void AGGameState::Multicast_OnPlayerFinish_Implementation()
{
	UGGIWhosDriving* gameInstance = GetGameInstance<UGGIWhosDriving>();
	gameInstance->CalculateMoneyOnFinish();

	OnPlayerFinish.Broadcast();
}

void AGGameState::PlusRepairParts(int32 amount)
{
	repairParts += amount;
	if (repairParts < 0)
	{
		repairParts = 0;
	}
	OnResourcesInfoUpdate.Broadcast(repairParts);
}

void AGGameState::IncrementEnemiesKills()
{
	enemyKills++;
}

