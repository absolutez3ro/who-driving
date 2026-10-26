// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GVehicleModule.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EGModuleState : uint8
{
	Full,
	Repair,
	Destroyed
};

UCLASS()
class WHOS_DRIVING_API UGVehicleModule : public UObject
{
	GENERATED_BODY()
	
public:
	void ApplyDamage(float damage);
	bool IsDestroyed() const;
	bool CanRepair() const;
	bool IsFull() const;
	virtual void ApplyReplicatedState(float inMaxHp, float inCurrentHp, EGModuleState state);

	UFUNCTION(BlueprintCallable)
	float GetHealthAsPercentage() const;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Module")
	FName moduleId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module", meta = (ClampMin = "1.0"))
	float maxHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module")
	float currentHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float repairThreshold = 0.5f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Module")
	EGModuleState moduleState = EGModuleState::Full;

protected:
	virtual void OnStateChanged(EGModuleState previousState, EGModuleState newState);
	void SetState(EGModuleState newState);
	virtual void OnDestroyed();
	
};
