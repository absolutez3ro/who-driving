#include "GVehicleCombatAnimInstance.h"
#include "GVehicleCombatComponent.h"
#include "GVehicleWeapon.h"
#include "GVehicleSeatComponent.h"
#include "GVehicleSeatingComponent.h"
#include "GPlayerCar.h"
#include "Whos_DrivingCharacter.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveVector.h"

void UGVehicleCombatAnimInstance::NativeUpdateAnimation(float deltaSeconds)
{
	Super::NativeUpdateAnimation(deltaSeconds);
	AWhos_DrivingCharacter* character = Cast<AWhos_DrivingCharacter>(TryGetPawnOwner());
	if (!character)
	{
		return;
	}
	UGVehicleCombatComponent* combat = character->GetCombatComponent();
	AGVehicleWeapon* weapon = combat->GetCurrentWeapon();
	UGWeaponDefinition* definition = weapon ? weapon->GetDefinition() : nullptr;
	seatAlpha = combat->GetSeatIndex() != INDEX_NONE ? 1.0f : 0.0f;
	weaponAlpha = definition && seatAlpha > 0.0f ? 1.0f : 0.0f;
	weaponPose = definition && definition->IdlePose ? definition->IdlePose : basePose;
	poseTime = 0.0f;
	const float now = combat->GetServerTime();
	float handReadiness = 1.0f;
	if (definition && weapon->IsReloading())
	{
		// Anchor the adapted reload near the vehicle camera. The source FPS arms
		// have a different origin; releasing IK puts their weapon below the lens.
		handReadiness = FMath::Max(0.95f, FMath::Clamp((now - weapon->GetReloadEndTime() + 0.2f) / 0.2f, 0.0f, 1.0f));
	}
	else if (definition)
	{
		handReadiness =
			FMath::Clamp((now - combat->GetEquipStartTime() - definition->EquipTime + 0.2f) / 0.2f, 0.0f, 1.0f);
	}
	handIkAlpha = definition ? seatAlpha * handReadiness : 0.0f;
	if (definition)
	{
		const FTransform meshTransform = GetSkelMeshComponent()->GetComponentTransform();
		const FTransform aimTransform(combat->GetAimDirection().Rotation(),
									  character->GetFirstPersonCameraComponent()->GetComponentLocation());
		// Donor guns point along mesh +Y. Solve the hand orientation from the grip
		// socket instead of retaining the retargeted hand's unrelated local axes.
		const FQuat gripRotation =
			GetSkelMeshComponent()->GetSocketTransform(definition->GripSocket, RTS_ParentBoneSpace).GetRotation() *
			definition->GripOffset.GetRotation();
		const FQuat weaponWorldRotation = aimTransform.GetRotation() * FRotator(0.0f, -90.0f, 0.0f).Quaternion();
		rightHandRotation =
			(meshTransform.GetRotation().Inverse() * weaponWorldRotation * gripRotation.Inverse()).Rotator();
		auto target = [&](const FVector& localOffset)
		{
			const FVector viewmodelOffset = GetSkelMeshComponent() == character->GetFirstPersonMesh()
												? combat->GetFirstPersonPoseOffset()
												: FVector::ZeroVector;
			return meshTransform.InverseTransformPosition(
				aimTransform.TransformPosition(localOffset + viewmodelOffset));
		};
		rightHandLocation = target(combat->IsAiming() ? definition->AimHandOffset : definition->RightHandOffset);
		leftHandLocation = target(
			definition->LeftHandOffset +
			(combat->IsAiming() ? definition->AimHandOffset - definition->RightHandOffset : FVector::ZeroVector));
		rightElbowLocation = target(FVector(-5.0f, 42.0f, -35.0f));
		leftElbowLocation = target(FVector(-5.0f, -42.0f, -35.0f));
	}
	if (definition && weapon->IsReloading())
	{
		weaponPose = weapon->IsEmptyReload() ? definition->EmptyReloadPose : definition->ReloadPose;
		poseTime = now - weapon->GetActionStartTime();
		if (weaponPose)
		{
			poseTime *= weaponPose->GetPlayLength() /
						FMath::Max(0.1f, weapon->GetReloadEndTime() - weapon->GetActionStartTime());
		}
	}
	else if (definition && now - combat->GetEquipStartTime() < definition->EquipTime)
	{
		weaponPose = definition->EquipPose ? definition->EquipPose : definition->IdlePose;
		poseTime = now - combat->GetEquipStartTime();
	}
	if (weaponPose)
	{
		poseTime = FMath::Clamp(poseTime, 0.0f, FMath::Max(0.0f, weaponPose->GetPlayLength() - 0.001f));
	}
	spineOffset = FVector::ZeroVector;
	spineRotation = FRotator::ZeroRotator;
	weaponRotation = FRotator::ZeroRotator;
	if (AGPlayerCar* vehicle = combat->GetVehicle())
	{
		if (UGVehicleSeatComponent* seat = vehicle->GetSeatingComponent()->GetSeatComponent(combat->GetSeatIndex()))
		{
			FVector offset = seat->windowOffset;
			offset.Y *= combat->GetSeatIndex() == 0 || combat->GetSeatIndex() == 2 ? -1.0f : 1.0f;
			const FVector worldOffset =
				character->GetActorTransform().TransformVectorNoScale(offset * combat->GetWindowAlpha());
			spineOffset =
				GetSkelMeshComponent()->GetComponentTransform().InverseTransformVectorNoScale(worldOffset) * 0.18f;
			const FRotator aim =
				character->GetActorTransform().InverseTransformVectorNoScale(combat->GetAimDirection()).Rotation();
			const float side = combat->GetSeatIndex() == 0 || combat->GetSeatIndex() == 2 ? -1.0f : 1.0f;
			spineRotation =
				FRotator(side * windowLeanDegrees * combat->GetWindowAlpha(), aim.Yaw / 3.0f, aim.Pitch / 3.0f);
		}
	}
	// M8 uses vector curves for recoil and procedural hand motion in its AnimBP.
	if (definition && definition->RecoilRotationCurve)
	{
		const float elapsed = now - combat->GetCosmeticShotTime();
		if (elapsed >= 0.0f && elapsed < 0.5f)
		{
			const FVector recoil = definition->RecoilRotationCurve->GetVectorValue(elapsed) * definition->Recoil;
			weaponRotation = FRotator(recoil.Y, recoil.Z, recoil.X);
		}
	}
}

void UGVehicleCombatAnimInstance::AnimNotify_MagOut_LeftHand()
{
	if (AWhos_DrivingCharacter* character = Cast<AWhos_DrivingCharacter>(TryGetPawnOwner()))
	{
		if (AGVehicleWeapon* weapon = character->GetCombatComponent()->GetCurrentWeapon())
		{
			weapon->SetMagazineInHand(true, true, GetSkelMeshComponent() == character->GetFirstPersonMesh());
		}
	}
}

void UGVehicleCombatAnimInstance::AnimNotify_MagOut_RightHand()
{
	if (AWhos_DrivingCharacter* character = Cast<AWhos_DrivingCharacter>(TryGetPawnOwner()))
	{
		if (AGVehicleWeapon* weapon = character->GetCombatComponent()->GetCurrentWeapon())
		{
			weapon->SetMagazineInHand(true, false, GetSkelMeshComponent() == character->GetFirstPersonMesh());
		}
	}
}

void UGVehicleCombatAnimInstance::AnimNotify_MagIn()
{
	if (AWhos_DrivingCharacter* character = Cast<AWhos_DrivingCharacter>(TryGetPawnOwner()))
	{
		if (AGVehicleWeapon* weapon = character->GetCombatComponent()->GetCurrentWeapon())
		{
			weapon->SetMagazineInHand(false, true, GetSkelMeshComponent() == character->GetFirstPersonMesh());
		}
	}
}

void UGVehicleCombatAnimInstance::AnimNotify_MagSpawn()
{
	AnimNotify_MagOut_LeftHand();
}
void UGVehicleCombatAnimInstance::AnimNotify_MagDestroy()
{
	AnimNotify_MagIn();
}
