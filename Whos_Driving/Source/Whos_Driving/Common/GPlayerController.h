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
class AGVehicleWeapon;
class UGVehicleCombatComponent;
class UInputMappingContext;
class UGSkillCheckDataAsset;
class UGInteractionFeedback;
class UGBoxComponent;
class UEnhancedInputLocalPlayerSubsystem;

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
	ChangeFireMode,
	Revive,
	WindowToggle
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSkillCheck, FName, moduleId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGOnPlayerWeaponAmmoChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGOnPlayerHealthChanged, float, health, float, maxHealth);

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
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void RequestRevive();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	void RequestReviveStop();
	UFUNCTION(Server, Reliable)
	void ServerRequestRevive(AWhos_DrivingCharacter* targetPlayer);
	UFUNCTION(Server, Reliable)
	void ServerCancelRevive();
	UFUNCTION()
	void CompleteRevive();

	void SendCombatView(const FVector& direction);
	void RequestPreviousTrack();
	void RequestNextTrack();

	UFUNCTION()
	void HandleKnockout();
	UFUNCTION()
	void HandleRevive();

	UFUNCTION(Client, Reliable)
	void ClientHandleKnockout();
	UFUNCTION(Client, Reliable)
	void ClientHandleRevive();

	UFUNCTION(Exec, BlueprintCallable, Category = "Debug")
	void ExecDamageSelf(float amount = 10.0f);

	UFUNCTION(Server, Reliable)
	void ServerExecDamageSelf(float amount);

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Modules")
	bool TraceVehicleModule(FName& outModuleId, float& outCurrentHPInPercentage, EGModuleState& outModuleState, int32& outRepairCost);

	// Checks only the seat rule; module state and repair parts are checked by the module itself.
	UFUNCTION(BlueprintPure, Category = "Vehicle|Modules")
	bool CanRepairModuleFromCurrentSeat(FName moduleId) const;

	// Ammunition of the weapon in this player's hands, for the HUD. Returns false when there is no weapon in the hands.
	UFUNCTION(BlueprintPure, Category = "Combat|UI")
	bool GetCurrentWeaponAmmo(int32& outMagazineAmmo, int32& outReserveAmmo) const;

	// Health of this player's character, for the HUD. Returns false while the controller has no character yet,
	// which on a client lasts until the character reference replicates.
	UFUNCTION(BlueprintPure, Category = "Combat|UI")
	bool GetCurrentHealth(float& outHealth, float& outMaxHealth) const;

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
	// Fires when this player's health changes, including when the controller gets another character.
	UPROPERTY(BlueprintAssignable, Category = "Combat|UI")
	FGOnPlayerHealthChanged OnHealthChanged;

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	void Tick(float deltaTime) override;
	void SetupInputComponent() override;
	void LookInput(const FInputActionValue& value);
	void RefreshVehicleView();
	void RefreshCombatSource();

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
	UFUNCTION(Server, Reliable)
	void ServerRequestAmmoRefill(AGPlayerCar* vehicle, FVector_NetQuantize localViewOrigin,
		FVector_NetQuantizeNormal localViewDirection);

	UPROPERTY(ReplicatedUsing = OnRep_VehicleContext)
	TObjectPtr<AWhos_DrivingCharacter> driverCharacter;

	UPROPERTY(ReplicatedUsing = OnRep_VehicleContext, BlueprintReadOnly, Category = "Vehicle|Seats")
	TObjectPtr<AGPlayerCar> seatedVehicle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Mapping Contexts")
	TObjectPtr<UInputMappingContext> vehicleMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Mapping Contexts")
	TObjectPtr<UInputMappingContext> walkingMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Mapping Contexts")
	TObjectPtr<UInputMappingContext> mouseLookMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Mapping Contexts")
	TObjectPtr<UInputMappingContext> skillcheckMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Mapping Contexts")
	TObjectPtr<UInputMappingContext> gameplayMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> lookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> repairAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	FKey seatInteractionKey = EKeys::F;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> interactAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> exitVehicleAction;

	// Each action selects the weapon with the same index in UGVehicleCombatComponent::loadout.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TArray<TObjectPtr<UInputAction>> weaponSelectActions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> windowLeftAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> windowRightAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> fireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> aimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> reloadAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> fireModeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> previousTrackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> nextTrackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> reviveAction;

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
	void RequestWindowToggle();
	void RefreshSeatedCombatInput(UEnhancedInputLocalPlayerSubsystem& input);
	bool TraceSeatOrRadio(FHitResult& hit, const FVector& origin, const FVector& direction) const;
	AGVehicleWeapon* FindWeaponForAmmoRefill(const FVector& origin, const FVector& direction) const;
	UGBoxComponent* FindLookedAtModule() const;
	bool GetModuleInfo(const UGBoxComponent* hitBox, FName& moduleId, float& healthPercent,
		EGModuleState& state, int32& repairCost) const;
	AWhos_DrivingCharacter* FindReviveTarget() const;
	void UpdateInteractionFeedback();
	FText GetInteractionKey(const UInputAction* action) const;
	bool IsRadioTargetValid(AGPlayerCar* vehicle, const FVector& origin, const FVector& direction) const;
	bool IsVehicleOccupant() const;
	UFUNCTION()
	void HandleWeaponAmmoChanged();
	UFUNCTION()
	void HandleHealthChanged(float health, float maxHealth);
	float lastRadioRequestTime = -1.0f;
	float lastAmmoRefillRequestTime = -1.0f;
	float lastSeatRequestTime = -1.0f;
	float lastRepairRequestTime = -1.0f;
	float lastCombatViewTime = -1.0f;
	UPROPERTY()
	TObjectPtr<UInputAction> windowToggleAction;
	UPROPERTY()
	TObjectPtr<UInputMappingContext> seatedCombatMappingContext;
	UPROPERTY(Transient)
	TObjectPtr<UGInteractionFeedback> interactionFeedback;
	TWeakObjectPtr<UGBoxComponent> lookedAtModule;
	TWeakObjectPtr<AGPlayerCar> skillCheckVehicle;
	FName skillCheckModule = NAME_None;
	bool bSkillCheckActive = false;
	bool bKnockedOut = false;
	// The character whose ammunition and health OnWeaponAmmoChanged and OnHealthChanged report.
	TWeakObjectPtr<UGVehicleCombatComponent> combatSource;
	UGVehicleCombatComponent* GetOwnedCombat() const;
	TWeakObjectPtr<AWhos_DrivingCharacter> reviveTarget;
	FTimerHandle reviveTimer;
};
