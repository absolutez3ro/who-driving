#include "GArcadeDrivetrain.h"
#include "GArcadeVehicleSettings.h"
#include "GArcadeVehicleTypes.h"
#include "Curves/CurveFloat.h"

namespace
{
	// Pedal travel that counts as a deliberate press.
	constexpr float pedalPressThreshold = 0.1f;
}

void FGArcadeDrivetrain::Reset()
{
	currentGear = 1;
	pendingGear = 1;
	shiftTimeRemaining = 0.0f;
	directionRequestTime = 0.0f;
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
	const bool bIsThrottlePressed = input.throttle > pedalPressThreshold;
	const bool bIsBrakePressed = input.brake > pedalPressThreshold;
	const bool bIsReversing = currentGear < 0;
	const bool bWantsReverse = !bIsReversing && bIsBrakePressed && !bIsThrottlePressed;
	const bool bWantsForward = bIsReversing && bIsThrottlePressed && !bIsBrakePressed;

	if (bIsNearlyStopped && (bWantsReverse || bWantsForward))
	{
		directionRequestTime += deltaTime;
	}
	else
	{
		directionRequestTime = 0.0f;
	}

	if (directionRequestTime >= transmission.ReverseEngageDelay && directionRequestTime > 0.0f)
	{
		SelectGearImmediately(bWantsReverse ? -1 : 1);
		directionRequestTime = 0.0f;
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

	const int32 gearCount = transmission.ForwardGearRatios.Num();
	const bool bCanUpshift = currentGear < gearCount && input.bHasGroundContact && driveDemand > pedalPressThreshold;
	if (bCanUpshift && engineRpm >= transmission.UpshiftRpm)
	{
		BeginShift(currentGear + 1, transmission);
		return;
	}
	if (currentGear > 1 && engineRpm <= transmission.DownshiftRpm)
	{
		// Skip a downshift that would land straight on the upshift point and start gear hunting.
		const float currentRatio = FMath::Max(FMath::Abs(GetGearRatio(transmission, currentGear)), UE_KINDA_SMALL_NUMBER);
		const float lowerRatio = FMath::Abs(GetGearRatio(transmission, currentGear - 1));
		const float rpmAfterShift = engineRpm * lowerRatio / currentRatio;
		if (rpmAfterShift < transmission.UpshiftRpm)
		{
			BeginShift(currentGear - 1, transmission);
		}
	}
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
