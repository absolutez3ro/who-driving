// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "AIController.h"
#include "GAIControllerCommonCar.generated.h"

class AGNewCommoncar;
class AGPlayerCar;
class UGCommonEnemyDataAsset;
class USplineComponent;
class AAGSplineActor;
class UChaosVehicleMovementComponent;



UCLASS()
class WHOS_DRIVING_API AGAIControllerCommonCar : public AAIController
{
	GENERATED_BODY()

public:
	void BeginPlay() override;
	void Tick(float deltaSeconds) override;

	UFUNCTION(Server, Reliable)
	void Shoot();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGCommonEnemyDataAsset> commonCarData;

	TObjectPtr<AAGSplineActor> splineOwnerRef;
	TObjectPtr<USplineComponent> splineComponent;

private:
	void DriveAlongSpline(float DeltaSeconds);
	void StartShootDelay();
	void SetPlayerRef();
	bool TraceShot();

	float fireRate = 1;
	float hitChance = 1;
	float minDamage = 1;
	float maxDamage = 1;
	float hitRange=50000.0f;
	TArray<float> weights;
	TArray<FName> moduleNames[6];
	FTimerHandle shootDelayHandle;
	TObjectPtr<USoundBase> hitSound;
	TObjectPtr<USoundBase> missSound;
	AGPlayerCar* playerCarRef;
	// Base distance ahead of the car to aim at (cm)
	float lookAheadDistance = 800.f;
	// Extra look-ahead per unit of speed (seconds). Faster = look further ahead
	float lookAheadTime = 0.4f;
	// Desired cruise speed (cm/s). 1500 = 54 km/h
	float maxSpeed = 1500.f;
	// Angle (deg) that maps to full steering lock
	float maxSteerAngle = 35.f;
	// How quickly steering input reaches its target (higher = snappier)
	float steerInterpSpeed = 15.f;
	// 0..1, how much to slow down at full steering lock
	float cornerSlowdown = 0.5f;
	float throttleGain = 0.002f;
	float brakeGain = 0.002f;
	// Sideways offset from the spline (cm), positive = right
	float laneOffset = 100.f;
	// On an open spline, start stopping this far before the end (cm)
	float stopDistance = 500.f;
	float currentSteer = 0.f;
	float roadWidth = 5.0f;
	float catchUpModificator = 0.1f;
	float playerSpeedThershhold = 10.0f;
	bool followPlayer = false;
};
