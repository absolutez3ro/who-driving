// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleModule.h"
#include "GameFramework/Actor.h"

void UGVehicleModule::ApplyDamage(float damage)
{
	if (!FMath::IsFinite(damage) || damage <= 0.0f || IsDestroyed())
	{
		return;
	}

	currentHP = FMath::Clamp(currentHP - damage, 0.0f, maxHP);

	if (currentHP <= 0.0f)
	{
		SetState(EGModuleState::Destroyed);
	}
	else if (currentHP < maxHP * repairThreshold)
	{
		SetState(EGModuleState::Repair);
	}
	else
	{
		SetState(EGModuleState::Full);
	}
}

void UGVehicleModule::ApplyRepair()
{
}

void UGVehicleModule::ApplyReplicatedState(float inMaxHP, float inCurrentHp, EGModuleState state)
{
	maxHP = inMaxHP;
	currentHP = inCurrentHp;
	SetState(state);
}

float UGVehicleModule::GetHealthAsPercentage() const
{
	return currentHP / maxHP * 100.0f;
}


void UGVehicleModule::SetState(EGModuleState newState)
{
	if (moduleState == newState)
	{
		return;
	}

	const EGModuleState previousState = moduleState;
	moduleState = newState;
	OnStateChanged(previousState, newState);
	if (newState == EGModuleState::Destroyed)
	{
		OnDestroyed();
	}
}

bool UGVehicleModule::IsDestroyed() const
{
	return moduleState == EGModuleState::Destroyed;
}

bool UGVehicleModule::CanRepair() const
{
	return (moduleState == EGModuleState::Repair || moduleState == EGModuleState::Destroyed);
}

bool UGVehicleModule::IsFull() const
{
	return moduleState == EGModuleState::Full;
}


void UGVehicleModule::OnDestroyed()
{
}

void UGVehicleModule::OnStateChanged(EGModuleState previousState, EGModuleState newState)
{
}