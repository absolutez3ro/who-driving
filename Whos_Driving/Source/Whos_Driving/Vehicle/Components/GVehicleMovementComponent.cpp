// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleMovementComponent.h"
#include "GCarDataAsset.h"
#include "Vehicle/ArcadePhysics/GArcadeVehicleComponent.h"
#include "GPlayerCar.h"
#include "GVehicleEngineModule.h"
#include "GVehicleSteeringWheelModule.h"
#include "GVehicleWheelModule.h"
#include "GVehicleModuleComponent.h"
#include "Net/UnrealNetwork.h"

UGVehicleMovementComponent::UGVehicleMovementComponent() 
{
	EngineSetup.MaxRPM = 6000.0f;
	EngineSetup.EngineIdleRPM = 800.0f;
	FRichCurve* torqueCurve = EngineSetup.TorqueCurve.GetRichCurve();
	if (torqueCurve) 
	{
		torqueCurve->AddKey(0.0f, 0.2f);
		torqueCurve->AddKey(1000.0f, 0.6f);
		torqueCurve->AddKey(3000.0f, 0.9f);
		torqueCurve->AddKey(5000.0f, 1.0f);
		torqueCurve->AddKey(6000.0f, 0.7f);
	}

	TransmissionSetup.bUseAutomaticGears = true;
	TransmissionSetup.GearChangeTime = 0.2f;
	TransmissionSetup.FinalRatio = 3.5f;
	TransmissionSetup.ForwardGearRatios = { 3.2f, 2.1f, 1.4f, 1.0f, 0.8f };
	TransmissionSetup.ReverseGearRatios = { 3.0f };
}

void UGVehicleMovementComponent::OnRep_MaxSpeedLevel()
{
	EngineSetup.MaxRPM = GetUpgradedMaxRPM();
	if (!UsesArcadePhysics())
	{
		RecreatePhysicsState();
	}
}

void UGVehicleMovementComponent::OnRep_AccelerationLevel()
{
	const int32 extraLevels = FMath::Clamp(currentAccelerationLevel, 1, FMath::Max(1, maxAccelerationLevel)) - 1;
	EngineSetup.EngineIdleRPM = baseIdleRPM + idleRPMPerLevel * extraLevels;
	EngineSetup.EngineRevUpMOI = baseRevUp + revUpPerLevel * extraLevels;
	if (!UsesArcadePhysics())
	{
		SetMaxEngineTorque(GetUpgradedTorque() * (bEngineBroken ? brokenEngineTorqueMultiplier : 1.0f));
		RecreatePhysicsState();
	}
}

void UGVehicleMovementComponent::SetUpgradeLevels(int32 accelerationLevel, int32 maxSpeedLevel)
{
	if (accelerationLevel > 0 && currentAccelerationLevel != accelerationLevel)
	{
		currentAccelerationLevel = FMath::Min(accelerationLevel, FMath::Max(1, maxAccelerationLevel));
		OnRep_AccelerationLevel();
	}
	if (maxSpeedLevel > 0 && currentMaxSpeedLevel != maxSpeedLevel)
	{
		currentMaxSpeedLevel = FMath::Min(maxSpeedLevel, FMath::Max(1, maxMaxSpeedLevel));
		OnRep_MaxSpeedLevel();
	}
}

float UGVehicleMovementComponent::GetUpgradedTorque() const
{
	return baseMaxTorque + maxTorquePerLevel * (FMath::Clamp(currentAccelerationLevel, 1, FMath::Max(1, maxAccelerationLevel)) - 1);
}

float UGVehicleMovementComponent::GetUpgradedMaxRPM() const
{
	return baseMaxRPM + maxRPMPerLevel * (FMath::Clamp(currentMaxSpeedLevel, 1, FMath::Max(1, maxMaxSpeedLevel)) - 1);
}

bool UGVehicleMovementComponent::UsesArcadePhysics() const
{
	const UGArcadeVehicleComponent* arcade = GetOwner() ? GetOwner()->FindComponentByClass<UGArcadeVehicleComponent>() : nullptr;
	return arcade && arcade->UsesPlayerCarControls();
}

bool UGVehicleMovementComponent::CanCreateVehicle() const
{
	return !UsesArcadePhysics() && Super::CanCreateVehicle();
}

float UGVehicleMovementComponent::GetArcadeTorqueMultiplier() const
{
	const float upgradeMultiplier = baseMaxTorque > 0.0f ? GetUpgradedTorque() / baseMaxTorque : 1.0f;
	return upgradeMultiplier * (bEngineBroken ? brokenEngineTorqueMultiplier : 1.0f);
}

float UGVehicleMovementComponent::GetArcadeRedlineMultiplier() const
{
	return baseMaxRPM > 0.0f ? GetUpgradedMaxRPM() / baseMaxRPM : 1.0f;
}

float UGVehicleMovementComponent::GetArcadeSteeringLimit() const
{
	return bSteeringBroken ? brokenSteeringWheelAngle : 90.0f;
}

float UGVehicleMovementComponent::GetArcadeWheelGripMultiplier(int32 wheelIndex) const
{
	return IsArcadeWheelDisabled(wheelIndex) ? brokenWheelFrictionMultiplier : 1.0f;
}

bool UGVehicleMovementComponent::IsArcadeWheelDisabled(int32 wheelIndex) const
{
	return brokenWheels.Contains(wheelIndex);
}

bool UGVehicleMovementComponent::CanArcadeEngineDrive(float speedKmh) const
{
	return !bEngineBroken || speedKmh < brokenEngineMaxSpeed;
}

FGArcadeVehicleInput UGVehicleMovementComponent::ReadArcadeInput(float deltaTime)
{
	VehicleState.CaptureState(GetBodyInstance(), GetGravityZ(), deltaTime);
	FGArcadeVehicleInput input;
	const AController* controller = GetController();
	if (!controller)
	{
		input.bHandbrake = true;
		return input;
	}
	if (bWindowDrivingLocked)
	{
		input.throttle = windowThrottle;
		return input;
	}
	if (controller->IsLocalController())
	{
		input.throttle = FMath::Clamp(RawThrottleInput, 0.0f, 1.0f);
		input.brake = FMath::Clamp(RawBrakeInput, 0.0f, 1.0f);
		input.steering = RawSteeringInput;
		input.bHandbrake = bRawHandbrakeInput;
		if (!GetOwner()->HasAuthority() && (input.throttle != lastArcadeInputSent.throttle
			|| input.brake != lastArcadeInputSent.brake || input.steering != lastArcadeInputSent.steering
			|| input.bHandbrake != lastArcadeInputSent.bHandbrake))
		{
			ServerSetArcadeInput(input);
			lastArcadeInputSent = input;
		}
		return input;
	}
	return remoteArcadeInput;
}

void UGVehicleMovementComponent::ServerSetArcadeInput_Implementation(const FGArcadeVehicleInput& input)
{
	if (GetController() && !bWindowDrivingLocked && UsesArcadePhysics())
	{
		remoteArcadeInput.throttle = FMath::IsFinite(input.throttle) ? FMath::Clamp(input.throttle, 0.0f, 1.0f) : 0.0f;
		remoteArcadeInput.brake = FMath::IsFinite(input.brake) ? FMath::Clamp(input.brake, 0.0f, 1.0f) : 0.0f;
		remoteArcadeInput.steering = FMath::IsFinite(input.steering) ? FMath::Clamp(input.steering, -1.0f, 1.0f) : 0.0f;
		remoteArcadeInput.bHandbrake = input.bHandbrake;
	}
}

void UGVehicleMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UsesArcadePhysics() && IsPhysicsStateCreated())
	{
		RecreatePhysicsState();
	}

	if (carData)
	{
		//maxRPM
		baseMaxRPM = carData->baseMaxRPM;
		maxRPMPerLevel = carData->maxRPMPerLevel;
		// Torque
		baseMaxTorque = carData->baseTorque;
		maxTorquePerLevel = carData->torquePerLevel;
		//Idle RPM
		idleRPMPerLevel = carData->IdleRPMPerLevel;
		baseIdleRPM = carData->baseIdleRPM;
		//RevUp
		baseRevUp = carData->baseRevUp;
		revUpPerLevel = carData->revUpPerLevel;
		// max speed level
		maxMaxSpeedLevel = FMath::Max(1, carData->maxMaxSpeedLevel);
		currentMaxSpeedLevel = FMath::Clamp(carData->startingMaxSpeedLevel, 1, maxMaxSpeedLevel);
		// acceleration level
		maxAccelerationLevel = FMath::Max(1, carData->maxAccelerationLevel);
		currentAccelerationLevel = FMath::Clamp(carData->startingAccelerationLevel, 1, maxAccelerationLevel);
		OnRep_MaxSpeedLevel();
		OnRep_AccelerationLevel();
		brokenWheelFrictionMultiplier = carData->BrokenWheelFrictionMultiplier;
		brokenSteeringWheelAngle = carData->BrokenSteeringWheelAngle;
		brokenEngineTorqueMultiplier = carData->BrokenEngineTorqueMultiplier;
		brokenEngineMaxSpeed = carData->BrokenEngineMaxSpeed;
	}
}

void UGVehicleMovementComponent::BreakWheel(int32 wheelIndex)
{
	brokenWheels.Add(wheelIndex);
	if (!Wheels.IsValidIndex(wheelIndex))
	{
		return;
	}

	SetAffectedByEngine(wheelIndex, false);
	SetAffectedByBrake(wheelIndex, false);
	SetAffectedBySteering(wheelIndex, true);

	SetWheelFrictionMultiplier(wheelIndex, brokenWheelFrictionMultiplier);
	SetWheelMaxBrakeTorque(wheelIndex, 0.0f);
	SetWheelHandbrakeTorque(wheelIndex, 0.0f);

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::Red, TEXT("The Wheel disabled"), true);
}

void UGVehicleMovementComponent::RepairWheel(int32 wheelIndex)
{
	brokenWheels.Remove(wheelIndex);
	if (!Wheels.IsValidIndex(wheelIndex))
	{
		return;
	}
	SetAffectedByEngine(wheelIndex, true);
	SetAffectedByBrake(wheelIndex, true);
	SetAffectedBySteering(wheelIndex, true);
	SetWheelFrictionMultiplier(wheelIndex, 1.0f);
	SetWheelMaxBrakeTorque(wheelIndex, 1.0f);
	SetWheelHandbrakeTorque(wheelIndex, 1.0f);
}

void UGVehicleMovementComponent::BreakSteeringWheel()
{
	bSteeringBroken = true;
	for (int i = 0; i < Wheels.Num(); ++i)
	{
		SetWheelMaxSteerAngle(i, brokenSteeringWheelAngle);
	}
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Red, TEXT("The steering whel is broken"), true);
}

void UGVehicleMovementComponent::RepairSteeringWheel()
{
	bSteeringBroken = false;
	for (int i = 0; i < Wheels.Num(); ++i)
	{
		SetWheelMaxSteerAngle(i, 45.0f);
	}
}

void UGVehicleMovementComponent::BreakEngine()
{
	bEngineBroken = true;
	if (!UsesArcadePhysics())
	{
		SetMaxEngineTorque(GetUpgradedTorque() * brokenEngineTorqueMultiplier);
	}
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Red, TEXT("The engine is broken"), true);
}

void UGVehicleMovementComponent::RepairEngine()
{
	bEngineBroken = false;
	if (!UsesArcadePhysics())
	{
		SetMaxEngineTorque(GetUpgradedTorque());
	}
}

void UGVehicleMovementComponent::StopDrivingInput()
{
	lastArcadeInputSent = FGArcadeVehicleInput();
	remoteArcadeInput = FGArcadeVehicleInput();
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ClearAllInput();
	bWindowDrivingLocked = false;
	windowThrottle = 0.0f;
	ReplicatedState = FVehicleReplicatedState();
	ReplicatedState.BrakeInput = 1.0f;
	ReplicatedState.HandbrakeInput = 1.0f;
	BrakeInput = 1.0f;
	HandbrakeInput = 1.0f;
	SetHandbrakeInput(true);
	SetTargetGear(0, true);
}

void UGVehicleMovementComponent::UpdateState(float deltaTime)
{
	// An unpossessed vehicle otherwise reuses the previous driver's replicated throttle.
	if (bRequiresControllerForInputs && GetOwner() && GetOwner()->HasAuthority() && !GetController())
	{
		StopDrivingInput();
	}
	if (bWindowDrivingLocked)
	{
		ApplyWindowDrivingLock();
	}
	Super::UpdateState(deltaTime);
	if (bWindowDrivingLocked)
	{
		ApplyWindowDrivingLock();
	}
}

void UGVehicleMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGVehicleMovementComponent, bWindowDrivingLocked);
	DOREPLIFETIME(UGVehicleMovementComponent, windowThrottle);
	//max speed level
	DOREPLIFETIME(UGVehicleMovementComponent, currentMaxSpeedLevel);
	DOREPLIFETIME(UGVehicleMovementComponent, maxMaxSpeedLevel);
	//accelaeration level
	DOREPLIFETIME(UGVehicleMovementComponent, currentAccelerationLevel);
	DOREPLIFETIME(UGVehicleMovementComponent, maxAccelerationLevel);
	//max rpm
	DOREPLIFETIME(UGVehicleMovementComponent, maxRPMPerLevel);
	DOREPLIFETIME(UGVehicleMovementComponent, baseMaxRPM);
	// max torque
	DOREPLIFETIME(UGVehicleMovementComponent, maxTorquePerLevel);
	DOREPLIFETIME(UGVehicleMovementComponent, baseMaxTorque);
	//idle rpm
	DOREPLIFETIME(UGVehicleMovementComponent, idleRPMPerLevel);
	DOREPLIFETIME(UGVehicleMovementComponent, baseIdleRPM);
	// rev
	DOREPLIFETIME(UGVehicleMovementComponent, revUpPerLevel);
	DOREPLIFETIME(UGVehicleMovementComponent, baseRevUp);
}

void UGVehicleMovementComponent::SetWindowDrivingLock(bool bLocked)
{
	const bool bHasAuthority = GetOwner() && GetOwner()->HasAuthority();
	if (!bHasAuthority || bWindowDrivingLocked == bLocked)
	{
		return;
	}
	if (bLocked)
	{
		const bool bLocalDriver = GetController() && GetController()->IsLocalController();
		const float remoteThrottle = UsesArcadePhysics() ? remoteArcadeInput.throttle : ReplicatedState.ThrottleInput;
		windowThrottle = FMath::Clamp(bLocalDriver ? GetThrottleInput() : remoteThrottle, 0.0f, 1.0f);
	}
	bWindowDrivingLocked = bLocked;
	OnRep_WindowDrivingLock();
	GetOwner()->ForceNetUpdate();
}

void UGVehicleMovementComponent::OnRep_WindowDrivingLock()
{

	if (bWindowDrivingLocked)
	{
		ApplyWindowDrivingLock();
		return;
	}
	// Releases received while outside must not leave a latched throttle on return.
	ClearRawInput();
	lastArcadeInputSent = FGArcadeVehicleInput();
	remoteArcadeInput = FGArcadeVehicleInput();
	if (GetController())
	{
		ThrottleInput = SteeringInput = BrakeInput = HandbrakeInput = 0.0f;
		ReplicatedState.ThrottleInput = ReplicatedState.SteeringInput = 0.0f;
		ReplicatedState.BrakeInput = ReplicatedState.HandbrakeInput = 0.0f;
	}
}


void UGVehicleMovementComponent::SubscribeOnDelegates()
{
	AGPlayerCar* car = Cast<AGPlayerCar>(GetOwner());
	UGVehicleModuleComponent* moduleComponent = car->GetModuleComponent();
	UGVehicleEngineModule* engine = Cast<UGVehicleEngineModule>(moduleComponent->FindModule("Engine"));
	engine->OnEngineChangeState.AddDynamic(this, &UGVehicleMovementComponent::HandleEngineState);
	UGVehicleSteeringWheelModule* steeringWheel = Cast<UGVehicleSteeringWheelModule>(moduleComponent->FindModule("Steering_Wheel"));
	steeringWheel->OnSteeringWheelChangeState.AddDynamic(this, &UGVehicleMovementComponent::HandleSteeringWheelState);
	auto wheels = moduleComponent->GetModulesOfType<UGVehicleWheelModule>();
	for (auto& wheel : wheels)
	{
		wheel->OnWheelChangeState.AddDynamic(this, &UGVehicleMovementComponent::HandleWheelState);
	}
}

void UGVehicleMovementComponent::HandleEngineState(EGModuleState newState)
{
	if (newState == EGModuleState::Destroyed)
	{
		BreakEngine();
	}
	else if (newState == EGModuleState::Full)
	{
		RepairEngine();
	}
}

void UGVehicleMovementComponent::HandleSteeringWheelState(EGModuleState newState)
{
	if (newState == EGModuleState::Destroyed)
	{
		BreakSteeringWheel();
	}
	else if (newState == EGModuleState::Full)
	{
		RepairSteeringWheel();
	}
}

void UGVehicleMovementComponent::HandleWheelState(int32 wheelIndex, EGModuleState newState)
{
	if (newState == EGModuleState::Destroyed)
	{
		BreakWheel(wheelIndex);
	}
	else if (newState == EGModuleState::Full)
	{
		RepairWheel(wheelIndex);
	}
}

void UGVehicleMovementComponent::ApplyWindowDrivingLock()
{
	SetThrottleInput(windowThrottle);
	SetSteeringInput(0.0f);
	SetBrakeInput(0.0f);
	SetHandbrakeInput(false);
	SteeringInput = 0.0f;
	BrakeInput = 0.0f;
	HandbrakeInput = 0.0f;
	ThrottleInput = windowThrottle;
	ReplicatedState.SteeringInput = 0.0f;
	ReplicatedState.BrakeInput = 0.0f;
	ReplicatedState.HandbrakeInput = 0.0f;
	ReplicatedState.ThrottleInput = windowThrottle;
}
