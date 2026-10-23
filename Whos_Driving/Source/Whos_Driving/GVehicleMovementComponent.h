// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GVehicleMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGVehicleMovementComponent : public UChaosWheeledVehicleMovementComponent
{
	GENERATED_BODY()
	
public:
	UGVehicleMovementComponent();
	void BreakWheel(int32 wheelIndex);
	void BreakSteeringWheel();
	void BreakEngine();
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
		return BrokenEngineMaxSpeed;
	}

protected:
	void UpdateState(float deltaTime) override;
	UPROPERTY(ReplicatedUsing = OnRep_WindowDrivingLock)
	bool bWindowDrivingLocked = false;
	UPROPERTY(Replicated)
	float windowThrottle = 0.0f;
	void ApplyWindowDrivingLock();
	UFUNCTION()
	void OnRep_WindowDrivingLock();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0"))
	float BrokenWheelFrictionMultiplier = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMax = "60", ClampMin = "0.0"))
	float BrokenSteeringWheelAngle = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0"))
	float BrokenEngineTorqueMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMax = "1.0", ClampMin = "0.0"))
	float BrokenEngineRPMMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float BrokenEngineMaxSpeed = 20.0f;

	bool bEngineBroken = false;


};
