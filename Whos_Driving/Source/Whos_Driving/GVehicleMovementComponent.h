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
	void DisableWheel(int32 wheelIndex);
	void StopDrivingInput();
	void SetWindowDrivingLock(bool bLocked);
	UFUNCTION(BlueprintPure, Category = "Vehicle|Combat")
	bool IsWindowDrivingLocked() const { return bWindowDrivingLocked; }
	UFUNCTION(BlueprintPure, Category = "Vehicle|Combat")
	float GetWindowThrottle() const { return windowThrottle; }
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

protected:
	void UpdateState(float deltaTime) override;
	UPROPERTY(ReplicatedUsing = OnRep_WindowDrivingLock)
	bool bWindowDrivingLocked = false;
	UPROPERTY(Replicated)
	float windowThrottle = 0.0f;
	void ApplyWindowDrivingLock();
	UFUNCTION()
	void OnRep_WindowDrivingLock();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float BrokenWheelFrictionMultiplier = 0.1f;
};
