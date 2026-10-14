// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GAIControllerCommonCar.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API AGAIControllerCommonCar : public AAIController
{
	GENERATED_BODY()
private:
	float acceleration=0;
	void CalculateAcceleration(float deltaX, float deltaSeconds);
	void ApplyAcceleration();
public:
	UPROPERTY(EditAnywhere, Category ="movement")
	float baseAcceleration = 1;

	UPROPERTY(EditAnywhere, Category = "movement")
	float maxAcceleration = 1000;

	UPROPERTY(EditAnywhere, Category = "movement")
	float minAcceleration = -1000;



	UFUNCTION(BlueprintCallable, Category = "Chase")
	void Chase(AActor* carReference, float deltaSeconds);
};
