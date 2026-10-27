#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/NetSerialization.h"
#include "G_Vehicle_Radio_Component.h"
#include "G_Weapon_Definition.h"
#include "GVehicleModule.h"

#include "GPlayerController.generated.h"

struct FInputActionValue;
class UInputAction;
class AWhos_DrivingCharacter;
class AGPlayerCar;
class UInputMappingContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnModuleInfoUpdated, FName, ModuleId, float, CurrentHpInPercentage, EGModuleState, moduleState);
DECLARE_DYNAMIC_DELEGATE_OneParam(FTryToRepair, FName, ModuleId);

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
	void RequestPreviousTrack();
	void RequestNextTrack();

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Modules")
	bool TraceVehicleModule(FName& outModuleId, float& outCurrentHPInPercentage, EGModuleState& outModuleState);

	UFUNCTION(Client, Reliable)
	void Client_SetupLobbyInput();

	UPROPERTY(BlueprintAssignable)
	FOnModuleInfoUpdated OnModuleInfoUpdated;

	UPROPERTY()
	FTryToRepair TryToRepair;

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	void Tick(float DeltaTime) override;
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

	UFUNCTION(Server, Reliable)
	void ServerRadioCommand(AGPlayerCar* vehicle, EGVehicleRadioCommand command,
		FVector_NetQuantize viewOrigin, FVector_NetQuantizeNormal viewDirection);

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> gameplayMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> repairAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FKey seatInteractionKey = EKeys::F;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> interactAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> exitVehicleAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> primaryWeaponAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> secondaryWeaponAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> windowLeftAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> windowRightAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> fireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> aimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> reloadAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> fireModeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> previousTrackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> nextTrackAction;

	UPROPERTY(BlueprintReadOnly)
	bool bIsLookingOnModule = false;
	FName currentModule = NAME_None;

	UFUNCTION(BlueprintImplementableEvent)
	void OnLookingModuleChanged(bool bIsLooking);

	void TryToSendDelegate();

private:
	bool IsRadioTargetValid(AGPlayerCar* vehicle, const FVector& origin, const FVector& direction) const;
	bool IsVehicleOccupant() const;
	float lastRadioRequestTime = -1.0f;
	float lastSeatRequestTime = -1.0f;
	float lastCombatViewTime = -1.0f;
	class UGVehicleCombatComponent* GetOwnedCombat() const;
};
