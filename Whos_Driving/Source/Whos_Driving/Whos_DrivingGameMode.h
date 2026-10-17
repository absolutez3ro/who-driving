// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Whos_DrivingGameMode.generated.h"

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class AWhos_DrivingGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWhos_DrivingGameMode();
};



