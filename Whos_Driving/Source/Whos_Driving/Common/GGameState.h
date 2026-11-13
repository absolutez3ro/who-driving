// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Whos_DrivingGameMode.h"
#include "GGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourcesInfoUpdate, int32, repairParts);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerFinish);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRaceFailed, EGRaceFailureReason, reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRaceFailedCleared);

class UGLevelsDataAsset;

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

	UFUNCTION()
	void HandleEnemyDeath(int32 scrapPrice);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnPlayerFinish();

	UFUNCTION(BlueprintCallable)
	bool IsRaceFailed() const;
	UFUNCTION(BlueprintCallable)
	EGRaceFailureReason GetFailureReason() const;

	void SetRaceFailed(EGRaceFailureReason reason);
	void ClearRaceFailed();

	UPROPERTY(BlueprintAssignable)
	FOnPlayerFinish OnPlayerFinish;

	UPROPERTY(BlueprintAssignable)
	FOnResourcesInfoUpdate OnResourcesInfoUpdate;

	UPROPERTY(BlueprintAssignable)
	FOnRaceFailed OnRaceFailed;

	UPROPERTY(BlueprintAssignable)
	FOnRaceFailedCleared OnRaceFailedCleared;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Race|Timer")
	float remainingTime = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Race|Timer")
	float elapsedTime = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Race|Timer")
	bool bTimerRunning = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Race|Timer")
	bool bHaveTime = true;

	UFUNCTION(BlueprintCallable, Category = "Race|Timer")
	void StartRaceTimer();

	UFUNCTION(BlueprintCallable, Category = "Race|Timer")
	void StopRaceTimer();

	UFUNCTION(BlueprintPure, Category = "Race|Timer")
	bool IsTimerRunning() const;

	UFUNCTION(BlueprintPure, Category = "Race|Timer")
	FText GetRemainingTimeText() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGLevelsDataAsset> levelsData;

protected:
	void BeginPlay() override;
	void Tick(float deltaTime) override;

	UFUNCTION()
	void OnRep_RaceFailed();

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 repairParts = 100;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 enemyKills = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Race|Timer")
	float timeBonusThreshold = 10.0f;

	UPROPERTY(ReplicatedUsing = OnRep_RaceFailed, BlueprintReadOnly, Category = "Race")
	bool bRaceFailed = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Race")
	EGRaceFailureReason failureReason = EGRaceFailureReason::None;

	void BroadcastTimerUpdate();
};
