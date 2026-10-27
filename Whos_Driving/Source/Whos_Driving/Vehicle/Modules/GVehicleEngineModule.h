// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GVehicleModule.h"
#include "GVehicleEngineModule.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGVehicleEngineModule : public UGVehicleModule
{
	GENERATED_BODY()
public:
	void ApplyRepair() override;

protected:
	void OnDestroyed() override;
};
