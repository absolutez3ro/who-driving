// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GPlayerCar.h"
#include "Whos_DrivingGameMode.generated.h"

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class AWhos_DrivingGameMode : public AGameModeBase
{
	GENERATED_BODY()
private:
	TObjectPtr<AGPlayerCar> car;
	void SpawnCar();
	void SpawnPlayer(APlayerController* NewPlayer);
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car")
	TSubclassOf<AGPlayerCar> carClass;
	AWhos_DrivingGameMode();
	void BeginPlay() override;
	void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
};



