// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleMovementComponent.h"

UGVehicleMovementComponent::UGVehicleMovementComponent() 
{
	EngineSetup.MaxTorque = 500.0f;
	EngineSetup.MaxRPM = 6000.0f;
	EngineSetup.EngineIdleRPM = 800.0f;

	FRichCurve* torqueCurve = EngineSetup.TorqueCurve.GetRichCurve();
	if (torqueCurve) 
	{
		torqueCurve->AddKey(0.0f, 100.0f);
		torqueCurve->AddKey(1000.0f, 300.0f);
		torqueCurve->AddKey(3000.0f, 450.0f);
		torqueCurve->AddKey(5000.0f, 500.0f);
		torqueCurve->AddKey(6000.0f, 350.0f);
	}

	TransmissionSetup.bUseAutomaticGears = true;
	TransmissionSetup.GearChangeTime = 0.2f;
	TransmissionSetup.FinalRatio = 3.5f;
	TransmissionSetup.ForwardGearRatios = { 3.2f, 2.1f, 1.4f, 1.0f, 0.8f };
	TransmissionSetup.ReverseGearRatios = { 3.0f };
}

void UGVehicleMovementComponent::DisableWheel(int32 wheelIndex)
{
	if (!Wheels.IsValidIndex(wheelIndex))
	{
		return;
	}

	SetAffectedByEngine(wheelIndex, false);
	SetAffectedByBrake(wheelIndex, false);
	SetAffectedBySteering(wheelIndex, false);
	SetAffectedByHandbrake(wheelIndex, false);

	SetWheelFrictionMultiplier(wheelIndex, 0.0f);
	SetWheelMaxBrakeTorque(wheelIndex, 0.0f);
	SetWheelMaxSteerAngle(wheelIndex, 0.0f);
	SetWheelHandbrakeTorque(wheelIndex, 0.0f);
}
