#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GVehicleAudioProfile.generated.h"

class USoundAttenuation;
class USoundBase;
class USoundWave;

USTRUCT(BlueprintType)
struct FGVehicleAudioEngineLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Engine")
	TObjectPtr<USoundWave> Loop;

	UPROPERTY(EditAnywhere, Category = "Engine", meta = (ClampMin = "1"))
	float RecordedRpm = 900.0f;

	UPROPERTY(EditAnywhere, Category = "Engine", meta = (ClampMin = "0"))
	float Volume = 1.0f;
};

USTRUCT(BlueprintType)
struct FGVehicleAudioEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Event")
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, Category = "Event")
	FName Socket;

	UPROPERTY(EditAnywhere, Category = "Event")
	FVector Offset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Event", meta = (ClampMin = "0"))
	float Volume = 1.0f;
};

UCLASS(BlueprintType)
class WHOS_DRIVING_API UGVehicleAudioProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Vehicle Audio")
	TObjectPtr<USoundAttenuation> Attenuation;

	UPROPERTY(EditAnywhere, Category = "Engine")
	TArray<FGVehicleAudioEngineLayer> EngineLayers;

	UPROPERTY(EditAnywhere, Category = "Engine")
	FGVehicleAudioEngineLayer LimiterLayer;

	UPROPERTY(EditAnywhere, Category = "Engine", meta = (ClampMin = "0", ClampMax = "1"))
	float MinimumLimiterLoad = 0.65f;

	// Portion of the operating RPM range over which the limiter sound comes in.
	UPROPERTY(EditAnywhere, Category = "Engine", meta = (ClampMin = "0.001", ClampMax = "1"))
	float LimiterBlendRange = 0.035f;

	UPROPERTY(EditAnywhere, Category = "Engine", meta = (ClampMin = "0"))
	float EngineVolume = 0.75f;

	UPROPERTY(EditAnywhere, Category = "Engine", meta = (ClampMin = "0", ClampMax = "1"))
	float CoastVolume = 0.56f;

	UPROPERTY(EditAnywhere, Category = "Engine", meta = (ClampMin = "20", Units = "Hz"))
	float CoastLowPass = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Engine", meta = (ClampMin = "20", Units = "Hz"))
	float LoadLowPass = 16000.0f;

	UPROPERTY(EditAnywhere, Category = "Engine", meta = (ClampMin = "0.01"))
	float EngineResponse = 15.0f;

	UPROPERTY(EditAnywhere, Category = "Engine", meta = (ClampMin = "0.01"))
	float ShiftResponse = 29.0f;

	UPROPERTY(EditAnywhere, Category = "Engine", meta = (ClampMin = "0", ClampMax = "1"))
	float ShiftVolume = 0.75f;

	UPROPERTY(EditAnywhere, Category = "Engine")
	FVector EngineOffset = FVector(105.0, 0.0, 35.0);

	UPROPERTY(EditAnywhere, Category = "Tires")
	TObjectPtr<USoundWave> TireLoop;

	UPROPERTY(EditAnywhere, Category = "Tires")
	FVector TireOffset = FVector(-115.0, 0.0, 10.0);

	UPROPERTY(EditAnywhere, Category = "Tires", meta = (ClampMin = "0"))
	float TireVolume = 0.50f;

	UPROPERTY(EditAnywhere, Category = "Tires", meta = (ClampMin = "0.01"))
	float TireAttack = 17.0f;

	UPROPERTY(EditAnywhere, Category = "Tires", meta = (ClampMin = "0.01"))
	float TireRelease = 9.0f;

	UPROPERTY(EditAnywhere, Category = "Tires", meta = (ClampMin = "0"))
	float TireMinimumSlipKmh = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Tires", meta = (ClampMin = "0"))
	float TireFullSlipKmh = 42.0f;

	// Minimum angle between chassis heading and travel direction for vehicles using Chaos physics.
	UPROPERTY(EditAnywhere, Category = "Tires", meta = (ClampMin = "0", ClampMax = "90", Units = "deg"))
	float ChaosDriftMinimumAngle = 16.0f;

	UPROPERTY(EditAnywhere, Category = "Tires", meta = (ClampMin = "0", ClampMax = "1"))
	float SustainedDriftLevel = 0.24f;

	UPROPERTY(EditAnywhere, Category = "Tires", meta = (ClampMin = "0.01"))
	float TireLowPitch = 0.84f;

	UPROPERTY(EditAnywhere, Category = "Tires", meta = (ClampMin = "0.01"))
	float TireHighPitch = 1.11f;

	UPROPERTY(EditAnywhere, Category = "Events")
	TMap<FName, FGVehicleAudioEvent> Events;
};
