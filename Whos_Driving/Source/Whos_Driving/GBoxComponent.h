// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GBoxComponent.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGBoxComponent : public UBoxComponent
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Transient)
	FName moduleId = NAME_None;
};
