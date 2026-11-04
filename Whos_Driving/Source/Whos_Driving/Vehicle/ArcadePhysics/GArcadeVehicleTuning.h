#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GArcadeVehicleSettings.h"
#include "GArcadeVehicleTuning.generated.h"

// Handling of an arcade vehicle, editable without recompiling. Several vehicles can share one asset.
UCLASS(BlueprintType)
class WHOS_DRIVING_API UGArcadeVehicleTuning : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis")
	FGArcadeChassisSettings Chassis;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	FGArcadeSuspensionSettings Suspension;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels")
	FGArcadeWheelSettings Wheels;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires")
	FGArcadeTireSettings Tires;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine")
	FGArcadeEngineSettings Engine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission")
	FGArcadeTransmissionSettings Transmission;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Brakes")
	FGArcadeBrakeSettings Brakes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering")
	FGArcadeSteeringSettings Steering;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aero")
	FGArcadeAeroSettings Aero;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Air")
	FGArcadeAirSettings Air;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists")
	FGArcadeAssistSettings Assists;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift")
	FGArcadeDriftSettings Drift;
};
