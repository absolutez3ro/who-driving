// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSkillCheckDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGSkillCheckDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skillcheck")
	int32 TotalCyclesForModuleInRepairState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skillcheck")
	int32 TotalCyclesForDestroyedState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skillcheck")
	float BaseGoodZoneWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skillcheck")
	float MaxHealthBonusWidth;
};
