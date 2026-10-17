// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GVehicleModule.h"
#include "GVehicleWheelModule.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGVehicleWheelModule : public UGVehicleModule
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheel")
	int32 wheelIndex = 0;
protected:
	void OnDestroyed() override;
};
