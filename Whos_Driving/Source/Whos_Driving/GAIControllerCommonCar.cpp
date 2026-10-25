// Fill out your copyright notice in the Description page of Project Settings.


#include "GAIControllerCommonCar.h"
#include "GCommonEnemyDataAsset.h"

void AGAIControllerCommonCar::ApplyAcceleration(float distance, FVector playerForward)
{
    APawn* selfActor = GetPawn();
    if (!selfActor) return;
    UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(selfActor->GetRootComponent());
    if (RootPrim && RootPrim->IsSimulatingPhysics())
    {
        // GetMass() returns the mass in kg (calculated by the engine or overridden)
        float Mass = RootPrim->GetMass();
        FVector ForceToApply = Mass * acceleration;
        RootPrim->AddForce(ForceToApply);
        FVector currentVelocity= RootPrim->GetPhysicsLinearVelocity();
        //UE_LOG(LogTemp, Warning, TEXT("velocity was: %f"), currentVelocityX);
        bool reverseMovement = FVector::DotProduct(currentVelocity, playerForward)<=0;
        UE_LOG(LogTemp, Warning, TEXT("reverseMovement is: %d"), reverseMovement);
        if(distance <0 and reverseMovement)
        {
            UE_LOG(LogTemp, Error, TEXT("velocity set to 0"));
            currentVelocity = FVector(0, 0, 0);
        }
        RootPrim->SetPhysicsLinearVelocity(currentVelocity);
        UE_LOG(LogTemp, Warning, TEXT("velocity is: %f %f %f"), currentVelocity.X, currentVelocity.Y, currentVelocity.Z);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RootPrim(%d) or IsSimulatingPhysics(%d) false!"), RootPrim, RootPrim->IsSimulatingPhysics());
    }

}

void AGAIControllerCommonCar::BeginPlay() {
    Super::BeginPlay();
    //its more memory efficient to copy them, surprisingly
    minAcceleration = commonCarData->minAcceleration;
    maxAcceleration = commonCarData->maxAcceleration;
    baseAcceleration = commonCarData->accelerationModifier;
    minYDistance = commonCarData->minYDistance;
    maxYDistance = commonCarData->maxYDistance;
    fireRate = commonCarData->fireRate;
    hitChance = commonCarData->hitChance;
    minDamage = commonCarData->minDamage;
    maxDamage = commonCarData->maxDamage;
    weights.Add(commonCarData->wheelWeight);
    totalWeight += commonCarData->wheelWeight;
    weights.Add(commonCarData->doorWeight);
    totalWeight += commonCarData->doorWeight;
    weights.Add(commonCarData->baseHealthWeight);
    totalWeight += commonCarData->baseHealthWeight;
    weights.Add(commonCarData->steeringWeight);
    totalWeight += commonCarData->steeringWeight;
    weights.Add(commonCarData->engineWeight);
    totalWeight += commonCarData->engineWeight;
    weights.Add(commonCarData->electronicsWeight);
    totalWeight += commonCarData->electronicsWeight;
    YDistance = FMath::FRandRange(minYDistance, maxYDistance);
    StartShootDelay();
}

void AGAIControllerCommonCar::Chase(AActor* carReference, float deltaSeconds)
{
    if (carReference)
    {
        APawn* selfActor = GetPawn();
        // get coordinates to scale speed
        FVector playerPos = carReference->GetActorLocation();
        FVector selfPos = selfActor->GetActorLocation();
        FVector playerForward = carReference->GetActorForwardVector();
        FVector toPlayerVector = playerPos - selfPos;
        float distance = FVector::DotProduct(toPlayerVector, playerForward);
        toPlayerVector.Normalize();
        CalculateAcceleration(toPlayerVector, playerForward, deltaSeconds);
        //ApplyAcceleration(distance, playerForward);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InteractWithActor called with a null carReference!"));
    }
}

void AGAIControllerCommonCar::CalculateAcceleration(FVector toPlayerVector, FVector playerForward, float deltaSeconds)
{
    FVector playerLeft = FVector3d(-playerForward.Y, playerForward.X, playerForward.Z);
    float distance= FVector::DotProduct(toPlayerVector,playerForward);
    float accelerationMod= baseAcceleration * pow(distance, 3);
    playerLeft.Normalize();
    playerLeft *= YDistance;
    acceleration = playerLeft + toPlayerVector;
    float accelarationLength = acceleration.Length();
    acceleration.Normalize();
    accelerationMod = FMath::Clamp(accelerationMod, minAcceleration, maxAcceleration);
    acceleration *= accelerationMod;
    UE_LOG(LogTemp, Warning, TEXT("acceleration is: %f %f %f"), acceleration.X, acceleration.Y, acceleration.Z);
    UE_LOG(LogTemp, Warning, TEXT("distance is: %f"), distance);
}


void AGAIControllerCommonCar::StartShootDelay()
{
    // Define your range, e.g., between 0.5s and 1.5s
    float RandomDelay = FMath::RandRange(1.0f, 1.0f)+fireRate;
    UE_LOG(LogTemp, Log, TEXT("shot timer started"));
    GetWorldTimerManager().SetTimer(shootDelayHandle,this, &AGAIControllerCommonCar::Shoot, RandomDelay, false);
}

void AGAIControllerCommonCar::Shoot_Implementation()
{
    UE_LOG(LogTemp, Log, TEXT("trying to shoot"));
    if (FMath::RandRange(0.0f, 1.0f)<=hitChance)
    {
        float damage = FMath::RandRange(minDamage, maxDamage);
        float accumulatedWeight = 0;
        float randomWeight = FMath::RandRange(0.0f, totalWeight);
        int32 partID;
        for (int32 i = 0; i < weights.Num(); i++) {
            if (randomWeight < accumulatedWeight) 
            {
                partID = i;
                break;
            }
            accumulatedWeight += weights[i];
        }
        if (partID) {
            UE_LOG(LogTemp, Log, TEXT("hit part with id: %d damage: %f"), partID, damage);
        }
        else 
        {
            UE_LOG(LogTemp, Error, TEXT("no part of car got shot for some reason"));
        }
    }
    else {
        UE_LOG(LogTemp, Log, TEXT("missed"));
    }
    StartShootDelay();
}