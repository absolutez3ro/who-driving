// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GVehicleModule.h"
#include "GVehicleWheelModule.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWheelBreak, int32, wheelIndex);

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
	void ApplyRepair() override;
	FOnWheelBreak OnWheelBreak;

protected:
	void OnDestroyed() override;
};
