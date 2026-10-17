// Fill out your copyright notice in the Description page of Project Settings.


#include "GVehicleModule.h"
#include "GameFramework/Actor.h"

void UGVehicleModule::ApplyDamage(float damage)
{
	const AActor* ownerActor = GetTypedOuter<AActor>();
	if ((ownerActor && !ownerActor->HasAuthority()) || bDestroyed || !FMath::IsFinite(damage) || damage <= 0.0f)
	{
		return;
	}

	currentHP = FMath::Max(currentHP - damage, 0.0f);
	OnDamageTaken(damage);

	if (currentHP <= 0.0f)
	{
		bDestroyed = true;
		OnDestroyed();
	}
}

bool UGVehicleModule::IsDestroyed() const
{
	return bDestroyed;
}

void UGVehicleModule::OnDamageTaken(float Damage)
{
}

void UGVehicleModule::OnDestroyed()
{
}
