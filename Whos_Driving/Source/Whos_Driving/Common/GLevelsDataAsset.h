// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GLevelsDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGLevelsDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Race|Timer", meta = (ClampMin = "0.0"))
	float FirstLevelTimerThreshold = 0.0f;
};
