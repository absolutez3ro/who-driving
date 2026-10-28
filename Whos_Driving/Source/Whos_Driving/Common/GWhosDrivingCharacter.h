// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GWhosDrivingCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UGVehicleCombatComponent;
class UBoxComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class AGWhosDrivingCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGWhosDrivingCharacter();
	float TakeDamage(float damageAmount, const FDamageEvent& damageEvent, AController* eventInstigator, AActor* damageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Combat")
	UGVehicleCombatComponent* GetCombatComponent() const
	{
		return combatComponent;
	}

	USceneComponent* GetCameraRoot() const
	{
		return cameraRoot;
	}

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoAim(float yaw, float pitch);

	void ResetVehicleLook();

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const
	{
		return firstPersonMesh;
	}

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const
	{
		return firstPersonCameraComponent;
	}

protected:
	/** Set up input action bindings */
	void SetupPlayerInputComponent(UInputComponent* playerInputComponent) override;

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& value);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float right, float forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* jumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* moveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* lookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* mouseLookAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USceneComponent> cameraRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float cameraSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float maxPitch = 80.0f;

	float cameraPitch = 0.0f;
	float vehicleCameraYaw = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UGVehicleCombatComponent> combatComponent;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UBoxComponent> combatBody;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UBoxComponent> combatHead;

private:
	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* firstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* firstPersonCameraComponent;
};
