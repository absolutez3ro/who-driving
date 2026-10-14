// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "GVehicleWheelModule.h"
#include "Components/TimelineComponent.h"
#include "GPlayerCar.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AWhos_DrivingCharacter;
/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API AGPlayerCar : public AWheeledVehiclePawn
{
	GENERATED_BODY()
	
public:
	AGPlayerCar(const FObjectInitializer& objectInitializer);

	void DamageWheel(int32 wheelIndex, float damage);
	void OnWheelDestroyed(int32 wheelIndex);
	void SwitchBomb(const FInputActionValue& value);

protected:
	void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	void BeginPlay() override;
	void Tick(float dt) override;
	void SpawnDriver();

	void InputThrottle(const FInputActionValue& value);
	void InputThrottleReleased(const FInputActionValue& value);
	void InputSteering(const FInputActionValue& value);
	void InputSteeringReleased(const FInputActionValue& value);
	void InputBrake(const FInputActionValue& value);
	void InputBrakeReleased(const FInputActionValue& value);

	void PossessedBy(AController* NewController) override;
	void UpdatePlayerController();

	void Explode();
	void CheckBomb();
	bool bBombActivate = false;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> throttleAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> steeringAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> brakeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Driver")
	TSubclassOf<AWhos_DrivingCharacter> driverCharacterClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Driver")
	TObjectPtr<USceneComponent> driverSeat;

	UPROPERTY()
	TObjectPtr<AWhos_DrivingCharacter> driverCharacter;
	
	UPROPERTY()
	TArray<TObjectPtr<UGVehicleWheelModule>> wheelModules;

	FTimerHandle bombTimerHandle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bomb")
	float bombActivateSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bomb")
	float bombDelay;

	bool bBombTimerStarted = false;


private:
	void StartBombTimer();
	void StopBombTimer();
};
