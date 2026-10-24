// Fill out your copyright notice in the Description page of Project Settings.


#include "GAIControllerCommonCar.h"


void AGAIControllerCommonCar::ApplyAcceleration() 
{
    APawn* selfActor = GetPawn();
    if (!selfActor) return;
    UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(selfActor->GetRootComponent());
    if (RootPrim && RootPrim->IsSimulatingPhysics())
    {
        // GetMass() returns the mass in kg (calculated by the engine or overridden)
        float Mass = RootPrim->GetMass();
        FVector movementDirection = FVector(1, 0, 0);
        FVector movementAcceleration = movementDirection * acceleration;
        FVector ForceToApply = Mass * movementAcceleration;
        RootPrim->AddForce(ForceToApply);
        FVector currentVelocity= RootPrim->GetPhysicsLinearVelocity();
        float currentVelocityX = currentVelocity.X;
        UE_LOG(LogTemp, Warning, TEXT("velocity was: %f"), currentVelocityX);
        if(acceleration<0 and currentVelocityX<0)
        {
            currentVelocityX =0;
        }
        FVector newVelocity = FVector(50, currentVelocity.Y, currentVelocity.Z);
        UE_LOG(LogTemp, Warning, TEXT("velocity is: %f"), currentVelocityX);
        RootPrim->SetPhysicsLinearVelocity(newVelocity);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RootPrim(%d) or IsSimulatingPhysics(%d) false!"), RootPrim, RootPrim->IsSimulatingPhysics());
    }

}



void AGAIControllerCommonCar::Chase(AActor* carReference, float deltaSeconds)
{
    if (carReference)
    {
        APawn* selfActor = GetPawn();
        // get coordinates to scale speed
        float playerXCoordinate = carReference->GetActorLocation().X;
        float selfXCoordinate = selfActor->GetActorLocation().X;

        float deltaX = playerXCoordinate - selfXCoordinate;
        CalculateAcceleration(deltaX, deltaSeconds);
        ApplyAcceleration();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("InteractWithActor called with a null carReference!"));
    }
}

void AGAIControllerCommonCar::CalculateAcceleration(float deltaX, float deltaSeconds)
{
    acceleration = baseAcceleration * pow(deltaX,3);
    acceleration = FMath::Clamp(acceleration, minAcceleration, maxAcceleration);
}
