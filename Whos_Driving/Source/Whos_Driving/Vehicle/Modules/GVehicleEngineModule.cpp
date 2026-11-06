// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleEngineModule.h"
#include "GPlayerCar.h"
#include "GVehicleMovementComponent.h"

void UGVehicleEngineModule::OnDestroyed()
{
}

void UGVehicleEngineModule::OnStateChanged(EGModuleState oldState, EGModuleState newState)
{
	OnEngineChangeState.Broadcast(newState);
}
