// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "GVehicleWheelModule.h"
#include "GVehicleModule.h"
#include "GPlayerCar.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AWhos_DrivingCharacter;
class UMeshComponent;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UGVehicleSeatComponent;
class UGVehicleSeatingComponent;
class UBoxComponent;
class UGVehicleModuleComponent;
class UGBoxComponent;
class UGCarDataAsset;
class UGVehicleRadioComponent;
class UChildActorComponent;

USTRUCT(BlueprintType)
struct FGVehicleModuleState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName moduleId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Damage")
	float maxHP = 50.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Damage")
	float baseMaxHP = 50.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Damage")
	float HpIncreasePerLevel = 10.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Damage")
	float currentHP = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Damage")
	EGModuleState moduleState = EGModuleState::Full;

	UPROPERTY(BlueprintReadOnly, Category = "VehicleDamage")
	int32 actualRepairCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "VehicleUpgrade")
	int32 moduleHpLevel = 1;
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

// Fired on the server and on every client after the replicated module states were applied.
DECLARE_MULTICAST_DELEGATE_OneParam(FGOnModuleStatesApplied, const TArray<FGVehicleModuleState>&);

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API AGPlayerCar : public AWheeledVehiclePawn
{
	GENERATED_BODY()
	
public:
	AGPlayerCar(const FObjectInitializer& objectInitializer);
	void SubscribeOnDelegates();
	float TakeDamage(float damageAmount, const FDamageEvent& damageEvent, AController* eventInstigator, AActor* damageCauser) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Vehicle|Damage")
	void DamageModule(const FName& moduleId, float damage);
	void DamageModuleInPercent(const FName& moduleId, float damageInPercent);
	void FailureSkillCheck(const FName& moduleId);

	// Server only. Repairs the module if the occupant of seatIndex may reach it (UGVehicleSeatComponent::repairableModuleIds).
	bool RepairModuleFromSeat(int32 seatIndex, FName moduleId);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
	void OnWheelDestroyed(int32 wheelIndex);

	void PublishModuleStates();
	float GetSpeed();

	UFUNCTION(BlueprintPure, Category = "Vehicle|Radio")
	UGVehicleRadioComponent* GetRadioComponent() const
	{
		return radioComponent;
	}

	UStaticMeshComponent* GetAmmoBox() const
	{
		return ammoBox;
	}

	UMeshComponent* GetAmmoResupplyMesh() const;
	FBox GetAmmoResupplyBounds() const;

	void OnConstruction(const FTransform& transform) override;
	void Tick(float deltaSeconds) override;
	TSubclassOf<AWhos_DrivingCharacter> GetSeatCharacterClass() const;
	UFUNCTION(Server, Reliable)
	void SpawnDriver(APlayerController* playerController);

	UFUNCTION(BlueprintPure, Category = "Vehicle|Damage")
	
	FGVehicleModuleState GetModuleDamageState(const FName& moduleId) const;

	UFUNCTION(BlueprintPure, Category = "Vehicle|Seats")
	UGVehicleSeatingComponent* GetSeatingComponent() const;
	
	UGVehicleModuleComponent* GetModuleComponent() const;
public:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Vehicle|Damage")
	float currentCarHealth = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Damage")
	float baseCarHealth;

	int32 baseRepairCostShop = 0;
	UPROPERTY(ReplicatedUsing = OnRep_ModuleStates, BlueprintReadOnly, Category = "Vehicle|Damage")
	TArray<FGVehicleModuleState> moduleStates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGCarDataAsset> carData;

	FGOnModuleStatesApplied onModuleStatesApplied;


	void ApplyModuleStates();
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Radio")
	TObjectPtr<UGVehicleRadioComponent> radioComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Ammo")
	TObjectPtr<UStaticMeshComponent> ammoBox;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> ammoTray;

	// Spawns the dashboard damage hologram (AGVehicleHologram). Class and placement are set in the Blueprint.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Hologram")
	TObjectPtr<UChildActorComponent> hologramComponent;

	void SetupPlayerInputComponent(UInputComponent* playerInputComponent) override;
	void BeginPlay() override;
	void UnPossessed() override;

	void ApplyWheelVisual(int32 wheelIndex, bool bDestroyed);

	UFUNCTION()
	void OnRep_ModuleStates();

	void InputThrottle(const FInputActionValue& value);
	void InputThrottleReleased(const FInputActionValue& value);
	void InputSteering(const FInputActionValue& value);
	void InputSteeringReleased(const FInputActionValue& value);
	void InputBrake(const FInputActionValue& value);
	void InputBrakeReleased(const FInputActionValue& value);
	void InputRepair(const FInputActionValue& value);

	void PossessedBy(AController* newController) override;

	void InitSeats();
	void InitWheels(const FObjectInitializer& objectInitializer);
	void InitDoors(const FObjectInitializer& objectInitializer);
	void InitSteeringWheel(const FObjectInitializer& objectInitializer);
	void InitEngine(const FObjectInitializer& objectInitializer);
	void InitElectrician(const FObjectInitializer& objectInitializer);
	void RecalculateCarHealth();

	void LoadCarData();

	void BreakDoor(FName moduleId);
	void RepairDoor(FName moduleId);

	UFUNCTION()
	void HandleDoorState(FName moduleId, EGModuleState newState);
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Damage")
	float failureDamagePercent = 10.0f;

	TArray<bool> appliedDestroyedWheels;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Seats")
	TObjectPtr<UGVehicleSeatingComponent> seatingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle|Seats")
	TArray<TObjectPtr<UGVehicleSeatComponent>> seatComponents;

private:
	float GetAmmoTrayExtension() const;

	FVector ammoTrayClosedLocation = FVector::ZeroVector;
	float ammoTrayProgress = 0.0f;
};
