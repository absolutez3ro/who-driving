// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourcesInfoUpdate, int32, repairParts);
/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API AGGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	int32 GetRepairParts() const;
	void PlusRepairParts(int32 amount);
	
	UPROPERTY(BlueprintAssignable)
	FOnResourcesInfoUpdate OnResourcesInfoUpdate;

	UFUNCTION()
	void HandleEnemyDeath(int32 scrapPrice);

protected:
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 repairParts;
};
