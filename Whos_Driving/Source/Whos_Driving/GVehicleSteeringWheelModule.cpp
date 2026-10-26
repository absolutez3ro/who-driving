// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleSteeringWheelModule.h"
#include "GPlayerCar.h"
#include "GVehicleMovementComponent.h"

void UGVehicleSteeringWheelModule::OnDestroyed()
{
	AGPlayerCar* car = Cast<AGPlayerCar>(GetOuter());

	UGVehicleMovementComponent* movement = Cast<UGVehicleMovementComponent>(car->GetMovementComponent());
	movement->BreakSteeringWheel();
}

void UGVehicleSteeringWheelModule::ApplyRepair()
{
	AGPlayerCar* car = Cast<AGPlayerCar>(GetOuter());

	currentHP = maxHP;
	SetState(EGModuleState::Full);
	UGVehicleMovementComponent* movement = Cast<UGVehicleMovementComponent>(car->GetMovementComponent());
	movement->RepairSteeringWheel();
}