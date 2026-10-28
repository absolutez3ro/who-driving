// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GBaseEnemy.generated.h"

class UGEnemyBaseDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGOnDeath, float, scrapPrice);

UCLASS()
class WHOS_DRIVING_API AGBaseEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGBaseEnemy();

	// Called every frame
	void Tick(float deltaTime) override;

	// Applies incoming damage on the server
	float TakeDamage(float damageAmount, FDamageEvent const& damageEvent, AController* eventInstigator, AActor* damageCauser) override;

	// handles enemy death event
	void HandleDeath();

	UPROPERTY(BlueprintAssignable, Category = "Game Events")
	FGOnDeath OnDeath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGEnemyBaseDataAsset> baseData;

protected:
	void BeginPlay() override;

	float maxHp = 1;
	float scrapPrice = 0;
	float currentHp = 1;
};
