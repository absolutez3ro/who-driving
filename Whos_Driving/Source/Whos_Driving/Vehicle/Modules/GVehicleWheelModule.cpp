// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleWheelModule.h"
#include "GPlayerCar.h"
#include "GVehicleMovementComponent.h"

void UGVehicleWheelModule::OnDestroyed()
{
}

void UGVehicleWheelModule::ApplyRepair()
{
	if (!CanRepair())
	{
		return;
	}

	currentHP = maxHP;
	SetState(EGModuleState::Full);
}

void UGVehicleWheelModule::OnStateChanged(EGModuleState oldState, EGModuleState newState)
{
	OnWheelChangeState.Broadcast(wheelIndex, newState);
}
