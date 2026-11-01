#pragma once

#include "CoreMinimal.h"
#include "GArcadeVehicleTypes.generated.h"

class UPrimitiveComponent;
class USceneComponent;

namespace GArcadeVehicleUnits
{
	constexpr float centimetersPerMeter = 100.0f;
	constexpr float metersPerCentimeter = 0.01f;
	constexpr float kilometersPerHourPerMeterPerSecond = 3.6f;
	constexpr float fullTurnRadians = static_cast<float>(UE_TWO_PI);
	constexpr float rpmPerRadianPerSecond = 60.0f / fullTurnRadians;
}

UENUM(BlueprintType)
enum class EGArcadeAxle : uint8
{
	Front,
	Rear
};

UENUM(BlueprintType)
enum class EGArcadeWheelSide : uint8
{
	Left,
	Right
};

// Placement of one wheel on a particular vehicle Blueprint.
USTRUCT(BlueprintType)
struct FGArcadeWheelSetup
{
	GENERATED_BODY()

	// Short label shown in the telemetry HUD.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheel")
	FName wheelName = NAME_None;

	// The front axle steers; the rear axle takes the handbrake.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheel")
	EGArcadeAxle axle = EGArcadeAxle::Front;

	// Pairs the wheels of one axle for the anti-roll bar.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheel")
	EGArcadeWheelSide side = EGArcadeWheelSide::Left;

	// Wheel centre at static ride height in chassis space. When zero, it is taken from the visual component bounds.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheel", meta = (Units = "cm"))
	FVector hubLocation = FVector::ZeroVector;

	// Scene component that is spun, steered and moved with the suspension. Optional.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheel")
	FName visualComponentName = NAME_None;
};

// Per-wheel simulation output, exposed for the HUD and gameplay code.
USTRUCT(BlueprintType)
struct FGArcadeWheelState
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	bool bHasContact = false;

	// True when the tire works past its grip peak or the friction circle limits it.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	bool bIsSliding = false;

	// Metres from full droop.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	float compression = 0.0f;

	// Compression relative to the suspension travel, 0..1.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	float compressionRatio = 0.0f;

	// Newtons along the chassis up axis.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	float suspensionForce = 0.0f;

	// Newtons pressing the tire into the ground (Fz).
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	float normalLoad = 0.0f;

	// Radians per second, positive when rolling forwards.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	float angularVelocity = 0.0f;

	// Degrees, positive to the right.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	float steerAngle = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	float slipRatio = 0.0f;

	// Degrees.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	float slipAngle = 0.0f;

	// Newtons in the tire's forward direction.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	float longitudinalForce = 0.0f;

	// Newtons in the tire's right direction.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	float lateralForce = 0.0f;

	// Grip multiplier of the surface under the wheel.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	float surfaceFriction = 1.0f;

	// World space, centimetres.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	FVector contactPoint = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wheel")
	FVector contactNormal = FVector::UpVector;

	// Radians, accumulated for the visual wheel.
	float spinAngle = 0.0f;
};

// Driver commands consumed by the simulation.
USTRUCT(BlueprintType)
struct FGArcadeVehicleInput
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Input")
	float throttle = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Input")
	float brake = 0.0f;

	// -1 is full left, 1 is full right.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Input")
	float steering = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Input")
	bool bHandbrake = false;
};

// Values derived from the wheel setup and tuning, refreshed every step so tuning can be edited live.
struct FGArcadeWheelRuntime
{
	// Wheel centre at static ride height, chassis space, centimetres.
	FVector hubLocation = FVector::ZeroVector;

	// Sprung mass carried by this wheel, kilograms.
	float cornerMass = 0.0f;

	// Newtons per metre.
	float springStiffness = 0.0f;

	// Newton seconds per metre.
	float damperCoefficient = 0.0f;

	// Fractions of the drivetrain torque and of the total brake torque.
	float driveTorqueShare = 0.0f;
	float brakeTorqueShare = 0.0f;

	// Gameplay modifier, for example from wheel damage.
	float gripMultiplier = 1.0f;

	// Tire load after smoothing, newtons.
	float smoothedLoad = 0.0f;

	// Slip ratio of the previous step; drives the tire stiffness used by the wheel spin solver.
	float previousSlipRatio = 0.0f;

	// Centimetres of hub compression shown by the visual wheel.
	float visualCompression = 0.0f;

	// Other wheel of the same axle, used by the anti-roll bar.
	int32 antiRollPartnerIndex = INDEX_NONE;

	// Contact patch velocity relative to the ground, metres per second, world space.
	FVector contactVelocity = FVector::ZeroVector;

	// Tire basis on the ground plane, world space.
	FVector longitudinalDirection = FVector::ForwardVector;
	FVector lateralDirection = FVector::RightVector;

	// Debug only: the suspension query segment, world space centimetres.
	FVector traceStart = FVector::ZeroVector;
	FVector traceEnd = FVector::ZeroVector;

	TWeakObjectPtr<USceneComponent> visualComponent;
	FTransform visualRestTransform = FTransform::Identity;
};

// Chassis state sampled once at the start of a simulation step.
struct FGArcadeChassisFrame
{
	FTransform transform = FTransform::Identity;
	FVector forward = FVector::ForwardVector;
	FVector right = FVector::RightVector;
	FVector up = FVector::UpVector;

	// Metres per second, world space.
	FVector velocity = FVector::ZeroVector;

	// Radians per second, world space.
	FVector angularVelocity = FVector::ZeroVector;

	// World space, centimetres.
	FVector centerOfMass = FVector::ZeroVector;

	// Metres per second squared, world space.
	FVector gravity = FVector(0.0f, 0.0f, -9.81f);

	// Metres per second along the chassis forward axis.
	float forwardSpeed = 0.0f;

	// Seconds.
	float deltaTime = 0.0f;

	int32 groundedWheelCount = 0;
};
