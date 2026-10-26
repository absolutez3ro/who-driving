// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GVehicleModule.h"
#include "GVehicleSteeringWheelModule.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGVehicleSteeringWheelModule : public UGVehicleModule
{
	GENERATED_BODY()
public:
	void ApplyRepair() override;

protected:
	void OnDestroyed() override;
};
