// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleDoorModule.h"
#include "GPlayerCar.h"


void UGVehicleDoorModule::OnDestroyed()
{
}

void UGVehicleDoorModule::OnStateChanged(EGModuleState oldState, EGModuleState newState)
{
	OnDoorChangeState.Broadcast(moduleId, newState);
}