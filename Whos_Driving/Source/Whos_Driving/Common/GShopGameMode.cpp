// Fill out your copyright notice in the Description page of Project Settings.


#include "Common/GShopGameMode.h"
#include "GameFramework/PlayerState.h"

#include "GVehicleModuleComponent.h"

void AGShopGameMode::SpawnCar()
{
	if (!car)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("NO car, spawning"));
		FActorSpawnParameters parameters;
		parameters.Owner = this;
		parameters.Instigator = GetInstigator();
		parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FTransform tr = FTransform::Identity;
		FTransform transform(FRotator(0,-130.0f,0),FVector(732.0f,-50.0f, -448.0f));
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
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Car already exists"));
	}
}

void AGShopGameMode::BeginPlay()
{
	gi = GetGameInstance<UGGIWhosDriving>();
	if (!gi)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("NO GI")));
		return;
	}
	money = gi->GetMoneyFromTotal();
	if (!HasAuthority()){
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::White, FString::Printf(TEXT("NO AUTHORITY")));
		return;
	}
	SpawnCar();
	LoadDataFromDataAsset();
	Super::BeginPlay();
}

void AGShopGameMode::GetRepairData(TArray<FName>& OutNames, TArray<FString>& OutDisplayNames, TArray<float>& OutCurrentHp, TArray<float>& OutMaxHp, TArray<int32>& OutPrices)
{
	if (!car) {
		SpawnCar();
	}
	float modulesHP = 0;
	// shouldn't call Gengine in begin play
	if (GEngine) 
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::White, FString::Printf(TEXT("mod state in car %i"), car->moduleStates.Num()));
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
			int32 actualRepairCost = module->baseRepairCostShop*(2- module->currentHP / module->maxHP);
			OutPrices.Add(actualRepairCost);
			modulesHP += module->currentHP;
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("name: %s chp: %f mhp: %f"), *ModuleNames[module->moduleId], module->currentHP, module->maxHP));
			}
		}
	}
	if (car->currentCarHealth - modulesHP < car->baseCarHealth)
	{
		OutDisplayNames.Add(TEXT("main hp"));
		OutCurrentHp.Add(car->currentCarHealth - modulesHP);
		OutMaxHp.Add(car->baseCarHealth);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("name: %s chp: %f mhp: %f"), TEXT("main hp"), car->currentCarHealth - modulesHP, car->baseCarHealth));
		}
	}
}

void AGShopGameMode::LoadDataFromDataAsset() 
{
	if (!car)
	{
		SpawnCar();
		if (!car)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("NO CAR")));
			return;
		}
	}
	for (TObjectPtr<UGVehicleModule> module : car->GetModuleComponent()->GetModules())
	{
		switch (moduleTypes[module->moduleId])
		{
		case EGModuleType::Wheel:
			module->baseRepairCostShop = dataAsset->wheelBaseRepairPrice;
			break;
		case EGModuleType::Door:
			module->baseRepairCostShop = dataAsset->doorBaseRepairPrice;
			break;
		case EGModuleType::Engine:
			module->baseRepairCostShop = dataAsset->engineBaseRepairPrice;
			break;
		case EGModuleType::SteeringWheel:
			module->baseRepairCostShop = dataAsset->steerBaseRepairPrice;
			break;
		case EGModuleType::Electrician:
			module->baseRepairCostShop = dataAsset->electricianBaseRepairPrice;
			break;
		}
	}

}

void AGShopGameMode::RepairModule(const FName & ModuleName)
{
	TObjectPtr<UGVehicleModule> moduleToFix= car->GetModuleComponent()->FindModule(ModuleName);
	int32 actualRepairCost = moduleToFix->baseRepairCostShop * (2 - moduleToFix->currentHP / moduleToFix->maxHP);
	if (money>= actualRepairCost)
	{
		moduleToFix->ApplyRepairInShop();
		car->PublishModuleStates();
		money -= actualRepairCost;
	}
}
void AGShopGameMode::RepairAll()
{
	int32 fullRepairCost = 0;
	if (brokenModules.Num() == 0) 
	{
		return;
	}
	for(FName name : brokenModules)
	{
		TObjectPtr<UGVehicleModule> moduleToFix = car->GetModuleComponent()->FindModule(name);
		fullRepairCost += moduleToFix->baseRepairCostShop * (2 - moduleToFix->currentHP / moduleToFix->maxHP);
	}
	if (money >= fullRepairCost)
	{
		for (FName name : brokenModules)
		{
			RepairModule(name);	
		}
	}
}
