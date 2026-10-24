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
class UGVehicleModuleComponent;
class UGBoxComponent;
class UGCarDataAsset;

USTRUCT(BlueprintType)
struct FGVehicleModuleState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName moduleId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Damage")
	float maxHP = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Damage")
	float currentHP = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Damage")
	EGModuleState moduleState = EGModuleState::Full;
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

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Vehicle|Damage")
	void DamageModule(const FName& moduleId, float damage);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void OnWheelDestroyed(int32 wheelIndex);

	void PublishModuleStates();
	float GetSpeed();

	void OnConstruction(const FTransform& transform) override;
	TSubclassOf<AWhos_DrivingCharacter> GetSeatCharacterClass() const;
	UFUNCTION(Server, Reliable)
	void SpawnDriver(APlayerController* PlayerController);

	UFUNCTION(BlueprintPure, Category = "Vehicle|Damage")
	FGVehicleModuleState GetModuleDamageState(const FName& moduleId) const;

	UFUNCTION(BlueprintPure, Category = "Vehicle|Seats")
	UGVehicleSeatingComponent* GetSeatingComponent() const;


public:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle|Damage")
	float currentCarHealth = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Damage")
	float baseCarHealth;

	UPROPERTY(ReplicatedUsing = OnRep_ModuleStates, BlueprintReadOnly, Category = "Vehicle|Damage")
	TArray<FGVehicleModuleState> moduleStates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGCarDataAsset> carData;
protected:

	void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	void BeginPlay() override;
	void UnPossessed() override;

	void ApplyModuleStates();
	void ApplyWheelVisual(int32 wheelIndex, bool bDestroyed);

	UFUNCTION()
	void OnRep_ModuleStates();

	void InputThrottle(const FInputActionValue& value);
	void InputThrottleReleased(const FInputActionValue& value);
	void InputSteering(const FInputActionValue& value);
	void InputSteeringReleased(const FInputActionValue& value);
	void InputBrake(const FInputActionValue& value);
	void InputBrakeReleased(const FInputActionValue& value);

	void PossessedBy(AController* NewController) override;

	void InitSeats();
	void InitWheels(const FObjectInitializer& objectInitializer);
	void InitDoors(const FObjectInitializer& objectInitializer);
	void InitSteeringWheel(const FObjectInitializer& objectInitializer);
	void InitEngine(const FObjectInitializer& objectInitializer);
	void InitElectrician(const FObjectInitializer& objectInitializer);
	void RecalculateCarHealth();

	void LoadCarData();

protected:

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
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	TObjectPtr<UGVehicleModuleComponent> moduleComponent;

	UPROPERTY(EditAnywhere, Category = "Vehicle|Damage")
	TArray<TObjectPtr<UGBoxComponent>> moduleHitBoxes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Damage")
	TArray<FGVehicleWheelVisual> wheelVisuals;

	TArray<bool> appliedDestroyedWheels;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Seats")
	TObjectPtr<UGVehicleSeatingComponent> seatingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Seats")
	TArray<TObjectPtr<UGVehicleSeatComponent>> seatComponents;
};
