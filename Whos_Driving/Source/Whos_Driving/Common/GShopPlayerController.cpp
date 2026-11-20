// Fill out your copyright notice in the Description page of Project Settings.


#include "Common/GShopPlayerController.h"

void AGShopPlayerController::BeginPlay()
{
	Super::BeginPlay();
	GS = GetWorld()->GetGameState<AGGSShop>();
}



void AGShopPlayerController::RequestRepair_Implementation(const FName& name)
{
	if (!GS) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("NO GS")));
		return;
	}
	GS->RepairModule(name);
}
void AGShopPlayerController::RequestRepairAll_Implementation()
{
	if (!GS) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("NO GS")));
		return;
	}
	GS->RepairAll();
}
void AGShopPlayerController::RequestLevelEnd_Implementation()
{
	if (!GS) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("NO GS")));
		return;
	}
	GS->OnLevelEnd();
}
void AGShopPlayerController::RequestHpUpgrade_Implementation(const EGModuleType& type)
{
	if (!GS) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("NO GS")));
		return;
	}
	GS->UpgradeModuleHp(type);
}
void AGShopPlayerController::RequestUpgradeMaxSpeed_Implementation()
{
	if (!GS) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("NO GS")));
		return;
	}
	GS->UpgradeMaxSpeed();
}
void AGShopPlayerController::RequestUpgradeAcceleration_Implementation()
{
	if (!GS) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("NO GS")));
		return;
	}
	GS->UpgradeAcceleration();
}
