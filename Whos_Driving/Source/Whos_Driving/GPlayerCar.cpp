// Fill out your copyright notice in the Description page of Project Settings.


#include "GPlayerCar.h"
#include "G_Vehicle_Seat_Component.h"
#include "G_Vehicle_Seating_Component.h"
#include "ChaosVehicleWheel.h"
#include "Components/BoxComponent.h"
#include "Engine/DamageEvents.h"
#include "Components/SkeletalMeshComponent.h"
#include "GVehicleMovementComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Whos_DrivingCharacter.h"
#include "GPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "GVehicleModuleComponent.h"
#include "GBoxComponent.h"
#include "GVehicleDoorModule.h"
#include "GVehicleSteeringWheelModule.h"
#include "GameFramework/CharacterMovementComponent.h"

AGPlayerCar::AGPlayerCar(const FObjectInitializer& objectInitializer) : Super(objectInitializer.SetDefaultSubobjectClass<UGVehicleMovementComponent>
	(AWheeledVehiclePawn::VehicleMovementComponentName))
{
	moduleComponent = CreateDefaultSubobject<UGVehicleModuleComponent>(TEXT("VehicleModules"));
	driverSeat = CreateDefaultSubobject<USceneComponent>(TEXT("Driver seat"));
	driverSeat->SetupAttachment(GetRootComponent());

	bReplicates = true;
	SetReplicateMovement(true);
	InitSeats();
	InitWheels(objectInitializer);
	InitDoors(objectInitializer);
	InitSteeringWheel(objectInitializer);
	RecalculateCarHealth();
}

void AGPlayerCar::InitSeats()
{
	seatingComponent = CreateDefaultSubobject<UGVehicleSeatingComponent>(TEXT("Vehicle Seating"));
	const FName seatNames[] = { TEXT("SeatDriver"), TEXT("SeatFrontPassenger"), TEXT("SeatRearLeft"), TEXT("SeatRearRight") };
	for (int32 seatIndex = 0; seatIndex < 4; ++seatIndex)
	{
		UGVehicleSeatComponent* seat = CreateDefaultSubobject<UGVehicleSeatComponent>(seatNames[seatIndex]);
		seat->SetupAttachment(GetMesh());
		seat->seatId = static_cast<EGVehicleSeat>(seatIndex);
		const float seatX = seatIndex < 2 ? 22.0f : -110.0f;
		const float seatY = seatIndex % 2 == 0 ? -59.0f : 59.0f;
		seat->SetRelativeLocation(FVector(seatX, seatY, 95.0f));
		seat->exitLocation = FVector(seatX, seatY < 0.0f ? -230.0f : 230.0f, 110.0f);
		seatComponents.Add(seat);
	}
}

void AGPlayerCar::InitWheels(const FObjectInitializer& objectInitializer)
{
	wheelModules.SetNum(4);
	appliedDestroyedWheels.Init(false, 4);

	for (int32 i = 0; i < 4; ++i)
	{
		const FName subobjectName(*FString::Printf(TEXT("WheelModule_%d"), i));

		UGVehicleWheelModule* wheelModule = objectInitializer.CreateDefaultSubobject<UGVehicleWheelModule>(this, subobjectName);

		const FName moduleId = FName(*FString::Printf(TEXT("Wheel_%d"), i));

		wheelModule->wheelIndex = i;
		wheelModule->moduleId = moduleId;
		wheelModule->maxHP = 50.0f;
		wheelModule->currentHP = wheelModule->maxHP;

		wheelModules[i] = wheelModule;

		if (moduleComponent)
		{
			moduleComponent->RegisterModule(wheelModule);
		}

		UGBoxComponent* hitBox = CreateDefaultSubobject<UGBoxComponent>(FName(*FString::Printf(TEXT("CombatWheel%d"), i)));
		hitBox->moduleId = moduleId;
		hitBox->SetupAttachment(GetMesh());
		hitBox->InitBoxExtent(FVector(40.0f, 20.0f, 40.0f));
		hitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		hitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		hitBox->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
		hitBox->SetGenerateOverlapEvents(false);

		moduleHitBoxes.Add(hitBox);
	}
}

void AGPlayerCar::InitDoors(const FObjectInitializer& objectInitializer)
{
	for (int32 i = 0; i < 2; ++i)
	{
		const FName subobjectName(*FString::Printf(TEXT("DoorModule_%d"), i));
		UGVehicleDoorModule* doorModule = objectInitializer.CreateDefaultSubobject<UGVehicleDoorModule>(this, subobjectName);

		const FName moduleId(*FString::Printf(TEXT("part_door_%d"), i));

		doorModule->doorId = i;
		doorModule->moduleId = moduleId;
		doorModule->maxHP = 50.0f;
		doorModule->currentHP = doorModule->maxHP;
		moduleComponent->RegisterModule(doorModule);

		UGBoxComponent* hitBox = CreateDefaultSubobject<UGBoxComponent>(FName(*FString::Printf(TEXT("CombatDoor%d"), i)));
		hitBox->moduleId = moduleId;
		hitBox->SetupAttachment(GetMesh());
		hitBox->InitBoxExtent(FVector(40.0f, 20.0f, 40.0f));
		hitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		hitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		hitBox->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
		hitBox->SetGenerateOverlapEvents(false);

		moduleHitBoxes.Add(hitBox);
	}
}

void AGPlayerCar::InitSteeringWheel(const FObjectInitializer& objectInitializer)
{
	UGVehicleSteeringWheelModule* steeringWheel = objectInitializer.CreateDefaultSubobject<UGVehicleSteeringWheelModule>(this, TEXT("SteeringWheel"));
	const FName moduleId("Steering_Wheel");
	steeringWheel->moduleId = moduleId;
	steeringWheel->maxHP = 50.0f;
	steeringWheel->currentHP = steeringWheel->maxHP;
	moduleComponent->RegisterModule(steeringWheel);

	TObjectPtr<UGBoxComponent> hitBox = objectInitializer.CreateDefaultSubobject<UGBoxComponent>(this, TEXT("CombatSteeringWheel"));
	hitBox->moduleId = moduleId;
	hitBox->SetupAttachment(GetMesh());
	hitBox->InitBoxExtent(FVector(20.0f, 20.0f, 25.0f));
	hitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	hitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	hitBox->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
	hitBox->SetGenerateOverlapEvents(false);
	hitBox->SetRelativeLocation(FVector(70.0f, -59.0f, 105.0f));

	moduleHitBoxes.Add(hitBox);
}

void AGPlayerCar::RecalculateCarHealth()
{
	currentCarHealth = baseCarHealth;
	for (auto& module : moduleComponent->GetModules())
	{
		currentCarHealth += module->currentHP;
	}
}

void AGPlayerCar::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGPlayerCar, currentCarHealth);
	DOREPLIFETIME(AGPlayerCar, moduleStates);
}

FGVehicleModuleState AGPlayerCar::GetModuleDamageState(const FName& moduleId) const
{
	for (const FGVehicleModuleState& state : moduleStates)
	{
		if (state.moduleId == moduleId)
		{
			return state;
		}
	}

	return FGVehicleModuleState();
}

void AGPlayerCar::DamageModule(const FName& moduleId, float damage)
{
	moduleComponent->DamageModule(moduleId, damage);
	PublishModuleStates();
}

void AGPlayerCar::PublishModuleStates()
{
	if (!HasAuthority())
	{
		return;
	}

	moduleStates.Reset();
	for (const auto& module : moduleComponent->GetModules())
	{
		if (!module)
		{
			continue;
		}

		FGVehicleModuleState state;
		state.moduleId = module->moduleId;
		state.maxHP = module->maxHP;
		state.currentHP = module->currentHP;
		state.moduleState = module->moduleState;

		moduleStates.Add(state);
	}

	ApplyModuleStates();
	ForceNetUpdate();
}

void AGPlayerCar::OnRep_ModuleStates()
{
	ApplyModuleStates();
}

void AGPlayerCar::ApplyModuleStates()
{
	for (const auto& state : moduleStates)
	{
		moduleComponent->ApplyReplicatedModuleState(
			state.moduleId,
			state.maxHP,
			state.currentHP,
			state.moduleState
		);
	}
}

void AGPlayerCar::ApplyWheelVisual(int32 wheelIndex, bool bDestroyed)
{
	TInlineComponentArray<UStaticMeshComponent*> visualComponents;
	GetComponents(visualComponents);

	for (const FGVehicleWheelVisual& visual : wheelVisuals)
	{
		if (visual.wheelIndex != wheelIndex || visual.componentName.IsNone())
		{
			continue;
		}

		UStaticMesh* desiredMesh = bDestroyed ? visual.destroyedMesh.Get() : visual.intactMesh.Get();
		if (!desiredMesh)
		{
			continue;
		}

		for (UStaticMeshComponent* component : visualComponents)
		{
			if (component && component->GetFName() == visual.componentName && component->GetStaticMesh() != desiredMesh)
			{
				component->SetStaticMesh(desiredMesh);
			}
		}
	}
}

void AGPlayerCar::BeginPlay()
{
	Super::BeginPlay();

	moduleComponent->RebuildModuleRegistry(this);

	if (HasAuthority())
	{
		PublishModuleStates();
	}
	else
	{
		ApplyModuleStates();
	}

	SpawnDriver();
	UpdatePlayerController();

	for (auto& module : moduleComponent->GetModules())
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::Blue, module->moduleId.ToString(), true);
	}
	GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Red, FString::FromInt(currentCarHealth), true);
}


TSubclassOf<AWhos_DrivingCharacter> AGPlayerCar::GetSeatCharacterClass() const
{
	return driverCharacterClass;
}

UGVehicleSeatingComponent* AGPlayerCar::GetSeatingComponent() const
{
	return seatingComponent;
}

void AGPlayerCar::OnConstruction(const FTransform& transform)
{
	Super::OnConstruction(transform);
	//UGVehicleMovementComponent* movement = CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent());
	//for (int32 index = 0; index < wheelHitBoxes.Num(); ++index)
	//{
	//	UBoxComponent* hitBox = wheelHitBoxes[index];
	//	const bool bValidWheel = movement->WheelSetups.IsValidIndex(index) && GetMesh()->GetBoneIndex(movement->WheelSetups[index].BoneName) != INDEX_NONE;
	//	hitBox->SetCollisionEnabled(bValidWheel ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	//	if (bValidWheel)
	//	{
	//		const FChaosWheelSetup& setup = movement->WheelSetups[index];
	//		hitBox->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, setup.BoneName);
	//		if (const UChaosVehicleWheel* wheel = setup.WheelClass ? setup.WheelClass->GetDefaultObject<UChaosVehicleWheel>() : nullptr)
	//		{
	//			hitBox->SetBoxExtent(FVector(wheel->WheelRadius, wheel->WheelWidth * 0.5f, wheel->WheelRadius));
	//		}
	//	}
	//}
	GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	TInlineComponentArray<USkeletalMeshComponent*> parts;
	GetComponents(parts);
	for (USkeletalMeshComponent* part : parts)
	{
		if (part != GetMesh() && part->ComponentHasTag(TEXT("VehiclePart")))
		{
			part->SetLeaderPoseComponent(GetMesh(), true, false);
			part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void AGPlayerCar::SpawnDriver()
{
	AGPlayerController* controller = Cast<AGPlayerController>(GetController());
	if (!HasAuthority() || !controller || !driverCharacterClass || !seatingComponent
		|| seatingComponent->GetSeatState(0).occupant)
	{
		return;
	}
	UGVehicleSeatComponent* seat = seatingComponent->GetSeatComponent(0);
	if (!seat)
	{
		return;
	}
	FActorSpawnParameters parameters;
	parameters.Owner = controller;
	parameters.Instigator = this;
	parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FTransform transform = seat->characterOffset * seat->GetComponentTransform();
	driverCharacter = GetWorld()->SpawnActor<AWhos_DrivingCharacter>(driverCharacterClass, transform, parameters);
	if (driverCharacter)
	{
		driverCharacter->SetReplicates(true);
		seatingComponent->InitializeDriver(controller, driverCharacter);
	}
}

void AGPlayerCar::PossessedBy(AController* newController)
{
	Super::PossessedBy(newController);
	GetVehicleMovementComponent()->SetBrakeInput(0.0f);
	GetVehicleMovementComponent()->SetHandbrakeInput(false);
	if (HasActorBegunPlay())
	{
		SpawnDriver();
	}
}

void AGPlayerCar::UnPossessed()
{
	Super::UnPossessed();
	if (UGVehicleMovementComponent* movement = Cast<UGVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		movement->StopDrivingInput();
	}
}

void AGPlayerCar::UpdatePlayerController()
{
	// Seating owns controller and view transitions; keep the existing entry point for serialized Blueprints.
}

void AGPlayerCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* enhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!enhancedInputComponent) 
	{
		return;
	}

	if (throttleAction) 
	{
		enhancedInputComponent->BindAction(throttleAction.Get(), ETriggerEvent::Triggered, this, &AGPlayerCar::InputThrottle);
		enhancedInputComponent->BindAction(throttleAction.Get(), ETriggerEvent::Completed, this, &AGPlayerCar::InputThrottleReleased);
	}

	if (steeringAction) 
	{
		enhancedInputComponent->BindAction(steeringAction.Get(), ETriggerEvent::Triggered, this, &AGPlayerCar::InputSteering);
		enhancedInputComponent->BindAction(steeringAction.Get(), ETriggerEvent::Completed, this, &AGPlayerCar::InputSteeringReleased);
	}

	if (brakeAction) 
	{
		enhancedInputComponent->BindAction(brakeAction.Get(), ETriggerEvent::Triggered, this, &AGPlayerCar::InputBrake);
		enhancedInputComponent->BindAction(brakeAction.Get(), ETriggerEvent::Completed, this, &AGPlayerCar::InputBrakeReleased);
	}
}

void AGPlayerCar::InputThrottle(const FInputActionValue& value) 
{
	if (CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent())->IsWindowDrivingLocked()) { return; }
	const float throttle = value.Get<float>();
	GetVehicleMovementComponent()->SetThrottleInput(throttle);
}

void AGPlayerCar::InputThrottleReleased(const FInputActionValue& value) {
	if (CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent())->IsWindowDrivingLocked()) { return; }
	GetVehicleMovementComponent()->SetThrottleInput(0.0f);
}

void AGPlayerCar::InputSteering(const FInputActionValue& value) 
{
	if (CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent())->IsWindowDrivingLocked()) { return; }
	const float steering = value.Get<float>();
	GetVehicleMovementComponent()->SetSteeringInput(steering);
}

void AGPlayerCar::InputSteeringReleased(const FInputActionValue& value) {
	if (CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent())->IsWindowDrivingLocked()) { return; }
	GetVehicleMovementComponent()->SetSteeringInput(0.0f);
}

void AGPlayerCar::InputBrake(const FInputActionValue& value) 
{
	if (CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent())->IsWindowDrivingLocked()) { return; }
	const float brake = value.Get<float>();
	GetVehicleMovementComponent()->SetBrakeInput(brake);
}

void AGPlayerCar::InputBrakeReleased(const FInputActionValue& value)
{
	if (CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent())->IsWindowDrivingLocked()) { return; }
	GetVehicleMovementComponent()->SetBrakeInput(0.0f);
}

float AGPlayerCar::TakeDamage(float damageAmount, const FDamageEvent& damageEvent, AController* eventInstigator, AActor* damageCauser)
{
	if (!HasAuthority() || !FMath::IsFinite(damageAmount) || damageAmount <= 0.0f) { return 0.0f; }
	const float applied = FMath::Min(currentCarHealth, damageAmount);
	if (damageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& pointDamage = static_cast<const FPointDamageEvent&>(damageEvent);
		const UGBoxComponent* damageHitbox = Cast<UGBoxComponent>(pointDamage.HitInfo.GetComponent());
		if (damageHitbox && !damageHitbox->moduleId.IsNone())
		{
			DamageModule(damageHitbox->moduleId, applied);
		}
	}
	RecalculateCarHealth();
	GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Red, FString::FromInt(currentCarHealth), false);
	ForceNetUpdate();
	return applied;
}
