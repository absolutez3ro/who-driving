// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "GVehicleWheelModule.h"
#include "GPlayerCar.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AWhos_DrivingCharacter;
class UStaticMesh;
class UGVehicleSeatComponent;
class UGVehicleSeatingComponent;
class UBoxComponent;

USTRUCT(BlueprintType)
struct FGVehicleWheelDamageState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Damage")
	float maxHP = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Damage")
	float currentHP = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Damage")
	bool bDestroyed = false;
};

USTRUCT(BlueprintType)
struct FGVehicleWheelVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Damage", meta = (ClampMin = "0", ClampMax = "3"))
	int32 wheelIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Damage")
	FName componentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Damage")
	TObjectPtr<UStaticMesh> intactMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Damage")
	TObjectPtr<UStaticMesh> destroyedMesh;
};
/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API AGPlayerCar : public AWheeledVehiclePawn
{
	GENERATED_BODY()
	
public:
	AGPlayerCar(const FObjectInitializer& objectInitializer);
	float TakeDamage(float damageAmount, const FDamageEvent& damageEvent, AController* eventInstigator, AActor* damageCauser) override;
	UFUNCTION(BlueprintPure, Category = "Vehicle|Damage")
	float GetHealth() const { return health; }

	// Call from authoritative hit processing. Clients cannot submit arbitrary damage.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Vehicle|Damage")
	void DamageWheel(int32 wheelIndex, float damage);

	UFUNCTION(BlueprintPure, Category = "Vehicle|Damage")
	FGVehicleWheelDamageState GetWheelDamageState(int32 wheelIndex) const;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void OnWheelDestroyed(int32 wheelIndex);

	UFUNCTION(BlueprintPure, Category = "Vehicle|Seats")
	UGVehicleSeatingComponent* GetSeatingComponent() const;

	void OnConstruction(const FTransform& transform) override;
	TSubclassOf<AWhos_DrivingCharacter> GetSeatCharacterClass() const;

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle|Damage")
	float health = 1000.0f;
	UPROPERTY(VisibleAnywhere, Category = "Vehicle|Damage")
	TArray<TObjectPtr<UBoxComponent>> wheelHitBoxes;
	void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	void BeginPlay() override;
	void UnPossessed() override;
	void SpawnDriver();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Seats")
	TObjectPtr<UGVehicleSeatingComponent> seatingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Seats")
	TArray<TObjectPtr<UGVehicleSeatComponent>> seatComponents;

	void PublishWheelDamageStates();
	void ApplyWheelDamageStates();
	void ApplyWheelVisual(int32 wheelIndex, bool bDestroyed);

	UFUNCTION()
	void OnRep_WheelDamageStates();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Damage")
	TArray<FGVehicleWheelVisual> wheelVisuals;

	UPROPERTY(ReplicatedUsing = OnRep_WheelDamageStates, BlueprintReadOnly, Category = "Vehicle|Damage")
	TArray<FGVehicleWheelDamageState> wheelDamageStates;

	// Local idempotence cache; authoritative state is carried by wheelDamageStates.
	TArray<bool> appliedDestroyedWheels;

	void InputThrottle(const FInputActionValue& value);
	void InputThrottleReleased(const FInputActionValue& value);
	void InputSteering(const FInputActionValue& value);
	void InputSteeringReleased(const FInputActionValue& value);
	void InputBrake(const FInputActionValue& value);
	void InputBrakeReleased(const FInputActionValue& value);

	void PossessedBy(AController* NewController) override;
	void UpdatePlayerController();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> throttleAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> steeringAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> brakeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Driver")
	TSubclassOf<AWhos_DrivingCharacter> driverCharacterClass;

	// Optional destruction diagnostic. Normal gameplay starts with four healthy wheels.
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Diagnostics")
	bool bApplyStartupWheelDamage = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Driver")
	TObjectPtr<USceneComponent> driverSeat;

	UPROPERTY()
	TObjectPtr<AWhos_DrivingCharacter> driverCharacter;
	
	UPROPERTY()
	TArray<TObjectPtr<UGVehicleWheelModule>> wheelModules;
};
