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
	float BaseCarHealth;

	UPROPERTY(EditAnywhere, Category = "Default Values", meta = (ToolTip = "Maximum engine torque"))
	float MaxTorque;

	UPROPERTY(EditAnywhere, Category = "Modules|Wheels")
	float WheelMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|Wheels", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WheelRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Modules|Doors")
	float DoorMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|Doors", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DoorRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Modules|Engine")
	float EngineMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|Engine", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EngineRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Modules|SteeringWheel")
	float SteeringWheelMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|SteeringWheel", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SteeringWheelRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Modules|Electrician")
	float ElectricianMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|Electrician", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ElectricianRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0", ToolTip = "Friction multiplier of broken wheels"))
	float BrokenWheelFrictionMultiplier;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMax = "90", ClampMin = "0.0", ToolTip = "Steering angle of a broken steering wheel (in degrees)"))
	float BrokenSteeringWheelAngle;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0", ToolTip = "Torque multiplier of a broken engine"))
	float BrokenEngineTorqueMultiplier;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0", ToolTip = "RPM multiplier of a broken engine"))
	float BrokenEngineRPMMultiplier;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ToolTip = "Maximum car speed with a broken engine (km/h)"))
	float BrokenEngineMaxSpeed;
};
