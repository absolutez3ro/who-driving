// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleModule.h"
#include "GameFramework/Actor.h"
#include "GGameState.h"

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
	RecalculateRepairCost();
}

void UGVehicleModule::RecalculateRepairCost()
{
	if (IsDestroyed())
	{
		actualRepairCost = FMath::RoundToInt(destroyedModuleCostMultiplier * static_cast<float>(baseRepairCost));
		return;
	}

	if (IsFull())
	{
		actualRepairCost = 0;
		return;
	}

	int32 extraCost = 0;
	const float healthPercent = (maxHP > 0.0f) ? (currentHP / maxHP * 100.0f) : 0.0f;
	if (healthPercent < 50.0f)
	{
		extraCost = FMath::FloorToInt((50.0f - healthPercent) / 10.0f);
	}
	actualRepairCost = baseRepairCost + extraCost;
}

void UGVehicleModule::ApplyRepair()
{
	if (!CanRepair())
	{
		return;
	}
	OnRepair.Broadcast();
	GetWorld()->GetGameState<AGGameState>()->PlusRepairParts(-actualRepairCost);
	currentHP = maxHP;
	SetState(EGModuleState::Full);
}

void UGVehicleModule::ApplyReplicatedState(float inMaxHP, float inCurrentHp, EGModuleState state)
{
	maxHP = inMaxHP;
	currentHP = inCurrentHp;
	SetState(state);
	RecalculateRepairCost();
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
	bool bCorrectState = moduleState == EGModuleState::Repair || moduleState == EGModuleState::Destroyed;
	bool bEnoughRepairParts = GetWorld()->GetGameState<AGGameState>()->GetRepairParts() >= actualRepairCost;
	return bCorrectState && bEnoughRepairParts;
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