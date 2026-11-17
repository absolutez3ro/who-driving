// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GPlayerCar.h"
#include "GGIWhosDriving.h"
#include "Common/GShopDataAsset.h"
#include "GameFramework/GameStateBase.h"
#include "GGSShop.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EGModuleType : uint8
{
	Wheel,
	Door,
	SteeringWheel,
	Engine,
	Electrician
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCarSpawned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopPurchase);


UCLASS()
class WHOS_DRIVING_API AGGSShop : public AGameStateBase
{
	GENERATED_BODY()

private:
	const TMap<FName, FString> ModuleNames =
	{
		{ TEXT("Wheel_0"),   TEXT("Front left wheel") },
		{ TEXT("Wheel_1"), TEXT("Front right wheel")  },
		{ TEXT("Wheel_2"),   TEXT("Rear right wheel") },
		{ TEXT("Wheel_3"), TEXT("Rear left wheel")  },
		{ TEXT("partDoor0"),   TEXT("Left door") },
		{ TEXT("partDoor1"), TEXT("Right door")  },
		{ TEXT("Steering_Wheel"),   TEXT("Steering wheel") },
		{ TEXT("Engine"), TEXT("Engine")  },
		{ TEXT("Electrician"),  TEXT("Electrician") }
	};
	const TMap<FName, EGModuleType> moduleTypes =
	{
		{ TEXT("Wheel_0"), EGModuleType::Wheel },
		{ TEXT("Wheel_1"), EGModuleType::Wheel },
		{ TEXT("Wheel_2"), EGModuleType::Wheel },
		{ TEXT("Wheel_3"), EGModuleType::Wheel },
		{ TEXT("partDoor0"), EGModuleType::Door },
		{ TEXT("partDoor1"), EGModuleType::Door },
		{ TEXT("Steering_Wheel"), EGModuleType::SteeringWheel },
		{ TEXT("Engine"), EGModuleType::Engine },
		{ TEXT("Electrician"), EGModuleType::Electrician }
	};
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;
public:
	UPROPERTY(BlueprintAssignable)
	FOnCarSpawned OnCarSpawned;
	UPROPERTY(BlueprintAssignable)
	FOnShopPurchase OnShopPurchase;
	UPROPERTY(ReplicatedUsing = OnRep_Car, BlueprintReadOnly, Category = "Reference")
	TObjectPtr<AGPlayerCar> car;
	UFUNCTION()
	void OnRep_Car();
	UPROPERTY(Replicated)
	TArray<FName> brokenModules;
	TObjectPtr<UGGIWhosDriving> gi;
	UPROPERTY(ReplicatedUsing = OnRep_Money, BlueprintReadOnly, Category = "economy")
	int32 money;
	UFUNCTION()
	void OnRep_Money();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGShopDataAsset> dataAsset;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car")
	TSubclassOf<AGPlayerCar> carClass;
	UFUNCTION(BlueprintCallable)
	void GetRepairData(TArray<FName>& Names, TArray<FString>& OutDisplayNames, TArray<float>& OutCurrentHp, TArray<float>& OutMaxHp, TArray<int32>& OutPrices);
	void SpawnCar();
	void BeginPlay();
	int32 GetRepairCostFromDataAsset(FName moduleName);
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable)
	void RepairModule(const FName& ModuleName);
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable)
	void RepairAll();
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable)
	void UpgradeModuleHp(const EGModuleType& type);
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable)
	void OnLevelEnd();
};
