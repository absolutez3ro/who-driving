// Fill out your copyright notice in the Description page of Project Settings.


#include "GBaseEnemy.h"

// Sets default values
AGBaseEnemy::AGBaseEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGBaseEnemy::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGBaseEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGBaseEnemy::HandleDeath() {
	if (!HasAuthority())
	{
		return;
	}
	Destroy(true, false);
}

// Called to bind functionality to input
float AGBaseEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
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

