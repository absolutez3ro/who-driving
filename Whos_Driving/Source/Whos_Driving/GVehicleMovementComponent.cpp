// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleMovementComponent.h"
#include "Net/UnrealNetwork.h"

UGVehicleMovementComponent::UGVehicleMovementComponent() 
{
	EngineSetup.MaxTorque = 500.0f;
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

void UGVehicleMovementComponent::BreakWheel(int32 wheelIndex)
{
	if (!Wheels.IsValidIndex(wheelIndex))
	{
		return;
	}

	SetAffectedByEngine(wheelIndex, false);
	SetAffectedByBrake(wheelIndex, false);
	SetAffectedBySteering(wheelIndex, true);

	SetWheelFrictionMultiplier(wheelIndex, BrokenWheelFrictionMultiplier);
	SetWheelMaxBrakeTorque(wheelIndex, 0.0f);
	SetWheelHandbrakeTorque(wheelIndex, 0.0f);

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::Red, TEXT("The Wheel disabled"), true);
}

void UGVehicleMovementComponent::BreakSteeringWheel()
{
	for (int i = 0; i < 4; ++i)
	{
		SetWheelMaxSteerAngle(i, BrokenSteeringWheelAngle);
	}
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Red, TEXT("The steering whel is broken"), true);
}

void UGVehicleMovementComponent::BreakEngine()
{
	bEngineBroken = true;
	EngineSetup.MaxTorque *= BrokenEngineTorqueMultiplier;
	EngineSetup.MaxRPM *= BrokenEngineRPMMultiplier;
	FRichCurve* curve = EngineSetup.TorqueCurve.GetRichCurve();
	const TArray<FRichCurveKey> curveKeys = curve->GetConstRefOfKeys();
	for (const auto& key : curveKeys)
	{
		const float time = key.Time;
		const float value = key.Value;
		curve->UpdateOrAddKey(time, value*BrokenEngineTorqueMultiplier);
	}
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Red, TEXT("The engine is broken"), true);
}

void UGVehicleMovementComponent::StopDrivingInput()
{
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
	if (bWindowDrivingLocked) { ApplyWindowDrivingLock(); }
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
}

void UGVehicleMovementComponent::SetWindowDrivingLock(bool bLocked)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bWindowDrivingLocked == bLocked) { return; }
	if (bLocked)
	{
		windowThrottle = FMath::Clamp(GetController() && GetController()->IsLocalController() ? GetThrottleInput() : ReplicatedState.ThrottleInput, 0.0f, 1.0f);
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
	if (GetController())
	{
		ThrottleInput = SteeringInput = BrakeInput = HandbrakeInput = 0.0f;
		ReplicatedState.ThrottleInput = ReplicatedState.SteeringInput = 0.0f;
		ReplicatedState.BrakeInput = ReplicatedState.HandbrakeInput = 0.0f;
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
