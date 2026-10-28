#pragma once

#include "CoreMinimal.h"
#include "GWhosDrivingGameMode.h"
#include "GVehicleGameMode.generated.h"

class AGPlayerCar;

UCLASS()
class WHOS_DRIVING_API AGVehicleGameMode : public AGWhosDrivingGameMode
{
	GENERATED_BODY()

public:
	void RestartPlayer(AController* newPlayer) override;

protected:
	UPROPERTY(Transient)
	TObjectPtr<AGPlayerCar> sharedVehicle;
};
