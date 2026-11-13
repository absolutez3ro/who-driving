// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GPlayerCar.h"
#include "Whos_DrivingGameMode.generated.h"

class AGPlayerController;

UENUM(BlueprintType)
enum class EGRaceFailureReason : uint8
{
	None,
	VehicleDestroyed,
	AllPlayersKnockedOut
};

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class AWhos_DrivingGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWhos_DrivingGameMode();
	void BeginPlay() override;
	void HandleStartingNewPlayer_Implementation(APlayerController* newPlayer) override;

	void RegisterParticipant(AGPlayerController* pc);
	void UnregisterParticipant(AGPlayerController* pc);

	UFUNCTION(BlueprintCallable, Category = "Race")
	void ReportFailure(EGRaceFailureReason reason);

	UFUNCTION(BlueprintCallable, Category = "Race")
	void CheckAllPlayersKnockedOut();

	UFUNCTION(BlueprintCallable, Category = "Race")
	void CheckVehicleDestroyed();

	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car")
	TSubclassOf<AGPlayerCar> carClass;

protected:
	void Logout(AController* exitingController) override;
	void RestartCurrentLevel();

private:
	void SpawnCar();
	void SpawnPlayer(APlayerController* newPlayer);

	UPROPERTY()
	TSet<TWeakObjectPtr<AGPlayerController>> participants;

	bool bFailureAccepted = false;

	TObjectPtr<AGPlayerCar> car;
};
