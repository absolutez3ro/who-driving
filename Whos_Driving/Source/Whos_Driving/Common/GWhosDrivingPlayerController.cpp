// Copyright Epic Games, Inc. All Rights Reserved.


#include "GWhosDrivingPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "GWhosDrivingCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Whos_Driving.h"
#include "Widgets/Input/SVirtualJoystick.h"

AGWhosDrivingPlayerController::AGWhosDrivingPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AGWhosDrivingCameraManager::StaticClass();
}

void AGWhosDrivingPlayerController::BeginPlay()
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
			UE_LOG(LogWhosDriving, Error, TEXT("Could not spawn mobile controls widget."));
		}

	}
}

void AGWhosDrivingPlayerController::SetupInputComponent()
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

bool AGWhosDrivingPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

