// Fill out your copyright notice in the Description page of Project Settings.


#include "Common/GGameState.h"
#include "Net/UnrealNetwork.h"
#include "GGIWhosDriving.h"

AGGameState::AGGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AGGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGGameState, repairParts);
	DOREPLIFETIME(AGGameState, enemyKills);
	DOREPLIFETIME(AGGameState, remainingTime);
	DOREPLIFETIME(AGGameState, elapsedTime);
	DOREPLIFETIME(AGGameState, bTimerRunning);
	DOREPLIFETIME(AGGameState, bHaveTime);
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

void AGGameState::StartRaceTimer()
{
	if (!HasAuthority())
	{
		return;
	}

	remainingTime = timeBonusThreshold;
	elapsedTime = 0.0f;
	bTimerRunning = true;
	bHaveTime = true;
	SetActorTickEnabled(true);
	ForceNetUpdate();
	BroadcastTimerUpdate();
}

void AGGameState::StopRaceTimer()
{
	if (!HasAuthority())
	{
		return;
	}

	bTimerRunning = false;
	SetActorTickEnabled(false);
	ForceNetUpdate();
	BroadcastTimerUpdate();
}

bool AGGameState::IsTimerRunning() const
{
	return bTimerRunning;
}

void AGGameState::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	if (!HasAuthority() || !bTimerRunning)
	{
		return;
	}

	elapsedTime += deltaTime;
	remainingTime = FMath::Max(0.0f, timeBonusThreshold - elapsedTime);

	static constexpr float broadcastInterval = 0.1f;
	static float timeSinceLastBroadcast = 0.0f;
	timeSinceLastBroadcast += deltaTime;
	if (timeSinceLastBroadcast >= broadcastInterval)
	{
		timeSinceLastBroadcast = 0.0f;
		BroadcastTimerUpdate();
	}

	if (remainingTime <= 0.0f)
	{
		bTimerRunning = false;
		bHaveTime = false;
		SetActorTickEnabled(false);
		ForceNetUpdate();
		BroadcastTimerUpdate();
	}
}


void AGGameState::BroadcastTimerUpdate()
{
	OnTimerUpdate.Broadcast();
	ForceNetUpdate();
}

FText AGGameState::GetRemainingTimeText() const
{
	const int32 totalSeconds = FMath::CeilToInt(remainingTime);
	const int32 minutes = totalSeconds / 60;
	const int32 seconds = totalSeconds % 60;
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), minutes, seconds));
}