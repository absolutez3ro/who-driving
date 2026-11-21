// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GVehicleModule.h"
#include "GVehicleHologram.generated.h"

class AGPlayerCar;
class UStaticMeshComponent;
struct FGVehicleModuleState;

/**
 * Dashboard hologram of the player car.
 * Every breakable module has its own mesh; its colour goes from green (full health) to red (destroyed).
 * The hologram material reads the damage from custom primitive data, so no dynamic material instances are needed.
 * The hologram is purely cosmetic: it is not replicated, every machine follows the replicated module states of its car.
 */
UCLASS()
class WHOS_DRIVING_API AGVehicleHologram : public AActor
{
	GENERATED_BODY()

public:
	AGVehicleHologram();

	void OnConstruction(const FTransform& transform) override;

	// Recolours the module meshes: damage = 1 - currentHP / maxHP.
	void ApplyModuleStates(const TArray<FGVehicleModuleState>& moduleStates);

protected:
	void BeginPlay() override;
	void Tick(float deltaTime) override;
	void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UFUNCTION()
	void HandleElectricianChangeState(EGModuleState newState);

	bool bGlitchActive = false;
	void StartGlitch();
	void StopGlitch();
	void ApplyGlitchTick();
	void ApplyRandomModuleColors();

	TArray<FGVehicleModuleState> cachedModuleStates;
private:
	UStaticMeshComponent* CreatePartMesh(const FName& componentName);
	void BindToOwnerCar();
	void SetModuleDamage(const FName& moduleId, float damage);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hologram")
	TObjectPtr<USceneComponent> hologramRoot;

	// Body silhouette. It is never damaged and glows dimmer than the module meshes.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hologram")
	TObjectPtr<UStaticMeshComponent> bodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hologram")
	TObjectPtr<UStaticMeshComponent> steeringWheelMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hologram")
	TObjectPtr<UStaticMeshComponent> doorLeftMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hologram")
	TObjectPtr<UStaticMeshComponent> doorRightMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hologram")
	TObjectPtr<UStaticMeshComponent> engineMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hologram")
	TObjectPtr<UStaticMeshComponent> electronicsMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hologram")
	TObjectPtr<UStaticMeshComponent> wheelFrontLeftMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hologram")
	TObjectPtr<UStaticMeshComponent> wheelFrontRightMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hologram")
	TObjectPtr<UStaticMeshComponent> wheelRearLeftMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hologram")
	TObjectPtr<UStaticMeshComponent> wheelRearRightMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hologram", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float bodyBrightness = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hologram", meta = (ClampMin = "0.0"))
	float modelRotationSpeed = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hologram", meta = (ClampMin = "0.1"))
	float glitchInterval = 0.5f;
	float glitchTimer = 0.0f;

private:
	// Module id (see AGPlayerCar::Init*) -> mesh that shows this module.
	// Filled by the constructor of every instance; the meshes are kept alive by the UPROPERTY members above.
	TMap<FName, UStaticMeshComponent*> moduleMeshes;

	TWeakObjectPtr<AGPlayerCar> ownerCar;

	FDelegateHandle moduleStatesAppliedHandle;
};
