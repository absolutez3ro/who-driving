// Fill out your copyright notice in the Description page of Project Settings.

#include "GAIControllerCommonCar.h"
#include "GBaseEnemyCar.h"
#include "GCommonEnemyDataAsset.h"
#include "GPlayerCar.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"

void AGAIControllerCommonCar::ApplyAcceleration(float distance, FVector playerForward)
{
	APawn* selfActor = GetPawn();
	if (!selfActor)
	{
		return;
	}
	UPrimitiveComponent* rootPrimitive = Cast<UPrimitiveComponent>(selfActor->GetRootComponent());
	if (rootPrimitive && rootPrimitive->IsSimulatingPhysics())
	{
		FVector currentVelocity = rootPrimitive->GetPhysicsLinearVelocity();
		const bool bReverseMovement = FVector::DotProduct(currentVelocity, playerForward) <= 0;
		//UE_LOG(LogTemp, Warning, TEXT("bReverseMovement is: %d"), bReverseMovement);
		if (distance < 0 && bReverseMovement)
		{
			//UE_LOG(LogTemp, Error, TEXT("velocity set to 0"));
			currentVelocity = FVector(0, 0, currentVelocity.Z);
			rootPrimitive->SetPhysicsLinearVelocity(currentVelocity);
		}
		else
		{
			// GetMass() returns the mass in kg (calculated by the engine or overridden)
			const float mass = rootPrimitive->GetMass();
			const FVector forceToApply = mass * acceleration;
			rootPrimitive->AddForce(forceToApply);
			currentVelocity.Z = 0;
			if (currentVelocity.Length() > 50)
			{
				const FRotator newRotation = FRotationMatrix::MakeFromX(currentVelocity).Rotator();
				selfActor->SetActorRotation(newRotation, ETeleportType::TeleportPhysics);
			}
		}
		//UE_LOG(LogTemp, Warning, TEXT("velocity is: %f %f %f"), currentVelocity.X, currentVelocity.Y, currentVelocity.Z);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("rootPrimitive(%d) or IsSimulatingPhysics (if not root primitive it is this) false!"), rootPrimitive);
	}
}

void AGAIControllerCommonCar::BeginPlay()
{
	Super::BeginPlay();
	// It is more memory efficient to copy them, surprisingly.
	minAcceleration = commonCarData->MinAcceleration;
	maxAcceleration = commonCarData->MaxAcceleration;
	baseAcceleration = commonCarData->AccelerationModifier;
	minYDistance = commonCarData->MinYDistanceCM;
	maxYDistance = commonCarData->MaxYDistanceCM;
	fireRate = commonCarData->FireRate;
	hitChance = commonCarData->HitChance;
	minDamage = commonCarData->MinDamage;
	maxDamage = commonCarData->MaxDamage;
	weights.Add(commonCarData->WheelWeight);
	weights.Add(commonCarData->DoorWeight);
	weights.Add(commonCarData->BaseHealthWeight);
	weights.Add(commonCarData->SteeringWeight);
	weights.Add(commonCarData->EngineWeight);
	weights.Add(commonCarData->ElectronicsWeight);
	// Must match the module ids created in AGPlayerCar; door ids are also the BP_GPlayerCar component names.
	moduleNames[0].Add(FName(TEXT("Wheel_0")));
	moduleNames[0].Add(FName(TEXT("Wheel_1")));
	moduleNames[0].Add(FName(TEXT("Wheel_2")));
	moduleNames[0].Add(FName(TEXT("Wheel_3")));
	moduleNames[1].Add(FName(TEXT("partDoor0")));
	moduleNames[1].Add(FName(TEXT("partDoor1")));
	moduleNames[2].Add(FName(TEXT("Steering_Wheel")));
	moduleNames[3].Add(FName(TEXT("Engine")));
	moduleNames[4].Add(FName(TEXT("Electrician")));
	yDistance = FMath::FRandRange(minYDistance, maxYDistance);
	hitSound = commonCarData->HitSound;
	missSound = commonCarData->MissSound;

	SetPlayerRef();
	StartShootDelay();
}

void AGAIControllerCommonCar::SetPlayerRef()
{
	AActor* foundActor = UGameplayStatics::GetActorOfClass(GetWorld(), AGPlayerCar::StaticClass());

	// 3. Cast to your specific type
	AGPlayerCar* carReference = Cast<AGPlayerCar>(foundActor);

	if (carReference)
	{
		playerCarRef = carReference;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Found Actor: %s"), *carReference->GetName());
	}
}

void AGAIControllerCommonCar::Chase(float deltaSeconds)
{
	if (!playerCarRef)
	{
		SetPlayerRef();
	}
	APawn* selfActor = GetPawn();
	// get coordinates to scale speed
	const FVector playerPosition = playerCarRef->GetActorLocation();
	const FVector selfPosition = selfActor->GetActorLocation();
	const FVector playerForward = playerCarRef->GetActorForwardVector();
	const FVector toPlayerVector = playerPosition - selfPosition;
	const float distance = FVector::DotProduct(toPlayerVector, playerForward);
	CalculateAcceleration(toPlayerVector, playerForward, deltaSeconds);
	ApplyAcceleration(distance, playerForward);
}

void AGAIControllerCommonCar::CalculateAcceleration(FVector toPlayerVector, FVector playerForward, float deltaSeconds)
{
	FVector playerLeft = FVector3d(-playerForward.Y, playerForward.X, playerForward.Z);
	const float distance = FVector::DotProduct(toPlayerVector, playerForward);
	float accelerationModifier = baseAcceleration * pow(abs(distance), 3);
	playerLeft.Normalize();
	playerLeft *= yDistance;
	acceleration = playerLeft + toPlayerVector;
	acceleration.Normalize();
	accelerationModifier = FMath::Clamp(accelerationModifier, minAcceleration, maxAcceleration);
	acceleration *= accelerationModifier;
	//UE_LOG(LogTemp, Warning, TEXT("acceleration is: %f %f %f"), acceleration.X, acceleration.Y, acceleration.Z);
	//UE_LOG(LogTemp, Warning, TEXT("accelerationModifier is: %f"), accelerationModifier);
	//UE_LOG(LogTemp, Warning, TEXT("distance is: %f"), distance);
}

void AGAIControllerCommonCar::Tick(float deltaSeconds)
{
	Chase(deltaSeconds);
}

void AGAIControllerCommonCar::StartShootDelay()
{
	// Define your range, e.g., between 0.5s and 1.5s
	const float randomDelay = FMath::RandRange(0.0f, 3.0f) + fireRate;
	UE_LOG(LogTemp, Log, TEXT("shot timer started"));
	GetWorldTimerManager().SetTimer(shootDelayHandle, this, &AGAIControllerCommonCar::Shoot, randomDelay, false);
}

void AGAIControllerCommonCar::Shoot_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("trying to shoot"));

	if (FMath::RandRange(0.0f, 1.0f) <= hitChance)
	{
		TArray<FName> shootableModuleNames[6];
		TArray<int32> shootableGroupIndices;
		const float damage = FMath::RandRange(minDamage, maxDamage);
		for (int32 i = 0; i < 6; i++)
		{
			float overallHp = 0;
			for (int32 j = 0; j < moduleNames[i].Num(); j++)
			{
				const float moduleHp = playerCarRef->GetModuleDamageState(moduleNames[i][j]).currentHP;
				overallHp += moduleHp;
				if (moduleHp > 0)
				{
					shootableModuleNames[i].Add(moduleNames[i][j]);
				}
			}
			if (overallHp > 0)
			{
				shootableGroupIndices.Add(i);
			}
		}
		// Add 5 manually because "base hp" is not a module.
		shootableGroupIndices.Add(5);

		float totalWeight = 0;
		for (int32 i = 0; i < shootableGroupIndices.Num(); i++)
		{
			totalWeight += weights[shootableGroupIndices[i]];
		}
		float accumulatedWeight = 0;
		const float randomWeight = FMath::RandRange(0.0f, totalWeight);
		int32 targetGroupIndex = -1;

		for (int32 i = 0; i < shootableGroupIndices.Num(); i++)
		{
			accumulatedWeight += weights[shootableGroupIndices[i]];
			if (randomWeight <= accumulatedWeight)
			{
				targetGroupIndex = shootableGroupIndices[i];
				break;
			}
		}

		if (targetGroupIndex >= 0)
		{
			if (targetGroupIndex == 5)
			{
				playerCarRef->TakeDamage(damage, FDamageEvent(), nullptr, this);
				UE_LOG(LogTemp, Log, TEXT("hit main hp %f"), damage);
			}
			else
			{
				const int32 lastModuleIndex = moduleNames[targetGroupIndex].Num() - 1;
				const FName targetedModule = moduleNames[targetGroupIndex][FMath::RandRange(0, lastModuleIndex)];
				playerCarRef->DamageModule(targetedModule, damage);
				UE_LOG(LogTemp, Log, TEXT("hit part %s damage: %f"), *targetedModule.ToString(), damage);
			}
			AGBaseEnemyCar* enemyCar = Cast<AGBaseEnemyCar>(GetPawn());
			enemyCar->PlayShotSound_Implementation();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("no part of car got shot for some reason"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("missed"));
		AGBaseEnemyCar* enemyCar = Cast<AGBaseEnemyCar>(GetPawn());
		enemyCar->PlayMissSound_Implementation();
	}
	StartShootDelay();
}
