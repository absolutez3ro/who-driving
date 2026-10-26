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
	UPROPERTY(EditAnywhere, Category = "Default Values", meta = (ToolTip = "Базовое здоровье машины. Не является общим здоровьем машины!"))
	float BaseCarHealth;

	UPROPERTY(EditAnywhere, Category = "Default Values", meta = (ToolTip = "Максимальный крутящий момент двигателя"))
	float MaxTorque;

	UPROPERTY(EditAnywhere, Category = "Default Values", meta = (ToolTip = "Максимальное количество оборотов в минуту"))
	float MaxRPM;
	
	UPROPERTY(EditAnywhere, Category = "Default Values", meta = (ToolTip = "Количество оборотов в минуту в покое"))
	float EngineIdleRPM;

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

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0", ToolTip = "Коэффициент трения сломанных колёс"))
	float BrokenWheelFrictionMultiplier;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMax = "90", ClampMin = "0.0", ToolTip = "Угол поворота сломанного руля (в градусах)"))
	float BrokenSteeringWheelAngle;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0", ToolTip = "Коэффициент крутящего момента сломанного двигателя"))
	float BrokenEngineTorqueMultiplier;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0", ToolTip = "Коэффициент оборотов в минуту сломанного двигателя"))
	float BrokenEngineRPMMultiplier;

	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ToolTip = "Максимальная скорость машины со сломанным двигателем (км/ч)"))
	float BrokenEngineMaxSpeed;
};
