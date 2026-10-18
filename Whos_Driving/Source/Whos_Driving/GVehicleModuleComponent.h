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
public:
	const TArray<TObjectPtr<UGVehicleModule>>& GetModules() const;

protected:
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Vehicle|Modules")
	TArray<TObjectPtr<UGVehicleModule>> modules;
		
};
