// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GVehicleModule.h"
#include "GVehicleElectricianModule.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGVehicleElectricianModule : public UGVehicleModule
{
	GENERATED_BODY()
	
protected:
	void OnDestroyed() override;
};
