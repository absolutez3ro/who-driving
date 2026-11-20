// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GVehicleModule.h"
#include "GVehicleMovementComponent.generated.h"

class UGCarDataAsset;
/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGVehicleMovementComponent : public UChaosWheeledVehicleMovementComponent
{
	GENERATED_BODY()
	
public:
	UGVehicleMovementComponent();
	void SubscribeOnDelegates();

	UFUNCTION()
	void BreakWheel(int32 wheelIndex);
	void RepairWheel(int32 wheelIndex);

	UFUNCTION()
	void BreakSteeringWheel();
	void RepairSteeringWheel();

	UFUNCTION()
	void BreakEngine();
	void RepairEngine();

	void StopDrivingInput();
	void SetWindowDrivingLock(bool bLocked);
	UFUNCTION(BlueprintPure, Category = "Vehicle|Combat")
	bool IsWindowDrivingLocked() const 
	{ 
		return bWindowDrivingLocked; 
	}

	UFUNCTION(BlueprintPure, Category = "Vehicle|Combat")
	float GetWindowThrottle() const 
	{ 
		return windowThrottle; 
	}

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
	UFUNCTION(BlueprintPure, Category = "Vehicle|Combat")
	bool IsEngineBroken()
	{
		return bEngineBroken;
	}

	UFUNCTION(BlueprintPure, Category = "Vehicle|Combat")
	float GetBrokenEngineMaxSpeed()
	{
		return brokenEngineMaxSpeed;
	}

	UFUNCTION()
	void OnRep_MaxSpeedLevel();
	UFUNCTION()
	void OnRep_AccelerationLevel();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGCarDataAsset> carData;
	//speed level
	UPROPERTY(ReplicatedUsing = OnRep_MaxSpeedLevel, BlueprintReadOnly, Category = "Vehicle|Upgrades")
	int32 currentMaxSpeedLevel = 1;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle|Upgrades")
	int32 maxMaxSpeedLevel = 0;
	// RPM
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle|Upgrades")
	float maxRPMPerLevel = 0;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle|Upgrades")
	float baseMaxRPM = 0;
	//acceleration level
	UPROPERTY(ReplicatedUsing = OnRep_AccelerationLevel, BlueprintReadOnly, Category = "Vehicle|Upgrades")
	int32 currentAccelerationLevel = 1;
	UPROPERTY(ReplicatedUsing = OnRep_AccelerationLevel, BlueprintReadOnly, Category = "Vehicle|Upgrades")
	int32 maxAccelerationLevel = 1;
	//Torque
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle|Upgrades")
	float maxTorquePerLevel = 30;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle|Upgrades")
	float baseMaxTorque = 300;
	// idle RPM
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle|Upgrades")
	float idleRPMPerLevel = 220;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle|Upgrades")
	float baseIdleRPM = 800;
	// RevUp
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle|Upgrades")
	float revUpPerLevel = 4;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle|Upgrades")
	float baseRevUp = 2;
protected:
	void BeginPlay() override;
	void UpdateState(float deltaTime) override;
	UPROPERTY(ReplicatedUsing = OnRep_WindowDrivingLock)
	bool bWindowDrivingLocked = false;
	UPROPERTY(Replicated)
	float windowThrottle = 0.0f;
	void ApplyWindowDrivingLock();
	UFUNCTION()
	void OnRep_WindowDrivingLock();

	UFUNCTION()
	void HandleEngineState(EGModuleState newState);

	UFUNCTION()
	void HandleSteeringWheelState(EGModuleState newState);

	UFUNCTION()
	void HandleWheelState(int32 wheelIndex, EGModuleState newState);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0"))
	float brokenWheelFrictionMultiplier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMax = "90", ClampMin = "0.0"))
	float brokenSteeringWheelAngle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0"))
	float brokenEngineTorqueMultiplier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float brokenEngineMaxSpeed;

	bool bEngineBroken = false;


};
