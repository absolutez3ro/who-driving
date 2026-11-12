// Fill out your copyright notice in the Description page of Project Settings.


#include "Common/GRaceFinish.h"
#include "Components/BoxComponent.h"
#include "Vehicle/Common/GPlayerCar.h"
#include "GGameState.h"

AGRaceFinish::AGRaceFinish()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	
	finishCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("FinishCollider"));
	finishCollider->SetupAttachment(RootComponent);

	finishCollider->OnComponentBeginOverlap.AddDynamic(this, &AGRaceFinish::OnOverlapBegin);
}

void AGRaceFinish::BeginPlay()
{
	Super::BeginPlay();
}


void AGRaceFinish::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AGPlayerCar* playerCar = Cast<AGPlayerCar>(OtherActor);
	if (!playerCar)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Purple, TEXT("FINISH"));
	
	AGGameState* gameState = GetWorld()->GetGameState<AGGameState>();
	gameState->Multicast_OnPlayerFinish();
}


