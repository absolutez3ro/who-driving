// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GVehicleModule.h"
#include "GVehicleElectricianModule.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnElectricianChangeState, EGModuleState, newState);

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGVehicleElectricianModule : public UGVehicleModule
{
	GENERATED_BODY()
public:
	void ApplyRepair() override;
	FOnElectricianChangeState OnElectricianChangeState;
protected:
	void OnDestroyed() override;
};
