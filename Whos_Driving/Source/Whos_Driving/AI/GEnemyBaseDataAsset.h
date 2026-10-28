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
	float MaxHp;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "base")
	float ScrapPrice;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "base")
	USkeletalMesh* Mesh;
};
