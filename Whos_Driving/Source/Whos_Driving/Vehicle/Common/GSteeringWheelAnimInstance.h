#pragma once

#include "Animation/AnimInstance.h"
#include "GSteeringWheelAnimInstance.generated.h"

UCLASS(Transient)
class WHOS_DRIVING_API UGSteeringWheelAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	static const FName SteeringBoneName;

protected:
	FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	void DestroyAnimInstanceProxy(FAnimInstanceProxy* proxy) override;
};
