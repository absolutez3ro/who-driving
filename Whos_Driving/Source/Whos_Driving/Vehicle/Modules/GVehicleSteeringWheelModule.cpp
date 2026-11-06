// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleSteeringWheelModule.h"
#include "GPlayerCar.h"
#include "GVehicleMovementComponent.h"

void UGVehicleSteeringWheelModule::OnDestroyed()
{
}

void UGVehicleSteeringWheelModule::OnStateChanged(EGModuleState oldState, EGModuleState newState)
{
	OnSteeringWheelChangeState.Broadcast(newState);
}