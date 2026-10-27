// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Pawn.h"
#include "GBaseEnemyCar.generated.h"

class UGEnemyBaseDataAsset;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDeath, float, scrapPrice_);
UCLASS()
class WHOS_DRIVING_API AGBaseEnemyCar : public APawn
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	float maxHp=1;
	float scrapPrice = 0;
	float currentHp = 1;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<USoundBase> HitSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<USoundBase> MissSound;

	UFUNCTION(NetMulticast, Unreliable)
	void PlayShotSound();

	UFUNCTION(NetMulticast, Unreliable)
	void PlayMissSound();
	UPROPERTY(BlueprintAssignable, Category = "Game Events")
	FOnEnemyDeath OnDeath;
	// Sets default values for this character's properties
	AGBaseEnemyCar();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGEnemyBaseDataAsset> baseData;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	// handles enemy death event
	void HandleDeath();

	// Cosmetic side of a shot (muzzle flash, sound, tracer); damage is applied by the AI controller
	//UFUNCTION(NetMulticast, Unreliable)
	//void MulticastShotFired(FVector target, bool bHit);

	//UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	//void OnShotFired(FVector target, bool bHit);

};
