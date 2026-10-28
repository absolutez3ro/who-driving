#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "ShooterWeapon.h"
#include "GWeaponDefinition.h"
#include "GVehicleWeapon.generated.h"

class AWhos_DrivingCharacter;
class UStaticMeshComponent;

// Uses the existing shooter weapon's first/third-person meshes; gameplay is server-authoritative.
UCLASS()
class WHOS_DRIVING_API AGVehicleWeapon : public AShooterWeapon
{
	GENERATED_BODY()

public:
	AGVehicleWeapon();
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
	void Initialize(UGWeaponDefinition* newDefinition);
	void SetEquipped(bool bNewEquipped);
	bool ConsumeRound();
	bool BeginReload();
	void CancelReload();
	bool CycleFireMode();
	float GetNextShotTime() const
	{
		return nextShotTime;
	}
	void SetNextShotTime(float value)
	{
		nextShotTime = value;
	}
	void SetMagazineInHand(bool bInHand, bool bLeftHand, bool bFirstPerson);
	void PlayEquip();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	UGWeaponDefinition* GetDefinition() const
	{
		return definition;
	}
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetReserveAmmo() const
	{
		return reserveAmmo;
	}
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsReloading() const
	{
		return bReloading;
	}
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsEmptyReload() const
	{
		return bEmptyReload;
	}
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsEquipped() const
	{
		return bEquipped;
	}
	UFUNCTION(BlueprintPure, Category = "Weapon")
	EGWeaponFireMode GetFireMode() const
	{
		return fireMode;
	}
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetActionStartTime() const
	{
		return actionStartTime;
	}
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetReloadEndTime() const
	{
		return reloadEndTime;
	}

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastShot(FVector_NetQuantize impact, bool bHit);

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	void Tick(float deltaSeconds) override;

	UPROPERTY(ReplicatedUsing = OnRep_Definition, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UGWeaponDefinition> definition;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon")
	int32 reserveAmmo = 0;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon")
	EGWeaponFireMode fireMode = EGWeaponFireMode::Single;
	UPROPERTY(ReplicatedUsing = OnRep_Presentation, BlueprintReadOnly, Category = "Weapon")
	bool bEquipped = false;
	UPROPERTY(ReplicatedUsing = OnRep_Reload, BlueprintReadOnly, Category = "Weapon")
	bool bReloading = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon")
	bool bEmptyReload = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon")
	float actionStartTime = 0.0f;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon")
	float reloadEndTime = 0.0f;
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> firstMagazine;
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> worldMagazine;

	UFUNCTION()
	void OnRep_Definition();
	UFUNCTION()
	void OnRep_Presentation();
	UFUNCTION()
	void OnRep_Reload();

private:
	void ConfigureMeshes();
	void AttachMeshes();
	void FinishReload();
	AWhos_DrivingCharacter* GetCharacter() const;
	FTimerHandle reloadTimer;
	float lastShotTime = -100.0f;
	float nextShotTime = -100.0f;
	TArray<TObjectPtr<UStaticMeshComponent>> attachmentMeshes;
};
