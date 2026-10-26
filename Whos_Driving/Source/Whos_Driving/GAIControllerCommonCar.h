// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GAIControllerCommonCar.generated.h"

/**
 * 
 */

class AGPlayerCar;
class UGCommonEnemyDataAsset;

UCLASS()
class WHOS_DRIVING_API AGAIControllerCommonCar : public AAIController
{
	GENERATED_BODY()
private:
	FVector acceleration;
	void CalculateAcceleration(FVector toPlayerVector, FVector playerForward, float deltaSeconds);
	void ApplyAcceleration(float distance, FVector playerForward);
	float baseAcceleration = 1;
	float maxAcceleration = 1000;
	float minAcceleration = -1000;
	float minYDistance = 100;
	float maxYDistance = 500;
	float YDistance;
	float fireRate=1;
	float hitChance=1;
	float minDamage=1;
	float maxDamage=1;
	TArray<float> weights;
	TArray<FName> module_names[6];
	FTimerHandle shootDelayHandle;
	AGPlayerCar* playerCarRef;
	void StartShootDelay();
	void SetPlayerRef();
public:
	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;
	UFUNCTION(Server, Reliable)
	void Shoot();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGCommonEnemyDataAsset> commonCarData;
	UFUNCTION(BlueprintCallable, Category = "Chase")
	void Chase(float deltaSeconds);
};
