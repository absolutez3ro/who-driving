// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GBaseEnemy.generated.h"
class UGEnemyBaseDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeath, float, scrapPrice_);

UCLASS()
class WHOS_DRIVING_API AGBaseEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGBaseEnemy();

protected:
	virtual void BeginPlay() override;
	float maxHp = 1;
	float scrapPrice = 0;
	float currentHp = 1;
public:
	UPROPERTY(BlueprintAssignable, Category = "Game Events")
	FOnDeath OnDeath;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGEnemyBaseDataAsset> baseData;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	// handles enemy death event
	void HandleDeath();

};
