// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GMenuGameState.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API AGMenuGameState : public AGameStateBase
{
	GENERATED_BODY()
	
protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
