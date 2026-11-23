// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GVehicleModule.h"
#include "Sound/SoundCue.h"
#include "GVehicleHologram.generated.h"

class UStaticMeshComponent;
struct FGVehicleModuleState;
class UAudioComponent;

USTRUCT()
struct FModuleSounds
{
	GENERATED_BODY();

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundCue> DamageSound = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundCue> DestroyedSound = nullptr;
};

struct FQueuedSound
{
	TObjectPtr<USoundCue> sound = nullptr;
	FName moduleId = NAME_None;
	EGModuleState targetState = EGModuleState::Full;
};


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
	void Tick(float deltaTime) override;

	bool bGlitchActive = false;
	void StartGlitch();
	void StopGlitch();
	void ApplyGlitchTick();
	void ApplyRandomModuleColors();

	void AddSoundInQueue(const FName& moduleId, const EGModuleState moduleState);
	void TryToPlaySound();
	void PlayFirstSoundImmediately();
	void PlaySoundAfterDelay();
	void PlaySoundImmediately();
	void StopSound();
	void ResetSoundQueue();

private:
	UStaticMeshComponent* CreatePartMesh(const FName& componentName);
	void SetModuleDamage(const FName& moduleId, float damage);

	void PlaySound();
	bool TryToGetValidSound(USoundCue*& outSound);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hologram")
	TObjectPtr<USceneComponent> hologramRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hologram")
	TObjectPtr<UAudioComponent> audioComponent;

	TQueue<FQueuedSound> soundQueue;

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
	float modelRotationSpeedDegrees = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hologram", meta = (ClampMin = "0.1"))
	float glitchIntervalSeconds = 0.5f;
	float glitchTimer = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hologram|Sound", meta = (ClampMin = "0.1"))
	float soundsIntervalSeconds = 5.0f;
	float soundsTimer = 0.0f;
	float firstSoundTimer = 5.0f;
	bool bHaveSoundsInQueue = false;
	bool bIsFirstSoundInTime = true;
private:
	// Module id (see AGPlayerCar::Init*) -> mesh that shows this module.
	// Filled by the constructor of every instance; the meshes are kept alive by the UPROPERTY members above.
	TMap<FName, UStaticMeshComponent*> moduleMeshes;
	TMap<FName, EGModuleState> previousModuleState;
};
