// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "AIController.h"
#include "GAIControllerCommonCar.generated.h"

class AGBaseEnemyCar;
class AGPlayerCar;
class UGCommonEnemyDataAsset;

UCLASS()
class WHOS_DRIVING_API AGAIControllerCommonCar : public AAIController
{
	GENERATED_BODY()

public:
	void BeginPlay() override;
	void Tick(float deltaSeconds) override;

	UFUNCTION(Server, Reliable)
	void Shoot();

	UFUNCTION(BlueprintCallable, Category = "Chase")
	void Chase(float deltaSeconds);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGCommonEnemyDataAsset> commonCarData;

private:
	void CalculateAcceleration(FVector toPlayerVector, FVector playerForward, float deltaSeconds);
	void ApplyAcceleration(float distance, FVector playerForward);
	void StartShootDelay();
	void SetPlayerRef();

	FVector acceleration;
	float baseAcceleration = 1;
	float maxAcceleration = 1000;
	float minAcceleration = -1000;
	float minYDistance = 100;
	float maxYDistance = 500;
	float yDistance;
	float fireRate = 1;
	float hitChance = 1;
	float minDamage = 1;
	float maxDamage = 1;
	TArray<float> weights;
	TArray<FName> moduleNames[6];
	FTimerHandle shootDelayHandle;
	TObjectPtr<USoundBase> hitSound;
	TObjectPtr<USoundBase> missSound;
	AGPlayerCar* playerCarRef;
};
