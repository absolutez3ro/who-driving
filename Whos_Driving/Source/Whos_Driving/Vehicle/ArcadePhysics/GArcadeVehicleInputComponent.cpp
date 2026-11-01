#include "GArcadeVehicleInputComponent.h"
#include "GArcadeVehicleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "UObject/UObjectGlobals.h"

UGArcadeVehicleInputComponent::UGArcadeVehicleInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGArcadeVehicleInputComponent::BeginPlay()
{
	Super::BeginPlay();

	vehicle = GetOwner()->FindComponentByClass<UGArcadeVehicleComponent>();
	APawn* pawn = Cast<APawn>(GetOwner());
	if (!pawn)
	{
		return;
	}
	pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UGArcadeVehicleInputComponent::HandleControllerChanged);
	HandleControllerChanged(pawn, nullptr, pawn->GetController());
}

void UGArcadeVehicleInputComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	UnbindFromController();
	if (APawn* pawn = Cast<APawn>(GetOwner()))
	{
		pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UGArcadeVehicleInputComponent::HandleControllerChanged);
	}
	Super::EndPlay(endPlayReason);
}

void UGArcadeVehicleInputComponent::HandleControllerChanged(APawn* pawn, AController* oldController, AController* newController)
{
	UnbindFromController();
	ClearVehicleInput();

	APlayerController* playerController = Cast<APlayerController>(newController);
	if (playerController && playerController->IsLocalController())
	{
		BindToController(playerController);
	}
}

void UGArcadeVehicleInputComponent::BindToController(APlayerController* playerController)
{
	if (!actionInputComponent)
	{
		// The name must be unique within the owner, which already holds this component under a similar name.
		const FName componentName = MakeUniqueObjectName(GetOwner(), UEnhancedInputComponent::StaticClass(), TEXT("ArcadeVehicleActionBindings"));
		actionInputComponent = NewObject<UEnhancedInputComponent>(GetOwner(), componentName);
		actionInputComponent->RegisterComponent();
		BindActions(*actionInputComponent);
	}
	playerController->PushInputComponent(actionInputComponent);

	ULocalPlayer* localPlayer = playerController->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* inputSubsystem =
		localPlayer ? localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (inputSubsystem && inputMappingContext)
	{
		inputSubsystem->AddMappingContext(inputMappingContext, mappingPriority);
	}
	boundController = playerController;
}

void UGArcadeVehicleInputComponent::UnbindFromController()
{
	APlayerController* playerController = boundController.Get();
	boundController.Reset();
	if (!playerController)
	{
		return;
	}

	if (actionInputComponent)
	{
		playerController->PopInputComponent(actionInputComponent);
	}
	ULocalPlayer* localPlayer = playerController->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* inputSubsystem =
		localPlayer ? localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (inputSubsystem && inputMappingContext)
	{
		inputSubsystem->RemoveMappingContext(inputMappingContext);
	}
}

void UGArcadeVehicleInputComponent::BindActions(UEnhancedInputComponent& enhancedInput)
{
	if (throttleAction)
	{
		enhancedInput.BindAction(throttleAction.Get(), ETriggerEvent::Triggered, this, &UGArcadeVehicleInputComponent::HandleThrottle);
		enhancedInput.BindAction(throttleAction.Get(), ETriggerEvent::Completed, this, &UGArcadeVehicleInputComponent::HandleThrottleReleased);
	}
	if (brakeAction)
	{
		enhancedInput.BindAction(brakeAction.Get(), ETriggerEvent::Triggered, this, &UGArcadeVehicleInputComponent::HandleBrake);
		enhancedInput.BindAction(brakeAction.Get(), ETriggerEvent::Completed, this, &UGArcadeVehicleInputComponent::HandleBrakeReleased);
	}
	if (steerAction)
	{
		enhancedInput.BindAction(steerAction.Get(), ETriggerEvent::Triggered, this, &UGArcadeVehicleInputComponent::HandleSteer);
		enhancedInput.BindAction(steerAction.Get(), ETriggerEvent::Completed, this, &UGArcadeVehicleInputComponent::HandleSteerReleased);
	}
	if (handbrakeAction)
	{
		enhancedInput.BindAction(handbrakeAction.Get(), ETriggerEvent::Started, this, &UGArcadeVehicleInputComponent::HandleHandbrakePressed);
		enhancedInput.BindAction(handbrakeAction.Get(), ETriggerEvent::Completed, this, &UGArcadeVehicleInputComponent::HandleHandbrakeReleased);
	}
	if (resetAction)
	{
		enhancedInput.BindAction(resetAction.Get(), ETriggerEvent::Started, this, &UGArcadeVehicleInputComponent::HandleReset);
	}
}

void UGArcadeVehicleInputComponent::ClearVehicleInput()
{
	if (!vehicle)
	{
		return;
	}
	vehicle->SetThrottle(0.0f);
	vehicle->SetBrake(0.0f);
	vehicle->SetSteering(0.0f);
	vehicle->SetHandbrake(false);
}

void UGArcadeVehicleInputComponent::HandleThrottle(const FInputActionValue& value)
{
	if (vehicle)
	{
		vehicle->SetThrottle(value.Get<float>());
	}
}

void UGArcadeVehicleInputComponent::HandleThrottleReleased(const FInputActionValue& value)
{
	if (vehicle)
	{
		vehicle->SetThrottle(0.0f);
	}
}

void UGArcadeVehicleInputComponent::HandleBrake(const FInputActionValue& value)
{
	if (vehicle)
	{
		vehicle->SetBrake(value.Get<float>());
	}
}

void UGArcadeVehicleInputComponent::HandleBrakeReleased(const FInputActionValue& value)
{
	if (vehicle)
	{
		vehicle->SetBrake(0.0f);
	}
}

void UGArcadeVehicleInputComponent::HandleSteer(const FInputActionValue& value)
{
	if (vehicle)
	{
		vehicle->SetSteering(value.Get<float>());
	}
}

void UGArcadeVehicleInputComponent::HandleSteerReleased(const FInputActionValue& value)
{
	if (vehicle)
	{
		vehicle->SetSteering(0.0f);
	}
}

void UGArcadeVehicleInputComponent::HandleHandbrakePressed(const FInputActionValue& value)
{
	if (vehicle)
	{
		vehicle->SetHandbrake(true);
	}
}

void UGArcadeVehicleInputComponent::HandleHandbrakeReleased(const FInputActionValue& value)
{
	if (vehicle)
	{
		vehicle->SetHandbrake(false);
	}
}

void UGArcadeVehicleInputComponent::HandleReset(const FInputActionValue& value)
{
	if (vehicle)
	{
		vehicle->ResetVehicle();
	}
}
