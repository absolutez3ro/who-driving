// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleHologram.h"
#include "GPlayerCar.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"

namespace
{
	// Custom primitive data slots read by M_Hologram.
	const int32 damageDataIndex = 0;
	const int32 brightnessDataIndex = 1;
	const FName electricianModuleId(TEXT("Electrician"));
}

AGVehicleHologram::AGVehicleHologram()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	hologramRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HologramRoot"));
	SetRootComponent(hologramRoot);

	bodyMesh = CreatePartMesh(TEXT("BodyMesh"));
	steeringWheelMesh = CreatePartMesh(TEXT("SteeringWheelMesh"));
	doorLeftMesh = CreatePartMesh(TEXT("DoorLeftMesh"));
	doorRightMesh = CreatePartMesh(TEXT("DoorRightMesh"));
	engineMesh = CreatePartMesh(TEXT("EngineMesh"));
	electronicsMesh = CreatePartMesh(TEXT("ElectronicsMesh"));
	wheelFrontLeftMesh = CreatePartMesh(TEXT("WheelFrontLeftMesh"));
	wheelFrontRightMesh = CreatePartMesh(TEXT("WheelFrontRightMesh"));
	wheelRearLeftMesh = CreatePartMesh(TEXT("WheelRearLeftMesh"));
	wheelRearRightMesh = CreatePartMesh(TEXT("WheelRearRightMesh"));

	moduleMeshes.Add(TEXT("Steering_Wheel"), steeringWheelMesh);
	moduleMeshes.Add(TEXT("partDoor0"), doorLeftMesh);
	moduleMeshes.Add(TEXT("partDoor1"), doorRightMesh);
	moduleMeshes.Add(TEXT("Engine"), engineMesh);
	moduleMeshes.Add(electricianModuleId, electronicsMesh);
	moduleMeshes.Add(TEXT("Wheel_0"), wheelFrontLeftMesh);
	moduleMeshes.Add(TEXT("Wheel_1"), wheelFrontRightMesh);
	moduleMeshes.Add(TEXT("Wheel_2"), wheelRearLeftMesh);
	moduleMeshes.Add(TEXT("Wheel_3"), wheelRearRightMesh);

	audioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	audioComponent->SetupAttachment(hologramRoot);
}

void AGVehicleHologram::OnConstruction(const FTransform& transform)
{
	Super::OnConstruction(transform);

	bodyMesh->SetDefaultCustomPrimitiveDataFloat(brightnessDataIndex, bodyBrightness);
}

void AGVehicleHologram::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	FRotator currentRotation = GetActorRotation();
	currentRotation.Add(0.0f, modelRotationSpeedDegrees, 0.0f);
	SetActorRotation(currentRotation);
	if (bGlitchActive)
	{
		ApplyGlitchTick();
	}
	if (bHaveSoundsInQueue)
	{
		TryToPlaySound();
	}
	else
	{
		firstSoundTimer += GetWorld()->GetDeltaSeconds();
	}
}

void AGVehicleHologram::StartGlitch()
{
	bGlitchActive = true;
	glitchTimer = 0.0f;
	ApplyRandomModuleColors();
	StopSound();
	ResetSoundQueue();
	AddSoundInQueue(electricianModuleId, EGModuleState::Destroyed);
	PlaySoundImmediately();
}

void AGVehicleHologram::StopGlitch()
{
	bGlitchActive = false;
	glitchTimer = 0.0f;
}

void AGVehicleHologram::ApplyGlitchTick()
{
	glitchTimer += GetWorld()->GetDeltaSeconds();
	if (glitchTimer >= glitchIntervalSeconds)
	{
		glitchTimer = 0.0f;
		ApplyRandomModuleColors();
	}
}

void AGVehicleHologram::ApplyRandomModuleColors()
{
	static const float glitchPalette[] = {0.0f, 0.5f, 1.0f};
	constexpr int32 paletteSize = UE_ARRAY_COUNT(glitchPalette);
	for (const TPair<FName, UStaticMeshComponent*>& pair : moduleMeshes)
	{
		if (!pair.Value)
		{
			continue;
		}
		const float randomValue = glitchPalette[FMath::RandRange(0, paletteSize - 1)];
		pair.Value->SetCustomPrimitiveDataFloat(damageDataIndex, randomValue);
	}
}

void AGVehicleHologram::ApplyModuleStates(const TArray<FGVehicleModuleState>& moduleStates)
{
	const FGVehicleModuleState* electricianState = moduleStates.FindByPredicate([](const FGVehicleModuleState& state)
	{
		return state.moduleId == electricianModuleId;
	});
	if (electricianState)
	{
		if (!bGlitchActive && electricianState->moduleState == EGModuleState::Destroyed)
		{
			StartGlitch();
		}
		else if (bGlitchActive && electricianState->moduleState == EGModuleState::Full)
		{
			StopGlitch();
		}
	}
	if (bGlitchActive)
	{
		return;
	}

	for (const FGVehicleModuleState& state : moduleStates)
	{
		const bool bHasHealth = state.maxHP > 0.0f;
		const float healthFraction = bHasHealth ? FMath::Clamp(state.currentHP / state.maxHP, 0.0f, 1.0f) : 0.0f;
		SetModuleDamage(state.moduleId, 1.0f - healthFraction);

		const EGModuleState* prev = previousModuleState.Find(state.moduleId);
		const bool bStateChanged = !prev || *prev != state.moduleState;
		if (bStateChanged)
		{
			AddSoundInQueue(state.moduleId, state.moduleState);
			previousModuleState.Add(state.moduleId, state.moduleState);
		}
	}
}

void AGVehicleHologram::AddSoundInQueue(const FName& moduleId, const EGModuleState moduleState)
{
	if (moduleState == EGModuleState::Full)
	{
		return;
	}

	static TMap<FName, FModuleSounds> runtimeSoundMap;
	static bool bInitialized = false;

	if (!bInitialized)
	{
		bInitialized = true;
		USoundCue* soundCue = nullptr;
		FModuleSounds* sounds = nullptr;

		// --- Steering Wheel ---
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_SteeringDamaged"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Steering_Wheel")); sounds->DamageSound = soundCue; }
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_SteeringDestroyed"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Steering_Wheel")); sounds->DestroyedSound = soundCue; }

		// --- Electrician ---
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_ElectricianDamaged"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Electrician")); sounds->DamageSound = soundCue; }
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_ElectricianDestroyed"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Electrician")); sounds->DestroyedSound = soundCue; }

		// --- Engine ---
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_EngineDamaged"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Engine")); sounds->DamageSound = soundCue; }
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_EngineDestroyed"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Engine")); sounds->DestroyedSound = soundCue; }

		// --- Front Left Wheel (Wheel_0) ---
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_FrontLeftWheelDamaged"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Wheel_0")); sounds->DamageSound = soundCue; }
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_FrontLeftWheelDestroyed"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Wheel_0")); sounds->DestroyedSound = soundCue; }

		// --- Front Right Wheel (Wheel_1) ---
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_FrontRightWheelDamaged"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Wheel_1")); sounds->DamageSound = soundCue; }
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_FrontRightWheelDestroyed"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Wheel_1")); sounds->DestroyedSound = soundCue; }

		// --- Rear Left Wheel (Wheel_2) ---
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_RearLeftWheelDamaged"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Wheel_2")); sounds->DamageSound = soundCue; }
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_RearLeftWheelDestroyed"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Wheel_2")); sounds->DestroyedSound = soundCue; }

		// --- Rear Right Wheel (Wheel_3) ---
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_RearRightWheelDamaged"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Wheel_3")); sounds->DamageSound = soundCue; }
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_RearRightWheelDestroyed"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("Wheel_3")); sounds->DestroyedSound = soundCue; }

		// --- Left Door (partDoor0) ---
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_LeftDoorDamaged"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("partDoor0")); sounds->DamageSound = soundCue; }
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_LeftDoorDestroyed"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("partDoor0")); sounds->DestroyedSound = soundCue; }

		// --- Right Door (partDoor1) ---
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_RightDoorDamaged"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("partDoor1")); sounds->DamageSound = soundCue; }
		soundCue = LoadObject<USoundCue>(nullptr, TEXT("/Game/Audio/sounds/Hologram/SC_RightDoorDestroyed"));
		if (soundCue) { sounds = &runtimeSoundMap.FindOrAdd(TEXT("partDoor1")); sounds->DestroyedSound = soundCue; }
	}


	const FModuleSounds* moduleSounds = runtimeSoundMap.Find(moduleId);
	if (!moduleSounds)
	{
		return;
	}

	USoundCue* soundCue = nullptr;
	switch (moduleState)
	{
		case EGModuleState::Repair:
			soundCue = moduleSounds->DamageSound;
			break;
		case EGModuleState::Destroyed:
			soundCue = moduleSounds->DestroyedSound;
			break;
		default:
			break;
	}
	if (soundCue != nullptr)
	{
		FQueuedSound sound;
		sound.sound = soundCue;
		sound.moduleId = moduleId;
		sound.targetState = moduleState;
		soundQueue.Enqueue(sound);
		bHaveSoundsInQueue = true;
	}
}


void AGVehicleHologram::TryToPlaySound()
{
	if (soundQueue.IsEmpty())
	{
		soundsTimer = 0.0f;
		bHaveSoundsInQueue = false;
		return;
	}

	if (firstSoundTimer >= soundsIntervalSeconds)
	{
		PlayFirstSoundImmediately();
		return;
	}

	soundsTimer += GetWorld()->GetDeltaSeconds();
	if (soundsTimer >= soundsIntervalSeconds)
	{
		PlaySoundAfterDelay();
	}
}

void AGVehicleHologram::PlayFirstSoundImmediately()
{
	firstSoundTimer = 0.0f;
	PlaySound();
}

void AGVehicleHologram::PlaySoundAfterDelay()
{
	soundsTimer = 0.0f;
	firstSoundTimer = 0.0f;
	PlaySound();
}

void AGVehicleHologram::PlaySound()
{
	USoundCue* sound = nullptr;
	if (TryToGetValidSound(sound))
	{
		audioComponent->SetSound(sound);
		audioComponent->Play();
	}
}

bool AGVehicleHologram::TryToGetValidSound(USoundCue*& outSound)
{
	FQueuedSound queuedSound;
	while (soundQueue.Dequeue(queuedSound))
	{
		const EGModuleState* currentState = previousModuleState.Find(queuedSound.moduleId);
		if (!currentState || *currentState == queuedSound.targetState)
		{
			outSound = queuedSound.sound;
			return true;
		}
	}
	return false;
}

void AGVehicleHologram::PlaySoundImmediately()
{
	if (soundQueue.IsEmpty())
	{
		return;
	}
	FQueuedSound temp;
	soundQueue.Dequeue(temp);
	audioComponent->SetSound(temp.sound);
	audioComponent->Play();
}

void AGVehicleHologram::StopSound()
{
	audioComponent->Stop();
}

void AGVehicleHologram::ResetSoundQueue()
{
	soundQueue.Empty();
}

UStaticMeshComponent* AGVehicleHologram::CreatePartMesh(const FName& componentName)
{
	UStaticMeshComponent* partMesh = CreateDefaultSubobject<UStaticMeshComponent>(componentName);
	partMesh->SetupAttachment(hologramRoot);
	partMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	partMesh->SetGenerateOverlapEvents(false);
	partMesh->SetCastShadow(false);
	partMesh->CanCharacterStepUpOn = ECB_No;
	// An unset custom primitive data slot reads as 0, which would render the part black.
	partMesh->SetDefaultCustomPrimitiveDataFloat(damageDataIndex, 0.0f);
	partMesh->SetDefaultCustomPrimitiveDataFloat(brightnessDataIndex, 1.0f);
	return partMesh;
}

void AGVehicleHologram::SetModuleDamage(const FName& moduleId, float damage)
{
	UStaticMeshComponent* partMesh = moduleMeshes.FindRef(moduleId);
	if (!partMesh)
	{
		return;
	}

	partMesh->SetCustomPrimitiveDataFloat(damageDataIndex, damage);
}
