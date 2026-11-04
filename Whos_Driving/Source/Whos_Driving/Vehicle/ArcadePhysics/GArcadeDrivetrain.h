#pragma once

#include "CoreMinimal.h"

struct FGArcadeEngineSettings;
struct FGArcadeTransmissionSettings;

struct FGArcadeDrivetrainInput
{
	// Raw pedal values, 0..1. Their meaning swaps while reversing.
	float throttle = 0.0f;
	float brake = 0.0f;

	// Radians per second, torque-share weighted average of the driven wheels.
	float drivenWheelAngularVelocity = 0.0f;

	// Radians per second the driven wheels would turn at the current road speed; spinning wheels do not raise it.
	float roadWheelAngularVelocity = 0.0f;

	// Metres per second along the chassis forward axis.
	float forwardSpeed = 0.0f;

	// Gameplay modifier of the engine torque, for example from engine damage.
	float torqueMultiplier = 1.0f;

	bool bHasGroundContact = false;
};

// Engine, automatic gearbox and pedal mapping. Gear -1 is reverse, 1..N are forward gears.
class WHOS_DRIVING_API FGArcadeDrivetrain
{
public:
	void Reset();

	void Update(const FGArcadeDrivetrainInput& input, const FGArcadeEngineSettings& engine,
				const FGArcadeTransmissionSettings& transmission, float deltaTime);

	// Newton metres summed over the driven wheels, signed in wheel rotation direction.
	float GetWheelTorque() const
	{
		return wheelTorque;
	}

	// Service brake request after pedal mapping, 0..1.
	float GetBrakeDemand() const
	{
		return brakeDemand;
	}

	// Drive request after pedal mapping, 0..1.
	float GetDriveDemand() const
	{
		return driveDemand;
	}

	float GetEngineRpm() const
	{
		return engineRpm;
	}

	int32 GetCurrentGear() const
	{
		return currentGear;
	}

	bool IsShifting() const
	{
		return shiftTimeRemaining > 0.0f;
	}

	// Newton metres at the crankshaft for full throttle.
	static float EvaluateEngineTorque(const FGArcadeEngineSettings& engine, float rpm);

	// Signed overall ratio including the final drive; zero for neutral or a missing gear.
	static float GetGearRatio(const FGArcadeTransmissionSettings& transmission, int32 gear);

private:
	void UpdateDirection(const FGArcadeDrivetrainInput& input, const FGArcadeTransmissionSettings& transmission,
						 float deltaTime);
	void UpdateEngineRpm(const FGArcadeDrivetrainInput& input, const FGArcadeEngineSettings& engine,
						 const FGArcadeTransmissionSettings& transmission);
	void UpdateAutomaticShifting(const FGArcadeDrivetrainInput& input, const FGArcadeTransmissionSettings& transmission,
								 float deltaTime);
	void UpdateWheelTorque(const FGArcadeDrivetrainInput& input, const FGArcadeEngineSettings& engine,
						   const FGArcadeTransmissionSettings& transmission);
	int32 FindGearForRoadSpeed(const FGArcadeDrivetrainInput& input, const FGArcadeTransmissionSettings& transmission) const;
	void BeginShift(int32 gear, const FGArcadeTransmissionSettings& transmission);
	void SelectGearImmediately(int32 gear);

	static float ComputeRoadRpm(const FGArcadeDrivetrainInput& input, const FGArcadeTransmissionSettings& transmission, int32 gear);

	int32 currentGear = 1;
	int32 pendingGear = 1;
	float shiftTimeRemaining = 0.0f;

	// Seconds the driver has been asking for reverse at low speed.
	float reverseRequestTime = 0.0f;

	float engineRpm = 0.0f;
	float driveDemand = 0.0f;
	float brakeDemand = 0.0f;
	float wheelTorque = 0.0f;
};
