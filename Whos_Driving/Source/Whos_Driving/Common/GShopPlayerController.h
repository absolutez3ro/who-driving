// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Common/GGSShop.h"
#include "GameFramework/PlayerController.h"
#include "GShopPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API AGShopPlayerController : public APlayerController
{
	GENERATED_BODY()
	void BeginPlay() override;
private:
	TObjectPtr<AGGSShop> GS;
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "RPC's")
	void RequestRepair(const FName& name);
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "RPC's")
	void RequestRepairAll();
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "RPC's")
	void RequestLevelEnd();
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "RPC's")
	void RequestUpgrade(const EGModuleType& type);
};
