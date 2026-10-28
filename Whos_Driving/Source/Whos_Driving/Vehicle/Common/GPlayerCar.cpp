// Fill out your copyright notice in the Description page of Project Settings.


#include "GPlayerCar.h"
#include "GVehicleSeatComponent.h"
#include "GVehicleRadioComponent.h"
#include "GVehicleSeatingComponent.h"
#include "ChaosVehicleWheel.h"
#include "Components/BoxComponent.h"
#include "Engine/DamageEvents.h"
#include "Components/SkeletalMeshComponent.h"
#include "GVehicleMovementComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GWhosDrivingCharacter.h"
#include "GPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "GVehicleModuleComponent.h"
#include "GBoxComponent.h"
#include "GVehicleDoorModule.h"
#include "GVehicleSteeringWheelModule.h"
#include "GVehicleEngineModule.h"
#include "GVehicleElectricianModule.h"
#include "GCarDataAsset.h"
#include "Kismet/GameplayStatics.h"
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
	InitEngine(objectInitializer);
	InitElectrician(objectInitializer);
	RecalculateCarHealth();
}

void AGPlayerCar::InitSeats()
{
	seatingComponent = CreateDefaultSubobject<UGVehicleSeatingComponent>(TEXT("Vehicle Seating"));
	radioComponent = CreateDefaultSubobject<UGVehicleRadioComponent>(TEXT("VehicleRadio"));
	radioComponent->SetupAttachment(GetMesh());
	radioComponent->SetRelativeLocation(FVector(88.0f, 0.0f, 88.0f));
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

		const FName moduleId(*FString::Printf(TEXT("partDoor%d"), i));

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
	const FName moduleId(TEXT("Steering_Wheel"));
	steeringWheel->moduleId = moduleId;
	steeringWheel->maxHP = 50.0f;
	steeringWheel->currentHP = steeringWheel->maxHP;
	moduleComponent->RegisterModule(steeringWheel);

	UGBoxComponent* hitBox = objectInitializer.CreateDefaultSubobject<UGBoxComponent>(this, TEXT("CombatSteeringWheel"));
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

void AGPlayerCar::InitEngine(const FObjectInitializer& objectInitializer)
{
	UGVehicleEngineModule* engineModule = objectInitializer.CreateDefaultSubobject<UGVehicleEngineModule>(this, TEXT("Engine"));
	const FName moduleId(TEXT("Engine"));
	engineModule->moduleId = moduleId;
	engineModule->maxHP = 50.0f;
	engineModule->currentHP = engineModule->maxHP;
	moduleComponent->RegisterModule(engineModule);

	UGBoxComponent* hitBox = objectInitializer.CreateDefaultSubobject<UGBoxComponent>(this, TEXT("CombatEngine"));
	hitBox->moduleId = moduleId;
	hitBox->SetupAttachment(GetMesh());
	hitBox->InitBoxExtent(FVector(80.0f, 80.0f, 40.0f));
	hitBox->SetRelativeLocation(FVector(205.0f, -4.0f, 110.0f));
	hitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	hitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	hitBox->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
	hitBox->SetGenerateOverlapEvents(false);
	moduleHitBoxes.Add(hitBox);
}

void AGPlayerCar::InitElectrician(const FObjectInitializer& objectInitializer)
{
	UGVehicleElectricianModule* electricianModule = objectInitializer.CreateDefaultSubobject<UGVehicleElectricianModule>(this, TEXT("Electrician"));
	const FName moduleId(TEXT("Electrician"));
	electricianModule->moduleId = moduleId;
	electricianModule->maxHP = 50.0f;
	electricianModule->currentHP = electricianModule->maxHP;
	moduleComponent->RegisterModule(electricianModule);

	UGBoxComponent* hitBox = objectInitializer.CreateDefaultSubobject<UGBoxComponent>(this, TEXT("CombatElectrician"));
	hitBox->moduleId = moduleId;
	hitBox->SetupAttachment(GetMesh());
	hitBox->InitBoxExtent(FVector(15.0f, 11.0f, 15.0f));
	hitBox->SetRelativeLocation(FVector(-30.0f, -1.0f, 85.0f));
	hitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	hitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	hitBox->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
	hitBox->SetGenerateOverlapEvents(false);
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

float AGPlayerCar::GetSpeed()
{
	return FVector::DotProduct(GetVelocity(), GetActorForwardVector()) * 0.036f;
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

	LoadCarData();

	if (HasAuthority())
	{
		PublishModuleStates();
	}
	else
	{
		ApplyModuleStates();
	}

	for (auto& module : moduleComponent->GetModules())
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::Blue, module->moduleId.ToString(), true);
	}
	GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Red, FString::FromInt(currentCarHealth), true);
	SubscribeToRepairEvent();
}

void AGPlayerCar::LoadCarData()
{
	baseCarHealth = carData->baseCarHealth;
	TArray<TObjectPtr<UGVehicleWheelModule>> wheels = moduleComponent->GetModulesOfType<UGVehicleWheelModule>();
	for (auto& wheel : wheels)
	{
		wheel->maxHP = carData->wheelMaxHealth;
		wheel->currentHP = wheel->maxHP;
		wheel->repairThreshold = carData->wheelRepairThreshold;
	}

	TArray<TObjectPtr<UGVehicleDoorModule>> doors = moduleComponent->GetModulesOfType<UGVehicleDoorModule>();
	for (auto& door : doors)
	{
		door->maxHP = carData->doorMaxHealth;
		door->currentHP = door->maxHP;
		door->repairThreshold = carData->doorRepairThreshold;
	}

	TArray<TObjectPtr<UGVehicleSteeringWheelModule>> steeringWheels = moduleComponent->GetModulesOfType<UGVehicleSteeringWheelModule>();
	for (auto& steeringWheel : steeringWheels)
	{
		steeringWheel->maxHP = carData->steeringWheelMaxHealth;
		steeringWheel->currentHP = steeringWheel->maxHP;
		steeringWheel->repairThreshold = carData->steeringWheelRepairThreshold;
	}

	TArray<TObjectPtr<UGVehicleEngineModule>> engines = moduleComponent->GetModulesOfType<UGVehicleEngineModule>();
	for (auto& engine : engines)
	{
		engine->maxHP = carData->engineMaxHealth;
		engine->currentHP = engine->maxHP;
		engine->repairThreshold = carData->engineRepairThreshold;
	}

	TArray<TObjectPtr<UGVehicleElectricianModule>> electricians = moduleComponent->GetModulesOfType<UGVehicleElectricianModule>();
	for (auto& electrician : electricians)
	{
		electrician->maxHP = carData->electricianMaxHealth;
		electrician->currentHP = electrician->maxHP;
		electrician->repairThreshold = carData->electricianRepairThreshold;
	}
}

void AGPlayerCar::SubscribeToRepairEvent()
{
	AGPlayerController* controller = Cast<AGPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!controller)
	{
		return;
	}

	controller->tryToRepair.BindDynamic(this, &AGPlayerCar::HandleToRepair);
}

void AGPlayerCar::HandleToRepair(FName moduleId)
{
	UGVehicleModule* module = moduleComponent->FindModule(moduleId);
	module->ApplyRepair();
	PublishModuleStates();
}


TSubclassOf<AGWhosDrivingCharacter> AGPlayerCar::GetSeatCharacterClass() const
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

void AGPlayerCar::SpawnDriver_Implementation(APlayerController* playerController)
{
	if (playerController->GetPawn() != nullptr) 
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, TEXT("player already posseses pawn"));
		return;
	}
	
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, TEXT("spawning Driver"));
	const bool bSeatTaken = IsValid(seatingComponent->GetSeatState(0).occupant);
	AGPlayerController* controller = Cast<AGPlayerController>(playerController);
	controller->bShowMouseCursor = false;
	controller->ClientSetupLobbyInput();
	if (!controller || !driverCharacterClass || !seatingComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("DIDN'T SPAWN DRIVER controller %d char class %d seat component %d "), (!controller), (!driverCharacterClass), (!seatingComponent));
		GEngine->AddOnScreenDebugMessage(-1,15.0f,FColor::Red, FString::Printf(TEXT("DIDN'T SPAWN DRIVER controller %d char class %d seat component %d"),
			!IsValid(controller),!IsValid(driverCharacterClass),!IsValid(seatingComponent)));
		return;
	}
	int32 seatIndex = 0;
	if (bSeatTaken)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("seat taken"));
		UE_LOG(LogTemp, Display, TEXT("seat taken"));
		seatIndex = 1;
	}
	else 
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("seat free"));
		UE_LOG(LogTemp, Display, TEXT("seat free"));
	}
	UE_LOG(LogTemp, Display, TEXT("taking seat number %d"), seatIndex);
	UGVehicleSeatComponent* seat = seatingComponent->GetSeatComponent(seatIndex);
	if (!seat)
	{
		UE_LOG(LogTemp, Warning, TEXT("seat invalid during driver spawn"));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("seat invalid during driver spawn"));
		return;
	}
	FActorSpawnParameters parameters;
	parameters.Owner = controller;
	parameters.Instigator = this;
	parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FTransform transform = seat->characterOffset * seat->GetComponentTransform();
	driverCharacter = GetWorld()->SpawnActor<AGWhosDrivingCharacter>(driverCharacterClass, transform, parameters);
	if (driverCharacter)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("spawned driver, initializing"));
		driverCharacter->SetReplicates(true);
		seatingComponent->InitializeDriver(controller, driverCharacter, seatIndex);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("DIDN'T SPAWN DRIVER"));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Character invalid, didn't spawn"));
	}
}

void AGPlayerCar::PossessedBy(AController* newController)
{
	Super::PossessedBy(newController);
	GetVehicleMovementComponent()->SetBrakeInput(0.0f);
	GetVehicleMovementComponent()->SetHandbrakeInput(false);
}

void AGPlayerCar::UnPossessed()
{
	Super::UnPossessed();
	if (UGVehicleMovementComponent* movement = Cast<UGVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		movement->StopDrivingInput();
	}
}


void AGPlayerCar::SetupPlayerInputComponent(UInputComponent* playerInputComponent)
{
	Super::SetupPlayerInputComponent(playerInputComponent);

	UEnhancedInputComponent* enhancedInputComponent = Cast<UEnhancedInputComponent>(playerInputComponent);
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
	if (CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent())->IsWindowDrivingLocked()) 
	{ 
		return; 
	}
	float throttle = value.Get<float>();
	UGVehicleMovementComponent* movement = Cast<UGVehicleMovementComponent>(GetMovementComponent());
	if (movement->IsEngineBroken())
	{
		float speed = GetSpeed();
		if (speed >= movement->GetBrokenEngineMaxSpeed())
		{
			throttle = 0.0f;
		}
	}
	GetVehicleMovementComponent()->SetThrottleInput(throttle);
}

void AGPlayerCar::InputThrottleReleased(const FInputActionValue& value)
{
	if (CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent())->IsWindowDrivingLocked()) 
	{ 
		return; 
	}
	GetVehicleMovementComponent()->SetThrottleInput(0.0f);
}

void AGPlayerCar::InputSteering(const FInputActionValue& value) 
{
	if (CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent())->IsWindowDrivingLocked()) 
	{ 
		return; 
	}
	const float steering = value.Get<float>();
	GetVehicleMovementComponent()->SetSteeringInput(steering);
}

void AGPlayerCar::InputSteeringReleased(const FInputActionValue& value)
{
	if (CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent())->IsWindowDrivingLocked()) 
	{ 
		return; 
	}
	GetVehicleMovementComponent()->SetSteeringInput(0.0f);
}

void AGPlayerCar::InputBrake(const FInputActionValue& value) 
{
	if (CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent())->IsWindowDrivingLocked()) 
	{ 
		return; 
	}
	const float brake = value.Get<float>();
	GetVehicleMovementComponent()->SetBrakeInput(brake);
}

void AGPlayerCar::InputBrakeReleased(const FInputActionValue& value)
{
	if (CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent())->IsWindowDrivingLocked()) 
	{ 
		return; 
	}
	GetVehicleMovementComponent()->SetBrakeInput(0.0f);
}

float AGPlayerCar::TakeDamage(float damageAmount, const FDamageEvent& damageEvent, AController* eventInstigator, AActor* damageCauser)
{
	const bool bIsDamageValid = FMath::IsFinite(damageAmount) && damageAmount > 0.0f;
	if (!HasAuthority() || !bIsDamageValid)
	{
		return 0.0f;
	}
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
