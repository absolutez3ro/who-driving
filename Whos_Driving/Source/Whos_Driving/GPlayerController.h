#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/NetSerialization.h"
#include "InputCoreTypes.h"
#include "G_Weapon_Definition.h"
#include "GPlayerController.generated.h"

struct FInputActionValue;
class UInputAction;
class AWhos_DrivingCharacter;
class AGPlayerCar;
class UInputMappingContext;

UCLASS()
class WHOS_DRIVING_API AGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGPlayerController();
	void SetDriverCharacter(AWhos_DrivingCharacter* newDriver);
	UFUNCTION(BlueprintPure, Category = "Vehicle|Seats")
	AWhos_DrivingCharacter* GetDriverCharacter() const;
	UFUNCTION(BlueprintPure, Category = "Vehicle|Seats")
	AGPlayerCar* GetSeatedVehicle() const;
	void SetVehicleContext(AGPlayerCar* vehicle, AWhos_DrivingCharacter* character);
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void OnRep_Pawn() override;

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Seats")
	void RequestLookedAtSeat();

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Seats")
	void RequestVehicleExit();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void RequestPrimaryWeapon();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void RequestSecondaryWeapon();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void RequestWindowLeft();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void RequestWindowRight();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void RequestFireStart();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void RequestFireStop();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void RequestAimStart();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void RequestAimStop();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void RequestReload();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void RequestFireMode();
	void SendCombatView(const FVector& direction);

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	void SetupInputComponent() override;
	void LookInput(const FInputActionValue& value);
	void RefreshVehicleView();

	UFUNCTION()
	void OnRep_VehicleContext();

	UFUNCTION(Server, Reliable)
	void ServerRequestVehicleSeat(AGPlayerCar* vehicle, int32 seatIndex, FVector_NetQuantize viewOrigin, FVector_NetQuantizeNormal viewDirection);

	UFUNCTION(Server, Reliable)
	void ServerRequestVehicleExit();
	UFUNCTION(Server, Reliable)
	void ServerCombatAction(uint8 action, FVector_NetQuantizeNormal direction);
	UFUNCTION(Server, Unreliable)
	void ServerCombatView(FVector_NetQuantizeNormal direction);

	UPROPERTY(ReplicatedUsing = OnRep_VehicleContext)
	TObjectPtr<AWhos_DrivingCharacter> driverCharacter;

	UPROPERTY(ReplicatedUsing = OnRep_VehicleContext, BlueprintReadOnly, Category = "Vehicle|Seats")
	TObjectPtr<AGPlayerCar> seatedVehicle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> vehicleMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> walkingMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> mouseLookMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> lookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FKey seatInteractionKey = EKeys::F;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FKey vehicleExitKey = EKeys::H;

private:
	float lastSeatRequestTime = -1.0f;
	float lastCombatViewTime = -1.0f;
	class UGVehicleCombatComponent* GetOwnedCombat() const;
};
