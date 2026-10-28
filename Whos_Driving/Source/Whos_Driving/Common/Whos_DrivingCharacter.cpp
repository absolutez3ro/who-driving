// Copyright Epic Games, Inc. All Rights Reserved.

#include "Whos_DrivingCharacter.h"
#include "GPlayerCar.h"
#include "GVehicleCombatComponent.h"
#include "Components/BoxComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "enhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Whos_Driving.h"

AWhos_DrivingCharacter::AWhos_DrivingCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	firstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	firstPersonMesh->SetupAttachment(GetMesh());
	firstPersonMesh->SetOnlyOwnerSee(true);
	firstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	firstPersonMesh->SetCollisionProfileName(FName(TEXT("NoCollision")));

	// Create the Camera Component	

	cameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Camera Root"));
	cameraRoot->SetupAttachment(GetRootComponent());
	firstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	firstPersonCameraComponent->SetupAttachment(cameraRoot);
	firstPersonCameraComponent->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	firstPersonCameraComponent->bUsePawnControlRotation = false;
	firstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	firstPersonCameraComponent->bEnableFirstPersonScale = true;
	firstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	firstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
	combatComponent = CreateDefaultSubobject<UGVehicleCombatComponent>(TEXT("Vehicle Combat"));
	combatBody = CreateDefaultSubobject<UBoxComponent>(TEXT("Combat Body"));
	combatHead = CreateDefaultSubobject<UBoxComponent>(TEXT("Combat Head"));
	combatBody->SetupAttachment(GetMesh(), TEXT("spine_03"));
	combatHead->SetupAttachment(GetMesh(), TEXT("head"));
	combatBody->InitBoxExtent(FVector(20.0f, 27.0f, 28.0f));
	combatHead->InitBoxExtent(FVector(13.0f));
	for (UBoxComponent* hitBox : { combatBody.Get(), combatHead.Get() })
	{
		hitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		hitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		hitBox->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
		hitBox->SetGenerateOverlapEvents(false);
	}
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

float AWhos_DrivingCharacter::TakeDamage(float damageAmount, const FDamageEvent& damageEvent,
	AController* eventInstigator, AActor* damageCauser)
{
	return combatComponent->ApplyDamage(damageAmount, eventInstigator);
}

void AWhos_DrivingCharacter::SetupPlayerInputComponent(UInputComponent* playerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* enhancedInputComponent = Cast<UEnhancedInputComponent>(playerInputComponent))
	{
		// Jumping
		enhancedInputComponent->BindAction(jumpAction, ETriggerEvent::Started, this, &AWhos_DrivingCharacter::DoJumpStart);
		enhancedInputComponent->BindAction(jumpAction, ETriggerEvent::Completed, this, &AWhos_DrivingCharacter::DoJumpEnd);

		// Moving
		enhancedInputComponent->BindAction(moveAction, ETriggerEvent::Triggered, this, &AWhos_DrivingCharacter::MoveInput);

		// Looking/Aiming
		enhancedInputComponent->BindAction(lookAction, ETriggerEvent::Triggered, this, &AWhos_DrivingCharacter::LookInput);
		enhancedInputComponent->BindAction(mouseLookAction, ETriggerEvent::Triggered, this, &AWhos_DrivingCharacter::LookInput);
	}
	else
	{
		UE_LOG(LogWhos_Driving, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AWhos_DrivingCharacter::MoveInput(const FInputActionValue& value)
{
	// get the Vector2D move axis
	FVector2D movementVector = value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(movementVector.X, movementVector.Y);

}

void AWhos_DrivingCharacter::LookInput(const FInputActionValue& value)
{
	// get the Vector2D look axis
	FVector2D lookAxisVector = value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(lookAxisVector.X, lookAxisVector.Y);

}

void AWhos_DrivingCharacter::DoAim(float yaw, float pitch)
{
	if (Cast<AGPlayerCar>(GetAttachParentActor()))
	{
		vehicleCameraYaw = FRotator::NormalizeAxis(vehicleCameraYaw + yaw * cameraSensitivity);
		cameraPitch = FMath::Clamp(cameraPitch + pitch * cameraSensitivity, -maxPitch, maxPitch);
		cameraRoot->SetRelativeRotation(FRotator(cameraPitch, vehicleCameraYaw, 0.0f));
		return;
	}
	const FRotator actorRotation = GetActorRotation();
	const FRotator viewRotation(0.0f, actorRotation.Yaw + yaw * cameraSensitivity, 0.0f);
	if (Controller)
	{
		Controller->SetControlRotation(viewRotation);
	}
	SetActorRotation(viewRotation);
	cameraPitch = FMath::Clamp(cameraPitch + pitch * cameraSensitivity, -maxPitch, maxPitch);
	cameraRoot->SetRelativeRotation(FRotator(cameraPitch, 0.0f, 0.0f));
}

void AWhos_DrivingCharacter::ResetVehicleLook()
{
	vehicleCameraYaw = 0.0f;
	cameraPitch = 0.0f;
	cameraRoot->SetRelativeRotation(FRotator::ZeroRotator);
}

void AWhos_DrivingCharacter::DoMove(float right, float forward)
{
	if (Cast<AGPlayerCar>(GetAttachParentActor()))
	{
		return;
	}
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), right);
		AddMovementInput(GetActorForwardVector(), forward);
	}
}

void AWhos_DrivingCharacter::DoJumpStart()
{
	if (!Cast<AGPlayerCar>(GetAttachParentActor()))
	{
		Jump();
	}
}

void AWhos_DrivingCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}
