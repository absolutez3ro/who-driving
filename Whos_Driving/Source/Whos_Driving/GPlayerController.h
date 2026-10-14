// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GPlayerController.generated.h"


struct FInputActionValue;
class UInputAction;
class AWhos_DrivingCharacter;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API AGPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AGPlayerController();
	void SetDriverCharacter(AWhos_DrivingCharacter* newDriver);
	AWhos_DrivingCharacter* GetDriverCharacter() const;

protected:
	void BeginPlay() override;
	void SetupInputComponent() override;
	void LookInput(const FInputActionValue& value);
	void DebugBombInput(const FInputActionValue& value);
	void DebugExitVehicleInput(const FInputActionValue& value);
	void DebugExitVehicle();
	void EnableCharacterInput();

	UPROPERTY()
	TObjectPtr<AWhos_DrivingCharacter> driverCharacter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> vehicleMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> mouseLookMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> debugMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> characterMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> lookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> bombAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> exitVehicleAction;
}; 
