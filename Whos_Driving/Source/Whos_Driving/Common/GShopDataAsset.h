// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GShopDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGShopDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "economy | upgrade")
	float priceGrowthExpBase = 1.4;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "economy | repair")
	int32 wheelBaseRepairPrice = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "economy | repair")
	int32 doorBaseRepairPrice = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "economy | repair")
	int32 engineBaseRepairPrice = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "economy | repair")
	int32 steerBaseRepairPrice = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "economy | repair")
	int32 electricianBaseRepairPrice = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "economy | repair")
	int32 mainHPBaseRepairPrice = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "economy | upgrade")
	int32 wheelBaseUpgradePrice = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "effect | upgrade")
	int32 wheelHpUpgradePerLevel = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "economy | upgrade")
	int32 doorBaseUpgradePrice = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "effect | upgrade")
	int32 doorHpUpgradePerLevel = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "economy | upgrade")
	int32 engineBaseUpgradePrice = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "effect | upgrade")
	int32 engineHpUpgradePerLevel = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "economy | upgrade")
	int32 steerBaseUpgradePrice = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "effect | upgrade")
	int32 steerHpUpgradePerLevel = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "economy | upgrade")
	int32 electricianBaseUpgradePrice = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "effect | upgrade")
	int32 electricianHpUpgradePerLevel = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "economy | upgrade")
	int32 mainHPBaseUpgradePrice = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "effect | upgrade")
	int32 mainHpUpgradePerLevel = 10;
};
