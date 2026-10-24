// Fill out your copyright notice in the Description page of Project Settings.


#include "GBaseEnemyCar.h"

// Sets default values
AGBaseEnemyCar::AGBaseEnemyCar()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGBaseEnemyCar::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AGBaseEnemyCar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGBaseEnemyCar::HandleDeath() {
	if (!HasAuthority())
	{
		return;
	}
	Destroy(true, false);
}

// Called to bind functionality to input
float AGBaseEnemyCar::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority())
	{
		return 0.0f;
	}
	if (DamageAmount > 0)
	{
		HandleDeath();
	}
	return DamageAmount;
}

