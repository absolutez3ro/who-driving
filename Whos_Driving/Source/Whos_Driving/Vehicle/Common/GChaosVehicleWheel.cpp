// Fill out your copyright notice in the Description page of Project Settings.


#include "GChaosVehicleWheel.h"

UGChaosVehicleWheel::UGChaosVehicleWheel() 
{
	WheelRadius = 31.0f;
	WheelWidth = 21.0f;
	WheelMass = 20.0f;

	SuspensionMaxRaise = 10.0f;
	SuspensionMaxDrop = 15.0f;
	SuspensionDampingRatio = 0.8f;
	
	FrictionForceMultiplier = 1.0f;
	MaxSteerAngle = 35.0f;
	bAffectedBySteering = false;
}