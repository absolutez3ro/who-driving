// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GVehicleModule.h"
#include "GVehicleSteeringWheelModule.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSteeringWheelChangeState, EGModuleState, newState);

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGVehicleSteeringWheelModule : public UGVehicleModule
{
	GENERATED_BODY()
public:
	void ApplyRepair() override;
	FOnSteeringWheelChangeState OnSteeringWheelChangeState;
protected:
	void OnDestroyed() override;
	void OnStateChanged(EGModuleState oldState, EGModuleState newState) override;
};
