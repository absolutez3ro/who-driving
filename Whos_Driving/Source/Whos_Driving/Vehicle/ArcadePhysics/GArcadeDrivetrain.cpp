#include "GArcadeDrivetrain.h"
#include "GArcadeVehicleSettings.h"
#include "GArcadeVehicleTypes.h"
#include "Curves/CurveFloat.h"

namespace
{
	// Pedal travel that counts as a deliberate press.
	constexpr float drivetrainPedalPressThreshold = 0.1f;
}

void FGArcadeDrivetrain::Reset()
{
	currentGear = 1;
	pendingGear = 1;
	shiftTimeRemaining = 0.0f;
	reverseRequestTime = 0.0f;
	engineRpm = 0.0f;
	driveDemand = 0.0f;
	brakeDemand = 0.0f;
	wheelTorque = 0.0f;
}

void FGArcadeDrivetrain::Update(const FGArcadeDrivetrainInput& input, const FGArcadeEngineSettings& engine,
								const FGArcadeTransmissionSettings& transmission, float deltaTime)
{
	UpdateDirection(input, transmission, deltaTime);
	UpdateEngineRpm(input, engine, transmission);
	UpdateAutomaticShifting(input, transmission, deltaTime);
	UpdateWheelTorque(input, engine, transmission);
}

float FGArcadeDrivetrain::EvaluateEngineTorque(const FGArcadeEngineSettings& engine, float rpm)
{
	if (engine.TorqueCurve)
	{
		return FMath::Max(engine.TorqueCurve->GetFloatValue(rpm), 0.0f);
	}

	const float rpmRange = FMath::Max(engine.RedlineRpm - engine.IdleRpm, 1.0f);
	const float curveTime = FMath::Clamp((rpm - engine.IdleRpm) / rpmRange, 0.0f, 1.0f);
	const float remainingTime = 1.0f - curveTime;
	const float normalizedTorque = remainingTime * remainingTime * engine.TorqueCurveStart
		+ 2.0f * remainingTime * curveTime * engine.TorqueCurveMiddle + curveTime * curveTime * engine.TorqueCurveEnd;
	return engine.PeakTorque * FMath::Max(normalizedTorque, 0.0f);
}

float FGArcadeDrivetrain::GetGearRatio(const FGArcadeTransmissionSettings& transmission, int32 gear)
{
	if (gear < 0)
	{
		return -transmission.ReverseGearRatio * transmission.FinalDriveRatio;
	}
	if (gear == 0 || !transmission.ForwardGearRatios.IsValidIndex(gear - 1))
	{
		return 0.0f;
	}
	return transmission.ForwardGearRatios[gear - 1] * transmission.FinalDriveRatio;
}

void FGArcadeDrivetrain::UpdateDirection(const FGArcadeDrivetrainInput& input,
										 const FGArcadeTransmissionSettings& transmission, float deltaTime)
{
	const bool bIsNearlyStopped = FMath::Abs(input.forwardSpeed) < transmission.ReverseEngageSpeed;
	const bool bIsThrottlePressed = input.throttle > drivetrainPedalPressThreshold;
	const bool bIsBrakePressed = input.brake > drivetrainPedalPressThreshold;
	const bool bIsReversing = currentGear < 0;
	const bool bWantsReverse = !bIsReversing && bIsBrakePressed && !bIsThrottlePressed;
	const bool bWantsForward = bIsReversing && bIsThrottlePressed && !bIsBrakePressed;

	// Throttle always means "go": leaving reverse must not wait, or the car sits still after a spin.
	if (bIsNearlyStopped && bWantsForward)
	{
		SelectGearImmediately(1);
	}

	reverseRequestTime = bIsNearlyStopped && bWantsReverse ? reverseRequestTime + deltaTime : 0.0f;
	if (bWantsReverse && reverseRequestTime > 0.0f && reverseRequestTime >= transmission.ReverseEngageDelay)
	{
		SelectGearImmediately(-1);
		reverseRequestTime = 0.0f;
	}

	// In reverse the brake pedal drives and the throttle pedal brakes.
	const bool bIsReversingNow = currentGear < 0;
	driveDemand = FMath::Clamp(bIsReversingNow ? input.brake : input.throttle, 0.0f, 1.0f);
	brakeDemand = FMath::Clamp(bIsReversingNow ? input.throttle : input.brake, 0.0f, 1.0f);
}

void FGArcadeDrivetrain::UpdateEngineRpm(const FGArcadeDrivetrainInput& input, const FGArcadeEngineSettings& engine,
										 const FGArcadeTransmissionSettings& transmission)
{
	const float ratio = GetGearRatio(transmission, currentGear);
	const float wheelDrivenRpm =
		FMath::Abs(input.drivenWheelAngularVelocity * ratio) * GArcadeVehicleUnits::rpmPerRadianPerSecond;
	engineRpm = FMath::Clamp(wheelDrivenRpm, engine.IdleRpm, engine.RedlineRpm);
}

void FGArcadeDrivetrain::UpdateAutomaticShifting(const FGArcadeDrivetrainInput& input,
												 const FGArcadeTransmissionSettings& transmission, float deltaTime)
{
	if (IsShifting())
	{
		shiftTimeRemaining -= deltaTime;
		if (shiftTimeRemaining <= 0.0f)
		{
			SelectGearImmediately(pendingGear);
		}
		return;
	}
	if (currentGear < 1)
	{
		return;
	}

	// Drop straight to the gear that suits the road speed, for example after a spin or a hard stop.
	const int32 roadSpeedGear = FindGearForRoadSpeed(input, transmission);
	if (roadSpeedGear < currentGear)
	{
		BeginShift(roadSpeedGear, transmission);
		return;
	}

	// Spinning wheels alone push the engine rpm up, so the road speed has to agree before an upshift.
	const int32 gearCount = transmission.ForwardGearRatios.Num();
	const bool bCanUpshift = currentGear < gearCount && input.bHasGroundContact && driveDemand > drivetrainPedalPressThreshold;
	const bool bIsRoadFastEnough = ComputeRoadRpm(input, transmission, currentGear) >= transmission.UpshiftRpm * transmission.UpshiftRoadSpeedRatio;
	if (bCanUpshift && bIsRoadFastEnough && engineRpm >= transmission.UpshiftRpm)
	{
		BeginShift(currentGear + 1, transmission);
	}
}

int32 FGArcadeDrivetrain::FindGearForRoadSpeed(const FGArcadeDrivetrainInput& input, const FGArcadeTransmissionSettings& transmission) const
{
	// Step down while the current gear lugs below the downshift point and the lower one would not over-rev.
	int32 gear = currentGear;
	while (gear > 1)
	{
		const bool bIsLugging = ComputeRoadRpm(input, transmission, gear) <= transmission.DownshiftRpm;
		const bool bLowerGearFits = ComputeRoadRpm(input, transmission, gear - 1) < transmission.UpshiftRpm;
		if (!bIsLugging || !bLowerGearFits)
		{
			break;
		}
		--gear;
	}
	return gear;
}

float FGArcadeDrivetrain::ComputeRoadRpm(const FGArcadeDrivetrainInput& input, const FGArcadeTransmissionSettings& transmission, int32 gear)
{
	return FMath::Abs(input.roadWheelAngularVelocity * GetGearRatio(transmission, gear)) * GArcadeVehicleUnits::rpmPerRadianPerSecond;
}

void FGArcadeDrivetrain::UpdateWheelTorque(const FGArcadeDrivetrainInput& input, const FGArcadeEngineSettings& engine,
										   const FGArcadeTransmissionSettings& transmission)
{
	const float ratio = GetGearRatio(transmission, currentGear);
	if (IsShifting() || FMath::IsNearlyZero(ratio))
	{
		wheelTorque = 0.0f;
		return;
	}

	if (driveDemand <= UE_KINDA_SMALL_NUMBER)
	{
		const float rpmRange = FMath::Max(engine.RedlineRpm - engine.IdleRpm, 1.0f);
		const float rpmAlpha = FMath::Clamp((engineRpm - engine.IdleRpm) / rpmRange, 0.0f, 1.0f);
		const float brakingTorque = engine.EngineBrakeTorque * rpmAlpha * FMath::Abs(ratio);
		wheelTorque = -FMath::Sign(input.drivenWheelAngularVelocity) * brakingTorque;
		return;
	}

	const float limiterRange = FMath::Max(engine.RevLimiterRange, 1.0f);
	const float limiterScale = FMath::Clamp((engine.RedlineRpm - engineRpm) / limiterRange, 0.0f, 1.0f);
	const float engineTorque = EvaluateEngineTorque(engine, engineRpm) * input.torqueMultiplier * driveDemand * limiterScale;
	wheelTorque = engineTorque * ratio * transmission.Efficiency;
}

void FGArcadeDrivetrain::BeginShift(int32 gear, const FGArcadeTransmissionSettings& transmission)
{
	if (transmission.ShiftDuration <= 0.0f)
	{
		SelectGearImmediately(gear);
		return;
	}
	pendingGear = gear;
	shiftTimeRemaining = transmission.ShiftDuration;
}

void FGArcadeDrivetrain::SelectGearImmediately(int32 gear)
{
	currentGear = gear;
	pendingGear = gear;
	shiftTimeRemaining = 0.0f;
}
