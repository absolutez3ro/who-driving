#include "GVehicleSeatComponent.h"
#include "GVehicleModule.h"
#include "GVehicleModuleComponent.h"
#include "GameFramework/Actor.h"

UGVehicleSeatComponent::UGVehicleSeatComponent()
{
	InitBoxExtent(FVector(38.0f, 26.0f, 28.0f));
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	SetHiddenInGame(true);
}

bool UGVehicleSeatComponent::CanRepairModule(FName moduleId) const
{
	return !moduleId.IsNone() && repairableModuleIds.Contains(moduleId);
}

TArray<FString> UGVehicleSeatComponent::GetRepairModuleOptions() const
{
	TArray<FString> options;
	// Templates edited in the Blueprint editor have no runtime owner, so the vehicle is taken from the outer chain.
	const AActor* vehicle = GetTypedOuter<AActor>();
	const UGVehicleModuleComponent* moduleComponent = vehicle ? vehicle->FindComponentByClass<UGVehicleModuleComponent>() : nullptr;
	if (!moduleComponent)
	{
		return options;
	}
	for (const UGVehicleModule* module : moduleComponent->GetModules())
	{
		if (module && !module->moduleId.IsNone())
		{
			options.AddUnique(module->moduleId.ToString());
		}
	}
	return options;
}
