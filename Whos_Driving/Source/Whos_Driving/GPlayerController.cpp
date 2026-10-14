// Fill out your copyright notice in the Description page of Project Settings.


#include "GPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Whos_DrivingCharacter.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "GPlayerCar.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

AGPlayerController::AGPlayerController() 
{
}

void AGPlayerController::BeginPlay() 
{
	Super::BeginPlay();

	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	if (ULocalPlayer* localPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* subsystem = localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (vehicleMappingContext)
			{
				subsystem->AddMappingContext(vehicleMappingContext, 1);
			}

			if (mouseLookMappingContext)
			{
				subsystem->AddMappingContext(mouseLookMappingContext, 2);
			}

			if (debugMappingContext)
			{
				subsystem->AddMappingContext(debugMappingContext, 0);
			}
		}
	}
}

void AGPlayerController::SetupInputComponent()
{
	APlayerController::SetupInputComponent();

	UEnhancedInputComponent* enhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);

	if (lookAction)
	{
		enhancedInputComponent->BindAction(lookAction.Get(), ETriggerEvent::Triggered, this, &AGPlayerController::LookInput);
	}

	if (bombAction)
	{
		enhancedInputComponent->BindAction(bombAction.Get(), ETriggerEvent::Started, this, &AGPlayerController::DebugBombInput);
	}

	if (exitVehicleAction)
	{
		enhancedInputComponent->BindAction(exitVehicleAction.Get(), ETriggerEvent::Started, this, &AGPlayerController::DebugExitVehicleInput);
	}
}

void AGPlayerController::LookInput(const FInputActionValue& value)
{
	const FVector2D lookAxis = value.Get<FVector2D>();
	if (driverCharacter)
	{
		driverCharacter->DoAim(lookAxis.X, lookAxis.Y);
	}
}

void AGPlayerController::DebugBombInput(const FInputActionValue& value)
{
	if (AGPlayerCar* playerCar = Cast<AGPlayerCar>(GetPawn()))
	{
		playerCar->SwitchBomb(value);
	}
}

void AGPlayerController::DebugExitVehicleInput(const FInputActionValue& value)
{
	DebugExitVehicle();
}

void AGPlayerController::DebugExitVehicle()
{
	AGPlayerCar* playerCar = Cast<AGPlayerCar>(GetPawn());

	if (!playerCar || !driverCharacter)
	{
		return;
	}

	AWhos_DrivingCharacter* character = driverCharacter;

	character->DetachFromActor(
		FDetachmentTransformRules::KeepWorldTransform
	);

	character->GetCharacterMovement()->GravityScale = 1.0f;
	character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	FVector exitLocation = playerCar->GetActorLocation() + playerCar->GetActorRightVector() * 150.0f;
	character->SetActorLocation(exitLocation);

	character->GetCapsuleComponent()->SetCollisionEnabled(
		ECollisionEnabled::QueryAndPhysics
	);

	character->GetMesh()->SetCollisionEnabled(
		ECollisionEnabled::QueryAndPhysics
	);

	Possess(character);
	SetViewTarget(character);
	EnableCharacterInput();
}

void AGPlayerController::EnableCharacterInput()
{
	if (ULocalPlayer* localPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* subsystem = localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (vehicleMappingContext)
			{
				subsystem->RemoveMappingContext(vehicleMappingContext);
			}

			if (characterMappingContext)
			{
				subsystem->AddMappingContext(characterMappingContext, 1);
			}
		}
	}
}

void AGPlayerController::SetDriverCharacter(AWhos_DrivingCharacter* newDriver) 
{
	driverCharacter = newDriver;
	if (driverCharacter)
	{
		SetViewTarget(driverCharacter);
	}
}

AWhos_DrivingCharacter* AGPlayerController::GetDriverCharacter() const
{
	return driverCharacter;
}