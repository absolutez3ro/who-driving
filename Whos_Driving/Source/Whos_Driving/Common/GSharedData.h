
/**/
#pragma once

#include "CoreMinimal.h"
#include "GPlayerCar.h"
#include "GSharedData.generated.h"

USTRUCT(BlueprintType)
struct FCarData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TArray<FGVehicleModuleState> moduleStates;
    UPROPERTY(BlueprintReadOnly)
    float currentCarHealth=5;
    UPROPERTY(BlueprintReadOnly)
    float baseCarHealth=10;
};

USTRUCT(BlueprintType)
struct FCarUpgradeData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TArray<int32> moduleLevels;
    UPROPERTY(BlueprintReadOnly)
    int32 generalHelthlevel=0;
};

USTRUCT(BlueprintType)
struct FCharacterUpgradeData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 damageLevel=0;
    UPROPERTY(BlueprintReadOnly)
    int32 ammoLevel=0;
};
