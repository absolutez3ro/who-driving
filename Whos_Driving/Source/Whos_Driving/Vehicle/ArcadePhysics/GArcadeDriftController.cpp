#include "GArcadeDriftController.h"
#include "GArcadeVehicleSettings.h"
#include "GArcadeVehicleTypes.h"

namespace
{
	// Pedal travel that counts as a deliberate press.
	constexpr float pedalPressThreshold = 0.3f;

	// Below this speed a path acceleration would ask for an absurd turning rate, metres per second.
	constexpr float pathMinimumSpeed = 1.0f;

	// Keep enough rear cornering force to arrest a slide without removing the brake-to-drift mechanic.
	constexpr float minimumControlledRearGrip = 0.75f;

	// Time for yaw and path control to build up after a brake tap or handbrake handover.
	constexpr float minimumDriftEntryTime = 0.26f;
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

	const float blendTime = settings.bUsePhysicalSlides && mode == EGArcadeSlideMode::Drift
		? FMath::Max(settings.BlendTime, minimumDriftEntryTime)
		: FMath::Max(settings.BlendTime, UE_KINDA_SMALL_NUMBER);
	const float forwardSpeedKilometersPerHour = input.forwardSpeed * GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond;
	const float driftExitSpeed = settings.BrakeDriftMinimumSpeed * settings.ExitSpeedRatio;
	const float fullDriftSpeed = FMath::Max(settings.BrakeDriftMinimumSpeed, driftExitSpeed + UE_KINDA_SMALL_NUMBER);
	const float driftSpeedBlend = FMath::SmoothStep(driftExitSpeed, fullDriftSpeed, forwardSpeedKilometersPerHour);
	const float targetBlend = mode == EGArcadeSlideMode::Drift ? driftSpeedBlend : (IsDrifting() ? 1.0f : 0.0f);
	driftBlend = FMath::FInterpConstantTo(driftBlend, targetBlend, deltaTime, 1.0f / blendTime);

	switch (mode)
	{
		case EGArcadeSlideMode::Drift:
			UpdateDriftControl(input, settings, deltaTime);
			yawAcceleration *= driftSpeedBlend;
			pathAcceleration *= driftSpeedBlend;
			break;
		case EGArcadeSlideMode::HandbrakeTurn:
			if (settings.bUsePhysicalSlides)
			{
				ClearControlOutputs(input);
			}
			else
			{
				UpdateHandbrakeTurnControl(input, settings);
			}
			break;
		case EGArcadeSlideMode::Grip:
		default:
			ClearControlOutputs(input);
			break;
	}
	UpdateSpeedHold(input, settings);
	speedHoldAcceleration *= driftSpeedBlend;
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
			const float minimumTurnSpeed = settings.HandbrakeDriftMinimumSpeed * settings.ExitSpeedRatio;
			const bool bIsTooSlow = input.planarSpeed * GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond <= minimumTurnSpeed;
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
	const float minimumDriftSpeed = settings.BrakeDriftMinimumSpeed * settings.ExitSpeedRatio;
	const bool bIsTooSlow = speedKilometersPerHour <= minimumDriftSpeed;
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
	const bool bIsFastEnough = speedKilometersPerHour > settings.BrakeDriftMinimumSpeed * settings.ExitSpeedRatio;
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
	// A brake press used during a slide must be released before it can start the next one.
	const bool bIsSlideTransition = mode != EGArcadeSlideMode::Grip || newMode != EGArcadeSlideMode::Grip;
	bIsBrakeLatched = bIsBrakeLatched || (bIsSlideTransition && input.brake > pedalPressThreshold);
	mode = newMode;
	driftDirection = newMode == EGArcadeSlideMode::Grip ? 0.0f : direction;
	modeTime = 0.0f;
	airTime = 0.0f;
	entrySpeed = input.planarSpeed;
	bHasPreviousSlipAngle = false;
	if (newMode != EGArcadeSlideMode::Grip)
	{
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

	// Ramp both controls together, even if a preceding handbrake slide already faded the grip assists.
	const float entryTime = FMath::Max(settings.BlendTime, minimumDriftEntryTime);
	const float entryBlend = settings.bUsePhysicalSlides ? FMath::SmoothStep(0.0f, entryTime, modeTime + deltaTime) : 1.0f;
	const float pathBlend = FMath::Min(driftBlend, entryBlend);
	pathAcceleration = driftDirection * ComputePathAcceleration(settings, steeringIntoDrift) * pathBlend;
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
	yawAcceleration = FMath::Clamp(angleControl + rateControl, -settings.MaxYawAcceleration, settings.MaxYawAcceleration) * entryBlend;
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
	if (settings.bUsePhysicalSlides || mode != EGArcadeSlideMode::Drift || !input.bIsGrounded || !bIsRequestingDrive)
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
	float slideGrip = mode == EGArcadeSlideMode::HandbrakeTurn ? settings.HandbrakeTurnRearGripMultiplier : settings.RearGripMultiplier;
	if (settings.bUsePhysicalSlides)
	{
		// Locked-wheel friction already handles the handbrake. A controlled brake drift only needs
		// a modest rear grip reduction; the yaw and path controller still initiates and steers it.
		slideGrip = mode == EGArcadeSlideMode::HandbrakeTurn
			? 1.0f
			: FMath::Clamp(settings.RearGripMultiplier, minimumControlledRearGrip, 1.0f);
	}
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
