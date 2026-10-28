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
	float MinAcceleration;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float MaxAcceleration;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float AccelerationModifier;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float MinYDistanceCM;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "movement")
	float MaxYDistanceCM;
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
