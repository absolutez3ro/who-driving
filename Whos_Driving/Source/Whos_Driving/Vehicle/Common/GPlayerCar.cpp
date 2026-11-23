// Fill out your copyright notice in the Description page of Project Settings.


#include "GPlayerCar.h"
#include "GVehicleHologram.h"
#include "GSteeringWheelAnimInstance.h"
#include "Vehicle/ArcadePhysics/GArcadeVehicleComponent.h"
#include "GVehicleAudioComponent.h"
#include "GVehicleSeatComponent.h"
#include "GVehicleRadioComponent.h"
#include "GVehicleSeatingComponent.h"
#include "ChaosVehicleWheel.h"
#include "Components/BoxComponent.h"
#include "GGIWhosDriving.h"
#include "Components/ChildActorComponent.h"
#include "Engine/CollisionProfile.h"
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
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GVehicleModuleComponent.h"
#include "GBoxComponent.h"
#include "GVehicleDoorModule.h"
#include "GVehicleSteeringWheelModule.h"
#include "GVehicleEngineModule.h"
#include "GVehicleElectricianModule.h"
#include "GCarDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Common/Whos_DrivingGameMode.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// The source rig slides Printer_Drawer 53 cm over 24 frames at 30 fps.
	constexpr float ammoTrayTravel = 53.0f;
	constexpr float ammoTrayDuration = 0.8f;
	constexpr float steeringWheelRatio = 6.0f;
}

AGPlayerCar::AGPlayerCar(const FObjectInitializer& objectInitializer) : Super(objectInitializer.SetDefaultSubobjectClass<UGVehicleMovementComponent>
	(AWheeledVehiclePawn::VehicleMovementComponentName))
{
	moduleComponent = CreateDefaultSubobject<UGVehicleModuleComponent>(TEXT("VehicleModules"));
	vehicleAudio = CreateDefaultSubobject<UGVehicleAudioComponent>(TEXT("VehicleAudio"));
	driverSeat = CreateDefaultSubobject<USceneComponent>(TEXT("Driver seat"));
	driverSeat->SetupAttachment(GetRootComponent());
	hologramComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("Hologram"));
	hologramComponent->SetupAttachment(GetMesh());
	bReplicates = true;
	SetReplicateMovement(true);
	InitSeats();
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ammoBoxMesh(TEXT("/Engine/BasicShapes/Cube"));
	ammoBox = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleAmmoBox"));
	ammoBox->SetupAttachment(GetMesh());
	ammoBox->SetStaticMesh(ammoBoxMesh.Object);
	ammoBox->SetRelativeLocation(FVector(-77.0f, 0.0f, 78.0f));
	ammoBox->SetRelativeScale3D(FVector(0.22f));
	ammoBox->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	ammoBox->SetCanEverAffectNavigation(false);
	InitWheels(objectInitializer);
	InitDoors(objectInitializer);
	InitSteeringWheel(objectInitializer);
	InitEngine(objectInitializer);
	InitElectrician(objectInitializer);
	RecalculateCarHealth();
}

void AGPlayerCar::SubscribeOnDelegates()
{
	auto doors = moduleComponent->GetModulesOfType<UGVehicleDoorModule>();
	for (auto& door : doors)
	{
		door->OnDoorChangeState.AddDynamic(this, &AGPlayerCar::HandleDoorState);
	}
}

void AGPlayerCar::HandleDoorState(FName moduleId, EGModuleState newState)
{
	if (newState == EGModuleState::Destroyed)
	{
		BreakDoor(moduleId);
	}
	else if (newState == EGModuleState::Full)
	{
		RepairDoor(moduleId);
	}
}

void AGPlayerCar::BreakDoor(FName moduleId)
{
	USkeletalMeshComponent* doorMesh = nullptr;
	TArray<USceneComponent*> children;
	GetMesh()->GetChildrenComponents(true, children);

	for (USceneComponent* child : children)
	{
		if (child->GetFName() == moduleId)
		{
			doorMesh = Cast<USkeletalMeshComponent>(child);
			break;
		}
	}

	if (doorMesh)
	{
		doorMesh->SetVisibility(false, true);
	}
}

void AGPlayerCar::RepairDoor(FName moduleId)
{	
	USkeletalMeshComponent* doorMesh = nullptr;
	TArray<USceneComponent*> children;
	GetMesh()->GetChildrenComponents(true, children);

	for (USceneComponent* child : children)
	{
		if (child->GetFName() == moduleId)
		{
			doorMesh = Cast<USkeletalMeshComponent>(child);
			break;
		}
	}

	if (doorMesh)
	{
		doorMesh->SetVisibility(true, true);
	}
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
		seat->side = (seatIndex % 2 == 0) ? EGComponentSide::Left : EGComponentSide::Right;
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
		hitBox->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block);
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
		doorModule->moduleSide = (i % 2 == 0) ? EGComponentSide::Left : EGComponentSide::Right;
		moduleComponent->RegisterModule(doorModule);

		UGBoxComponent* hitBox = CreateDefaultSubobject<UGBoxComponent>(FName(*FString::Printf(TEXT("CombatDoor%d"), i)));
		hitBox->moduleId = moduleId;
		hitBox->SetupAttachment(GetMesh());
		hitBox->InitBoxExtent(FVector(40.0f, 20.0f, 40.0f));
		hitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		hitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		hitBox->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block);
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
	hitBox->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block);
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
	hitBox->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block);
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
	hitBox->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block);
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
	if (currentCarHealth <= 0)
	{
		AWhos_DrivingGameMode* gameMode = GetWorld()->GetAuthGameMode<AWhos_DrivingGameMode>();
		gameMode->CheckVehicleDestroyed();
	}
}

void AGPlayerCar::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGPlayerCar, currentCarHealth);
	DOREPLIFETIME(AGPlayerCar, moduleStates);
	DOREPLIFETIME(AGPlayerCar, steeringWheelAngle);
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

void AGPlayerCar::DamageModuleInPercent(const FName& moduleId, const float damageInPercent)
{
	moduleComponent->DamageModuleInPercent(moduleId, damageInPercent);
	PublishModuleStates();
}

void AGPlayerCar::FailureSkillCheck(const FName& moduleId)
{
	DamageModuleInPercent(moduleId, failureDamagePercent);
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
		state.baseMaxHP = module->baseMaxHP;
		state.moduleHpLevel = module->moduleHPLevel;
		state.HpIncreasePerLevel = module->HPIncreasePerLevel;
		state.maxHP = state.baseMaxHP + state.HpIncreasePerLevel * (state.moduleHpLevel - 1);
		state.currentHP = module->currentHP;
		state.moduleState = module->moduleState;
		state.actualRepairCost = module->actualRepairCost;
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
			state.baseMaxHP,
			state.currentHP,
			state.moduleState,
			state.HpIncreasePerLevel,
			state.moduleHpLevel
		);
	}
	UpdateHologram(hologramComponent->GetChildActor());
}

void AGPlayerCar::UpdateHologram(AActor* childActor)
{
	if (AGVehicleHologram* hologram = Cast<AGVehicleHologram>(childActor))
	{
		hologram->ApplyModuleStates(moduleStates);
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

void AGPlayerCar::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// The hologram can be recreated after the module states have already arrived.
	hologramComponent->OnChildActorCreated().AddUObject(this, &AGPlayerCar::UpdateHologram);
	UpdateHologram(hologramComponent->GetChildActor());
}

void AGPlayerCar::BeginPlay()
{
	Super::BeginPlay();
	arcadeSteeringSource = FindComponentByClass<UGArcadeVehicleComponent>();

	TInlineComponentArray<USkeletalMeshComponent*> meshes(this);
	for (USkeletalMeshComponent* mesh : meshes)
	{
		if (mesh->GetFName() == TEXT("partPrinterDrawer") && mesh->GetSkeletalMeshAsset())
		{
			ammoTray = mesh;
			ammoTrayClosedLocation = mesh->GetRelativeLocation();
			ammoBox->SetHiddenInGame(true);
		}
		else if (mesh->GetFName() == TEXT("partSteeringWheel") && mesh->GetBoneIndex(UGSteeringWheelAnimInstance::SteeringBoneName) != INDEX_NONE)
		{
			// The other modular parts still follow the chassis pose. The wheel has its own animated joint.
			mesh->SetLeaderPoseComponent(nullptr, true);
			mesh->SetAnimInstanceClass(UGSteeringWheelAnimInstance::StaticClass());
		}
	}

	moduleComponent->RebuildModuleRegistry(this);
	LoadCarData();
	if (HasAuthority())
	{
		seatingComponent->LogRepairAccessProblems();
	}

	UGVehicleMovementComponent* movementComponent = Cast<UGVehicleMovementComponent>(GetVehicleMovementComponent());
	movementComponent->SubscribeOnDelegates();
	SubscribeOnDelegates();
	if (HasAuthority()) {
		TObjectPtr<UGGIWhosDriving> gi = GetGameInstance<UGGIWhosDriving>();
		if (gi)
		{
			movementComponent->SetUpgradeLevels(gi->carUpgradeData.AccelerationLevel, gi->carUpgradeData.maxSpeedLevel);
			if (gi->GetCarData().moduleStates.Num() != 0) {
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("yes car info in gi, loading")));
				moduleStates = gi->GetCarData().moduleStates;
				for (FGVehicleModuleState state : moduleStates)
				{
					//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Blue, FString::Printf(TEXT("name: module chp: %f mhp: %f"), state.currentHP, state.maxHP));
				}
				currentCarHealth = gi->GetCarData().currentCarHealth;
				baseCarHealth = gi->GetCarData().baseCarHealth;
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan, FString::Printf(TEXT("no car info in gi")));
			}
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("NO GI")));
		}
	}
	ApplyModuleStates();
	PublishModuleStates();
	RecalculateCarHealth();
	for (auto& module : moduleComponent->GetModules())
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::Blue, module->moduleId.ToString(), true);
	}
	GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Red, FString::FromInt(currentCarHealth), true);
}

void AGPlayerCar::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);
	if (HasAuthority())
	{
		UpdateSteeringWheelAngle();
	}
	const double absRoll = FMath::Abs(GetActorRotation().Roll);
	if (absRoll > 60.0f)
	{
		flipTimer += GetWorld()->GetDeltaSeconds();
		if (flipTimer >= flipTimeSeconds)
		{
			flipTimer = 0.0f;
			if (!bIsFlipped)
			{
				bIsFlipped = true;
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Red, TEXT("CAR IS FLIPPED"));
			}
		}
	}
	else
	{
		flipTimer = 0.0f;
	}

	if (!IsValid(ammoTray))
	{
		return;
	}

	// Both the server and clients use the replicated occupants for the shared drawer.
	const auto hasOccupant = [this](EGVehicleSeat seat)
		{
			const FGVehicleSeatState state = seatingComponent->GetSeatState(static_cast<int32>(seat));
			return IsValid(state.occupant) || IsValid(state.playerState);
		};
	const float target = hasOccupant(EGVehicleSeat::RearLeft) || hasOccupant(EGVehicleSeat::RearRight) ? 1.0f : 0.0f;
	if (ammoTrayProgress != target)
	{
		ammoTrayProgress = FMath::FInterpConstantTo(ammoTrayProgress, target, deltaSeconds, 1.0f / ammoTrayDuration);
		ammoTray->SetRelativeLocation(ammoTrayClosedLocation + FVector(GetAmmoTrayExtension(), 0.0, 0.0));
	}
}

void AGPlayerCar::UpdateSteeringWheelAngle()
{
	float roadWheelAngle = 0.0f;
	if (arcadeSteeringSource && arcadeSteeringSource->UsesPlayerCarControls())
	{
		roadWheelAngle = arcadeSteeringSource->GetSteeringAngle();
	}
	else if (const UChaosWheeledVehicleMovementComponent* movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent());
		movement && movement->HasValidPhysicsState() && movement->Wheels.IsValidIndex(0) && movement->Wheels[0])
	{
		roadWheelAngle = movement->Wheels[0]->GetSteerAngle();
	}
	steeringWheelAngle = static_cast<int16>(FMath::RoundToInt(roadWheelAngle * steeringWheelRatio));
}

void AGPlayerCar::LoadCarData()
{
	baseCarHealth = carData->BaseCarHealth;
	failureDamagePercent = carData->FailureDamagePercent;
	TArray<TObjectPtr<UGVehicleWheelModule>> wheels = moduleComponent->GetModulesOfType<UGVehicleWheelModule>();
	for (auto& wheel : wheels)
	{
		wheel->baseMaxHP = carData->WheelBaseMaxHealth;
		wheel->HPIncreasePerLevel = carData->WheelHpIncreasePerLevel;
		wheel->moduleHPLevel = carData->WheelHpLevel;
		wheel->maxHP = wheel->baseMaxHP + wheel->HPIncreasePerLevel * (wheel->moduleHPLevel - 1);
		wheel->currentHP = wheel->maxHP;
		wheel->repairThreshold = carData->WheelRepairThreshold;
		wheel->baseRepairCost = carData->WheelBaseRepairCost;
		wheel->destroyedModuleCostMultiplier = carData->WheelDestroyedModuleCostMultiplier;
	}

	TArray<TObjectPtr<UGVehicleDoorModule>> doors = moduleComponent->GetModulesOfType<UGVehicleDoorModule>();
	for (auto& door : doors)
	{
		door->baseMaxHP = carData->DoorBaseMaxHealth;
		door->HPIncreasePerLevel = carData->DoorHpIncreasePerLevel;
		door->moduleHPLevel = carData->DoorHpLevel;
		door->maxHP = door->baseMaxHP + door->HPIncreasePerLevel * (door->moduleHPLevel - 1);
		door->currentHP = door->maxHP;
		door->repairThreshold = carData->DoorRepairThreshold;
		door->baseRepairCost = carData->DoorBaseRepairCost;
		door->destroyedModuleCostMultiplier = carData->DoorDestroyedModuleCostMultiplier;
	}

	TArray<TObjectPtr<UGVehicleSteeringWheelModule>> steeringWheels = moduleComponent->GetModulesOfType<UGVehicleSteeringWheelModule>();
	for (auto& steeringWheel : steeringWheels)
	{
		steeringWheel->baseMaxHP = carData->SteeringWheelBaseMaxHealth;
		steeringWheel->HPIncreasePerLevel = carData->SteeringWheelHpIncreasePerLevel;
		steeringWheel->moduleHPLevel = carData->SteeringWheelHpLevel;
		steeringWheel->maxHP = steeringWheel->baseMaxHP + steeringWheel->HPIncreasePerLevel * (steeringWheel->moduleHPLevel - 1);
		steeringWheel->currentHP = steeringWheel->maxHP;
		steeringWheel->repairThreshold = carData->SteeringWheelRepairThreshold;
		steeringWheel->baseRepairCost = carData->SteeringWheelBaseRepairCost;
		steeringWheel->destroyedModuleCostMultiplier = carData->SteeringWheelDestroyedCostMultiplier;
	}

	TArray<TObjectPtr<UGVehicleEngineModule>> engines = moduleComponent->GetModulesOfType<UGVehicleEngineModule>();
	for (auto& engine : engines)
	{
		engine->baseMaxHP = carData->EngineBaseMaxHealth;
		engine->HPIncreasePerLevel = carData->EngineHpIncreasePerLevel;
		engine->moduleHPLevel = carData->EngineHpLevel;
		engine->maxHP = engine->baseMaxHP + engine->HPIncreasePerLevel * (engine->moduleHPLevel - 1);
		engine->currentHP = engine->maxHP;
		engine->repairThreshold = carData->EngineRepairThreshold;
		engine->baseRepairCost = carData->EngineBaseRepairCost;
		engine->destroyedModuleCostMultiplier = carData->EngineDestroyedCostMultiplier;
	}

	TArray<TObjectPtr<UGVehicleElectricianModule>> electricians = moduleComponent->GetModulesOfType<UGVehicleElectricianModule>();
	for (auto& electrician : electricians)
	{
		electrician->baseMaxHP = carData->ElectricianBaseMaxHealth;
		electrician->HPIncreasePerLevel = carData->ElectricianHpIncreasePerLevel;
		electrician->moduleHPLevel = carData->ElectricianHpLevel;
		electrician->maxHP = electrician->baseMaxHP + electrician->HPIncreasePerLevel * (electrician->moduleHPLevel - 1);
		electrician->currentHP = electrician->maxHP;
		electrician->repairThreshold = carData->ElectricianRepairThreshold;
		electrician->baseRepairCost = carData->ElectricianBaseRepairCost;
		electrician->destroyedModuleCostMultiplier = carData->ElectricianDestroyedCostMultiplier;
	}
}


bool AGPlayerCar::RepairModuleFromSeat(int32 seatIndex, FName moduleId)
{
	if (!HasAuthority())
	{
		return false;
	}
	if (!seatingComponent->CanRepairModuleFromSeat(seatIndex, moduleId))
	{
		UE_LOG(LogTemp, Display, TEXT("Repair rejected: module %s is not repairable from seat %d"), *moduleId.ToString(), seatIndex);
		return false;
	}
	UGVehicleModule* module = moduleComponent->FindModule(moduleId);
	if (!module || !module->CanRepair())
	{
		return false;
	}
	module->ApplyRepair();
	PublishModuleStates();
	return true;
}


TSubclassOf<AWhos_DrivingCharacter> AGPlayerCar::GetSeatCharacterClass() const
{
	return driverCharacterClass;
}

UGVehicleSeatingComponent* AGPlayerCar::GetSeatingComponent() const
{
	return seatingComponent;
}

UGVehicleModuleComponent* AGPlayerCar::GetModuleComponent() const
{
	return moduleComponent;
}

UMeshComponent* AGPlayerCar::GetAmmoResupplyMesh() const
{
	return IsValid(ammoTray) ? static_cast<UMeshComponent*>(ammoTray.Get()) : ammoBox.Get();
}

FBox AGPlayerCar::GetAmmoResupplyBounds() const
{
	if (IsValid(ammoTray))
	{
		if (const USkeletalMesh* mesh = ammoTray->GetSkeletalMeshAsset())
		{
			FBox bounds = mesh->GetImportedBounds().GetBox();
			// The front of the drawer is exposed at rest. The rest slides out of the housing as it opens.
			bounds.Min.X = FMath::Max(bounds.Min.X, bounds.Max.X - 15.0 - GetAmmoTrayExtension());
			return bounds;
		}
	}
	else if (ammoBox->GetStaticMesh())
	{
		FVector min, max;
		ammoBox->GetLocalBounds(min, max);
		return FBox(min, max);
	}
	return FBox(ForceInit);
}

float AGPlayerCar::GetAmmoTrayExtension() const
{
	return ammoTrayTravel * FMath::SmoothStep(0.0f, 1.0f, ammoTrayProgress);
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
	driverCharacter = GetWorld()->SpawnActor<AWhos_DrivingCharacter>(driverCharacterClass, transform, parameters);
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
		enhancedInputComponent->BindAction(throttleAction.Get(), ETriggerEvent::Canceled, this, &AGPlayerCar::InputThrottleReleased);
	}

	if (steeringAction) 
	{
		enhancedInputComponent->BindAction(steeringAction.Get(), ETriggerEvent::Triggered, this, &AGPlayerCar::InputSteering);
		enhancedInputComponent->BindAction(steeringAction.Get(), ETriggerEvent::Completed, this, &AGPlayerCar::InputSteeringReleased);
		enhancedInputComponent->BindAction(steeringAction.Get(), ETriggerEvent::Canceled, this, &AGPlayerCar::InputSteeringReleased);
	}

	if (brakeAction) 
	{
		enhancedInputComponent->BindAction(brakeAction.Get(), ETriggerEvent::Triggered, this, &AGPlayerCar::InputBrake);
		enhancedInputComponent->BindAction(brakeAction.Get(), ETriggerEvent::Completed, this, &AGPlayerCar::InputBrakeReleased);
		enhancedInputComponent->BindAction(brakeAction.Get(), ETriggerEvent::Canceled, this, &AGPlayerCar::InputBrakeReleased);
	}
	if (handbrakeAction)
	{
		enhancedInputComponent->BindAction(handbrakeAction.Get(), ETriggerEvent::Started, this, &AGPlayerCar::InputHandbrakePressed);
		enhancedInputComponent->BindAction(handbrakeAction.Get(), ETriggerEvent::Completed, this, &AGPlayerCar::InputHandbrakeReleased);
		enhancedInputComponent->BindAction(handbrakeAction.Get(), ETriggerEvent::Canceled, this, &AGPlayerCar::InputHandbrakeReleased);
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
	if (!movement->UsesArcadePhysics() && movement->IsEngineBroken())
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
	UGVehicleMovementComponent* movement = CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent());
	if (movement->IsWindowDrivingLocked())
	{ 
		return; 
	}
	const float steering = value.Get<float>();
	if (!movement->UsesArcadePhysics() && TryApplyFlipInput(steering))
	{
		return;
	}

	movement->SetSteeringInput(steering);
}

bool AGPlayerCar::TryApplyFlipInput(float steering)
{
	if (!bIsFlipped)
	{
		return false;
	}
	if (!FMath::IsNearlyZero(steering))
	{
		ApplyFlipInput(steering);
	}
	return true;
}

void AGPlayerCar::ApplyFlipInput(float inputValue)
{
	if (!HasAuthority())
	{
		return;
	}

	FVector forwardAxis = GetMesh()->GetForwardVector();
	FVector torque = forwardAxis * inputValue * flipTorqueStrength;
	GetMesh()->AddTorqueInRadians(torque, NAME_None, true);

	if (!GetWorldTimerManager().IsTimerActive(flipRecoveryTimer))
	{
		GetWorldTimerManager().SetTimer(flipRecoveryTimer, [this]()
			{
				bIsRecoveringFromFlip = false;
				const float absRoll = FMath::Abs(GetActorRotation().Roll);
				if (absRoll < 30.0f)
				{
					bIsFlipped = false;
					flipTimer = 0.0f;
					GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Green, TEXT("CAR HAS NORMAL ROLL"));
				}
			}, flipRecoveryTime, false);
	}
}

void AGPlayerCar::ExecFlipCar()
{
	SetActorRelativeRotation(FRotator(180.0f, 0.0f, 0.0f));
}

void AGPlayerCar::ExecDamageModule(const FName& moduleId, float damage)
{
	DamageModule(moduleId, damage);
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

void AGPlayerCar::InputHandbrakePressed()
{
	UGVehicleMovementComponent* movement = CastChecked<UGVehicleMovementComponent>(GetVehicleMovementComponent());
	if (!movement->IsWindowDrivingLocked())
	{
		movement->SetHandbrakeInput(true);
	}
}

void AGPlayerCar::InputHandbrakeReleased()
{
	GetVehicleMovementComponent()->SetHandbrakeInput(false);
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
