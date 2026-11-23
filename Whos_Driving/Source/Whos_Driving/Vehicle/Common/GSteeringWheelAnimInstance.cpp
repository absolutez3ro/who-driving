#include "GSteeringWheelAnimInstance.h"

#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "GPlayerCar.h"

const FName UGSteeringWheelAnimInstance::SteeringBoneName(TEXT("SteeringWheel"));

namespace
{
	class FGSteeringWheelAnimProxy final : public FAnimInstanceProxy
	{
	public:
		using FAnimInstanceProxy::FAnimInstanceProxy;

		void PreUpdate(UAnimInstance* instance, float deltaSeconds) override
		{
			FAnimInstanceProxy::PreUpdate(instance, deltaSeconds);
			boneIndex = instance->GetSkelMeshComponent()->GetBoneIndex(UGSteeringWheelAnimInstance::SteeringBoneName);
			const AGPlayerCar* car = Cast<AGPlayerCar>(instance->GetOwningActor());
			const float targetAngle = car ? car->GetSteeringWheelAngle() : 0.0f;
			angle = FMath::FInterpTo(angle, targetAngle, deltaSeconds, 18.0f);
		}

		bool Evaluate(FPoseContext& output) override
		{
			output.ResetToRefPose();
			if (boneIndex != INDEX_NONE)
			{
				const FCompactPoseBoneIndex poseIndex = output.Pose.GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(boneIndex));
				if (poseIndex.IsValid())
				{
					// The column points forward along local +Y. A right turn is clockwise from the driver's seat.
					FTransform& joint = output.Pose[poseIndex];
					const FQuat spin(FVector::YAxisVector, FMath::DegreesToRadians(-angle));
					joint.SetRotation((joint.GetRotation() * spin).GetNormalized());
				}
			}
			return true;
		}

	private:
		int32 boneIndex = INDEX_NONE;
		float angle = 0.0f;
	};
}

FAnimInstanceProxy* UGSteeringWheelAnimInstance::CreateAnimInstanceProxy()
{
	return new FGSteeringWheelAnimProxy(this);
}

void UGSteeringWheelAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* proxy)
{
	delete proxy;
}
