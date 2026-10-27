// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GVehicleModule.h"
#include "GVehicleModuleComponent.generated.h"


UCLASS( ClassGroup=(Vehicle), meta=(BlueprintSpawnableComponent) )
class WHOS_DRIVING_API UGVehicleModuleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Vehicle|Modules")
	void DamageModule(FName moduleId, float damage);

	UGVehicleModule* FindModule(FName moduleId) const;
	void RegisterModule(UGVehicleModule* module);
	void ApplyReplicatedModuleState(const FName& moduleId, float maxHp, float currentHp, EGModuleState moduleState);
	void RebuildModuleRegistry(AActor* ownerActor);

	template<typename T>
	TArray<TObjectPtr<T>> GetModulesOfType()
	{
		TArray<TObjectPtr<T>> modulesWithType;
		for (auto& module : modules)
		{
			if (T* moduleWithType = Cast<T>(module))
			{
				modulesWithType.Add(moduleWithType);
			}
		} 
		return modulesWithType;
	}

public:
	const TArray<TObjectPtr<UGVehicleModule>>& GetModules() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Modules")
	TArray<TObjectPtr<UGVehicleModule>> modules;
};
