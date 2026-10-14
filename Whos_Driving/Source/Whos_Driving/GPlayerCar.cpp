// Fill out your copyright notice in the Description page of Project Settings.


#include "GPlayerCar.h"
#include "GVehicleMovementComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Whos_DrivingCharacter.h"
#include "GPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AGPlayerCar::AGPlayerCar(const FObjectInitializer& objectInitializer) : Super(objectInitializer.SetDefaultSubobjectClass<UGVehicleMovementComponent>
	(AWheeledVehiclePawn::VehicleMovementComponentName))
{
	driverSeat = CreateDefaultSubobject<USceneComponent>(TEXT("Driver seat"));
	driverSeat->SetupAttachment(GetRootComponent());

	wheelModules.SetNum(4);

	for (int32 i = 0; i < 4; ++i)
	{
		FName wheelName = FName(FString::FromInt(i));
		UGVehicleWheelModule* wheelModule = objectInitializer.CreateDefaultSubobject<UGVehicleWheelModule>(this, wheelName);
		wheelModule->wheelIndex = i;
		wheelModule->maxHP = 50.0f;
		wheelModule->currentHP = wheelModule->maxHP;
		wheelModules[i] = wheelModule;
	}
}

void AGPlayerCar::DamageWheel(int32 wheelIndex, float damage)
{
	if (!wheelModules.IsValidIndex(wheelIndex))
	{
		return;
	}
	wheelModules[wheelIndex]->ApplyDamage(damage);
}

void AGPlayerCar::OnWheelDestroyed(int32 wheelIndex)
{
	if (UGVehicleMovementComponent* vehicleMovement =
		Cast<UGVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		vehicleMovement->DisableWheel(wheelIndex);
	}
}

void AGPlayerCar::BeginPlay() {
	Super::BeginPlay();

	SpawnDriver();

	UpdatePlayerController();



	//Debug
	//DamageWheel(0, 100.0f);
}

void AGPlayerCar::Tick(float dt)
{
	Super::Tick(dt);

	float forwardSpeedKmh = FVector::DotProduct(GetVelocity(), GetActorForwardVector()) * 0.036f;
	FString message = FString::Printf(TEXT("Speed (KmH): %.1f"), forwardSpeedKmh);
	GEngine->AddOnScreenDebugMessage(0, 1.0f, FColor::Green, message, false);

	if (!bBombActivate)
	{
		return;
	}

	if (forwardSpeedKmh <= bombActivateSpeed)
	{
		StartBombTimer();
		float remainingTime = GetWorldTimerManager().GetTimerRemaining(bombTimerHandle);
		FString bombMessage = FString::Printf(TEXT("Bomb: %.1f"), remainingTime);
		GEngine->AddOnScreenDebugMessage(2, 1.0f, FColor::Red, bombMessage, false);
	}
	else
	{
		StopBombTimer();
	}
}

void AGPlayerCar::StartBombTimer()
{
	if (!bBombTimerStarted)
	{
		bBombTimerStarted = true;

		GetWorldTimerManager().SetTimer(
			bombTimerHandle,
			this,
			&AGPlayerCar::Explode,
			bombDelay,
			false
		);
	}
}

void AGPlayerCar::StopBombTimer()
{
	if (bBombTimerStarted)
	{
		bBombTimerStarted = false;

		GetWorldTimerManager().ClearTimer(bombTimerHandle);
	}
}

void AGPlayerCar::SpawnDriver() {
	if (!driverCharacterClass)
	{
		UE_LOG(LogTemp, Error, TEXT("driverCharacterClass is null"));
		return;
	}

	if (!driverSeat)
	{
		UE_LOG(LogTemp, Error, TEXT("DriverSeat is NULL"));
		return;
	}

	FActorSpawnParameters spawnParams;
	spawnParams.Owner = this;
	spawnParams.Instigator = this;

	const FTransform driverTransform = driverSeat->GetComponentTransform();
	driverCharacter = GetWorld()->SpawnActor<AWhos_DrivingCharacter>(driverCharacterClass, driverTransform, spawnParams);

	if (driverCharacter)
	{
		driverCharacter->AttachToComponent(
			driverSeat,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale
		);

		driverCharacter->GetCharacterMovement()->DisableMovement();
		driverCharacter->GetCharacterMovement()->GravityScale = 0.0f;

		driverCharacter->GetCapsuleComponent()->SetCollisionEnabled(
			ECollisionEnabled::NoCollision
		);

		driverCharacter->GetMesh()->SetCollisionEnabled(
			ECollisionEnabled::NoCollision
		);
	}
	AGPlayerController* playerController =
		Cast<AGPlayerController>(GetController());

	if (!playerController)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("AGPlayerCar: AGPlayerController is NULL")
		);

		return;
	}

	playerController->SetDriverCharacter(driverCharacter);
}

void AGPlayerCar::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("AGPlayerCar: PossessedBy %s"),
		*GetNameSafe(NewController)


	);

	UpdatePlayerController();

	if (AGPlayerController* PlayerController = Cast<AGPlayerController>(NewController))
	{
		PlayerController->SetDriverCharacter(driverCharacter);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("AGPlayerCar: Controller is not AGPlayerController")
		);
	}
}

void AGPlayerCar::UpdatePlayerController()
{
	if (!driverCharacter)
	{
		return;
	}

	if (AGPlayerController* PlayerController = Cast<AGPlayerController>(GetController()))
	{
		PlayerController->SetDriverCharacter(driverCharacter);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("AGPlayerCar: Driver assigned to PlayerController: %s"),
			*GetNameSafe(driverCharacter)
		);
	}
}

void AGPlayerCar::Explode()
{
	Destroy();
}

void AGPlayerCar::CheckBomb()
{

}

void AGPlayerCar::SwitchBomb(const FInputActionValue& value)
{
	bBombActivate = !bBombActivate;
	FString message = "Bomb is now: ";
	message += FString::FromInt(bBombActivate);
	GEngine->AddOnScreenDebugMessage(1, 2.0f, FColor::Purple, message, false);
}

void AGPlayerCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* enhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!enhancedInputComponent) 
	{
		return;
	}

	if (throttleAction) 
	{
		enhancedInputComponent->BindAction(throttleAction.Get(), ETriggerEvent::Triggered, this, &AGPlayerCar::InputThrottle);
		enhancedInputComponent->BindAction(throttleAction.Get(), ETriggerEvent::Completed, this, &AGPlayerCar::InputThrottleReleased);
	}

	if (steeringAction) 
	{
		enhancedInputComponent->BindAction(steeringAction.Get(), ETriggerEvent::Triggered, this, &AGPlayerCar::InputSteering);
		enhancedInputComponent->BindAction(steeringAction.Get(), ETriggerEvent::Completed, this, &AGPlayerCar::InputSteeringReleased);
	}

	if (brakeAction) 
	{
		enhancedInputComponent->BindAction(brakeAction.Get(), ETriggerEvent::Triggered, this, &AGPlayerCar::InputBrake);
		enhancedInputComponent->BindAction(brakeAction.Get(), ETriggerEvent::Completed, this, &AGPlayerCar::InputBrakeReleased);
	}
}

void AGPlayerCar::InputThrottle(const FInputActionValue& value) 
{
	const float throttle = value.Get<float>();
	GetVehicleMovementComponent()->SetThrottleInput(throttle);
}

void AGPlayerCar::InputThrottleReleased(const FInputActionValue& value) {
	GetVehicleMovementComponent()->SetThrottleInput(0.0f);
}

void AGPlayerCar::InputSteering(const FInputActionValue& value) 
{
	const float steering = value.Get<float>();
	GetVehicleMovementComponent()->SetSteeringInput(steering);
}

void AGPlayerCar::InputSteeringReleased(const FInputActionValue& value) {
	GetVehicleMovementComponent()->SetSteeringInput(0.0f);
}

void AGPlayerCar::InputBrake(const FInputActionValue& value) 
{
	const float brake = value.Get<float>();
	GetVehicleMovementComponent()->SetBrakeInput(brake);
}

void AGPlayerCar::InputBrakeReleased(const FInputActionValue& value)
{
	GetVehicleMovementComponent()->SetBrakeInput(0.0f);
}
