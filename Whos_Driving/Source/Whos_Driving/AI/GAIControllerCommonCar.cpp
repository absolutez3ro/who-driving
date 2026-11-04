// Fill out your copyright notice in the Description page of Project Settings.

#include "GAIControllerCommonCar.h"
#include "GNewCommoncar.h"
#include "GCommonEnemyDataAsset.h"
#include "GPlayerCar.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineComponent.h"
#include "ChaosVehicleMovementComponent.h"
#include "AI/AGSplineActor.h"
void AGAIControllerCommonCar::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("YES ROAD SPLINE ON MAP"));
	splineOwnerRef = Cast<AAGSplineActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AAGSplineActor::StaticClass()));
	if (splineOwnerRef)
	{
		splineComponent = splineOwnerRef->FindComponentByClass<USplineComponent>();
		if (!splineComponent) {
			UE_LOG(LogTemp, Error, TEXT("NO SPLINE FOUND IN ROAD SPLINE"));
		}
		else {
			UE_LOG(LogTemp, Log, TEXT("SPLINE FOUND IN ROAD SPLINE"));
		}
	}else
	{
		UE_LOG(LogTemp, Error, TEXT("NO ROAD SPLINE ON MAP"));
	}
	// It is more memory efficient to copy them, surprisingly.
	if (commonCarData)
		{
		//movement
		lookAheadDistance = commonCarData->lookAheadDistance;
		lookAheadTime = commonCarData->lookAheadTime;
		maxSpeed = commonCarData->maxSpeed;
		maxSteerAngle = commonCarData->maxSteerAngle;
		steerInterpSpeed = commonCarData->steerInterpSpeed;
		cornerSlowdown = commonCarData->cornerSlowdown;
		throttleGain = commonCarData->throttleGain;
		brakeGain = commonCarData->brakeGain;
		laneOffset = commonCarData->laneOffset;
		stopDistance = commonCarData->stopDistance;
		roadWidth = commonCarData->roadWidth;
		catchUpModificator = commonCarData->catchUpModificator;
		//combat
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
		hitSound = commonCarData->HitSound;
		missSound = commonCarData->MissSound;
	}
	else 
	{
		UE_LOG(LogTemp, Error, TEXT("NO COMMON CAR DATA ASSET PROVIDED"));
	}
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

void AGAIControllerCommonCar::Tick(float deltaSeconds)
{
	DriveAlongSpline(deltaSeconds);
	//Chase(deltaSeconds);
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
	FVector toPlayer = playerCarRef->GetActorLocation() - GetPawn()->GetActorLocation();
	float distance = toPlayer.Length();
	if (FMath::RandRange(0.0f, 1.0f) <= hitChance && distance<=hitRange)
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
			AGNewCommoncar* enemyCar = Cast<AGNewCommoncar>(GetPawn());
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
		AGNewCommoncar* enemyCar = Cast<AGNewCommoncar>(GetPawn());
		enemyCar->PlayMissSound_Implementation();
	}
	StartShootDelay();
}

void AGAIControllerCommonCar::DriveAlongSpline(float DeltaSeconds)
{
	AWheeledVehiclePawn* Vehicle = Cast<AWheeledVehiclePawn>(GetPawn());
	if (!Vehicle || !splineComponent)
	{
		UE_LOG(LogTemp, Log, TEXT("NO VEHICLE OR SPLINE"));
		return;
	}

	UChaosVehicleMovementComponent* Movement = Vehicle->GetVehicleMovementComponent();

	if (!Movement)
	{
		UE_LOG(LogTemp, Error, TEXT("NO Movement component"));
		return;
	}
	UChaosVehicleMovementComponent* playerMovement = playerCarRef->GetVehicleMovementComponent();

	if (!playerMovement)
	{
		UE_LOG(LogTemp, Error, TEXT("NO player Movement component"));
		return;
	}
	const FVector vehicleLoc = Vehicle->GetActorLocation();
	const float   SplineLength = splineComponent->GetSplineLength();
	const bool    bLoop = splineComponent->IsClosedLoop();
	const float   Speed = Movement->GetForwardSpeed(); // cm/s, negative when reversing

	//0. are we off the road?
	FVector ClosestPoint = splineComponent->FindLocationClosestToWorldLocation(vehicleLoc, ESplineCoordinateSpace::World);
	float DistanceToSpline = FVector::Dist(vehicleLoc, ClosestPoint);
	bool offTheRoad = DistanceToSpline > roadWidth;

	// 1. Where are we on the spline?
	float inputKey = splineComponent->FindInputKeyClosestToWorldLocation(vehicleLoc);
	const float currentDist = splineComponent->GetDistanceAlongSplineAtSplineInputKey(inputKey);

	bool endOfPath = !bLoop && currentDist >= SplineLength - stopDistance;
	const FVector playerLoc = playerCarRef->GetActorLocation();
	// Where player on the spline?
	inputKey = splineComponent->FindInputKeyClosestToWorldLocation(playerLoc);
	const float playerDist = splineComponent->GetDistanceAlongSplineAtSplineInputKey(inputKey);
	const float playerSpeed = playerMovement->GetForwardSpeed();
	bool aheadOfStoppedPlayer = (currentDist - playerDist > 0.0f) && (playerMovement->GetForwardSpeed() <=0);
	// 2. End of an open path: or ahead of a stopped player - stop
	if (endOfPath)// || aheadOfStoppedPlayer)
	{
		Movement->SetThrottleInput(0.f);
		Movement->SetSteeringInput(0.f);
		Movement->SetBrakeInput(1.f);
		Movement->SetHandbrakeInput(Speed < 50.f); // hold it once nearly stopped
		return;
	}
	Movement->SetHandbrakeInput(false);

	
	FVector selfForward = Vehicle->GetActorForwardVector();
	selfForward = FVector::VectorPlaneProject(selfForward, FVector::UpVector).GetSafeNormal();
	FVector desiredDirection;
	float TargetDist = currentDist + lookAheadDistance + FMath::Max(Speed, 0.f) * lookAheadTime;
	if (offTheRoad) {
		FVector returnTo = splineComponent->GetLocationAtDistanceAlongSpline(TargetDist, ESplineCoordinateSpace::World);
		FVector toSpline = returnTo - vehicleLoc;
		desiredDirection = FVector::VectorPlaneProject(toSpline, FVector::UpVector).GetSafeNormal();
		toSpline = toSpline.GetSafeNormal();
	}
	else
	{
		// 3. Pick a target point ahead of us (further ahead at higher speed)
		TargetDist = bLoop ? FMath::Fmod(TargetDist, SplineLength)
			: FMath::Min(TargetDist, SplineLength);
		FVector splineForward = splineComponent->GetDirectionAtDistanceAlongSpline(TargetDist, ESplineCoordinateSpace::World);
		desiredDirection = FVector::VectorPlaneProject(splineForward, FVector::UpVector).GetSafeNormal();

	}
	
	const float Dot = FMath::Clamp(FVector::DotProduct(selfForward, desiredDirection),	-1.f,1.f);
	const float CrossZ = FVector::CrossProduct(selfForward, desiredDirection).Z;
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));
	
	const float DesiredSteer = FMath::Clamp(AngleDeg/ maxSteerAngle, -1.0f, 1.0f);

	currentSteer = FMath::FInterpTo(currentSteer, DesiredSteer, DeltaSeconds, steerInterpSpeed);
	UE_LOG(LogTemp, Log, TEXT("GetSteeringInput %f"), Movement->GetSteeringInput());
	Movement->SetSteeringInput(DesiredSteer);
	//if chosing a speed to match
	float targetSpeed = 0;
	if (playerSpeed <= 0) {
		targetSpeed = maxSpeed;
	}else
	{
		targetSpeed = FMath::Min(playerSpeed, maxSpeed);
	}
	// 5. speed control
	targetSpeed = maxSpeed;
	// calculating catch up multiplier
	float catchUp = 0;
	if ((playerDist - currentDist) <= 1.f) {
		catchUp = 1.0f;
	}
	else
	{
		catchUp = catchUpModificator * (playerDist - currentDist);
	}
	const float DesiredSpeed = targetSpeed * (1.f - cornerSlowdown * FMath::Abs(currentSteer));// *catchUp;
	const float SpeedError = DesiredSpeed - Speed;

	if (SpeedError >= 0.f)
	{
		Movement->SetThrottleInput(FMath::Clamp(maxSpeed * throttleGain, 0.f, 1.f));
		Movement->SetBrakeInput(0.f);
	}
	else
	{
		Movement->SetThrottleInput(0.f);
		Movement->SetBrakeInput(FMath::Clamp(-SpeedError * brakeGain, 0.f, 1.f));
	}
}