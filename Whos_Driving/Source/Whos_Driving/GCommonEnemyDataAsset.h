// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GCommonEnemyDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGCommonEnemyDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float minAcceleration;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float maxAcceleration;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float accelerationModifier;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float minYDistance;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float maxYDistance;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float fireRate;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float hitChance;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float minDamage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float maxDamage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float wheelWeight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float doorWeight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float baseHealthWeight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float steeringWeight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float engineWeight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float electronicsWeight;
};
