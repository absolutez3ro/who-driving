// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleEngineModule.h"
#include "GPlayerCar.h"
#include "GVehicleMovementComponent.h"

void UGVehicleEngineModule::OnDestroyed()
{
	AGPlayerCar* car = Cast<AGPlayerCar>(GetOuter());
	UGVehicleMovementComponent* movement = Cast<UGVehicleMovementComponent>(car->GetMovementComponent());

	movement->BreakEngine();
}

void UGVehicleEngineModule::ApplyRepair()
{
	if (!CanRepair())
	{
		return;
	}

	currentHP = maxHP;
	SetState(EGModuleState::Full);

	AGPlayerCar* car = Cast<AGPlayerCar>(GetOuter());
	UGVehicleMovementComponent* movement = Cast<UGVehicleMovementComponent>(car->GetMovementComponent());
	movement->RepairEngine();
}
