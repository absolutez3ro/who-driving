// Copyright Epic Games, Inc. All Rights Reserved.

#include "Whos_DrivingGameMode.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Common/GGIWhosDriving.h"
#include "GPlayerController.h"
#include "GVehicleWeapon.h"
#include "Whos_DrivingCharacter.h"
#include "Vehicle/Components/GVehicleCombatComponent.h"
#include "GGameState.h"
#include "GRaceFinish.h"
#include "Kismet/GameplayStatics.h"

AWhos_DrivingGameMode::AWhos_DrivingGameMode()
{
	GameStateClass = AGGameState::StaticClass();
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
		static const FName carSpawnTag(TEXT("RaceCarSpawn"));
		FTransform transform = FTransform::Identity;
		for (TActorIterator<ATargetPoint> marker(GetWorld()); marker; ++marker)
		{
			if (marker->ActorHasTag(carSpawnTag))
			{
				transform = FTransform(marker->GetActorQuat(), marker->GetActorLocation());
				break;
			}
		}
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
	TObjectPtr<AGGameState> GS =GetGameState<AGGameState>();
	if (GS)
	{
		GS->OnPlayerFinish.AddDynamic(this, &AWhos_DrivingGameMode::OnLevelEnd);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("FAILED TO GET GAME STATE IN GAMEMODE")));
	}
	SpawnCar();
	if (car)
	{
		UE_LOG(LogTemp, Log, TEXT("Successfully spawned %s at %s."), *car->GetName(), *car->GetActorLocation().ToString());
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Car spawned"));
	}
	if (GS)
	{
		for (TActorIterator<AGRaceFinish> line(GetWorld()); line; ++line)
		{
			if (line->ActorHasTag(AGRaceFinish::StartLineTag))
			{
				return;
			}
		}
		GS->StartRaceTimer();
	}
}

void AWhos_DrivingGameMode::HandleStartingNewPlayer_Implementation(APlayerController* newPlayer)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("EVENT STARTING NEW PLAYER IN LOBBY"));
	Super::HandleStartingNewPlayer_Implementation(newPlayer);
	SpawnPlayer(newPlayer);
	AGPlayerController* pc = Cast<AGPlayerController>(newPlayer);
	RegisterParticipant(pc);
}

void AWhos_DrivingGameMode::OnLevelEnd()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("Level ended traveling")));
	GetGameState<AGGameState>()->OnPlayerFinish.RemoveDynamic(this, &AWhos_DrivingGameMode::OnLevelEnd);
	TObjectPtr<UGGIWhosDriving> gi = GetGameInstance<UGGIWhosDriving>();
	if (gi)
	{
		gi->moneyStored += gi->GetMoneyFromTotal();
		UGVehicleMovementComponent* movementComponent = Cast<UGVehicleMovementComponent>(car->GetVehicleMovementComponent());
		gi->carUpgradeData.AccelerationLevel = movementComponent->currentAccelerationLevel;
		gi->carUpgradeData.maxSpeedLevel = movementComponent->currentMaxSpeedLevel;
		//idgaf which they have same upgrades
		TObjectPtr<AWhos_DrivingCharacter> character = Cast<AWhos_DrivingCharacter>(UGameplayStatics::GetActorOfClass(GetWorld(), AWhos_DrivingCharacter::StaticClass()));
		for(TObjectPtr<AGVehicleWeapon> weapon : character->GetCombatComponent()->GetWeapons())
		{
			if (weapon->GetDefinition()->bUsableByDriverInCabin) {
				gi->characterUpgradeData.ammoLevelPistol = weapon->GetMagazineSizeLevel();
				gi->characterUpgradeData.damageLevelPistol = weapon->GetDamageLevel();

			}
			else
			{
				gi->characterUpgradeData.ammoLevelRifle = weapon->GetMagazineSizeLevel();
				gi->characterUpgradeData.damageLevelRifle = weapon->GetDamageLevel();
			}
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("NO GI")));
	}
	GetGameInstance<UGGIWhosDriving>()->FetchCarData();

	if (UWorld* World = GetWorld())
	{
		const FString url = shopLevel.GetLongPackageName() + TEXT("?listen");
		World->ServerTravel(url);
	}
}
void AWhos_DrivingGameMode::RegisterParticipant(AGPlayerController* pc)
{
	if (HasAuthority() && IsValid(pc))
	{
		participants.Add(pc);
	}
}

void AWhos_DrivingGameMode::UnregisterParticipant(AGPlayerController* pc)
{
	if (HasAuthority() && IsValid(pc))
	{
		participants.Remove(pc);
	}
}

void AWhos_DrivingGameMode::ReportFailure(EGRaceFailureReason reason)
{
	if (!HasAuthority() || bFailureAccepted || reason == EGRaceFailureReason::None)
	{
		return;
	}

	bFailureAccepted = true;
	AGGameState* gameState = GetGameState<AGGameState>();
	gameState->SetRaceFailed(reason);
	FTimerHandle restartTimer;
	GetWorld()->GetTimerManager().SetTimer(restartTimer, this, &AWhos_DrivingGameMode::RestartCurrentLevel, 3.0f, false);
}

void AWhos_DrivingGameMode::CheckAllPlayersKnockedOut()
{
	if (!HasAuthority())
	{
		return;
	}
	int32 validParticipants = 0;
	for (const auto& weakPc : participants)
	{
		AGPlayerController* pc = weakPc.Get();
		if (!pc || !pc->GetPlayerState<APlayerState>())
		{
			continue;
		}
		const AWhos_DrivingCharacter* character = pc->GetDriverCharacter();
		if (!IsValid(character) || !character->GetCombatComponent())
		{
			continue;
		}

		++validParticipants;
		if (!character->GetCombatComponent()->bInKnockout)
		{
			return;
		}
	}
	if (validParticipants > 0)
	{
		ReportFailure(EGRaceFailureReason::AllPlayersKnockedOut);
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Red, TEXT("RACE FAILED BECAUSE ALL PLAYERS ARE KNOCKED OUT"));
	}
}

void AWhos_DrivingGameMode::CheckVehicleDestroyed()
{
	ReportFailure(EGRaceFailureReason::VehicleDestroyed);
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Red, TEXT("RACE FAILED BECAUSE VEHICLE IS DESTROYED"));
}

void AWhos_DrivingGameMode::Logout(AController* exitingController)
{
	if (AGPlayerController* pc = Cast<AGPlayerController>(exitingController))
	{
		UnregisterParticipant(pc);
	}
	Super::Logout(exitingController);
}

void AWhos_DrivingGameMode::RestartCurrentLevel()
{
	FString currentLevelName = GetWorld()->GetName();
	UGameplayStatics::OpenLevel(GetWorld(), FName(*currentLevelName));
}
