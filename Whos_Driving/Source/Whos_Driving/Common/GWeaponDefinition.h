#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GWeaponDefinition.generated.h"

class FDataValidationContext;
class UAnimSequence;
class UCameraShakeBase;
class UCurveVector;
class UDamageType;
class UParticleSystem;
class USkeletalMesh;
class UStaticMesh;
class USoundBase;
class UTexture2D;
class AGVehicleWeapon;

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
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName Socket;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FTransform Offset;
};

// The single place for a weapon's balance and presentation. A new hitscan weapon only needs a new asset
// added to the loadout of UGVehicleCombatComponent.
UCLASS(BlueprintType)
class WHOS_DRIVING_API UGWeaponDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AGVehicleWeapon> WeaponClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMesh> MagazineMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName GripSocket = TEXT("M8_Weapon_R");
	// Places the weapon in the hand relative to GripSocket. For the kit's weapons this comes from their idle
	// animations, which hold the weapon on the ik_hand_gun bone rather than on the hand itself.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FTransform GripOffset;
	// The driver can only use this weapon inside the cabin; anything else requires leaning out of the window.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bUsableByDriverInCabin = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Sockets")
	FName MuzzleSocket = TEXT("MuzzleSocket");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Sockets")
	FName ShellSocket = TEXT("ShellSocket");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Sockets")
	FName MagazineSocket = TEXT("MagazineSocket");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Sockets")
	FName LeftHandMagazineSocket = TEXT("M8_Magazine_L");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Sockets")
	FName RightHandMagazineSocket = TEXT("M8_Magazine_R");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "0"))
	float Damage = 20.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	TSubclassOf<UDamageType> DamageType;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "1"))
	float Range = 30000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "0", ClampMax = "45"))
	float SpreadDegrees = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ClampMin = "0", ClampMax = "1"))
	float AimSpreadMultiplier = 0.25f;
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
	// Random horizontal kick per shot, as a fraction of Recoil.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling", meta = (ClampMin = "0"))
	float RecoilYawRange = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FVector RightHandOffset = FVector(27.5f, 15.2f, -14.7f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FVector LeftHandOffset = FVector(50.2f, 5.7f, -17.0f);
	// Orientation of the left hand bone relative to the view.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FRotator LeftHandRotation = FRotator(10.2f, 47.5f, 105.6f);
	// Right hand target while aiming. The sights only line up when the sight line runs through the camera,
	// so this has to put the weapon's OpticSocket at the centre of the view.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FVector AimHandOffset = FVector(25.6f, 4.0f, -11.1f);
	// Added to the hand targets in the first-person view only.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FVector ViewmodelOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FVector RightElbowOffset = FVector(0.0f, 25.0f, -45.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FVector LeftElbowOffset = FVector(0.0f, -25.0f, -45.0f);
	// Rotates the mesh so that it points along the view. The current donor guns point along mesh +Y.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	FRotator MeshForwardRotation = FRotator(0.0f, -90.0f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose", meta = (ClampMin = "0.01"))
	float HandIkBlendTime = 0.2f;
	// How strongly the hands stay on their targets during a reload: 1 pins them, 0 leaves only the reload pose.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose", meta = (ClampMin = "0", ClampMax = "1"))
	float ReloadHandIkAlpha = 0.95f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose", meta = (ClampMin = "0", ClampMax = "1"))
	float ReloadLeftHandIkAlpha = 0.3f;
	// Moves the hands up during a reload, or down with a negative value.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose")
	float ReloadHandLift = -5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Handling|Pose", meta = (ClampMin = "0.01"))
	float ReloadHandLiftTime = 0.2f;
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
	TObjectPtr<UCurveVector> RecoilRotationCurve;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0"))
	float RecoilRotationDuration = 0.5f;
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
