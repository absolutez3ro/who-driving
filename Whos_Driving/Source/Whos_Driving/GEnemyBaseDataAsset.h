// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GEnemyBaseDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGEnemyBaseDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "base")
	float maxHp;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "base")
	float scrapPrice;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "base")
	USkeletalMesh* mesh;
};
