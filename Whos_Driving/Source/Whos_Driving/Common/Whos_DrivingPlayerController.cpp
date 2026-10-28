// Copyright Epic Games, Inc. All Rights Reserved.


#include "Whos_DrivingPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Whos_DrivingCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Whos_Driving.h"
#include "Widgets/Input/SVirtualJoystick.h"

AWhos_DrivingPlayerController::AWhos_DrivingPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AWhos_DrivingCameraManager::StaticClass();
}

void AWhos_DrivingPlayerController::BeginPlay()
{
	Super::BeginPlay();

	
	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		mobileControlsWidget = CreateWidget<UUserWidget>(this, mobileControlsWidgetClass);

		if (mobileControlsWidget)
		{
			// add the controls to the player screen
			mobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogWhos_Driving, Error, TEXT("Could not spawn mobile controls widget."));
		}

	}
}

void AWhos_DrivingPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* currentContext : defaultMappingContexts)
			{
				subsystem->AddMappingContext(currentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* currentContext : mobileExcludedMappingContexts)
				{
					subsystem->AddMappingContext(currentContext, 0);
				}
			}
		}
	}
	
}

bool AWhos_DrivingPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

