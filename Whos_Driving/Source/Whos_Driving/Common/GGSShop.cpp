// Fill out your copyright notice in the Description page of Project Settings.


#include "Common/GGSShop.h"

#include "Net/UnrealNetwork.h" 
// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFramework/PlayerState.h"

#include "GVehicleModuleComponent.h"

void AGGSShop::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGGSShop, money);
	DOREPLIFETIME(AGGSShop, car);
	DOREPLIFETIME(AGGSShop, brokenModules);
}

void AGGSShop::SpawnCar()
{
	if (!HasAuthority()) 
	{
		return;
	}
	if (!car)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("NO car, spawning"));
		FActorSpawnParameters parameters;
		parameters.Owner = this;
		parameters.Instigator = GetInstigator();
		parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FTransform tr = FTransform::Identity;
		FTransform transform(FRotator(0, -130.0f, 0), FVector(732.0f, -50.0f, -448.0f));
		car = GetWorld()->SpawnActor<AGPlayerCar>(carClass, transform, parameters);
		if (!gi) {
			gi = GetGameInstance<UGGIWhosDriving>();
			if (!gi)
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("NO GI")));
				return;
			}
		}
		car->moduleStates = gi->GetCarData().moduleStates;
		//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::White, FString::Printf(TEXT(" mod state in gi %i"), gi->GetCarData().moduleStates.Num()));
		car->currentCarHealth = gi->GetCarData().currentCarHealth;
		car->baseCarHealth = gi->GetCarData().baseCarHealth;
		car->ApplyModuleStates();
		OnRep_Car();
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Car already exists"));
		OnRep_Car();
	}
}
void AGGSShop::OnRep_Car()
{
	OnCarSpawned.Broadcast();
}

void AGGSShop::OnRep_Money()
{
	OnShopPurchase.Broadcast();
}

void AGGSShop::BeginPlay()
{
	gi = GetGameInstance<UGGIWhosDriving>();
	if (!gi)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("NO GI")));
		return;
	}
	if (HasAuthority())
	{
		money = gi->moneyStored;
	}
	SpawnCar();
	Super::BeginPlay();
}


//REPAIR
void AGGSShop::GetRepairData(TArray<FName>& OutNames, TArray<FString>& OutDisplayNames, TArray<float>& OutCurrentHp, TArray<float>& OutMaxHp, TArray<int32>& OutPrices)
{
	if (GEngine)
	{
		if (HasAuthority())
		{
			//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::White, FString::Printf(TEXT("GetRepairData")));
		}
		else
		{
			//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("GetRepairData")));
		}
	}
	if (!car) {
		SpawnCar();
	}
	if (!car) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("WASN'T ANBLE TO SPAWN OR GET CAR")));
		return;
	}
	// shouldn't call Gengine in begin play
	if (GEngine)
	{
		if (HasAuthority())
		{
			//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::White, FString::Printf(TEXT("mod state in car %i"), car->moduleStates.Num()));
		}
		else
		{
			//GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("mod state in car %i"), car->moduleStates.Num()));
		}
	}
	brokenModules.Empty();
	for (TObjectPtr<UGVehicleModule> module : car->GetModuleComponent()->GetModules()) {
		if (module->currentHP < module->maxHP)
		{
			OutNames.Add(module->moduleId);
			brokenModules.Add(module->moduleId);
			OutDisplayNames.Add(ModuleNames[module->moduleId]);
			OutCurrentHp.Add(module->currentHP);
			OutMaxHp.Add(module->maxHP);
			int32 baseRepairCost = GetRepairCostFromDataAsset(module->moduleId);
			int32 actualRepairCost = baseRepairCost * (2 - module->currentHP / module->maxHP);
			OutPrices.Add(actualRepairCost);
			if (GEngine)
			{
				//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("name: %s chp: %f mhp: %f"), *ModuleNames[module->moduleId], module->currentHP, module->maxHP));
			}
		}
		else {
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Black, FString::Printf(TEXT("name: %s chp: %f mhp: %f"), *ModuleNames[module->moduleId], module->currentHP, module->maxHP));
		}
	} 
}
void AGGSShop::RepairModule(const FName& ModuleName)
{
	int32 baseRepairCost;
	if (ModuleNames.Contains(ModuleName))
	{
		TObjectPtr<UGVehicleModule> moduleToFix = car->GetModuleComponent()->FindModule(ModuleName);
		baseRepairCost = GetRepairCostFromDataAsset(moduleToFix->moduleId);
		if(moduleToFix->currentHP== moduleToFix->maxHP)
		{
			return;
		}
		int32 actualRepairCost = baseRepairCost * (2 - moduleToFix->currentHP / moduleToFix->maxHP);
		if (money >= actualRepairCost)
		{
			moduleToFix->ApplyRepairInShop();
			car->PublishModuleStates();
			money -= actualRepairCost;
			OnRep_Money();
		}
	}
	else
	{
		return;
	}

}
void AGGSShop::RepairAll()
{
	int32 fullRepairCost = 0;
	if (brokenModules.Num() == 0)
	{
		return;
	}
	for (FName name : brokenModules)
	{
		TObjectPtr<UGVehicleModule> moduleToFix = car->GetModuleComponent()->FindModule(name);
		int32 baseRepairCost = GetRepairCostFromDataAsset(name);
		fullRepairCost += baseRepairCost * (2 - moduleToFix->currentHP / moduleToFix->maxHP);
	}
	if (money >= fullRepairCost)
	{
		for (FName name : brokenModules)
		{
			RepairModule(name);
		}
	}
}

//UPGRADE
void AGGSShop::UpgradeModuleHp(const EGModuleType& type)
{
	int32 actualPrice;
	for (FGVehicleModuleState& state : car->moduleStates)
	{
		if (moduleTypes[state.moduleId]==type)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan, FString::Printf(TEXT("upgrading %s"), *ModuleNames[state.moduleId]));
			state.moduleHpLevel++;
			if (state.currentHP>0.01)
			{
				state.currentHP += state.HpIncreasePerLevel;
			}
			int32 basePrice = GetUpgradeCostFromDataAsset(moduleTypes[state.moduleId]);
			actualPrice = basePrice * pow(dataAsset->priceGrowthExpBase, state.moduleHpLevel - 1);
		}
	}
	car->ApplyModuleStates();
	car->PublishModuleStates();
	money -= actualPrice;
	OnRep_Money();

}
void AGGSShop::GetUpgradeHpData(TMap<EGModuleType, int32>& OutPrices)
{
	for (TObjectPtr<UGVehicleModule> module : car->GetModuleComponent()->GetModules())
	{
		if (!OutPrices.Contains(moduleTypes[module->moduleId]))
		{
			int32 basePrice = GetUpgradeCostFromDataAsset(moduleTypes[module->moduleId]);
			int32 actualPrice = basePrice * pow(dataAsset->priceGrowthExpBase, module->moduleHPLevel-1);
			OutPrices.Add(moduleTypes[module->moduleId], actualPrice);
		}
	}
}
// Data Asset
int32 AGGSShop::GetRepairCostFromDataAsset(FName moduleName)
{
	switch (moduleTypes[moduleName])
	{
	case EGModuleType::Wheel:
		return dataAsset->wheelBaseRepairPrice;
	case EGModuleType::Door:
		return dataAsset->doorBaseRepairPrice;
	case EGModuleType::Engine:
		return dataAsset->engineBaseRepairPrice;
	case EGModuleType::SteeringWheel:
		return dataAsset->steerBaseRepairPrice;
	case EGModuleType::Electrician:
		return dataAsset->electricianBaseRepairPrice;
	default:
		return dataAsset->mainHPBaseRepairPrice;
	}
}
int32 AGGSShop::GetUpgradeCostFromDataAsset(EGModuleType type)
{
	switch (type)
	{
	case EGModuleType::Wheel:
		return dataAsset->wheelBaseUpgradePrice;
	case EGModuleType::Door:
		return dataAsset->doorBaseUpgradePrice;
	case EGModuleType::Engine:
		return dataAsset->engineBaseUpgradePrice;
	case EGModuleType::SteeringWheel:
		return dataAsset->steerBaseUpgradePrice;
	case EGModuleType::Electrician:
		return dataAsset->electricianBaseUpgradePrice;
	default:
		return dataAsset->mainHPBaseUpgradePrice;
	}
}


void AGGSShop::OnLevelEnd()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("Level ended traveling")));
	GetGameInstance<UGGIWhosDriving>()->FetchCarData();
	if (!HasAuthority())
	{
		return;
	}
	gi->GetCarData().moduleStates = car->moduleStates;
	gi->GetCarData().currentCarHealth = car->currentCarHealth;
	gi->GetCarData().baseCarHealth = car->baseCarHealth;
	for (FGVehicleModuleState state : gi->GetCarData().moduleStates) 
	{
		//GEngine->AddOnScreenDebugMessage(-1,15.f, FColor::White, FString::Printf(TEXT("name: %s chp: %f mhp: %f"), *ModuleNames[state.moduleId], state.currentHP, state.maxHP));
	}
	if (UWorld* World = GetWorld())
	{
		World->ServerTravel(TEXT("/Game/Greybox/Lvl_Greybox_Wasteland?listen"));
	}
}