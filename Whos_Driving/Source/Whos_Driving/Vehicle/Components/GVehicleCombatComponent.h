#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/NetSerialization.h"
#include "GWeaponDefinition.h"
#include "GVehicleCombatComponent.generated.h"

class AWhos_DrivingCharacter;
class AGPlayerCar;
class AGPlayerController;
class AGVehicleWeapon;
class UAnimInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGOnWeaponAmmoChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthUpdate, float, health, float, maxHealth);

USTRUCT(BlueprintType)
struct FGVehicleWindowState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly)
	bool bOutside = false;
	UPROPERTY(BlueprintReadOnly)
	float startAlpha = 0.0f;
	UPROPERTY(BlueprintReadOnly)
	float startTime = 0.0f;
	UPROPERTY(BlueprintReadOnly)
	float duration = 0.45f;
};

UCLASS(ClassGroup = (Vehicle), meta = (BlueprintSpawnableComponent))
class WHOS_DRIVING_API UGVehicleCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGVehicleCombatComponent();
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
	void TickComponent(float deltaSeconds, ELevelTick tickType, FActorComponentTickFunction* tickFunction) override;
	bool RequestWindow(bool bOutside);
	// Q/E: leans towards the given side of the camera view, so the keys swap when looking backward.
	bool RequestWindowSide(bool bLeftSide, const FVector& viewDirection);
	bool SelectWeapon(int32 weaponIndex);
	bool SetTrigger(bool bPressed, const FVector& direction);
	bool SetAiming(bool bNewAiming);
	bool RequestReload();
	bool ChangeFireMode();
	void UpdateView(const FVector& direction);
	void CancelActions(bool bResetWindow);
	void SeatChanged();
	float ApplyDamage(float amount, AController* instigatorController);
	void ApplyShotRecoil(float amount);
	void RecordCosmeticShot();
	// Weapons report their replicated state here, so listeners only ever deal with the selected weapon.
	void NotifyWeaponStateChanged(const AGVehicleWeapon* weapon);

	UFUNCTION(BlueprintPure, Category = "Combat")
	AGVehicleWeapon* GetCurrentWeapon() const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	AGVehicleWeapon* GetWeapon(int32 weaponIndex) const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetSelectedWeaponIndex() const
	{
		return selectedWeaponIndex;
	}
	// Ammunition of the weapon in the hands. Returns false when the character holds no weapon.
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool GetCurrentWeaponAmmo(int32& outMagazineAmmo, int32& outReserveAmmo) const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanFire() const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetWindowAlpha() const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsOutside() const
	{
		return windowState.bOutside;
	}
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAiming() const
	{
		return bAiming;
	}
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsLocallyOwned() const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsDrivingLocked() const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetSeatIndex() const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	AGPlayerCar* GetVehicle() const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	FVector GetAimDirection() const;
	// View orientation including roll; use it instead of GetAimDirection().Rotation() for anything placed relative to the camera.
	UFUNCTION(BlueprintPure, Category = "Combat")
	FRotator GetAimRotation() const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetHealth() const
	{
		return health;
	}
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetCosmeticShotTime() const
	{
		return cosmeticShotTime;
	}
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetEquipStartTime() const
	{
		return equipStartTime;
	}
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetConfirmedShots() const
	{
		return confirmedShots;
	}
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetServerTime() const;
	UFUNCTION(BlueprintPure, Category = "Combat|Animation")
	FVector GetFirstPersonPoseOffset() const;

	// Fires when the selected weapon's ammunition changes and when another weapon becomes selected.
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FGOnWeaponAmmoChanged OnWeaponAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Health")
	FOnHealthUpdate OnHealthUpdate;

	// Weapon order matches AGPlayerController::weaponSelectActions.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Loadout")
	TArray<TObjectPtr<UGWeaponDefinition>> loadout;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Health")
	float maxHealth = 100.0f;
	// Moves the first-person mesh while seated, in character space with a level camera. The camera sits well in
	// front of the seated body's shoulders, and this keeps the hand targets within the visible arms' reach.
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	FVector seatedFirstPersonOffset = FVector::ZeroVector;
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TSubclassOf<UAnimInstance> seatedAnimClass;
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TSubclassOf<UAnimInstance> seatedFirstPersonAnimClass;
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TSubclassOf<UAnimInstance> walkingAnimClass;
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TSubclassOf<UAnimInstance> walkingFirstPersonAnimClass;

protected:
	void InitializeComponent() override;
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	void RegenHealth(float deltaSeconds);
	UPROPERTY(ReplicatedUsing = OnRep_Loadout, BlueprintReadOnly, Category = "Combat")
	TArray<TObjectPtr<AGVehicleWeapon>> weapons;
	UPROPERTY(ReplicatedUsing = OnRep_Loadout, BlueprintReadOnly, Category = "Combat")
	int32 selectedWeaponIndex = INDEX_NONE;
	UPROPERTY(ReplicatedUsing = OnRep_Window, BlueprintReadOnly, Category = "Combat")
	FGVehicleWindowState windowState;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat")
	FVector_NetQuantizeNormal aimDirection = FVector::ForwardVector;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat")
	bool bAiming = false;

	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Combat|Health")
	float health = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Health")
	float regenCooldown = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Health")
	float healthRegenRate = 5.0f;

	float lastDamageTime = -FLT_MAX;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat")
	float equipStartTime = -100.0f;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat")
	int32 confirmedShots = 0;

	UFUNCTION()
	void OnRep_Loadout();
	UFUNCTION()
	void OnRep_Window();
	UFUNCTION()
	void OnRep_Health();

private:
	AWhos_DrivingCharacter* GetCharacter() const;
	AGPlayerController* GetPlayerController() const;
	bool IsAuthority() const;
	bool IsPlayerValid() const;
	void SpawnLoadout();
	int32 FindDriverCabinWeaponIndex() const;
	void StopFiring();
	void TryFire();
	void RefreshPresentation();
	void RefreshFirstPersonMesh(const FVector& seatedCameraLocation);
	void RefreshDriverLock();
	bool bTriggerHeld = false;
	int32 burstRemaining = 0;
	float cosmeticShotTime = -100.0f;
	float lastViewUpdate = -100.0f;
	float lastWindowRequest = -100.0f;
	FTimerHandle fireTimer;
	FVector initialCameraLocation;
	FVector initialFirstPersonLocation;
	float initialCameraFov = 90.0f;
};
