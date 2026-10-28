#include "GVehicleGameMode.h"
#include "GPlayerCar.h"
#include "GPlayerController.h"
#include "GVehicleSeatComponent.h"
#include "GVehicleSeatingComponent.h"
#include "GWhosDrivingCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void AGVehicleGameMode::RestartPlayer(AController* newPlayer)
{
	AGPlayerController* controller = Cast<AGPlayerController>(newPlayer);
	if (!controller || controller->GetPawn() || !DefaultPawnClass || !DefaultPawnClass->IsChildOf(AGPlayerCar::StaticClass()))
	{
		Super::RestartPlayer(newPlayer);
		return;
	}
	if (!IsValid(sharedVehicle))
	{
		for (TActorIterator<AGPlayerCar> iterator(GetWorld()); iterator; ++iterator)
		{
			if (iterator->IsA(DefaultPawnClass) && !iterator->IsActorBeingDestroyed())
			{
				sharedVehicle = *iterator;
				break;
			}
		}
	}
	if (!IsValid(sharedVehicle))
	{
		Super::RestartPlayer(newPlayer);
		sharedVehicle = Cast<AGPlayerCar>(controller->GetPawn());
		return;
	}
	UGVehicleSeatingComponent* seating = sharedVehicle->GetSeatingComponent();
	for (int32 seatIndex = 0; seatIndex < 4; ++seatIndex)
	{
		const FGVehicleSeatState state = seating->GetSeatState(seatIndex);
		UGVehicleSeatComponent* seat = seating->GetSeatComponent(seatIndex);
		if (!seat || state.occupant || state.reservedBy)
		{
			continue;
		}
		FActorSpawnParameters parameters;
		parameters.Owner = controller;
		parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FTransform transform = seat->characterOffset * seat->GetComponentTransform();
		AGWhosDrivingCharacter* character = GetWorld()->SpawnActor<AGWhosDrivingCharacter>(
			sharedVehicle->GetSeatCharacterClass(), transform, parameters);
		if (character && seating->InitializeOccupant(controller, character, seatIndex))
		{
			return;
		}
		if (character)
		{
			character->Destroy();
		}
	}
	// A full car does not prevent an additional player from joining on foot nearby.
	FActorSpawnParameters parameters;
	parameters.Owner = controller;
	parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector location = sharedVehicle->GetMesh()->GetComponentTransform().TransformPosition(FVector(-350.0f, 230.0f, 110.0f));
	AGWhosDrivingCharacter* character = GetWorld()->SpawnActor<AGWhosDrivingCharacter>(
		sharedVehicle->GetSeatCharacterClass(), location, sharedVehicle->GetActorRotation(), parameters);
	if (character)
	{
		controller->Possess(character);
		controller->SetVehicleContext(nullptr, character);
	}
}
