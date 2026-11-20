// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GVehicleModule.h"
#include "GVehicleDoorModule.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDoorChangeState, FName, moduleId, EGModuleState, newState);

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGVehicleDoorModule : public UGVehicleModule
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	int32 doorId = INDEX_NONE;
	EGComponentSide moduleSide;
	FOnDoorChangeState OnDoorChangeState;
protected:
	void OnDestroyed() override;
	void OnStateChanged(EGModuleState oldState, EGModuleState newState) override;
};
