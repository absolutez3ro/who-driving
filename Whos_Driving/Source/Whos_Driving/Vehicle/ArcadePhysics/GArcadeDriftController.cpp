#include "GArcadeDriftController.h"
#include "GArcadeVehicleSettings.h"
#include "GArcadeVehicleTypes.h"

namespace
{
	// Pedal travel that counts as a deliberate press.
	constexpr float pedalPressThreshold = 0.3f;

	// Below this speed a path acceleration would ask for an absurd turning rate, metres per second.
	constexpr float pathMinimumSpeed = 1.0f;
}

void FGArcadeDriftController::Reset()
{
	mode = EGArcadeSlideMode::Grip;
	bIsBrakeLatched = false;
	bIsHandbrakeLatched = false;
	driftDirection = 0.0f;
	driftBlend = 0.0f;
	modeTime = 0.0f;
	airTime = 0.0f;
	entrySpeed = 0.0f;
	previousSlipAngle = 0.0f;
	bHasPreviousSlipAngle = false;
	targetSlipAngle = 0.0f;
	yawAcceleration = 0.0f;
	pathAcceleration = 0.0f;
	speedHoldAcceleration = 0.0f;
	rearGripScale = 1.0f;
}

void FGArcadeDriftController::Update(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings, float deltaTime)
{
	UpdateMode(input, settings, deltaTime);
	UpdateEntryLatches(input);

	const float blendRate = deltaTime / FMath::Max(settings.BlendTime, UE_KINDA_SMALL_NUMBER);
	driftBlend = FMath::Clamp(driftBlend + (IsDrifting() ? blendRate : -blendRate), 0.0f, 1.0f);

	switch (mode)
	{
		case EGArcadeSlideMode::Drift:
			UpdateDriftControl(input, settings, deltaTime);
			break;
		case EGArcadeSlideMode::HandbrakeTurn:
			UpdateHandbrakeTurnControl(input, settings);
			break;
		case EGArcadeSlideMode::Grip:
		default:
			ClearControlOutputs(input);
			break;
	}
	UpdateSpeedHold(input, settings);
	UpdateRearGrip(settings);
}

float FGArcadeDriftController::ComputeTargetAngle(const FGArcadeDriftSettings& settings, float steeringIntoDrift)
{
	const float steering = FMath::Clamp(steeringIntoDrift, -1.0f, 1.0f);
	if (steering >= 0.0f)
	{
		return FMath::Lerp(settings.BaseAngle, settings.MaxAngle, steering);
	}
	return FMath::Lerp(settings.BaseAngle, 0.0f, -steering);
}

float FGArcadeDriftController::ComputePathAcceleration(const FGArcadeDriftSettings& settings, float steeringIntoDrift)
{
	const float steering = FMath::Clamp(steeringIntoDrift, -1.0f, 1.0f);
	if (steering >= 0.0f)
	{
		return FMath::Lerp(settings.PathAcceleration, settings.PathAccelerationIntoDrift, steering);
	}
	return FMath::Lerp(settings.PathAcceleration, settings.PathAccelerationCounterSteer, -steering);
}

void FGArcadeDriftController::UpdateMode(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings, float deltaTime)
{
	modeTime += deltaTime;
	airTime = input.bIsGrounded ? 0.0f : airTime + deltaTime;
	const float steeringSide = FMath::Sign(input.steering);

	switch (mode)
	{
		case EGArcadeSlideMode::Grip:
			if (ShouldStartHandbrakeTurn(input, settings))
			{
				EnterMode(EGArcadeSlideMode::HandbrakeTurn, steeringSide, input);
			}
			else if (ShouldStartDrift(input, settings))
			{
				EnterMode(EGArcadeSlideMode::Drift, steeringSide, input);
			}
			break;
		case EGArcadeSlideMode::Drift:
			if (ShouldStartHandbrakeTurn(input, settings))
			{
				EnterMode(EGArcadeSlideMode::HandbrakeTurn, steeringSide, input);
			}
			else if (ShouldEndDrift(input, settings))
			{
				EnterMode(EGArcadeSlideMode::Grip, 0.0f, input);
			}
			break;
		case EGArcadeSlideMode::HandbrakeTurn:
		{
			const bool bIsTooSlow = input.planarSpeed * GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond < ComputeMinimumSlideSpeed(settings);
			const bool bHasBeenAirborneTooLong = airTime > settings.MaxAirTime;
			if (!input.bHandbrake || bIsTooSlow || bHasBeenAirborneTooLong)
			{
				FinishHandbrakeTurn(input, settings);
			}
			break;
		}
		default:
			break;
	}
}

bool FGArcadeDriftController::ShouldStartDrift(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings) const
{
	const bool bIsSteering = FMath::Abs(input.steering) >= settings.EntrySteering;
	if (!settings.bEnableBrakeDrift || !input.bIsGrounded || !bIsSteering)
	{
		return false;
	}

	const float speedKilometersPerHour = input.forwardSpeed * GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond;
	const bool bIsBrakeTap = input.brake > pedalPressThreshold && !bIsBrakeLatched && input.throttle < settings.EntryThrottleLimit;
	return bIsBrakeTap && speedKilometersPerHour >= settings.BrakeDriftMinimumSpeed;
}

bool FGArcadeDriftController::ShouldStartHandbrakeTurn(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings) const
{
	const bool bIsSteering = FMath::Abs(input.steering) >= settings.EntrySteering;
	const bool bIsHandbrakePull = input.bHandbrake && !bIsHandbrakeLatched;
	if (!settings.bEnableHandbrakeDrift || !input.bIsGrounded || !bIsSteering || !bIsHandbrakePull)
	{
		return false;
	}
	const float speedKilometersPerHour = input.forwardSpeed * GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond;
	return speedKilometersPerHour >= settings.HandbrakeDriftMinimumSpeed;
}

bool FGArcadeDriftController::ShouldEndDrift(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings) const
{
	const float speedKilometersPerHour = input.forwardSpeed * GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond;
	const bool bIsTooSlow = speedKilometersPerHour < ComputeMinimumSlideSpeed(settings);
	const bool bHasStraightened = modeTime >= settings.MinimumDuration && FMath::Abs(input.bodySlipAngle) < settings.ExitAngle;
	const bool bHasBeenAirborneTooLong = airTime > settings.MaxAirTime;
	return bIsTooSlow || bHasStraightened || bHasBeenAirborneTooLong;
}

void FGArcadeDriftController::FinishHandbrakeTurn(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings)
{
	// A shallow handbrake slide flows into a drift; a car that has swung round just grips again and drives off.
	const float slideAngle = FMath::Abs(input.travelAngle);
	const float speedKilometersPerHour = input.forwardSpeed * GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond;
	const bool bIsSliding = slideAngle >= settings.ExitAngle && slideAngle <= settings.HandbrakeTurnHandoverAngle;
	const bool bIsFastEnough = speedKilometersPerHour >= ComputeMinimumSlideSpeed(settings);
	if (input.bIsGrounded && bIsSliding && bIsFastEnough)
	{
		// The slide side is opposite to where the velocity points relative to the nose.
		EnterMode(EGArcadeSlideMode::Drift, -FMath::Sign(input.travelAngle), input);
		return;
	}
	EnterMode(EGArcadeSlideMode::Grip, 0.0f, input);
}

void FGArcadeDriftController::EnterMode(EGArcadeSlideMode newMode, float direction, const FGArcadeDriftInput& input)
{
	mode = newMode;
	driftDirection = newMode == EGArcadeSlideMode::Grip ? 0.0f : direction;
	modeTime = 0.0f;
	airTime = 0.0f;
	entrySpeed = input.planarSpeed;
	bHasPreviousSlipAngle = false;
	if (newMode != EGArcadeSlideMode::Grip)
	{
		bIsBrakeLatched = bIsBrakeLatched || input.brake > pedalPressThreshold;
		bIsHandbrakeLatched = bIsHandbrakeLatched || input.bHandbrake;
	}
}

void FGArcadeDriftController::UpdateEntryLatches(const FGArcadeDriftInput& input)
{
	if (input.brake <= pedalPressThreshold)
	{
		bIsBrakeLatched = false;
	}
	if (!input.bHandbrake)
	{
		bIsHandbrakeLatched = false;
	}
}

void FGArcadeDriftController::UpdateDriftControl(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings, float deltaTime)
{
	if (!input.bIsGrounded)
	{
		ClearControlOutputs(input);
		return;
	}

	// Drifting right puts the nose right of the velocity, which is a negative body slip angle.
	const float steeringIntoDrift = input.steering * driftDirection;
	targetSlipAngle = -driftDirection * ComputeTargetAngle(settings, steeringIntoDrift);

	// The path channel bends the velocity through the corner; the yaw channel keeps the nose ahead of it.
	pathAcceleration = driftDirection * ComputePathAcceleration(settings, steeringIntoDrift) * driftBlend;
	const float pathYawRate = pathAcceleration / FMath::Max(input.planarSpeed, pathMinimumSpeed);

	// More yaw towards the drift side lowers the slip angle, so the error and its rate both feed yaw directly:
	// slip'' = -yaw'' turns this into a damped spring around the target angle. The rate term adds the turning
	// rate the path asks for, which the spring alone would only reach with a lag.
	const float angleError = FMath::DegreesToRadians(input.bodySlipAngle - targetSlipAngle);
	const float slipRate = bHasPreviousSlipAngle && deltaTime > UE_KINDA_SMALL_NUMBER
		? FMath::DegreesToRadians(input.bodySlipAngle - previousSlipAngle) / deltaTime
		: 0.0f;
	const float angleControl = settings.AngleStiffness * angleError + settings.AngleDamping * slipRate;
	const float rateControl = settings.YawRateMatchStrength * (pathYawRate - input.yawRate);
	yawAcceleration = FMath::Clamp(angleControl + rateControl, -settings.MaxYawAcceleration, settings.MaxYawAcceleration);
	previousSlipAngle = input.bodySlipAngle;
	bHasPreviousSlipAngle = true;
}

void FGArcadeDriftController::UpdateHandbrakeTurnControl(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings)
{
	targetSlipAngle = 0.0f;
	pathAcceleration = 0.0f;
	previousSlipAngle = input.bodySlipAngle;
	bHasPreviousSlipAngle = false;
	if (!input.bIsGrounded)
	{
		yawAcceleration = 0.0f;
		return;
	}

	// The steering sets the spin rate, so letting go of it stops the rotation. Once the car faces back the way
	// it came, the rotation stops as well and the throttle drives it out of the turn.
	const bool bHasTurnedAround = FMath::Abs(input.travelAngle) >= settings.HandbrakeTurnStopAngle;
	const float requestedYawRate = input.steering * FMath::DegreesToRadians(settings.HandbrakeTurnYawRate);
	const float targetYawRate = bHasTurnedAround ? 0.0f : requestedYawRate;
	const float requestedAcceleration = (targetYawRate - input.yawRate) * settings.HandbrakeTurnResponse;
	yawAcceleration = FMath::Clamp(requestedAcceleration, -settings.HandbrakeTurnMaxYawAcceleration, settings.HandbrakeTurnMaxYawAcceleration);
}

void FGArcadeDriftController::UpdateSpeedHold(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings)
{
	const bool bIsRequestingDrive = input.throttle > pedalPressThreshold && input.brake <= pedalPressThreshold && !input.bHandbrake;
	if (mode != EGArcadeSlideMode::Drift || !input.bIsGrounded || !bIsRequestingDrive)
	{
		speedHoldAcceleration = 0.0f;
		// Coasting or braking must not build a speed debt that is repaid on the next throttle press.
		entrySpeed = input.planarSpeed;
		return;
	}
	entrySpeed = FMath::Max(entrySpeed, input.planarSpeed);
	const float lostSpeed = FMath::Max(entrySpeed - input.planarSpeed, 0.0f);
	speedHoldAcceleration = FMath::Min(lostSpeed * settings.SpeedHoldGain, settings.SpeedHoldMaxAcceleration);
}

void FGArcadeDriftController::UpdateRearGrip(const FGArcadeDriftSettings& settings)
{
	const float slideGrip = mode == EGArcadeSlideMode::HandbrakeTurn ? settings.HandbrakeTurnRearGripMultiplier : settings.RearGripMultiplier;
	rearGripScale = FMath::Lerp(1.0f, slideGrip, driftBlend);
}

void FGArcadeDriftController::ClearControlOutputs(const FGArcadeDriftInput& input)
{
	yawAcceleration = 0.0f;
	pathAcceleration = 0.0f;
	targetSlipAngle = 0.0f;
	previousSlipAngle = input.bodySlipAngle;
	bHasPreviousSlipAngle = false;
}

float FGArcadeDriftController::ComputeMinimumSlideSpeed(const FGArcadeDriftSettings& settings)
{
	return FMath::Min(settings.BrakeDriftMinimumSpeed, settings.HandbrakeDriftMinimumSpeed) * settings.ExitSpeedRatio;
}
