#pragma once

#include "CoreMinimal.h"

struct FGArcadeDriftSettings;

// How the rear axle is being used.
enum class EGArcadeSlideMode : uint8
{
	// Normal driving; the grip assists are in charge.
	Grip,

	// Controlled drift: steering picks the slide angle and how tightly the path bends.
	Drift,

	// The handbrake swings the car round towards the steering side, up to a full turn-around.
	HandbrakeTurn
};

struct FGArcadeDriftInput
{
	// Raw driver input; steering is -1 (left) .. 1 (right).
	float throttle = 0.0f;
	float brake = 0.0f;
	float steering = 0.0f;
	bool bHandbrake = false;

	// Metres per second along the chassis forward axis.
	float forwardSpeed = 0.0f;

	// Metres per second in the chassis tangent plane; independent of its heading in that plane.
	float planarSpeed = 0.0f;

	// Degrees between heading and velocity, positive when the velocity points right of the nose.
	// Measured against the absolute forward speed, so it stays within -90..90.
	float bodySlipAngle = 0.0f;

	// Degrees from the nose to the velocity over the full circle, -180..180; 180 means sliding backwards.
	float travelAngle = 0.0f;

	// Radians per second about the chassis up axis, positive turns right.
	float yawRate = 0.0f;

	bool bIsGrounded = false;
};

// Arcade slide state machine. A brake tap at speed starts a drift towards the steering side; the handbrake
// with steering swings the car round. While drifting, steering picks both the slide angle and how tightly
// the path bends, and the controller asks for the yaw and sideways acceleration needed for both.
class WHOS_DRIVING_API FGArcadeDriftController
{
public:
	void Reset();
	void Update(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings, float deltaTime);

	// True in a drift and in a handbrake turn.
	bool IsDrifting() const
	{
		return mode != EGArcadeSlideMode::Grip;
	}

	EGArcadeSlideMode GetMode() const
	{
		return mode;
	}

	// 0 while gripping, 1 while fully sliding; grip and braking changes follow it.
	float GetDriftBlend() const
	{
		return driftBlend;
	}

	// 1 for a slide to the right, -1 to the left, 0 without one.
	float GetDriftDirection() const
	{
		return driftDirection;
	}

	// Radians per second squared about the chassis up axis, positive turns right.
	float GetYawAcceleration() const
	{
		return yawAcceleration;
	}

	// Metres per second squared at right angles to the velocity, positive to the right; bends the drift path.
	float GetPathAcceleration() const
	{
		return pathAcceleration;
	}

	// Metres per second squared along the velocity, which keeps a drift from bleeding speed.
	float GetSpeedHoldAcceleration() const
	{
		return speedHoldAcceleration;
	}

	// Multiplier of the rear lateral grip.
	float GetRearGripScale() const
	{
		return rearGripScale;
	}

	// Signed body slip angle the drift aims for, degrees.
	float GetTargetSlipAngle() const
	{
		return targetSlipAngle;
	}

	// Drift angle magnitude for a steering input relative to the drift side, -1 (full counter) .. 1 (full into).
	static float ComputeTargetAngle(const FGArcadeDriftSettings& settings, float steeringIntoDrift);

	// Path acceleration magnitude for a steering input relative to the drift side, m/s^2.
	static float ComputePathAcceleration(const FGArcadeDriftSettings& settings, float steeringIntoDrift);

private:
	void UpdateMode(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings, float deltaTime);
	bool ShouldStartDrift(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings) const;
	bool ShouldStartHandbrakeTurn(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings) const;
	bool ShouldEndDrift(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings) const;
	void FinishHandbrakeTurn(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings);
	void EnterMode(EGArcadeSlideMode newMode, float direction, const FGArcadeDriftInput& input);
	void UpdateEntryLatches(const FGArcadeDriftInput& input);
	void UpdateDriftControl(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings, float deltaTime);
	void UpdateHandbrakeTurnControl(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings);
	void UpdateSpeedHold(const FGArcadeDriftInput& input, const FGArcadeDriftSettings& settings);
	void UpdateRearGrip(const FGArcadeDriftSettings& settings);
	void ClearControlOutputs(const FGArcadeDriftInput& input);

	static float ComputeMinimumSlideSpeed(const FGArcadeDriftSettings& settings);

	EGArcadeSlideMode mode = EGArcadeSlideMode::Grip;

	// A held pedal starts at most one slide; it has to be released before the next one.
	bool bIsBrakeLatched = false;
	bool bIsHandbrakeLatched = false;

	float driftDirection = 0.0f;
	float driftBlend = 0.0f;

	// Seconds since the current mode started, and since the wheels last touched the ground.
	float modeTime = 0.0f;
	float airTime = 0.0f;

	// Metres per second the speed hold aims for.
	float entrySpeed = 0.0f;

	// Degrees, for the damping term of the yaw control.
	float previousSlipAngle = 0.0f;
	bool bHasPreviousSlipAngle = false;

	float targetSlipAngle = 0.0f;
	float yawAcceleration = 0.0f;
	float pathAcceleration = 0.0f;
	float speedHoldAcceleration = 0.0f;
	float rearGripScale = 1.0f;
};
