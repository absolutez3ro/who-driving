// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleDoorModule.h"
#include "GPlayerCar.h"


void UGVehicleDoorModule::OnDestroyed()
{
	AGPlayerCar* car = Cast<AGPlayerCar>(GetOuter());

	if (!car)
	{
		return;
	}

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
		doorMesh->SetVisibility(false, true);
	}
}