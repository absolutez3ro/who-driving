// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GEconomicDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGEconomicDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economic")
	int32 EnemiesKillsToMoneyConversionRate = 200;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economic")
	int32 PartsToMoneyConversionRate = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economic")
	float TimeBonusMultiplier = 2.0f;
};
