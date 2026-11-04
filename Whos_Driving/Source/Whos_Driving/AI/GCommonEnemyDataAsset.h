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
	float lookAheadDistance = 800.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float lookAheadTime = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float maxSpeed = 1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float maxSteerAngle = 35.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float steerInterpSpeed = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float cornerSlowdown = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float throttleGain = 0.002f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float brakeGain = 0.002f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float laneOffset = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float stopDistance = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float roadWidth = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float catchUpModificator = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float hitRange = 500000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float FireRate;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float HitChance;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float MinDamage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float MaxDamage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float WheelWeight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float DoorWeight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float BaseHealthWeight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float SteeringWeight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float EngineWeight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "combat")
	float ElectronicsWeight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> HitSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> MissSound;
};
