#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GArcadeVehicleInputComponent.generated.h"

class AController;
class APawn;
class APlayerController;
class UEnhancedInputComponent;
class UGArcadeVehicleComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

// Feeds Enhanced Input actions into the owner's arcade vehicle while a local player controls the pawn.
// The mapping context is added on possession and removed on unpossession, never globally.
UCLASS(ClassGroup = (Vehicle), meta = (BlueprintSpawnableComponent))
class WHOS_DRIVING_API UGArcadeVehicleInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGArcadeVehicleInputComponent();

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> inputMappingContext;

	// Higher than the default vehicle context, so these actions win over shared keys.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 mappingPriority = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> throttleAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> brakeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> steerAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> handbrakeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> resetAction;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* pawn, AController* oldController, AController* newController);

	void BindToController(APlayerController* playerController);
	void UnbindFromController();
	void BindActions(UEnhancedInputComponent& enhancedInput);
	void ClearVehicleInput();

	void HandleThrottle(const FInputActionValue& value);
	void HandleThrottleReleased(const FInputActionValue& value);
	void HandleBrake(const FInputActionValue& value);
	void HandleBrakeReleased(const FInputActionValue& value);
	void HandleSteer(const FInputActionValue& value);
	void HandleSteerReleased(const FInputActionValue& value);
	void HandleHandbrakePressed(const FInputActionValue& value);
	void HandleHandbrakeReleased(const FInputActionValue& value);
	void HandleReset(const FInputActionValue& value);

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> actionInputComponent;

	UPROPERTY(Transient)
	TObjectPtr<UGArcadeVehicleComponent> vehicle;

	TWeakObjectPtr<APlayerController> boundController;
};
