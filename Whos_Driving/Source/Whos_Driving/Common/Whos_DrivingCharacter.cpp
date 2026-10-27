// Copyright Epic Games, Inc. All Rights Reserved.

#include "Whos_DrivingCharacter.h"
#include "GPlayerCar.h"
#include "G_Vehicle_Combat_Component.h"
#include "Components/BoxComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Whos_Driving.h"

AWhos_DrivingCharacter::AWhos_DrivingCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	

	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Camera Root"));
	CameraRoot->SetupAttachment(GetRootComponent());
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(CameraRoot);
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	FirstPersonCameraComponent->bUsePawnControlRotation = false;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

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

void AWhos_DrivingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AWhos_DrivingCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AWhos_DrivingCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWhos_DrivingCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWhos_DrivingCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AWhos_DrivingCharacter::LookInput);
	}
	else
	{
		UE_LOG(LogWhos_Driving, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AWhos_DrivingCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AWhos_DrivingCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AWhos_DrivingCharacter::DoAim(float Yaw, float Pitch)
{
	if (Cast<AGPlayerCar>(GetAttachParentActor()))
	{
		vehicleCameraYaw = FRotator::NormalizeAxis(vehicleCameraYaw + Yaw * CameraSensitivity);
		CameraPitch = FMath::Clamp(CameraPitch + Pitch * CameraSensitivity, -MaxPitch, MaxPitch);
		CameraRoot->SetRelativeRotation(FRotator(CameraPitch, vehicleCameraYaw, 0.0f));
		return;
	}
	const FRotator actorRotation = GetActorRotation();
	const FRotator viewRotation(0.0f, actorRotation.Yaw + Yaw * CameraSensitivity, 0.0f);
	if (Controller)
	{
		Controller->SetControlRotation(viewRotation);
	}
	SetActorRotation(viewRotation);
	CameraPitch = FMath::Clamp(CameraPitch + Pitch * CameraSensitivity, -MaxPitch, MaxPitch);
	CameraRoot->SetRelativeRotation(FRotator(CameraPitch, 0.0f, 0.0f));
}

void AWhos_DrivingCharacter::ResetVehicleLook()
{
	vehicleCameraYaw = 0.0f;
	CameraPitch = 0.0f;
	CameraRoot->SetRelativeRotation(FRotator::ZeroRotator);
}

void AWhos_DrivingCharacter::DoMove(float Right, float Forward)
{
	if (Cast<AGPlayerCar>(GetAttachParentActor()))
	{
		return;
	}
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
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
