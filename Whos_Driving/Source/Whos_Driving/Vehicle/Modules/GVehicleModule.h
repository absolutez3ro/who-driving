// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GVehicleModule.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRepair);

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

UENUM(BlueprintType)
enum class EGComponentSide : uint8
{
	Left,
	Right
};

UCLASS()
class WHOS_DRIVING_API UGVehicleModule : public UObject
{
	GENERATED_BODY()
	
public:
	void ApplyDamage(const float damage);
	void ApplyRepair();
	bool IsDestroyed() const;
	bool CanRepair() const;
	bool IsFull() const;
	virtual void ApplyReplicatedState(float inMaxHp, float inCurrentHp, EGModuleState state);

	UFUNCTION(BlueprintCallable)
	float GetHealthAsPercentage() const;

	UPROPERTY(BlueprintAssignable)
	FOnRepair OnRepair;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Module")
	FName moduleId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module", meta = (ClampMin = "1.0"))
	float maxHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module")
	float currentHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float repairThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Module")
	int32 baseRepairCost = 1;

	int32 actualRepairCost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Module")
	float destroyedModuleCostMultiplier = 2.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Module")
	EGModuleState moduleState = EGModuleState::Full;

protected:
	virtual void OnStateChanged(EGModuleState previousState, EGModuleState newState);
	void SetState(EGModuleState newState);
	virtual void OnDestroyed();
	void RecalculateRepairCost();
};
