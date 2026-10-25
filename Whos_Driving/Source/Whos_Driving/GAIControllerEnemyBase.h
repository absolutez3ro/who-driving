// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GAIControllerEnemyBase.generated.h"

/**
 * 
 */
class UGEnemyBaseDataAsset;

UCLASS()
class WHOS_DRIVING_API AGAIControllerEnemyBase : public AAIController
{
	GENERATED_BODY()
private:
	float acceleration = 0;
	//void CalculateAcceleration(float deltaX, float deltaSeconds);
	//void ApplyAcceleration();
	float baseAcceleration = 1;
	float maxAcceleration = 1000;
	float minAcceleration = -1000;
public:
	void BeginPlay() override;
};
