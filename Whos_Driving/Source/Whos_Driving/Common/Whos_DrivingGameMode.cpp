// Copyright Epic Games, Inc. All Rights Reserved.

#include "Whos_DrivingGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

AWhos_DrivingGameMode::AWhos_DrivingGameMode()
{
	// stub
}

void AWhos_DrivingGameMode::SpawnCar()
{
	if (!car)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("NO car, spawning"));
		FActorSpawnParameters parameters;
		parameters.Owner = this;
		parameters.Instigator = GetInstigator();
		parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FTransform transform = FTransform::Identity;
		car = GetWorld()->SpawnActor<AGPlayerCar>(carClass, transform, parameters);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Car already exists"));
	}
}

void AWhos_DrivingGameMode::SpawnPlayer(APlayerController* newPlayer)
{
	UE_LOG(LogTemp, Log, TEXT("Iterating over Player Controller: %s"), *newPlayer->GetName());
	if (car)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, TEXT("Car is there, spawning driver"));
		car->SpawnDriver(newPlayer);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("NO CAR"));
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, TEXT("no car to spawn player in, spawning"));
		SpawnCar();
		if (car)
		{
			car->SpawnDriver(newPlayer);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, TEXT("no car even after, spawning"));
		}
	}
}

void AWhos_DrivingGameMode::BeginPlay()
{
	SpawnCar();
	if (car)
	{
		UE_LOG(LogTemp, Log, TEXT("Successfully spawned %s at World Center."), *car->GetName());
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Car spawned"));

	}
	return;
}

void AWhos_DrivingGameMode::HandleStartingNewPlayer_Implementation(APlayerController* newPlayer)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("EVENT STARTING NEW PLAYER IN LOBBY"));
	Super::HandleStartingNewPlayer_Implementation(newPlayer);
	SpawnPlayer(newPlayer);
}
