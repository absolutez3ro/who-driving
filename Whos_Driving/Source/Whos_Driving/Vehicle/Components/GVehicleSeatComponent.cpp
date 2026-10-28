#include "GVehicleSeatComponent.h"

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
