// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleDoorModule.h"
#include "GPlayerCar.h"


void UGVehicleDoorModule::OnDestroyed()
{
	OnDoorBreak.Broadcast(moduleId);
}

void UGVehicleDoorModule::ApplyRepair()
{
	AGPlayerCar* car = Cast<AGPlayerCar>(GetOuter());

	USkeletalMeshComponent* doorMesh = nullptr;
	TArray<USceneComponent*> children;
	car->GetMesh()->GetChildrenComponents(true, children);

	for (USceneComponent* child : children)
	{
		if (child->GetFName() == moduleId)
		{
			doorMesh = Cast<USkeletalMeshComponent>(child);
			break;
		}
	}

	if (doorMesh)
	{
		doorMesh->SetVisibility(true, true);
	}

	currentHP = maxHP;
	SetState(EGModuleState::Full);
}