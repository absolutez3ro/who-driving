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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wheels", meta = (ClampMin = "0.05", Units = "m"))
	float Radius = 0.436f;

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
	float PeakTorque = 450.0f;

	// Quadratic Bezier over idle..redline in fractions of PeakTorque. The middle value is a control point,
	// so the curve stays below it.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine", meta = (ClampMin = "0.0"))
	float TorqueCurveStart = 0.6f;

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission", meta = (ClampMin = "0.0", Units = "s"))
	float ReverseEngageDelay = 0.35f;
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

	// Angular acceleration per radian of tilt away from world up, 1/s^2.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Air", meta = (ClampMin = "0.0", EditCondition = "bEnableAirControl"))
	float LevelingStrength = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Air", meta = (ClampMin = "0.0", EditCondition = "bEnableAirControl"))
	float AngularDamping = 0.8f;
};

USTRUCT(BlueprintType)
struct FGArcadeAssistSettings
{
	GENERATED_BODY()

	// Adds steering towards the direction of travel while sliding.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists")
	bool bEnableCounterSteerAssist = true;

	// Degrees of steering per degree of body slip angle.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assists", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableCounterSteerAssist"))
	float CounterSteerStrength = 0.15f;

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
	float TractionLimitSlip = 0.35f;
};
