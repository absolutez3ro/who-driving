// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleElectricianModule.h"

void UGVehicleElectricianModule::OnDestroyed()
{
}

void UGVehicleElectricianModule::OnStateChanged(EGModuleState oldState, EGModuleState newState)
{
	OnElectricianChangeState.Broadcast(newState);
}
