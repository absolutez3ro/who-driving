// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleModuleComponent.h"
#include "GVehicleModule.h"

UGVehicleModule* UGVehicleModuleComponent::FindModule(FName moduleId) const
{
	if (moduleId.IsNone())
	{
		return nullptr;
	}

	for (UGVehicleModule* module : modules)
	{
		if (module && module->moduleId == moduleId)
		{
			return module;
		}
	}
	return nullptr;
}

void UGVehicleModuleComponent::DamageModule(FName moduleId, float damage)
{
	AActor* ownerActor = GetOwner();
	if (!ownerActor || !ownerActor->HasAuthority())
	{
		return;
	}

	UGVehicleModule* module = FindModule(moduleId);
	if (!module)
	{
		return;
	}

	module->ApplyDamage(damage);
}

void UGVehicleModuleComponent::RegisterModule(UGVehicleModule* module)
{
	if (!module || module->moduleId.IsNone())
	{
		return;
	}

	modules.AddUnique(module);
}

void UGVehicleModuleComponent::ApplyReplicatedModuleState(const FName& moduleId, float maxHp, float currentHp, EGModuleState moduleState)
{
	UGVehicleModule* module = FindModule(moduleId);

	if (!module)
	{
		return;
	}

	module->ApplyReplicatedState(maxHp, currentHp, moduleState);
}

void UGVehicleModuleComponent::RebuildModuleRegistry(AActor* ownerActor)
{
	if (!ownerActor)
	{
		return;
	}

	modules.Reset();

	TArray<UObject*> subobjects;
	GetObjectsWithOuter(ownerActor, subobjects, false);

	for (UObject* object : subobjects)
	{
		UGVehicleModule* module =
			Cast<UGVehicleModule>(object);

		if (module)
		{
			RegisterModule(module);
		}

	}
}

const TArray<TObjectPtr<UGVehicleModule>>& UGVehicleModuleComponent::GetModules() const
{
	return modules;
}
