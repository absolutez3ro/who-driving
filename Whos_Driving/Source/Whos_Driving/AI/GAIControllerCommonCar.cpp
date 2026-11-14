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
#include "CollisionQueryParams.h"
#include "Common/Whos_DrivingCharacter.h"
#include "Vehicle/Components/GVehicleCombatComponent.h"

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
		followPlayer = commonCarData->followPlayer;
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
		playerSpeedThershhold = commonCarData->playerSpeedThershhold;
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
	AActor* foundCar = UGameplayStatics::GetActorOfClass(GetWorld(), AGPlayerCar::StaticClass());

	// 3. Cast to your specific type
	AGPlayerCar* carReference = Cast<AGPlayerCar>(foundCar);

	if (carReference)
	{
		playerCarRef = carReference;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Found Actor: %s"), *carReference->GetName());
	}

	playerCharacters.Empty();
	TArray<AActor*> foundPlayers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWhos_DrivingCharacter::StaticClass(), foundPlayers);
	for (AActor* actor : foundPlayers)
	{
		if (AWhos_DrivingCharacter* character =
			Cast<AWhos_DrivingCharacter>(actor))
		{
			playerCharacters.Add(character);
		}
	}
}

void AGAIControllerCommonCar::Tick(float deltaSeconds)
{
	DriveAlongSpline(deltaSeconds);
	//Chase(deltaSeconds);
}

bool AGAIControllerCommonCar::TraceShot()
{
	AWheeledVehiclePawn* Vehicle = Cast<AWheeledVehiclePawn>(GetPawn());
	const FVector vehicleLoc = Vehicle->GetActorLocation()+ FVector(0.f, 0.f, 100.f);
	const FVector playerLoc = playerCarRef->GetActorLocation()+ FVector(0.f, 0.f, 100.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PawnLOS), /*bTraceComplex*/ false);
	Params.AddIgnoredActor(Vehicle);
	Params.AddIgnoredActor(playerCarRef);
	FHitResult OutHit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, vehicleLoc, playerLoc, ECC_Visibility, Params);
	if (bHit) 
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
			FString::Printf(TEXT("Blocked by %s"), *GetNameSafe(OutHit.GetActor())));
	}
	return bHit;
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
	UE_LOG(LogTemp, Log, TEXT("Trying to shoot"));

	TArray<AActor*> availableTargets;

	if (IsValid(playerCarRef))
	{
		availableTargets.Add(playerCarRef);
	}

	for (int32 i = playerCharacters.Num() - 1; i >= 0; --i)
	{
		AWhos_DrivingCharacter* character = playerCharacters[i].Get();

		if (!IsValid(character))
		{
			playerCharacters.RemoveAt(i);
			continue;
		}

		availableTargets.Add(character);
	}

	TArray<AActor*> visibleTargets;

	const FVector enemyLocation = GetPawn()->GetActorLocation();

	for (AActor* target : availableTargets)
	{
		if (!IsValid(target))
		{
			continue;
		}

		const float distanceSquared = FVector::DistSquared(
			enemyLocation,
			target->GetActorLocation()
		);

		if (distanceSquared > FMath::Square(hitRange))
		{
			continue;
		}
		visibleTargets.Add(target);
	}

	if (visibleTargets.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("No visible player targets"));
		StartShootDelay();
		return;
	}

	AActor* targetActor = visibleTargets[
		FMath::RandRange(0, visibleTargets.Num() - 1)
	];

	if (FMath::RandRange(0.0f, 1.0f) > hitChance)
	{
		UE_LOG(LogTemp, Log, TEXT("Missed"));

		if (AGNewCommoncar* enemyCar = Cast<AGNewCommoncar>(GetPawn()))
		{
			enemyCar->PlayMissSound_Implementation();
		}

		StartShootDelay();
		return;
	}

	const float damage = FMath::RandRange(minDamage, maxDamage);

	if (AGPlayerCar* targetCar = Cast<AGPlayerCar>(targetActor))
	{
		TArray<FName> shootableModuleNames[6];
		TArray<int32> shootableGroupIndices;

		for (int32 i = 0; i < 6; i++)
		{
			float overallHp = 0.f;

			for (int32 j = 0; j < moduleNames[i].Num(); j++)
			{
				const float moduleHp =
					targetCar->GetModuleDamageState(
						moduleNames[i][j]
					).currentHP;

				overallHp += moduleHp;

				if (moduleHp > 0.f)
				{
					shootableModuleNames[i].Add(moduleNames[i][j]);
				}
			}

			if (overallHp > 0.f)
			{
				shootableGroupIndices.Add(i);
			}
		}

		shootableGroupIndices.Add(5);

		float totalWeight = 0.f;

		for (int32 groupIndex : shootableGroupIndices)
		{
			totalWeight += weights[groupIndex];
		}

		if (totalWeight <= 0.f)
		{
			StartShootDelay();
			return;
		}

		const float randomWeight = FMath::RandRange(0.f, totalWeight);
		float accumulatedWeight = 0.f;
		int32 targetGroupIndex = -1;

		for (int32 groupIndex : shootableGroupIndices)
		{
			accumulatedWeight += weights[groupIndex];

			if (randomWeight <= accumulatedWeight)
			{
				targetGroupIndex = groupIndex;
				break;
			}
		}

		if (targetGroupIndex == 5)
		{
			targetCar->TakeDamage(
				damage,
				FDamageEvent(),
				nullptr,
				this
			);

			UE_LOG(LogTemp, Log, TEXT("Hit car main HP: %f"), damage);
		}
		else if (targetGroupIndex >= 0 &&
			shootableModuleNames[targetGroupIndex].Num() > 0)
		{
			const TArray<FName>& modules =
				shootableModuleNames[targetGroupIndex];

			const FName targetedModule = modules[
				FMath::RandRange(0, modules.Num() - 1)
			];

			targetCar->DamageModule(targetedModule, damage);

			UE_LOG(
				LogTemp,
				Log,
				TEXT("Hit module %s, damage: %f"),
				*targetedModule.ToString(),
				damage
			);
		}
	}

	else if (AWhos_DrivingCharacter* targetCharacter =
		Cast<AWhos_DrivingCharacter>(targetActor))
	{
		targetCharacter->GetCombatComponent()->ApplyDamage(damage, targetCharacter->GetController());
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Blue, TEXT("ENEMY SHOOTING AT PLAYER"));
		UE_LOG(
			LogTemp,
			Log,
			TEXT("Hit player character: %s, damage: %f"),
			*GetNameSafe(targetCharacter),
			damage
		);
	}

	if (AGNewCommoncar* enemyCar = Cast<AGNewCommoncar>(GetPawn()))
	{
		enemyCar->PlayShotSound_Implementation();
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
	bool aheadOfStoppedPlayer = (currentDist - playerDist > 0.0f) && (playerMovement->GetForwardSpeed() <= playerSpeedThershhold);
	// 2. End of an open path: or ahead of a stopped player - stop
	if (endOfPath || (aheadOfStoppedPlayer && followPlayer))
	{
		Movement->SetThrottleInput(0.f);
		Movement->SetSteeringInput(0.f);
		Movement->SetBrakeInput(1.f);
		Movement->SetHandbrakeInput(Speed < 50.f); // hold it once nearly stopped
		UE_LOG(LogTemp, Error, TEXT("CAR STOPPED"));
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
	if (playerSpeed <= 0 || !followPlayer) {
		targetSpeed = maxSpeed;
	}else
	{
		targetSpeed = FMath::Min(playerSpeed, maxSpeed);
		targetSpeed = FMath::Max(playerSpeed, 0.0f);

	}
	// 5. speed control
	// calculating catch up multiplier
	float catchUp = 0;
	if ((playerDist - currentDist) <= 1.f || !followPlayer) {
		catchUp = 1.0f;
	}
	else
	{
		catchUp = catchUpModificator * (playerDist - currentDist);
	}
	const float DesiredSpeed = targetSpeed * (1.f - cornerSlowdown * FMath::Abs(currentSteer)) *catchUp;
	const float SpeedError = DesiredSpeed - Speed;
	//UE_LOG(LogTemp, Error, TEXT("catchUp %f"), catchUp);
	//UE_LOG(LogTemp, Error, TEXT("SpeedError %f"), SpeedError);
	//UE_LOG(LogTemp, Error, TEXT("playerMovement->GetForwardSpeed() %f"), playerMovement->GetForwardSpeed());
	if (SpeedError >= 0.f)
	{
		Movement->SetThrottleInput(FMath::Clamp(SpeedError * throttleGain, 0.f, 1.f));
		Movement->SetBrakeInput(0.f);
	}
	else
	{
		Movement->SetThrottleInput(0.f);
		Movement->SetBrakeInput(FMath::Clamp(-SpeedError * brakeGain, 0.f, 1.f));
	}
}