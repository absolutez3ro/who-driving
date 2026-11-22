// Fill out your copyright notice in the Description page of Project Settings.


#include "Common/GRaceFinish.h"
#include "Components/BoxComponent.h"
#include "Vehicle/Common/GPlayerCar.h"
#include "GGameState.h"

const FName AGRaceFinish::StartLineTag(TEXT("RaceStart"));
const FName AGRaceFinish::FinishLineTag(TEXT("RaceFinish"));

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
	if (!HasAuthority() || !Cast<AGPlayerCar>(OtherActor))
	{
		return;
	}

	AGGameState* gameState = GetWorld()->GetGameState<AGGameState>();
	if (!gameState || gameState->IsRaceFailed())
	{
		return;
	}

	const bool bRaceStarted = gameState->IsTimerRunning() || gameState->elapsedTime > 0.0f;
	if (ActorHasTag(StartLineTag))
	{
		if (!bRaceStarted)
		{
			finishCollider->SetGenerateOverlapEvents(false);
			gameState->StartRaceTimer();
		}
		return;
	}

	if (ActorHasTag(FinishLineTag) && !bRaceStarted)
	{
		return;
	}

	finishCollider->SetGenerateOverlapEvents(false);
	gameState->StopRaceTimer();
	gameState->Multicast_OnPlayerFinish();
}


