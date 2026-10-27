#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "G_Vehicle_Combat_Anim_Instance.generated.h"

class UAnimSequence;

// Presentation values only. The editable AnimBP owns pose evaluation and blending.
UCLASS(Transient, Blueprintable)
class WHOS_DRIVING_API UGVehicleCombatAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	void NativeUpdateAnimation(float deltaSeconds) override;
	UFUNCTION()
	void AnimNotify_MagOut_LeftHand();
	UFUNCTION()
	void AnimNotify_MagOut_RightHand();
	UFUNCTION()
	void AnimNotify_MagIn();
	UFUNCTION()
	void AnimNotify_MagSpawn();
	UFUNCTION()
	void AnimNotify_MagDestroy();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAnimSequence> basePose;
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAnimSequence> weaponPose;
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float poseTime = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float seatAlpha = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float weaponAlpha = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Combat|IK")
	float handIkAlpha = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Combat|IK")
	FVector rightHandLocation;
	UPROPERTY(BlueprintReadOnly, Category = "Combat|IK")
	FRotator rightHandRotation;
	UPROPERTY(BlueprintReadOnly, Category = "Combat|IK")
	FVector leftHandLocation;
	UPROPERTY(BlueprintReadOnly, Category = "Combat|IK")
	FVector rightElbowLocation;
	UPROPERTY(BlueprintReadOnly, Category = "Combat|IK")
	FVector leftElbowLocation;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Pose")
	FVector pelvisOffset = FVector(0.0f, 0.0f, -15.0f);
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FVector spineOffset = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FRotator spineRotation = FRotator::ZeroRotator;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Pose")
	FRotator thighRotation = FRotator(0.0f, 0.0f, 75.0f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Pose")
	FRotator calfRotation = FRotator(0.0f, 0.0f, -95.0f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Pose")
	float windowLeanDegrees = 12.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FRotator weaponRotation = FRotator::ZeroRotator;
};
