// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GCarDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGCarDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Default Values", meta = (ToolTip = "Base car health. This is not the total car health!"))
	float baseCarHealth;

	UPROPERTY(EditAnywhere, Category = "Default Values", meta = (ToolTip = "Maximum engine torque"))
	float maxTorque;

	UPROPERTY(EditAnywhere, Category = "Modules|Wheels")
	float wheelMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|Wheels", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float wheelRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Modules|Doors")
	float doorMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|Doors", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float doorRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Modules|Engine")
	float engineMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|Engine", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float engineRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Modules|SteeringWheel")
	float steeringWheelMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|SteeringWheel", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float steeringWheelRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Modules|Electrician")
	float electricianMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|Electrician", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float electricianRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0", ToolTip = "Friction multiplier of broken wheels"))
	float brokenWheelFrictionMultiplier;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMax = "90", ClampMin = "0.0", ToolTip = "Steering angle of a broken steering wheel (in degrees)"))
	float brokenSteeringWheelAngle;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0", ToolTip = "Torque multiplier of a broken engine"))
	float brokenEngineTorqueMultiplier;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0", ToolTip = "RPM multiplier of a broken engine"))
	float brokenEngineRpmMultiplier;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ToolTip = "Maximum car speed with a broken engine (km/h)"))
	float brokenEngineMaxSpeed;
};
