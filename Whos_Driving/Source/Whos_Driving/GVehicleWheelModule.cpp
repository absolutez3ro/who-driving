// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleWheelModule.h"
#include "GPlayerCar.h"

void UGVehicleWheelModule::OnDestroyed()
{
	AGPlayerCar* car = Cast<AGPlayerCar>(GetOuter());
	if (car) 
	{
		car->OnWheelDestroyed(wheelIndex);
		GEngine->AddOnScreenDebugMessage(0, 2.0f, FColor::Red, TEXT("Wheel is destroyed"));
	}
}
