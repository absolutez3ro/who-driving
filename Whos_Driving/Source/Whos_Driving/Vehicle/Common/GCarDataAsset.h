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
	UPROPERTY(EditAnywhere, Category = "General Values", meta = (ToolTip = "Base car health. This is not the total car health!"))
	float BaseCarHealth;




	UPROPERTY(EditAnywhere, Category = "Acceleration | Upgrades", meta = (ToolTip = "base maximum engine torque"))
	float baseTorque;

	UPROPERTY(EditAnywhere, Category = "Acceleration | Upgrades", meta = (ToolTip = "maximum engine torque per level"))
	float torquePerLevel;

	UPROPERTY(EditAnywhere, Category = "max speed | Upgrades", meta = (ToolTip = "base maximum engine RPM"))
	float baseMaxRPM;

	UPROPERTY(EditAnywhere, Category = "max speed | Upgrades", meta = (ToolTip = "maximum engine RPM per level"))
	float maxRPMPerLevel;

	UPROPERTY(EditAnywhere, Category = "Acceleration | Upgrades", meta = (ToolTip = "base rev up moi value"))
	float baseRevUp;

	UPROPERTY(EditAnywhere, Category = "Acceleration | Upgrades", meta = (ToolTip = "rev up moi value per level"))
	float revUpPerLevel;

	UPROPERTY(EditAnywhere, Category = "Acceleration | Upgrades", meta = (ToolTip = "base idle engine RPM"))
	float baseIdleRPM;

	UPROPERTY(EditAnywhere, Category = "Acceleration | Upgrades", meta = (ToolTip = "idle engine RPM per level"))
	float IdleRPMPerLevel;

	UPROPERTY(EditAnywhere, Category = "Acceleration | Upgrades", meta = (ToolTip = "starting level of acceleration (affects idle rpm, rev up, torque)"))
	int32 startingAccelerationLevel;

	UPROPERTY(EditAnywhere, Category = "max speed | Upgrades", meta = (ToolTip = "starting level of acceleration (affects max rpm)"))
	int32 startingMaxSpeedLevel;

	UPROPERTY(EditAnywhere, Category = "Acceleration | Upgrades", meta = (ToolTip = "maximum level of acceleration (affects idle rpm, rev up, torque)"))
	int32 maxAccelerationLevel;

	UPROPERTY(EditAnywhere, Category = "max speed | Upgrades", meta = (ToolTip = "maximum level of acceleration (affects max rpm)"))
	int32 maxMaxSpeedLevel;





	UPROPERTY(EditAnywhere, Category = "Modules|Wheels")
	float WheelBaseMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|Wheels")
	int32 WheelHpLevel;

	UPROPERTY(EditAnywhere, Category = "Modules|Wheels")
	float WheelHpIncreasePerLevel;

	UPROPERTY(EditAnywhere, Category = "Modules|Wheels", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WheelRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Modules|Wheels", meta = (ClampMin = "0"))
	int32 WheelBaseRepairCost;

	UPROPERTY(EditAnywhere, Category = "Modules|Wheels", meta = (ClampMin = "1.0"))
	float WheelDestroyedModuleCostMultiplier;

	UPROPERTY(EditAnywhere, Category = "Modules|Doors")
	float DoorBaseMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|Doors")
	int32 DoorHpLevel;

	UPROPERTY(EditAnywhere, Category = "Modules|Doors")
	float DoorHpIncreasePerLevel;

	UPROPERTY(EditAnywhere, Category = "Modules|Doors", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DoorRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Modules|Doors", meta = (ClampMin = "0"))
	int32 DoorBaseRepairCost;

	UPROPERTY(EditAnywhere, Category = "Modules|Doors", meta = (ClampMin = "1.0"))
	float DoorDestroyedModuleCostMultiplier;

	UPROPERTY(EditAnywhere, Category = "Modules|Engine")
	float EngineBaseMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|Engine")
	int32 EngineHpLevel;

	UPROPERTY(EditAnywhere, Category = "Modules|Engine")
	float EngineHpIncreasePerLevel;

	UPROPERTY(EditAnywhere, Category = "Modules|Engine", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EngineRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Modules|Engine", meta = (ClampMin = "0"))
	int32 EngineBaseRepairCost;

	UPROPERTY(EditAnywhere, Category = "Modules|Engine", meta = (ClampMin = "1.0"))
	float EngineDestroyedCostMultiplier;

	UPROPERTY(EditAnywhere, Category = "Modules|SteeringWheel")
	float SteeringWheelBaseMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|SteeringWheel")
	int32 SteeringWheelHpLevel;

	UPROPERTY(EditAnywhere, Category = "Modules|SteeringWheel")
	float SteeringWheelHpIncreasePerLevel;

	UPROPERTY(EditAnywhere, Category = "Modules|SteeringWheel", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SteeringWheelRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Modules|SteeringWheel", meta = (ClampMin = "0"))
	int32 SteeringWheelBaseRepairCost;

	UPROPERTY(EditAnywhere, Category = "Modules|SteeringWheel", meta = (ClampMin = "1.0"))
	float SteeringWheelDestroyedCostMultiplier;

	UPROPERTY(EditAnywhere, Category = "Modules|Electrician")
	float ElectricianBaseMaxHealth;

	UPROPERTY(EditAnywhere, Category = "Modules|Electrician")
	int32 ElectricianHpLevel;

	UPROPERTY(EditAnywhere, Category = "Modules|Electrician")
	float ElectricianHpIncreasePerLevel;

	UPROPERTY(EditAnywhere, Category = "Modules|Electrician", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ElectricianRepairThreshold;

	UPROPERTY(EditAnywhere, Category = "Modules|Electrician", meta = (ClampMin = "0"))
	int32 ElectricianBaseRepairCost;

	UPROPERTY(EditAnywhere, Category = "Modules|Electrician", meta = (ClampMin = "1.0"))
	float ElectricianDestroyedCostMultiplier;

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

	UPROPERTY(EditAnywhere, Category = "SkillCheck", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float FailureDamagePercent;
};
