// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleDoorModule.h"
#include "GPlayerCar.h"


void UGVehicleDoorModule::OnDestroyed()
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
		//doorMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		//doorMesh->AddWorldOffset(FVector(0.0f, 100.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
		//doorMesh->SetMobility(EComponentMobility::Movable);
		//doorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		//doorMesh->SetEnableGravity(true);
		//doorMesh->SetSimulatePhysics(true);
		//doorMesh->AddImpulse(FVector(0.0f, 100000.0f, 20000.0f), NAME_None, true);
		doorMesh->SetVisibility(false, true);
	}
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Red, TEXT("Door is broken"));
}