#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GWeaponDefinition.generated.h"

class UAnimSequence;
class UCameraShakeBase;
class UCurveVector;
class UParticleSystem;
class USkeletalMesh;
class UStaticMesh;
class USoundBase;
class UTexture2D;
class AGVehicleWeapon;

UENUM(BlueprintType)
enum class EGWeaponSlot : uint8
{
	Primary,
	Secondary
};

UENUM(BlueprintType)
enum class EGWeaponFireMode : uint8
{
	Single,
	Burst,
	Auto
};

USTRUCT(BlueprintType)
struct FGWeaponAttachment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName id = TEXT("Base");

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName socket;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FTransform offset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float damageModifier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 magazineCapacity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float aimFov = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> fireSound;
};

// M8 weapon defaults and presentation assets, separated from its Survival character and UI.
UCLASS(BlueprintType)
class WHOS_DRIVING_API UGWeaponDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AGVehicleWeapon> weaponClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText displayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EGWeaponSlot slot = EGWeaponSlot::Primary;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UTexture2D> icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMesh> mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMesh> magazineMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName gripSocket = TEXT("M8_Weapon_R");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FTransform gripOffset;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "0"))
	float damage = 20.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "1"))
	float range = 30000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "0", ClampMax = "45"))
	float spreadDegrees = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "0.03"))
	float fireInterval = 0.12f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "0.03"))
	float burstInterval = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "1", ClampMax = "10"))
	int32 burstSize = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	TArray<EGWeaponFireMode> fireModes;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "1"))
	int32 magazineSize = 40;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0"))
	int32 initialReserve = 120;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0.1"))
	float reloadTime = 1.8f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0.1"))
	float emptyReloadTime = 2.3f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling", meta = (ClampMin = "0.05"))
	float equipTime = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling", meta = (ClampMin = "0.05"))
	float aimTime = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling")
	float aimFov = 70.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling")
	float recoil = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FVector rightHandOffset = FVector(30.0f, 12.0f, -12.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FVector leftHandOffset = FVector(50.0f, -6.0f, -10.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FVector aimHandOffset = FVector(25.0f, 0.0f, -10.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> idlePose;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> equipPose;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> reloadPose;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> emptyReloadPose;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> weaponFire;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> weaponEquip;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> weaponReload;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> weaponEmptyReload;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UCurveVector> recoilLocationCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UCurveVector> recoilRotationCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UCurveVector> sustainedRecoilCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UCurveVector> equipCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<USoundBase> fireSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<USoundBase> equipSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UParticleSystem> muzzleEffect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UParticleSystem> shellEffect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UParticleSystem> impactEffect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TSubclassOf<UCameraShakeBase> cameraShake;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attachments")
	TArray<FGWeaponAttachment> attachments;
};
