#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GArcadeVehicleSettings.generated.h"

class UCurveFloat;
class UPhysicalMaterial;

// Handling parameters of the arcade vehicle. The structs are owned by UGArcadeVehicleTuning, so their
// fields follow the project's data asset naming. Dynamics are in SI units, positions in centimetres.

UENUM(BlueprintType)
enum class EGArcadeSuspensionTrace : uint8
{
	Line,
	Sphere
};

USTRUCT(BlueprintType)
struct FGArcadeChassisSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis", meta = (ClampMin = "1.0", Units = "kg"))
	float Mass = 1400.0f;

	// Added to the centre of mass computed from the physics asset, in chassis space. Lower is more stable.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis", meta = (Units = "cm"))
	FVector CenterOfMassOffset = FVector(0.0f, 0.0f, -15.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis")
	FVector InertiaTensorScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis", meta = (ClampMin = "0.0"))
	float AngularDamping = 0.3f;

	// Aerodynamic drag is simulated explicitly, so the engine's linear damping is normally zero.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis", meta = (ClampMin = "0.0"))
	float LinearDamping = 0.0f;
};

USTRUCT(BlueprintType)
struct FGArcadeSuspensionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	EGArcadeSuspensionTrace TraceType = EGArcadeSuspensionTrace::Line;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// Full stroke of the hub between full droop and full compression.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension", meta = (ClampMin = "0.01", Units = "m"))
	float Travel = 0.15f;

	// Compression at rest as a fraction of travel. Spring rate is k = cornerMass * g / (Travel * StaticSagRatio);
	// a value of 1 gives the plain k = m * g / travel.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float StaticSagRatio = 0.5f;

	// Fraction of critical damping. 0.5-0.7 settles the body within one or two oscillations.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DampingRatio = 0.6f;

	// Damping while the spring extends, relative to compression. Real dampers are stiffer on rebound,
	// which keeps a heavy car from bouncing back up after a landing.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension", meta = (ClampMin = "0.0"))
	float ReboundDampingMultiplier = 1.6f;

	// Anti-roll bar rate as a fraction of the spring rate, applied between the wheels of one axle.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension", meta = (ClampMin = "0.0"))
	float AntiRollStiffnessRatio = 0.4f;

	// Extra spring rate once the hub runs past full compression.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension", meta = (ClampMin = "0.0"))
	float BumpStopStiffnessMultiplier = 8.0f;

	// Sphere trace radius relative to the wheel radius; only used by the sphere trace.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suspension", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float SphereTraceRadiusRatio = 0.6f;
};

USTRUCT(BlueprintType)
struct FGArcadeWheelSettings
{
	GENERATED_BODY()

	// Only used by wheels without a visual component; otherwise the radius is read from the wheel mesh.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels", meta = (ClampMin = "0.05", Units = "m"))
	float Radius = 0.38f;

	// Wheel inertia is derived as a solid disc: 0.5 * Mass * Radius^2.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels", meta = (ClampMin = "0.5", Units = "kg"))
	float Mass = 20.0f;

	// Exponential spin decay of a wheel without ground contact, per second.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels", meta = (ClampMin = "0.0"))
	float AirSpinDamping = 0.6f;
};

// Piecewise linear grip curve: rises linearly to the peak, falls linearly to the floor, then stays flat.
USTRUCT(BlueprintType)
struct FGArcadeTireCurve
{
	GENERATED_BODY()

	FGArcadeTireCurve() = default;

	FGArcadeTireCurve(float peakSlip, float peakValue, float floorSlip, float floorValue)
		: PeakSlip(peakSlip)
		, PeakValue(peakValue)
		, FloorSlip(floorSlip)
		, FloorValue(floorValue)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires", meta = (ClampMin = "0.001"))
	float PeakSlip = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires", meta = (ClampMin = "0.0"))
	float PeakValue = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires", meta = (ClampMin = "0.001"))
	float FloorSlip = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires", meta = (ClampMin = "0.0"))
	float FloorValue = 0.7f;
};

USTRUCT(BlueprintType)
struct FGArcadeTireSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires", meta = (ClampMin = "0.0"))
	float FrictionCoefficient = 1.65f;

	// Slip ratio (R * omega - v) / max(|v|, SlipReferenceSpeed) to grip fraction.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires")
	FGArcadeTireCurve LongitudinalCurve;

	// Slip angle in degrees to grip fraction.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires")
	FGArcadeTireCurve LateralCurve = FGArcadeTireCurve(8.0f, 1.0f, 25.0f, 0.7f);

	// Extra reduction of the lateral force when the combined force exceeds the friction circle.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LateralSuppression = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires", meta = (ClampMin = "0.1", Units = "m/s"))
	float SlipReferenceSpeed = 1.0f;

	// Below this forward speed the slip angle blends into a lateral damper, which keeps a parked car still.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires", meta = (ClampMin = "0.1", Units = "m/s"))
	float SlipAngleReferenceSpeed = 3.0f;

	// Raises the tire force application point from the contact patch (0) to the centre of mass height (1).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ForceApplicationHeightRatio = 0.3f;

	// Caps the tire load relative to the static corner weight, so hard landings do not spike grip.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires", meta = (ClampMin = "1.0"))
	float MaxLoadRatio = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires", meta = (ClampMin = "0.0", Units = "s"))
	float LoadSmoothingTime = 0.03f;

	// Below this speed without drive input the tires hold the car in place, including on slopes.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires", meta = (ClampMin = "0.0", Units = "m/s"))
	float StandstillSpeed = 0.5f;

	// Grip multiplier per surface; materials that are not listed use 1.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tires")
	TMap<TObjectPtr<UPhysicalMaterial>, float> SurfaceFrictionMultipliers;
};

USTRUCT(BlueprintType)
struct FGArcadeEngineSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine", meta = (ClampMin = "100.0"))
	float IdleRpm = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine", meta = (ClampMin = "500.0"))
	float RedlineRpm = 6800.0f;

	// Torque fades to zero over this range below the redline.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine", meta = (ClampMin = "1.0"))
	float RevLimiterRange = 250.0f;

	// Newton metres.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine", meta = (ClampMin = "0.0"))
	float PeakTorque = 600.0f;

	// Quadratic Bezier over idle..redline in fractions of PeakTorque. The middle value is a control point,
	// so the curve stays below it. A high start value gives the strong launch of an arcade car.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine", meta = (ClampMin = "0.0"))
	float TorqueCurveStart = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine", meta = (ClampMin = "0.0"))
	float TorqueCurveMiddle = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine", meta = (ClampMin = "0.0"))
	float TorqueCurveEnd = 0.7f;

	// Optional torque in newton metres by rpm; replaces the Bezier curve when set.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine")
	TObjectPtr<UCurveFloat> TorqueCurve;

	// Engine braking at the redline with closed throttle, newton metres at the crankshaft.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine", meta = (ClampMin = "0.0"))
	float EngineBrakeTorque = 80.0f;
};

USTRUCT(BlueprintType)
struct FGArcadeTransmissionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission")
	TArray<float> ForwardGearRatios = {3.2f, 2.2f, 1.65f, 1.35f, 1.15f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission", meta = (ClampMin = "0.1"))
	float ReverseGearRatio = 3.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission", meta = (ClampMin = "0.1"))
	float FinalDriveRatio = 3.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float Efficiency = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission", meta = (ClampMin = "500.0"))
	float UpshiftRpm = 6300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission", meta = (ClampMin = "500.0"))
	float DownshiftRpm = 3300.0f;

	// Drive torque is cut while a gear change is in progress.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission", meta = (ClampMin = "0.0", Units = "s"))
	float ShiftDuration = 0.2f;

	// Share of drive torque sent to the front axle; each axle splits its share equally.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FrontTorqueShare = 0.3f;

	// The direction of travel can only change below this speed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission", meta = (ClampMin = "0.0", Units = "m/s"))
	float ReverseEngageSpeed = 1.0f;

	// Brake has to be held this long at low speed before reverse engages, so braking to a stop holds the car.
	// Throttle switches back to a forward gear at once.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission", meta = (ClampMin = "0.0", Units = "s"))
	float ReverseEngageDelay = 0.35f;

	// Upshifts wait until the road speed, not only spinning wheels, reaches this share of UpshiftRpm.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float UpshiftRoadSpeedRatio = 0.9f;
};

USTRUCT(BlueprintType)
struct FGArcadeBrakeSettings
{
	GENERATED_BODY()

	// Newton metres summed over all wheels at full brake input.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Brakes", meta = (ClampMin = "0.0"))
	float TotalBrakeTorque = 8000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Brakes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FrontBias = 0.62f;

	// Newton metres per rear wheel.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Brakes", meta = (ClampMin = "0.0"))
	float HandbrakeTorque = 3500.0f;

	// Rear lateral grip while the handbrake is held; this is what starts a controllable slide.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Brakes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HandbrakeLateralGripMultiplier = 0.5f;
};

USTRUCT(BlueprintType)
struct FGArcadeSteeringSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering", meta = (ClampMin = "0.0", ClampMax = "80.0", Units = "deg"))
	float MaxAngleAtRest = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering", meta = (ClampMin = "0.0", ClampMax = "80.0", Units = "deg"))
	float MaxAngleAtHighSpeed = 9.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering", meta = (ClampMin = "1.0", Units = "km/h"))
	float HighSpeed = 200.0f;

	// Shape of the falloff from MaxAngleAtRest to MaxAngleAtHighSpeed; below 1 the angle drops early.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering", meta = (ClampMin = "0.1"))
	float SpeedFalloffExponent = 0.75f;

	// Optional maximum angle in degrees by speed in km/h; replaces the falloff above when set.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering")
	TObjectPtr<UCurveFloat> MaxAngleBySpeedCurve;

	// Degrees per second while turning away from the centre.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering", meta = (ClampMin = "1.0"))
	float SteerRate = 120.0f;

	// Degrees per second while returning towards the centre.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steering", meta = (ClampMin = "1.0"))
	float ReturnRate = 220.0f;
};

USTRUCT(BlueprintType)
struct FGArcadeAeroSettings
{
	GENERATED_BODY()

	// F = -DragCoefficient * |v| * v, in N / (m/s)^2.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aero", meta = (ClampMin = "0.0"))
	float DragCoefficient = 0.4f;

	// Lift coefficient times reference area: F = 0.5 * AirDensity * |v|^2 * DownforceArea.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aero", meta = (ClampMin = "0.0"))
	float DownforceArea = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aero", meta = (ClampMin = "0.0"))
	float AirDensity = 1.225f;

	// Linear force against planar velocity while on the ground, in N / (m/s).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aero", meta = (ClampMin = "0.0"))
	float RollingResistance = 8.0f;
};

USTRUCT(BlueprintType)
struct FGArcadeAirSettings
{
	GENERATED_BODY()

	// Weak levelling and rotation damping without wheel contact. Never steers the trajectory.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Air")
	bool bEnableAirControl = true;

	// Angular acceleration per radian of tilt away from the target attitude, 1/s^2.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Air", meta = (ClampMin = "0.0", EditCondition = "bEnableAirControl"))
	float LevelingStrength = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Air", meta = (ClampMin = "0.0", EditCondition = "bEnableAirControl"))
	float AngularDamping = 0.8f;

	// How far the target attitude follows the flight path: 0 stays level, 1 points the nose along the arc.
	// Dipping the nose on the way down is what makes a heavy car read as heavy.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Air", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableAirControl"))
	float TrajectoryAlignment = 0.35f;

	// Near the ground the target attitude turns to the surface below, so the car lands on all four wheels.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Air", meta = (ClampMin = "0.0", Units = "m", EditCondition = "bEnableAirControl"))
	float LandingProbeDistance = 6.0f;

	// Gravity multiplier without wheel contact. Real gravity makes a car this size float over jumps.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Air", meta = (ClampMin = "1.0"))
	float GravityScale = 1.8f;

	// Time over which the extra gravity builds up after the wheels leave the ground; short hops stay real.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Air", meta = (ClampMin = "0.0", Units = "s"))
	float GravityRampTime = 0.15f;
};

USTRUCT(BlueprintType)
struct FGArcadeAssistSettings
{
	GENERATED_BODY()

	// Adds steering towards the direction of travel while sliding outside a drift.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists")
	bool bEnableCounterSteerAssist = true;

	// Degrees of steering per degree of body slip angle past the activation angle.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableCounterSteerAssist"))
	float CounterSteerStrength = 0.15f;

	// A normal corner has a few degrees of body slip; counter steering below this would only widen the turn.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists", meta = (ClampMin = "0.0", Units = "deg", EditCondition = "bEnableCounterSteerAssist"))
	float CounterSteerActivationAngle = 8.0f;

	// Drives the yaw rate towards a target proportional to the steering input. Full steering asks for the
	// lower of the full-lock yaw rate and the arcade lateral acceleration limit, so control stays linear at speed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists")
	bool bEnableTurnAssist = true;

	// Lateral acceleration the turn assist allows, m/s^2. Real tires give about 10; arcade cars turn harder.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists", meta = (ClampMin = "1.0", EditCondition = "bEnableTurnAssist"))
	float TurnAssistLateralAcceleration = 24.0f;

	// How fast the yaw rate error is closed, 1/s.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists", meta = (ClampMin = "0.0", EditCondition = "bEnableTurnAssist"))
	float TurnAssistStrength = 4.0f;

	// Pulls the sideways velocity towards zero so the car follows its nose instead of washing wide.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists")
	bool bEnableGripAssist = true;

	// Share of the sideways velocity removed per second.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists", meta = (ClampMin = "0.0", EditCondition = "bEnableGripAssist"))
	float GripAssistRate = 2.5f;

	// Upper bound of the grip assist, m/s^2.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists", meta = (ClampMin = "0.0", EditCondition = "bEnableGripAssist"))
	float GripAssistMaxAcceleration = 10.0f;

	// Both cornering assists stay off below this speed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists", meta = (ClampMin = "0.0", Units = "km/h"))
	float CorneringAssistMinimumSpeed = 15.0f;

	// Damps yaw rotation while there is no steering input.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists")
	bool bEnableYawDamping = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists", meta = (ClampMin = "0.0", EditCondition = "bEnableYawDamping"))
	float YawDampingStrength = 0.4f;

	// Damps roll rotation on the ground, which keeps the car from tipping in fast corners.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists")
	bool bEnableRollStabilization = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists", meta = (ClampMin = "0.0", EditCondition = "bEnableRollStabilization"))
	float RollDampingStrength = 1.5f;

	// Scales drive torque down once the driven tires slip past this ratio.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists")
	bool bEnableTractionLimit = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists", meta = (ClampMin = "0.01", EditCondition = "bEnableTractionLimit"))
	float TractionLimitSlip = 0.6f;

	// The traction limit never removes more than this share of the drive torque.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableTractionLimit"))
	float TractionLimitMinimumScale = 0.6f;
};

// Controlled brake-to-drift with optional tire grip improvements and physical handbrake slides.
USTRUCT(BlueprintType)
struct FGArcadeDriftSettings
{
	GENERATED_BODY()

	// Preserve controlled brake-to-drift, retain more rear grip and disable artificial speed hold.
	// Handbrake slides use wheel lock and combined tire slip instead of a scripted spin.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift")
	bool bUsePhysicalSlides = true;

	// Lift off, steer and tap the brake to start a controlled drift.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift")
	bool bEnableBrakeDrift = true;

	// Fade driving assists while steering with the handbrake, then control shallow slides on release.
	// Legacy mode also commands a spin while the handbrake is held.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift")
	bool bEnableHandbrakeDrift = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", Units = "km/h"))
	float BrakeDriftMinimumSpeed = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", Units = "km/h"))
	float HandbrakeDriftMinimumSpeed = 25.0f;

	// Steering input needed to start a drift; its sign picks the drift direction.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EntrySteering = 0.3f;

	// A brake drift only starts while the throttle is below this.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EntryThrottleLimit = 0.4f;

	// Body slip angle held with neutral steering.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", ClampMax = "80.0", Units = "deg"))
	float BaseAngle = 28.0f;

	// Body slip angle held with full steering into the drift. Full counter steer asks for zero and ends the drift.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", ClampMax = "80.0", Units = "deg"))
	float MaxAngle = 45.0f;

	// Yaw acceleration per radian of drift angle error, 1/s^2.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0"))
	float AngleStiffness = 25.0f;

	// Yaw acceleration per radian per second of drift angle change, 1/s.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0"))
	float AngleDamping = 7.0f;

	// Upper bound of the drift yaw acceleration, rad/s^2.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0"))
	float MaxYawAcceleration = 12.0f;

	// Sideways acceleration that bends the path through the corner with neutral steering, m/s^2. This is what
	// lets a drift take a corner tighter than grip alone; the yaw control then keeps the nose ahead of the path.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0"))
	float PathAcceleration = 12.0f;

	// Path acceleration with full steering into the drift, m/s^2.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0"))
	float PathAccelerationIntoDrift = 22.0f;

	// Path acceleration with full counter steer, m/s^2; the drift opens up and runs wide.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0"))
	float PathAccelerationCounterSteer = 2.0f;

	// How strongly the yaw rate follows the turning rate of the path, 1/s. Without it the nose lags the corner.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0"))
	float YawRateMatchStrength = 3.0f;

	// Yaw rate of a handbrake turn at full steering.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", Units = "deg/s"))
	float HandbrakeTurnYawRate = 220.0f;

	// How fast the handbrake turn reaches its yaw rate, 1/s.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0"))
	float HandbrakeTurnResponse = 6.0f;

	// Upper bound of the handbrake turn yaw acceleration, rad/s^2.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0"))
	float HandbrakeTurnMaxYawAcceleration = 14.0f;

	// Rear lateral grip during a handbrake turn.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HandbrakeTurnRearGripMultiplier = 0.25f;

	// The turn stops rotating once the car has swung this far from its direction of travel, which leaves it
	// facing back the way it came for a J-turn.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float HandbrakeTurnStopAngle = 170.0f;

	// Releasing the handbrake below this slide angle continues as a drift; above it the car simply grips again.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "deg"))
	float HandbrakeTurnHandoverAngle = 60.0f;

	// Rear lateral grip while drifting. Improved tire grip keeps this at or above 0.75.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RearGripMultiplier = 0.45f;

	// Service brake strength while drifting, so the brake tap does not kill the speed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BrakeScale = 0.15f;

	// Acceleration along the velocity per m/s of speed lost since the drift started, 1/s.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0"))
	float SpeedHoldGain = 1.5f;

	// Upper bound of the speed hold, m/s^2.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0"))
	float SpeedHoldMaxAcceleration = 6.0f;

	// The drift ends once the car straightens below this angle.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", Units = "deg"))
	float ExitAngle = 7.0f;

	// Each slide mode ends below this share of its own minimum entry speed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExitSpeedRatio = 0.6f;

	// Shortest drift, so the tail has time to swing out before the exit angle is checked.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", Units = "s"))
	float MinimumDuration = 0.4f;

	// Time to blend grip and braking changes in and out.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.01", Units = "s"))
	float BlendTime = 0.15f;

	// The drift ends after this long without wheel contact.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift", meta = (ClampMin = "0.0", Units = "s"))
	float MaxAirTime = 0.6f;
};
