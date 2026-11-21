// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleHologram.h"
#include "GPlayerCar.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "GVehicleElectricianModule.h"
#include "GVehicleModuleComponent.h"

namespace
{
	// Custom primitive data slots read by M_Hologram.
	const int32 damageDataIndex = 0;
	const int32 brightnessDataIndex = 1;
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

	// Module ids match AGPlayerCar::Init*: wheel 0 is front left, door 0 is on the left side (-Y).
	moduleMeshes.Add(TEXT("Steering_Wheel"), steeringWheelMesh);
	moduleMeshes.Add(TEXT("partDoor0"), doorLeftMesh);
	moduleMeshes.Add(TEXT("partDoor1"), doorRightMesh);
	moduleMeshes.Add(TEXT("Engine"), engineMesh);
	moduleMeshes.Add(TEXT("Electrician"), electronicsMesh);
	moduleMeshes.Add(TEXT("Wheel_0"), wheelFrontLeftMesh);
	moduleMeshes.Add(TEXT("Wheel_1"), wheelFrontRightMesh);
	moduleMeshes.Add(TEXT("Wheel_2"), wheelRearLeftMesh);
	moduleMeshes.Add(TEXT("Wheel_3"), wheelRearRightMesh);
}

void AGVehicleHologram::OnConstruction(const FTransform& transform)
{
	Super::OnConstruction(transform);

	bodyMesh->SetDefaultCustomPrimitiveDataFloat(brightnessDataIndex, bodyBrightness);
}

void AGVehicleHologram::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->OnWorldBeginPlay.AddUObject(this, &AGVehicleHologram::BindToOwnerCar);
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
}

void AGVehicleHologram::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (ownerCar.IsValid())
	{
		ownerCar->onModuleStatesApplied.Remove(moduleStatesAppliedHandle);
	}
	moduleStatesAppliedHandle.Reset();

	Super::EndPlay(endPlayReason);
}

void AGVehicleHologram::HandleElectricianChangeState(EGModuleState newState)
{
	switch (newState)
	{
		case EGModuleState::Destroyed:
			StartGlitch();
			break;
		case EGModuleState::Full:
			StopGlitch();
			break;
		default:
			break;
	}
}

void AGVehicleHologram::StartGlitch()
{
	bGlitchActive = true;
	glitchTimer = 0.0f;
	ApplyRandomModuleColors();
}

void AGVehicleHologram::StopGlitch()
{
	bGlitchActive = false;
	glitchTimer = 0.0f;
	for (const FGVehicleModuleState& state : cachedModuleStates)
	{
		const bool bHasHealth = state.maxHP > 0.0f;
		const float healthFraction = bHasHealth ? FMath::Clamp(state.currentHP / state.maxHP, 0.0f, 1.0f) : 0.0f;
		SetModuleDamage(state.moduleId, 1.0f - healthFraction);
	}
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
	cachedModuleStates = moduleStates;
	if (bGlitchActive)
	{
		return;
	}

	for (const FGVehicleModuleState& state : moduleStates)
	{
		const bool bHasHealth = state.maxHP > 0.0f;
		const float healthFraction = bHasHealth ? FMath::Clamp(state.currentHP / state.maxHP, 0.0f, 1.0f) : 0.0f;
		SetModuleDamage(state.moduleId, 1.0f - healthFraction);
	}
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

void AGVehicleHologram::BindToOwnerCar()
{
	AGPlayerCar* car = Cast<AGPlayerCar>(GetParentActor());
	if (!car)
	{
		car = Cast<AGPlayerCar>(GetAttachParentActor());
	}

	if (!car)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: hologram is not attached to an AGPlayerCar, damage colours will not update"), *GetName());
		return;
	}

	ownerCar = car;
	moduleStatesAppliedHandle = car->onModuleStatesApplied.AddUObject(this, &AGVehicleHologram::ApplyModuleStates);
	ApplyModuleStates(car->moduleStates);

	UGVehicleModuleComponent* moduleComponent = car->GetModuleComponent();
	auto electrician = moduleComponent->GetModulesOfType<UGVehicleElectricianModule>();
	for (auto& elec : electrician)
	{
		elec->OnElectricianChangeState.AddDynamic(this, &AGVehicleHologram::HandleElectricianChangeState);
	}
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
