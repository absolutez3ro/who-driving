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
	bool SelectWeapon(EGWeaponSlot slot);
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

	UFUNCTION(BlueprintPure, Category = "Combat")
	AGVehicleWeapon* GetCurrentWeapon() const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	AGVehicleWeapon* GetWeapon(EGWeaponSlot slot) const;
	UFUNCTION(BlueprintPure, Category = "Combat")
	EGWeaponSlot GetSelectedSlot() const
	{
		return selectedSlot;
	}
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Loadout")
	TObjectPtr<UGWeaponDefinition> primaryDefinition;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Loadout")
	TObjectPtr<UGWeaponDefinition> secondaryDefinition;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Health")
	float maxHealth = 100.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	FVector firstPersonPoseOffset = FVector(35.0f, 0.0f, 0.0f);
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TSubclassOf<UAnimInstance> seatedAnimClass;
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TSubclassOf<UAnimInstance> seatedFirstPersonAnimClass;
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TSubclassOf<UAnimInstance> walkingAnimClass;
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TSubclassOf<UAnimInstance> walkingFirstPersonAnimClass;

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	UPROPERTY(ReplicatedUsing = OnRep_Loadout, BlueprintReadOnly, Category = "Combat")
	TArray<TObjectPtr<AGVehicleWeapon>> weapons;
	UPROPERTY(ReplicatedUsing = OnRep_Loadout, BlueprintReadOnly, Category = "Combat")
	EGWeaponSlot selectedSlot = EGWeaponSlot::Secondary;
	UPROPERTY(ReplicatedUsing = OnRep_Window, BlueprintReadOnly, Category = "Combat")
	FGVehicleWindowState windowState;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat")
	FVector_NetQuantizeNormal aimDirection = FVector::ForwardVector;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat")
	bool bAiming = false;
	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Combat")
	float health = 100.0f;
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
	void TryFire();
	void RefreshPresentation();
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
