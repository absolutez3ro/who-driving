#include "GArcadeTireModel.h"
#include "GArcadeVehicleSettings.h"

float FGArcadeTireModel::EvaluateCurve(const FGArcadeTireCurve& curve, float slip)
{
	const float slipMagnitude = FMath::Abs(slip);
	const float peakSlip = FMath::Max(curve.PeakSlip, UE_KINDA_SMALL_NUMBER);
	const float floorSlip = FMath::Max(curve.FloorSlip, peakSlip + UE_KINDA_SMALL_NUMBER);

	float response = curve.FloorValue;
	if (slipMagnitude <= peakSlip)
	{
		response = curve.PeakValue * slipMagnitude / peakSlip;
	}
	else if (slipMagnitude < floorSlip)
	{
		const float fallAlpha = (slipMagnitude - peakSlip) / (floorSlip - peakSlip);
		response = FMath::Lerp(curve.PeakValue, curve.FloorValue, fallAlpha);
	}
	return FMath::Sign(slip) * response;
}

float FGArcadeTireModel::ComputeSlipRatio(float wheelSurfaceSpeed, float longitudinalSpeed, float referenceSpeed)
{
	const float denominator = FMath::Max(FMath::Abs(longitudinalSpeed), referenceSpeed);
	return (wheelSurfaceSpeed - longitudinalSpeed) / denominator;
}

float FGArcadeTireModel::ComputeSlipAngle(float lateralSpeed, float longitudinalSpeed, float referenceSpeed)
{
	const float denominator = FMath::Max(FMath::Abs(longitudinalSpeed), referenceSpeed);
	return FMath::RadiansToDegrees(FMath::Atan2(lateralSpeed, denominator));
}

float FGArcadeTireModel::ComputeSecantStiffness(const FGArcadeTireCurve& curve, float slip, float frictionLimit)
{
	const float peakSlip = FMath::Max(curve.PeakSlip, UE_KINDA_SMALL_NUMBER);
	const float slipMagnitude = FMath::Abs(slip);
	if (slipMagnitude <= peakSlip)
	{
		return curve.PeakValue / peakSlip * frictionLimit;
	}
	return FMath::Abs(EvaluateCurve(curve, slip)) * frictionLimit / slipMagnitude;
}

FVector2D FGArcadeTireModel::LimitToFrictionCircle(const FVector2D& force, float frictionLimit, float lateralSuppression)
{
	if (frictionLimit <= 0.0f)
	{
		return FVector2D::ZeroVector;
	}
	if (force.SizeSquared() <= FMath::Square(frictionLimit))
	{
		return force;
	}

	const FVector2D suppressedForce(force.X, force.Y * (1.0f - lateralSuppression));
	const float suppressedMagnitude = suppressedForce.Size();
	if (suppressedMagnitude <= UE_KINDA_SMALL_NUMBER)
	{
		return FVector2D::ZeroVector;
	}
	return suppressedForce * (frictionLimit / suppressedMagnitude);
}

float FGArcadeTireModel::IntegrateWheelSpin(const FGArcadeWheelSpinInput& input)
{
	// Linearised tire force F = stiffness * (R * omega - v) / vRef, solved for the end of the step:
	// omega' = omega + dt / I * (drive - R * F(omega')).
	const float inertia = FMath::Max(input.inertia, UE_KINDA_SMALL_NUMBER);
	const float referenceSpeed = FMath::Max(input.referenceSpeed, UE_KINDA_SMALL_NUMBER);
	const float stepPerInertia = input.deltaTime / inertia;
	const float couplingRate = stepPerInertia * FMath::Square(input.radius) * input.tireStiffness / referenceSpeed;
	const float groundTorque = input.radius * input.tireStiffness * input.longitudinalSpeed / referenceSpeed;
	const float denominator = 1.0f + couplingRate;
	const float unbrakedVelocity = (input.angularVelocity + stepPerInertia * (input.driveTorque + groundTorque)) / denominator;

	// Brake torque only removes rotation; crossing zero would mean the brake drives the wheel backwards.
	const float brakeVelocityChange = stepPerInertia * FMath::Max(input.brakeTorque, 0.0f) / denominator;
	if (FMath::Abs(unbrakedVelocity) <= brakeVelocityChange)
	{
		return 0.0f;
	}
	return unbrakedVelocity - FMath::Sign(unbrakedVelocity) * brakeVelocityChange;
}
