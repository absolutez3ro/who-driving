// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Pawn.h"
#include "GBaseEnemyCar.generated.h"

class UGEnemyBaseDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGOnEnemyDeath, float, scrapPrice);

UCLASS()
class WHOS_DRIVING_API AGBaseEnemyCar : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGBaseEnemyCar();

	// Called every frame
	void Tick(float deltaTime) override;

	// Applies incoming damage on the server
	float TakeDamage(float damageAmount, FDamageEvent const& damageEvent, AController* eventInstigator, AActor* damageCauser) override;

	// handles enemy death event
	void HandleDeath();

	UFUNCTION(NetMulticast, Unreliable)
	void PlayShotSound();

	UFUNCTION(NetMulticast, Unreliable)
	void PlayMissSound();

	// Cosmetic side of a shot (muzzle flash, sound, tracer); damage is applied by the AI controller
	//UFUNCTION(NetMulticast, Unreliable)
	//void MulticastShotFired(FVector target, bool bHit);

	//UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	//void OnShotFired(FVector target, bool bHit);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<USoundBase> hitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<USoundBase> missSound;

	UPROPERTY(BlueprintAssignable, Category = "Game Events")
	FGOnEnemyDeath OnDeath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGEnemyBaseDataAsset> baseData;

protected:
	void BeginPlay() override;

	float maxHp = 1;
	float scrapPrice = 0;
	float currentHp = 1;
};
