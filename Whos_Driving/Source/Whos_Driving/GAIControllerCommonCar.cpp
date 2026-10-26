// Fill out your copyright notice in the Description page of Project Settings.

#include "GAIControllerCommonCar.h"
#include "GCommonEnemyDataAsset.h"
#include "GPlayerCar.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
void AGAIControllerCommonCar::ApplyAcceleration(float distance, FVector playerForward)
{
    APawn* selfActor = GetPawn();
    if (!selfActor) return;
    UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(selfActor->GetRootComponent());
    if (RootPrim && RootPrim->IsSimulatingPhysics())
    {
        FVector currentVelocity= RootPrim->GetPhysicsLinearVelocity();
        bool reverseMovement = FVector::DotProduct(currentVelocity, playerForward)<=0;
        UE_LOG(LogTemp, Warning, TEXT("reverseMovement is: %d"), reverseMovement);
        if(distance <0 and reverseMovement)
        {
            UE_LOG(LogTemp, Error, TEXT("velocity set to 0"));
            currentVelocity = FVector(0, 0, currentVelocity.Z);
            RootPrim->SetPhysicsLinearVelocity(currentVelocity);
        }
        else 
        {
            // GetMass() returns the mass in kg (calculated by the engine or overridden)
            float Mass = RootPrim->GetMass();
            FVector ForceToApply = Mass * acceleration;
            RootPrim->AddForce(ForceToApply);
                currentVelocity.Z = 0;
            if(currentVelocity.Length()>50)
            {
                FRotator NewRotation = FRotationMatrix::MakeFromX(currentVelocity).Rotator();
                selfActor->SetActorRotation(NewRotation, ETeleportType::TeleportPhysics);
            }

        }
        UE_LOG(LogTemp, Warning, TEXT("velocity is: %f %f %f"), currentVelocity.X, currentVelocity.Y, currentVelocity.Z);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RootPrim(%d) or IsSimulatingPhysics (if not root prim its this) false!"), RootPrim);
    }

}

void AGAIControllerCommonCar::BeginPlay() {
    Super::BeginPlay();
    //its more memory efficient to copy them, surprisingly
    minAcceleration = commonCarData->minAcceleration;
    maxAcceleration = commonCarData->maxAcceleration;
    baseAcceleration = commonCarData->accelerationModifier;
    minYDistance = commonCarData->minYDistanceCM;
    maxYDistance = commonCarData->maxYDistanceCM;
    fireRate = commonCarData->fireRate;
    hitChance = commonCarData->hitChance;
    minDamage = commonCarData->minDamage;
    maxDamage = commonCarData->maxDamage;
    weights.Add(commonCarData->wheelWeight);
    weights.Add(commonCarData->doorWeight);
    weights.Add(commonCarData->baseHealthWeight);
    weights.Add(commonCarData->steeringWeight);
    weights.Add(commonCarData->engineWeight);
    weights.Add(commonCarData->electronicsWeight);
    module_names[0].Add(FName("Wheel_0"));
    module_names[0].Add(FName("Wheel_1"));
    module_names[0].Add(FName("Wheel_2"));
    module_names[0].Add(FName("Wheel_3"));
    module_names[1].Add(FName("part_door_0"));
    module_names[1].Add(FName("part_door_1"));
    module_names[2].Add(FName("Steering_Wheel"));
    module_names[3].Add(FName("Engine"));
    module_names[4].Add(FName("Electician"));
    YDistance = FMath::FRandRange(minYDistance, maxYDistance);
    SetPlayerRef();
    StartShootDelay();
}


void AGAIControllerCommonCar::SetPlayerRef()
{
    AActor* FoundActorRaw = UGameplayStatics::GetActorOfClass(GetWorld(), AGPlayerCar::StaticClass());

    // 3. Cast to your specific type
    AGPlayerCar* carReference = Cast<AGPlayerCar>(FoundActorRaw);

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
    FVector playerPos = playerCarRef->GetActorLocation();
    FVector selfPos = selfActor->GetActorLocation();
    FVector playerForward = playerCarRef->GetActorForwardVector();
    FVector toPlayerVector = playerPos - selfPos;
    float distance = FVector::DotProduct(toPlayerVector, playerForward);
    CalculateAcceleration(toPlayerVector, playerForward, deltaSeconds);
    ApplyAcceleration(distance, playerForward);
}

void AGAIControllerCommonCar::CalculateAcceleration(FVector toPlayerVector, FVector playerForward, float deltaSeconds)
{
    FVector playerLeft = FVector3d(-playerForward.Y, playerForward.X, playerForward.Z);
    float distance= FVector::DotProduct(toPlayerVector,playerForward);
    float accelerationMod= baseAcceleration * pow(abs(distance), 3);
    playerLeft.Normalize();
    playerLeft *= YDistance;
    acceleration = playerLeft + toPlayerVector;
    float accelarationLength = acceleration.Length();
    acceleration.Normalize();
    accelerationMod = FMath::Clamp(accelerationMod, minAcceleration, maxAcceleration);
    acceleration *= accelerationMod;
    UE_LOG(LogTemp, Warning, TEXT("acceleration is: %f %f %f"), acceleration.X, acceleration.Y, acceleration.Z);
    UE_LOG(LogTemp, Warning, TEXT("accelerationMod is: %f"), accelerationMod);
    UE_LOG(LogTemp, Warning, TEXT("distance is: %f"), distance);
}

void AGAIControllerCommonCar::Tick(float DeltaSeconds)
{
    Chase(DeltaSeconds);
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
        TArray<FName> shootable_module_names[6];
        TArray<int32> shootable_wheight_ids;
        float damage = FMath::RandRange(minDamage, maxDamage);
        for (int32 i=0; i<6; i++)
        {

            float OverallHp = 0;
            for (int32 j=0; j<module_names[i].Num();j++)
            {
                float moduleHp = playerCarRef->GetModuleDamageState(module_names[i][j]).currentHP;
                OverallHp += moduleHp;
                if (moduleHp > 0) 
                {
                    shootable_module_names[i].Add(module_names[i][j]);
                }
            }
            if (OverallHp>0)
            {
                shootable_wheight_ids.Add(i);
            }
        }
        shootable_wheight_ids.Add(5); // add 5 manually cause "base hp" is not a module 

        float totalWeight = 0;
        for (int32 i=0; i< shootable_wheight_ids.Num();i++)
        {
            totalWeight += weights[shootable_wheight_ids[i]];
        }
        float accumulatedWeight = 0;
        float randomWeight = FMath::RandRange(0.0f, totalWeight);
        int32 moduleID=-1;

        for (int32 i = 0; i < shootable_wheight_ids.Num(); i++) {
            accumulatedWeight += weights[shootable_wheight_ids[i]];
            if (randomWeight <= accumulatedWeight) 
            {
                moduleID = shootable_wheight_ids[i];
                break;
            }
        }

        if (moduleID>=0) {
            if (moduleID == 5) {
                playerCarRef->TakeDamage(damage, FDamageEvent(), nullptr, this);
                UE_LOG(LogTemp, Log, TEXT("hit main hp %f"), damage);
            }
            else
            {
                int availableModulesAmount = (module_names[moduleID].Num() - 1);
                FName TargetedModule= module_names[moduleID][FMath::RandRange(0, availableModulesAmount)];
                playerCarRef->DamageModule(TargetedModule, damage);
                UE_LOG(LogTemp, Log, TEXT("hit part %s damage: %f"), *TargetedModule.ToString(), damage);
            }
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