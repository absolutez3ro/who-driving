// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleElectricianModule.h"

void UGVehicleElectricianModule::OnDestroyed()
{
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Red, TEXT("Electrician is broken"));
}

void UGVehicleElectricianModule::ApplyRepair()
{
	currentHP = maxHP;
	SetState(EGModuleState::Full);
}