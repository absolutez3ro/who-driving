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
	FName Id = TEXT("Base");

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName Socket;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FTransform Offset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DamageModifier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MagazineCapacity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float AimFov = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> FireSound;
};

// M8 weapon defaults and presentation assets, separated from its Survival character and UI.
UCLASS(BlueprintType)
class WHOS_DRIVING_API UGWeaponDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AGVehicleWeapon> WeaponClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EGWeaponSlot Slot = EGWeaponSlot::Primary;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMesh> MagazineMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName GripSocket = TEXT("M8_Weapon_R");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FTransform GripOffset;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "0"))
	float Damage = 20.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "1"))
	float Range = 30000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "0", ClampMax = "45"))
	float SpreadDegrees = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "0.03"))
	float FireInterval = 0.12f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "0.03"))
	float BurstInterval = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "1", ClampMax = "10"))
	int32 BurstSize = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	TArray<EGWeaponFireMode> FireModes;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "1"))
	int32 MagazineSize = 40;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0"))
	int32 InitialReserve = 120;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0.1"))
	float ReloadTime = 1.8f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0.1"))
	float EmptyReloadTime = 2.3f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling", meta = (ClampMin = "0.05"))
	float EquipTime = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling", meta = (ClampMin = "0.05"))
	float AimTime = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling")
	float AimFov = 70.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling")
	float Recoil = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FVector RightHandOffset = FVector(30.0f, 12.0f, -12.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FVector LeftHandOffset = FVector(50.0f, -6.0f, -10.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FVector AimHandOffset = FVector(25.0f, 0.0f, -10.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> IdlePose;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> EquipPose;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> ReloadPose;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> EmptyReloadPose;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> WeaponFire;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> WeaponEquip;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> WeaponReload;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequence> WeaponEmptyReload;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UCurveVector> RecoilLocationCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UCurveVector> RecoilRotationCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UCurveVector> SustainedRecoilCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UCurveVector> EquipCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<USoundBase> FireSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<USoundBase> EquipSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UParticleSystem> MuzzleEffect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UParticleSystem> ShellEffect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UParticleSystem> ImpactEffect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TSubclassOf<UCameraShakeBase> CameraShake;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attachments")
	TArray<FGWeaponAttachment> Attachments;
};
