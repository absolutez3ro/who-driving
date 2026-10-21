// Copyright Epic Games, Inc. All Rights Reserved.

#include "Whos_DrivingGameMode.h"

AWhos_DrivingGameMode::AWhos_DrivingGameMode()
{
	// stub
}

void AWhos_DrivingGameMode::SpawnCar() {
	if (!car) {
		FActorSpawnParameters parameters;
		parameters.Owner = this;
		parameters.Instigator = GetInstigator();
		parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FTransform transform = FTransform::Identity;
		car = GetWorld()->SpawnActor<AGPlayerCar>(carClass, transform, parameters);
	}
}
void AWhos_DrivingGameMode::BeginPlay() 
{
	SpawnCar();
	if (car)
	{
		UE_LOG(LogTemp, Log, TEXT("Successfully spawned %s at World Center."), *car->GetName());
	}
	return;
}

void AWhos_DrivingGameMode::PostLogin(APlayerController* NewPlayer)
{
	UE_LOG(LogTemp, Log, TEXT("Iterating over Player Controller: %s"), *NewPlayer->GetName());
	if (car)
	{
		car->SpawnDriver(NewPlayer);
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("NO CAR"));
		SpawnCar();
		car->SpawnDriver(NewPlayer);
	}
}