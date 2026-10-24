// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GBaseEnemyCar.generated.h"

UCLASS()
class WHOS_DRIVING_API AGBaseEnemyCar : public APawn
{
	GENERATED_BODY()


public:
	// Sets default values for this character's properties
	AGBaseEnemyCar();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	// handles enemy death event
	void HandleDeath();

};
