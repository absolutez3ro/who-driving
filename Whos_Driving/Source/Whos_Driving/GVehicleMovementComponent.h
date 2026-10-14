// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GVehicleMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGVehicleMovementComponent : public UChaosWheeledVehicleMovementComponent
{
	GENERATED_BODY()
	
public:
	UGVehicleMovementComponent();
	void DisableWheel(int32 wheelIndex);
};
