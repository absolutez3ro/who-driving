#pragma once

#include "CoreMinimal.h"
#include "Whos_DrivingGameMode.h"
#include "GVehicleGameMode.generated.h"

class AGPlayerCar;

UCLASS()
class WHOS_DRIVING_API AGVehicleGameMode : public AWhos_DrivingGameMode
{
	GENERATED_BODY()

public:
	void RestartPlayer(AController* newPlayer) override;

protected:
	UPROPERTY(Transient)
	TObjectPtr<AGPlayerCar> sharedVehicle;
};
