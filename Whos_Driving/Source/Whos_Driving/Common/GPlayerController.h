#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/NetSerialization.h"
#include "GVehicleRadioComponent.h"
#include "GVehicleModule.h"

#include "GPlayerController.generated.h"

struct FInputActionValue;
class UInputAction;
class AWhos_DrivingCharacter;
class AGPlayerCar;
class UGVehicleCombatComponent;
class UInputMappingContext;
class UGSkillCheckDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FGOnModuleInfoUpdated, FName, moduleId, float, currentHpInPercentage, EGModuleState, moduleState, int32, actualRepairCost);
UENUM()
enum class EGCombatAction : uint8
{
	WindowLeft,
	WindowRight,
	FireStart,
	FireStop,
	AimStart,
	AimStop,
	Reload,
	ChangeFireMode
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSkillCheck, FName, moduleId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGOnPlayerWeaponAmmoChanged);

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
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
	void OnRep_Pawn() override;

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Seats")
	void RequestLookedAtSeat();

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Seats")
	void RequestVehicleExit();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void RequestWeapon(int32 weaponIndex);
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

	UFUNCTION(Exec, BlueprintCallable, Category = "Debug")
	void ExecDamageSelf(float amount = 10.0f);

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Modules")
	bool TraceVehicleModule(FName& outModuleId, float& outCurrentHPInPercentage, EGModuleState& outModuleState, int32& outRepairCost);

	// Checks only the seat rule; module state and repair parts are checked by the module itself.
	UFUNCTION(BlueprintPure, Category = "Vehicle|Modules")
	bool CanRepairModuleFromCurrentSeat(FName moduleId) const;

	// Ammunition of the weapon in this player's hands, for the HUD. Returns false when there is no weapon in the hands.
	UFUNCTION(BlueprintPure, Category = "Combat|UI")
	bool GetCurrentWeaponAmmo(int32& outMagazineAmmo, int32& outReserveAmmo) const;

	UFUNCTION(Client, Reliable)
	void ClientSetupLobbyInput();

	UFUNCTION(BlueprintCallable)
	bool CanRepairCurrentModule() const;

	UPROPERTY(BlueprintAssignable)
	FGOnModuleInfoUpdated OnModuleInfoUpdated;

	UPROPERTY()
	FOnSkillCheck OnSkillCheck;
	// Fires when the ammunition in the hands changes, including another weapon or another character.
	UPROPERTY(BlueprintAssignable, Category = "Combat|UI")
	FGOnPlayerWeaponAmmoChanged OnWeaponAmmoChanged;

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	void Tick(float deltaTime) override;
	void SetupInputComponent() override;
	void LookInput(const FInputActionValue& value);
	void RefreshVehicleView();
	void RefreshAmmoSource();

	UFUNCTION(BlueprintCallable)
	void StartSkillCheck();

	UFUNCTION(BlueprintCallable)
	void EndSkillCheck(bool bSuccess);

	void FailSkillCheck(const FName& moduleId);

	UFUNCTION()
	void OnRep_VehicleContext();

	UFUNCTION(Server, Reliable)
	void ServerRequestRepair(const FName& moduleId);

	UFUNCTION(Server, Reliable)
	void ServerRequestVehicleSeat(AGPlayerCar* vehicle, int32 seatIndex, FVector_NetQuantize viewOrigin, FVector_NetQuantizeNormal viewDirection);

	UFUNCTION(Server, Reliable)
	void ServerRequestVehicleExit();
	UFUNCTION(Server, Reliable)
	void ServerRequestModuleRepair(FName moduleId);
	UFUNCTION(Server, Reliable)
	void ServerSelectWeapon(int32 weaponIndex);
	UFUNCTION(Server, Reliable)
	void ServerCombatAction(EGCombatAction action, FVector_NetQuantizeNormal direction);
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
	TObjectPtr<UInputMappingContext> skillcheckMappingContext;

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

	// Each action selects the weapon with the same index in UGVehicleCombatComponent::loadout.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TArray<TObjectPtr<UInputAction>> weaponSelectActions;

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
	UFUNCTION(BlueprintImplementableEvent)
	void CallSkillcheck(int32 cycleCounts, float goodZoneWidth);

	UFUNCTION()
	void TryToCallSkillcheck();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UGSkillCheckDataAsset> skillCheckData;

private:
	bool IsRadioTargetValid(AGPlayerCar* vehicle, const FVector& origin, const FVector& direction) const;
	bool IsVehicleOccupant() const;
	UFUNCTION()
	void HandleWeaponAmmoChanged();
	float lastRadioRequestTime = -1.0f;
	float lastSeatRequestTime = -1.0f;
	float lastRepairRequestTime = -1.0f;
	float lastCombatViewTime = -1.0f;
	// The character whose ammunition OnWeaponAmmoChanged reports.
	TWeakObjectPtr<UGVehicleCombatComponent> ammoSource;
	UGVehicleCombatComponent* GetOwnedCombat() const;
};
