// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GVehicleModule.h"
#include "GVehicleEngineModule.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEngineChangeState, EGModuleState, newState);

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGVehicleEngineModule : public UGVehicleModule
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable)
	FOnEngineChangeState OnEngineChangeState;

protected:
	void OnDestroyed() override;
	void OnStateChanged(EGModuleState oldState, EGModuleState newState) override;
};
