// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GVehicleModule.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGVehicleModule : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module")
	float maxHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module")
	float currentHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module")
	bool bDestroyed = false;

	void ApplyDamage(float damage);
	bool IsDestroyed() const;

protected:
	virtual void OnDestroyed();
	virtual void OnDamageTaken(float damage);
};
