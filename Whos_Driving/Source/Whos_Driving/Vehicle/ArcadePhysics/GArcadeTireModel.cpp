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

	// Blend suppression beyond the boundary: a discontinuity here can leave the implicit
	// wheel equation without a solution and produce an abrupt change of heading.
	const float overload = FMath::Clamp(static_cast<float>(force.Size()) / frictionLimit - 1.0f, 0.0f, 1.0f);
	const float suppression = FMath::Clamp(lateralSuppression, 0.0f, 1.0f) * overload;
	const FVector2D suppressedForce(force.X, force.Y * (1.0f - suppression));
	const float suppressedMagnitude = suppressedForce.Size();
	if (suppressedMagnitude <= UE_KINDA_SMALL_NUMBER)
	{
		return FVector2D::ZeroVector;
	}
	return suppressedForce * FMath::Min(frictionLimit / suppressedMagnitude, 1.0f);
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

FGArcadeTireContactResult FGArcadeTireModel::SolveContact(const FGArcadeTireContactInput& input, const FGArcadeTireCurve& curve)
{
	FGArcadeTireContactResult result;
	result.angularVelocity = input.spin.angularVelocity;
	if (input.spin.deltaTime <= 0.0f)
	{
		return result;
	}

	const double inertia = FMath::Max(input.spin.inertia, UE_KINDA_SMALL_NUMBER);
	const double stepPerInertia = input.spin.deltaTime / inertia;
	const double radius = FMath::Max(input.spin.radius, 0.0f);
	const double freeVelocity = input.spin.angularVelocity + stepPerInertia * input.spin.driveTorque;
	const double brakeStep = stepPerInertia * FMath::Max(input.spin.brakeTorque, 0.0f);
	const float frictionLimit = FMath::Max(input.frictionLimit, 0.0f);
	const float referenceSpeed = FMath::Max(input.spin.referenceSpeed, UE_KINDA_SMALL_NUMBER);
	const float maxBrakingForce = FMath::Max(input.maxBrakingForce, 0.0f);

	const auto applyBrake = [brakeStep](double angularVelocity)
	{
		return FMath::Sign(angularVelocity) * FMath::Max(FMath::Abs(angularVelocity) - brakeStep, 0.0);
	};
	const auto evaluateForce = [&](double angularVelocity)
	{
		const float slipRatio = static_cast<float>((radius * angularVelocity - input.spin.longitudinalSpeed) / referenceSpeed);
		float longitudinalForce = EvaluateCurve(curve, slipRatio) * frictionLimit;
		if (longitudinalForce * input.spin.longitudinalSpeed < 0.0f)
		{
			longitudinalForce = FMath::Clamp(longitudinalForce, -maxBrakingForce, maxBrakingForce);
		}
		return FVector2D(longitudinalForce, input.lateralForce);
	};
	const auto limitForce = [&](double angularVelocity)
	{
		return LimitToFrictionCircle(evaluateForce(angularVelocity), frictionLimit, input.lateralSuppression);
	};

	// Any admissible tire reaction lies within +/- frictionLimit. Braking is monotone,
	// so these bounds enclose a root even on the falling part of the tire curve.
	const double maximumContactStep = stepPerInertia * radius * frictionLimit;
	double lowerVelocity = applyBrake(freeVelocity - maximumContactStep);
	double upperVelocity = applyBrake(freeVelocity + maximumContactStep);
	constexpr int32 maximumIterations = 40;
	for (int32 iteration = 0; iteration < maximumIterations; ++iteration)
	{
		const double candidateVelocity = 0.5 * (lowerVelocity + upperVelocity);
		const FVector2D candidateForce = limitForce(candidateVelocity);
		const double reactedVelocity = applyBrake(freeVelocity - stepPerInertia * radius * candidateForce.X);
		if (candidateVelocity > reactedVelocity)
		{
			upperVelocity = candidateVelocity;
		}
		else
		{
			lowerVelocity = candidateVelocity;
		}
	}

	const double solvedVelocity = 0.5 * (lowerVelocity + upperVelocity);
	const FVector2D requestedForce = evaluateForce(solvedVelocity);
	result.force = limitForce(solvedVelocity);
	const double unbrakedVelocity = freeVelocity - stepPerInertia * radius * result.force.X;
	const double finalVelocity = applyBrake(unbrakedVelocity);
	result.angularVelocity = static_cast<float>(finalVelocity);
	result.signedBrakeTorque = static_cast<float>((unbrakedVelocity - finalVelocity) / stepPerInertia);
	result.bIsSaturated = !result.force.Equals(requestedForce, 1.0f);
	return result;
}
