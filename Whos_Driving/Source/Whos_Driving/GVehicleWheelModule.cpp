// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleWheelModule.h"
#include "GPlayerCar.h"
#include "GVehicleMovementComponent.h"

void UGVehicleWheelModule::OnDestroyed()
{
	AGPlayerCar* car = Cast<AGPlayerCar>(GetOuter());
	if (!car) 
	{
		return;
	}

	UGVehicleMovementComponent* movementComponent = Cast<UGVehicleMovementComponent>(car->GetMovementComponent());
	movementComponent->DisableWheel(wheelIndex);

	car->PublishModuleStates();
}
