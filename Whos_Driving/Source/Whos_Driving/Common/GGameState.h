// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourcesInfoUpdate, int32, repairParts);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerFinish);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimerUpdate);


UCLASS()
class WHOS_DRIVING_API AGGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	AGGameState();
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

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Race|Timer")
	float remainingTime = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Race|Timer")
	float elapsedTime = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Race|Timer")
	bool bTimerRunning = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Race|Timer")
	void StartRaceTimer();

	UFUNCTION(BlueprintCallable, Category = "Race|Timer")
	void StopRaceTimer();

	UFUNCTION(BlueprintPure, Category = "Race|Timer")
	bool IsTimerRunning() const;

	UFUNCTION(BlueprintPure, Category = "Race|Timer")
	FText GetRemainingTimeText() const;

	UPROPERTY(BlueprintAssignable)
	FOnTimerUpdate OnTimerUpdate;

protected:
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 repairParts = 0;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 enemyKills = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Race|Timer")
	float timeBonusThreshold = 10.0f;

	void Tick(float deltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Race|Timer")
	void OnRaceTimeExpired();

	void BroadcastTimerUpdate();
};
