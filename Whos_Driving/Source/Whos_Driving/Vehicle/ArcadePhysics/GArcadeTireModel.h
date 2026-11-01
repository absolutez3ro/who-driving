#pragma once

#include "CoreMinimal.h"

struct FGArcadeTireCurve;

// Inputs of one wheel spin step, SI units.
struct FGArcadeWheelSpinInput
{
	// Radians per second.
	float angularVelocity = 0.0f;

	// Kilogram square metres.
	float inertia = 1.0f;

	// Metres.
	float radius = 0.3f;

	// Newton metres, signed; positive accelerates forward rotation.
	float driveTorque = 0.0f;

	// Newton metres, magnitude; never reverses the rotation.
	float brakeTorque = 0.0f;

	// Metres per second of the contact patch along the tire's forward direction.
	float longitudinalSpeed = 0.0f;

	// Newtons per unit of slip ratio; zero without ground contact.
	float tireStiffness = 0.0f;

	// Metres per second; the slip ratio denominator max(|longitudinalSpeed|, floor).
	float referenceSpeed = 1.0f;

	// Seconds.
	float deltaTime = 0.0f;
};

// Stateless tire and wheel maths shared by the vehicle component and the automation tests.
class WHOS_DRIVING_API FGArcadeTireModel
{
public:
	// Signed grip fraction of the piecewise linear curve for a signed slip value.
	static float EvaluateCurve(const FGArcadeTireCurve& curve, float slip);

	static float ComputeSlipRatio(float wheelSurfaceSpeed, float longitudinalSpeed, float referenceSpeed);

	// Degrees. The reference speed blends the angle into a lateral damper at walking pace.
	static float ComputeSlipAngle(float lateralSpeed, float longitudinalSpeed, float referenceSpeed);

	// Force per unit slip of the line from the origin through the curve at the given slip.
	static float ComputeSecantStiffness(const FGArcadeTireCurve& curve, float slip, float frictionLimit);

	// X is longitudinal and Y is lateral force. Past the limit the lateral part is suppressed further,
	// then the vector is scaled back onto the limit, which keeps a sliding car driving forward.
	static FVector2D LimitToFrictionCircle(const FVector2D& force, float frictionLimit, float lateralSuppression);

	// Returns the new angular velocity. The tire force is treated implicitly through its stiffness,
	// so the step stays stable at low frame rates where an explicit update would oscillate.
	static float IntegrateWheelSpin(const FGArcadeWheelSpinInput& input);
};
