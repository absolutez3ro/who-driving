// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GPlayerCar.h"
#include "GGIWhosDriving.h"
#include "Common/GShopDataAsset.h"
#include "GameFramework/GameModeBase.h"
#include "GShopGameMode.generated.h"

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


UCLASS()
class WHOS_DRIVING_API AGShopGameMode : public AGameModeBase
{
	GENERATED_BODY()
private:
	TObjectPtr<AGPlayerCar> car;
	TArray<FName> brokenModules;
	TObjectPtr<UGGIWhosDriving> gi;
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
public:
	UPROPERTY(BlueprintReadOnly, Category = "economy")
	int32 money;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGShopDataAsset> dataAsset;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car")
	TSubclassOf<AGPlayerCar> carClass;
	UFUNCTION(BlueprintCallable)
	void GetRepairData(TArray<FName>& Names, TArray<FString>& OutDisplayNames, TArray<float>& OutCurrentHp, TArray<float>& OutMaxHp, TArray<int32>& OutPrices);
	void SpawnCar();
	void BeginPlay();
	void LoadDataFromDataAsset();
	UFUNCTION(BlueprintCallable)
	void RepairModule(const FName& ModuleName);
	UFUNCTION(BlueprintCallable)
	void RepairAll();
};
