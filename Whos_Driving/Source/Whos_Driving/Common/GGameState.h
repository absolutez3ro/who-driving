// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourcesInfoUpdate, int32, repairParts);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerFinish);


UCLASS()
class WHOS_DRIVING_API AGGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	int32 GetRepairParts() const;
	int32 GetEnemyKills() const;
	void PlusRepairParts(int32 amount);
	void IncrementEnemiesKills();
	
	UPROPERTY(BlueprintAssignable)
	FOnResourcesInfoUpdate OnResourcesInfoUpdate;

	UFUNCTION()
	void HandleEnemyDeath(int32 scrapPrice);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnPlayerFinish();

	UPROPERTY(BlueprintAssignable)
	FOnPlayerFinish OnPlayerFinish;


protected:
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 repairParts = 0;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 enemyKills = 0;

	UPROPERTY(Replicated, BlueprintReadOnly)
	float timeBonusThreshold = 10.0f;
};
