// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GVehicleModule.h"
#include "GVehicleDoorModule.generated.h"

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

	void ApplyRepair() override;
protected:
	void OnDestroyed() override;
};
