// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GPlayersDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGPlayersDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Health")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Health")
	float RegenCooldownSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Health")
	float HealthRegenRate = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Health")
	float ReviveHealth = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Health")
	float ReviveTimeSeconds = 1.0f;
};
