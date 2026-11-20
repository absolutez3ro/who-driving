#include "GArcadeVehicleComponent.h"
#include "GArcadeTireModel.h"
#include "GArcadeVehicleTuning.h"
#include "ChaosVehicleMovementComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/BodyInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogArcadeVehicle, Log, All);

namespace
{
	TAutoConsoleVariable<int32> arcadeVehicleDebugVariable(
		TEXT("vp.Debug"),
		0,
		TEXT("Draws arcade vehicle suspension traces, contacts, tire forces and the centre of mass. 0 = off, 1 = on."),
		ECVF_Cheat);

	// The counter-steer assist stays off below this forward speed, metres per second.
	constexpr float counterSteerMinimumSpeed = 5.0f;

	// Drive requests below this keep the standstill hold active.
	constexpr float holdDriveDemandThreshold = 0.05f;

	// Steering input below this counts as hands off for the yaw assists.
	constexpr float steeringDeadZone = 0.05f;

	// Below this speed the body slip angle is meaningless noise, metres per second.
	constexpr float slipAngleMinimumSpeed = 1.0f;

	// Rate at which a visual wheel follows its simulated suspension position.
	constexpr float visualFollowSpeed = 30.0f;

	// Ground speed at which the air attitude fully follows the flight path, metres per second.
	constexpr float trajectoryAlignmentFullSpeed = 10.0f;

	// Steepest nose-up or nose-down attitude the flight path may ask for, radians.
	constexpr float trajectoryMaxPitch = 0.6f;

	// Share of the creep velocity the standstill hold removes per step.
	constexpr float standstillVelocityGain = 0.5f;
}

UGArcadeVehicleComponent::UGArcadeVehicleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UGArcadeVehicleComponent::BeginPlay()
{
	Super::BeginPlay();

	chassis = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
	if (!chassis)
	{
		UE_LOG(LogArcadeVehicle, Warning, TEXT("%s: the arcade vehicle needs a primitive root component."), *GetOwner()->GetName());
		SetComponentTickEnabled(false);
		return;
	}
	if (wheelSetups.IsEmpty())
	{
		UE_LOG(LogArcadeVehicle, Warning, TEXT("%s: no wheels are set up on the arcade vehicle."), *GetOwner()->GetName());
	}

	drivetrain.Reset();
	driftController.Reset();
	InitializeWheels();
	ApplyChassisTuning();
}

void UGArcadeVehicleComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
	Super::TickComponent(deltaTime, tickType, thisTickFunction);

	if (wheelStates.Num() != wheelSetups.Num())
	{
		InitializeWheels();
	}
	if (!CanSimulate())
	{
		return;
	}
	const float stepTime = FMath::Min(deltaTime, maxSimulationStep);
	if (stepTime <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}

	FGArcadeChassisFrame frame = CaptureChassisFrame(stepTime);
	speedKmh = frame.velocity.Size() * GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond;
	forwardSpeedKmh = frame.forwardSpeed * GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond;

	RefreshWheelParameters(frame);
	UpdateSuspension(frame);
	UpdateDrift(frame);
	UpdateSteering(frame);
	UpdateDrivetrain(frame);
	UpdateTires(frame);
	ApplyAerodynamics(frame);
	ApplyExtraGravity(frame);
	ApplyStabilization(frame);
	ApplyAssistForces(frame);
	UpdateWheelVisuals(stepTime);
	DrawDebug(frame);
}

void UGArcadeVehicleComponent::SetThrottle(float value)
{
	vehicleInput.throttle = FMath::Clamp(value, 0.0f, 1.0f);
}

void UGArcadeVehicleComponent::SetBrake(float value)
{
	vehicleInput.brake = FMath::Clamp(value, 0.0f, 1.0f);
}

void UGArcadeVehicleComponent::SetSteering(float value)
{
	vehicleInput.steering = FMath::Clamp(value, -1.0f, 1.0f);
}

void UGArcadeVehicleComponent::SetHandbrake(bool bEngaged)
{
	vehicleInput.bHandbrake = bEngaged;
}

void UGArcadeVehicleComponent::ResetVehicle()
{
	AActor* owner = GetOwner();
	if (!chassis || !owner->HasAuthority())
	{
		return;
	}

	const FRotator uprightRotation(0.0f, owner->GetActorRotation().Yaw, 0.0f);
	const FVector liftedLocation = owner->GetActorLocation() + FVector::UpVector * resetLiftHeight;
	owner->SetActorLocationAndRotation(liftedLocation, uprightRotation, false, nullptr, ETeleportType::TeleportPhysics);
	chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);
	chassis->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);

	drivetrain.Reset();
	driftController.Reset();
	currentSteerAngle = 0.0f;
	airborneTime = 0.0f;
	for (FGArcadeWheelState& state : wheelStates)
	{
		state.angularVelocity = 0.0f;
	}
	for (FGArcadeWheelRuntime& runtime : wheelRuntimes)
	{
		runtime.previousSlipRatio = 0.0f;
	}
}

void UGArcadeVehicleComponent::ApplyChassisTuning()
{
	FBodyInstance* body = chassis ? chassis->GetBodyInstance() : nullptr;
	if (!body)
	{
		return;
	}

	const FGArcadeChassisSettings& chassisSettings = GetTuningData().Chassis;
	SynchronizeChaosVehicleMass(chassisSettings.Mass);
	body->InertiaTensorScale = chassisSettings.InertiaTensorScale;
	body->COMNudge = chassisSettings.CenterOfMassOffset;
	body->SetMassOverride(chassisSettings.Mass, true);
	body->LinearDamping = chassisSettings.LinearDamping;
	body->AngularDamping = chassisSettings.AngularDamping;
	body->UpdateDampingProperties();
	body->UpdateMassProperties();
}

void UGArcadeVehicleComponent::SetEngineTorqueMultiplier(float multiplier)
{
	engineTorqueMultiplier = FMath::Max(multiplier, 0.0f);
}

void UGArcadeVehicleComponent::SetWheelGripMultiplier(int32 wheelIndex, float multiplier)
{
	if (wheelRuntimes.IsValidIndex(wheelIndex))
	{
		wheelRuntimes[wheelIndex].gripMultiplier = FMath::Max(multiplier, 0.0f);
	}
}

bool UGArcadeVehicleComponent::IsInAir() const
{
	for (const FGArcadeWheelState& state : wheelStates)
	{
		if (state.bHasContact)
		{
			return false;
		}
	}
	return !wheelStates.IsEmpty();
}

const UGArcadeVehicleTuning& UGArcadeVehicleComponent::GetTuningData() const
{
	if (tuning)
	{
		return *tuning;
	}
	return *GetDefault<UGArcadeVehicleTuning>();
}

bool UGArcadeVehicleComponent::CanSimulate() const
{
	const AActor* owner = GetOwner();
	const bool bHasAuthority = owner && owner->HasAuthority();
	const bool bHasSimulatedChassis = chassis && chassis->IsSimulatingPhysics();
	return bHasAuthority && bHasSimulatedChassis && wheelStates.Num() == wheelRuntimes.Num();
}

void UGArcadeVehicleComponent::InitializeWheels()
{
	wheelStates.SetNum(wheelSetups.Num());
	wheelRuntimes.SetNum(wheelSetups.Num());
	for (int32 wheelIndex = 0; wheelIndex < wheelSetups.Num(); ++wheelIndex)
	{
		wheelStates[wheelIndex] = FGArcadeWheelState();
		wheelRuntimes[wheelIndex] = FGArcadeWheelRuntime();
		wheelRuntimes[wheelIndex].hubLocation = wheelSetups[wheelIndex].hubLocation;
		ResolveWheelVisual(wheelIndex);

		for (int32 otherIndex = 0; otherIndex < wheelSetups.Num(); ++otherIndex)
		{
			const bool bIsSameAxle = wheelSetups[otherIndex].axle == wheelSetups[wheelIndex].axle;
			const bool bIsOppositeSide = wheelSetups[otherIndex].side != wheelSetups[wheelIndex].side;
			if (otherIndex != wheelIndex && bIsSameAxle && bIsOppositeSide)
			{
				wheelRuntimes[wheelIndex].antiRollPartnerIndex = otherIndex;
				break;
			}
		}
	}
}

void UGArcadeVehicleComponent::ResolveWheelVisual(int32 wheelIndex)
{
	const FName visualName = wheelSetups[wheelIndex].visualComponentName;
	if (visualName.IsNone())
	{
		return;
	}

	USceneComponent* visual = nullptr;
	TInlineComponentArray<USceneComponent*> sceneComponents(GetOwner());
	for (USceneComponent* candidate : sceneComponents)
	{
		if (candidate && candidate->GetFName() == visualName)
		{
			visual = candidate;
			break;
		}
	}
	if (!visual)
	{
		UE_LOG(LogArcadeVehicle, Warning, TEXT("%s: wheel visual component '%s' was not found."), *GetOwner()->GetName(), *visualName.ToString());
		return;
	}

	// Wheel meshes of the vehicle are modelled in chassis space, so the bounds centre is the hub and the
	// larger of the length and height extents is the radius. Reading them keeps the physics on the mesh
	// when the car model is swapped.
	FGArcadeWheelRuntime& runtime = wheelRuntimes[wheelIndex];
	const FBoxSphereBounds visualBounds = visual->CalcBounds(visual->GetRelativeTransform());
	const float visualRadius = FMath::Max(visualBounds.BoxExtent.X, visualBounds.BoxExtent.Z);
	runtime.visualComponent = visual;
	runtime.visualRestTransform = visual->GetRelativeTransform();
	runtime.visualRadius = visualRadius * GArcadeVehicleUnits::metersPerCentimeter;
	if (runtime.hubLocation.IsNearlyZero())
	{
		runtime.hubLocation = visualBounds.Origin;
	}
}

void UGArcadeVehicleComponent::SynchronizeChaosVehicleMass(float mass)
{
	// A Chaos vehicle movement component re-applies its own mass whenever the body recalculates
	// mass properties. Vehicles copied from a Chaos car keep that component, so it has to agree.
	TInlineComponentArray<UChaosVehicleMovementComponent*> chaosMovements(GetOwner());
	for (UChaosVehicleMovementComponent* chaosMovement : chaosMovements)
	{
		chaosMovement->Mass = mass;
	}
}

FGArcadeChassisFrame UGArcadeVehicleComponent::CaptureChassisFrame(float deltaTime) const
{
	FGArcadeChassisFrame frame;
	frame.transform = chassis->GetComponentTransform();
	frame.forward = frame.transform.GetUnitAxis(EAxis::X);
	frame.right = frame.transform.GetUnitAxis(EAxis::Y);
	frame.up = frame.transform.GetUnitAxis(EAxis::Z);
	frame.velocity = chassis->GetPhysicsLinearVelocity() * GArcadeVehicleUnits::metersPerCentimeter;
	frame.angularVelocity = chassis->GetPhysicsAngularVelocityInRadians();
	frame.centerOfMass = chassis->GetCenterOfMass();
	frame.gravity = FVector(0.0f, 0.0f, GetWorld()->GetGravityZ() * GArcadeVehicleUnits::metersPerCentimeter);
	frame.forwardSpeed = FVector::DotProduct(frame.velocity, frame.forward);
	frame.deltaTime = deltaTime;
	return frame;
}

void UGArcadeVehicleComponent::RefreshWheelParameters(const FGArcadeChassisFrame& frame)
{
	const UGArcadeVehicleTuning& tuningData = GetTuningData();
	const FGArcadeSuspensionSettings& suspension = tuningData.Suspension;

	int32 frontWheelCount = 0;
	int32 rearWheelCount = 0;
	float frontAxlePosition = 0.0f;
	float rearAxlePosition = 0.0f;
	for (int32 wheelIndex = 0; wheelIndex < wheelRuntimes.Num(); ++wheelIndex)
	{
		if (IsRearWheel(wheelIndex))
		{
			++rearWheelCount;
			rearAxlePosition += wheelRuntimes[wheelIndex].hubLocation.X;
		}
		else
		{
			++frontWheelCount;
			frontAxlePosition += wheelRuntimes[wheelIndex].hubLocation.X;
		}
	}
	if (frontWheelCount == 0 && rearWheelCount == 0)
	{
		return;
	}

	// Static axle loads follow from where the centre of mass sits between the axles.
	float frontWeightShare = frontWheelCount > 0 ? 1.0f : 0.0f;
	float frontTorqueShare = frontWeightShare;
	float frontBrakeShare = frontWeightShare;
	if (frontWheelCount > 0 && rearWheelCount > 0)
	{
		frontAxlePosition /= frontWheelCount;
		rearAxlePosition /= rearWheelCount;
		const float axleDistance = FMath::Max(frontAxlePosition - rearAxlePosition, UE_KINDA_SMALL_NUMBER);
		const float centerOfMassPosition = frame.transform.InverseTransformPosition(frame.centerOfMass).X;
		frontWeightShare = FMath::Clamp((centerOfMassPosition - rearAxlePosition) / axleDistance, 0.05f, 0.95f);
		wheelbase = axleDistance * GArcadeVehicleUnits::metersPerCentimeter;
		frontTorqueShare = tuningData.Transmission.FrontTorqueShare;
		frontBrakeShare = tuningData.Brakes.FrontBias;
	}

	const float mass = chassis->GetMass();
	const float gravity = FMath::Max(frame.gravity.Size(), UE_KINDA_SMALL_NUMBER);
	const float staticCompression = FMath::Max(suspension.Travel * suspension.StaticSagRatio, UE_KINDA_SMALL_NUMBER);
	for (int32 wheelIndex = 0; wheelIndex < wheelRuntimes.Num(); ++wheelIndex)
	{
		const bool bIsRear = IsRearWheel(wheelIndex);
		const int32 axleWheelCount = bIsRear ? rearWheelCount : frontWheelCount;
		const float axleWeightShare = bIsRear ? 1.0f - frontWeightShare : frontWeightShare;
		const float axleTorqueShare = bIsRear ? 1.0f - frontTorqueShare : frontTorqueShare;
		const float axleBrakeShare = bIsRear ? 1.0f - frontBrakeShare : frontBrakeShare;

		FGArcadeWheelRuntime& runtime = wheelRuntimes[wheelIndex];
		runtime.cornerMass = mass * axleWeightShare / axleWheelCount;
		runtime.springStiffness = runtime.cornerMass * gravity / staticCompression;
		runtime.damperCoefficient = 2.0f * suspension.DampingRatio * FMath::Sqrt(runtime.springStiffness * runtime.cornerMass);
		runtime.driveTorqueShare = axleTorqueShare / axleWheelCount;
		runtime.brakeTorqueShare = axleBrakeShare / axleWheelCount;
	}
}

FCollisionQueryParams UGArcadeVehicleComponent::BuildSuspensionQueryParams() const
{
	FCollisionQueryParams queryParams(SCENE_QUERY_STAT(ArcadeVehicleSuspension), false, GetOwner());
	queryParams.bReturnPhysicalMaterial = true;

	// Seated characters are attached to the vehicle and must not hold it up.
	TArray<AActor*> attachedActors;
	GetOwner()->GetAttachedActors(attachedActors, true, true);
	queryParams.AddIgnoredActors(attachedActors);
	return queryParams;
}

void UGArcadeVehicleComponent::UpdateSuspension(FGArcadeChassisFrame& frame)
{
	const FCollisionQueryParams queryParams = BuildSuspensionQueryParams();
	frame.groundedWheelCount = 0;
	for (int32 wheelIndex = 0; wheelIndex < wheelStates.Num(); ++wheelIndex)
	{
		TraceWheel(wheelIndex, frame, queryParams);
		ComputeSuspensionForce(wheelIndex, frame);
		if (wheelStates[wheelIndex].bHasContact)
		{
			++frame.groundedWheelCount;
		}
	}

	ApplyAntiRollBars();
	for (int32 wheelIndex = 0; wheelIndex < wheelStates.Num(); ++wheelIndex)
	{
		ApplySuspensionForce(wheelIndex, frame);
	}
}

void UGArcadeVehicleComponent::TraceWheel(int32 wheelIndex, const FGArcadeChassisFrame& frame, const FCollisionQueryParams& queryParams)
{
	const UGArcadeVehicleTuning& tuningData = GetTuningData();
	const FGArcadeSuspensionSettings& suspension = tuningData.Suspension;
	FGArcadeWheelState& state = wheelStates[wheelIndex];
	FGArcadeWheelRuntime& runtime = wheelRuntimes[wheelIndex];

	const float travel = suspension.Travel * GArcadeVehicleUnits::centimetersPerMeter;
	const float staticCompression = travel * suspension.StaticSagRatio;
	const float radius = GetWheelRadius(wheelIndex) * GArcadeVehicleUnits::centimetersPerMeter;
	const FVector hubRestLocation = frame.transform.TransformPosition(runtime.hubLocation);
	const FVector hubTopLocation = hubRestLocation + frame.up * (travel - staticCompression);

	// Distance of the hub below its fully compressed position, centimetres.
	float hubDrop = travel;
	FHitResult hit;
	bool bHasHit = false;
	if (suspension.TraceType == EGArcadeSuspensionTrace::Sphere)
	{
		// The sphere bottom lines up with the tire bottom, so the sweep distance is the hub drop.
		const float sphereRadius = radius * suspension.SphereTraceRadiusRatio;
		runtime.traceStart = hubTopLocation - frame.up * (radius - sphereRadius);
		runtime.traceEnd = runtime.traceStart - frame.up * travel;
		bHasHit = GetWorld()->SweepSingleByChannel(hit, runtime.traceStart, runtime.traceEnd, FQuat::Identity,
												   suspension.TraceChannel, FCollisionShape::MakeSphere(sphereRadius), queryParams);
		if (bHasHit)
		{
			hubDrop = hit.bStartPenetrating ? 0.0f : hit.Distance;
		}
	}
	else
	{
		runtime.traceStart = hubTopLocation;
		runtime.traceEnd = hubTopLocation - frame.up * (travel + radius);
		bHasHit = GetWorld()->LineTraceSingleByChannel(hit, runtime.traceStart, runtime.traceEnd, suspension.TraceChannel, queryParams);
		if (bHasHit)
		{
			hubDrop = hit.Distance - radius;
		}
	}

	// Compression can exceed the travel when the tire is pressed past the bump stop.
	state.bHasContact = bHasHit;
	state.compression = bHasHit ? (travel - hubDrop) * GArcadeVehicleUnits::metersPerCentimeter : 0.0f;
	state.compressionRatio = FMath::Clamp(state.compression / suspension.Travel, 0.0f, 1.0f);
	if (!bHasHit)
	{
		state.contactPoint = runtime.traceEnd;
		state.contactNormal = frame.up;
		state.surfaceFriction = 1.0f;
		runtime.contactVelocity = FVector::ZeroVector;
		return;
	}

	state.contactPoint = hit.ImpactPoint;
	state.contactNormal = hit.ImpactNormal;
	const float* surfaceMultiplier = hit.PhysMaterial.IsValid()
		? tuningData.Tires.SurfaceFrictionMultipliers.Find(hit.PhysMaterial.Get())
		: nullptr;
	state.surfaceFriction = surfaceMultiplier ? *surfaceMultiplier : 1.0f;

	FVector groundVelocity = FVector::ZeroVector;
	UPrimitiveComponent* groundComponent = hit.GetComponent();
	if (groundComponent && groundComponent->IsSimulatingPhysics())
	{
		groundVelocity = groundComponent->GetPhysicsLinearVelocityAtPoint(hit.ImpactPoint);
	}
	const FVector chassisVelocity = chassis->GetPhysicsLinearVelocityAtPoint(hit.ImpactPoint);
	runtime.contactVelocity = (chassisVelocity - groundVelocity) * GArcadeVehicleUnits::metersPerCentimeter;
}

void UGArcadeVehicleComponent::ComputeSuspensionForce(int32 wheelIndex, const FGArcadeChassisFrame& frame)
{
	FGArcadeWheelState& state = wheelStates[wheelIndex];
	const FGArcadeWheelRuntime& runtime = wheelRuntimes[wheelIndex];
	if (!state.bHasContact)
	{
		state.suspensionForce = 0.0f;
		return;
	}

	const FGArcadeSuspensionSettings& suspension = GetTuningData().Suspension;
	const float springCompression = FMath::Clamp(state.compression, 0.0f, suspension.Travel);
	const float bumpStopCompression = FMath::Max(state.compression - suspension.Travel, 0.0f);
	const float compressionSpeed = -FVector::DotProduct(runtime.contactVelocity, frame.up);

	const float springForce = runtime.springStiffness * springCompression;
	const float bumpStopForce = runtime.springStiffness * suspension.BumpStopStiffnessMultiplier * bumpStopCompression;
	const bool bIsExtending = compressionSpeed < 0.0f;
	const float damperScale = bIsExtending ? suspension.ReboundDampingMultiplier : 1.0f;
	const float damperForce = runtime.damperCoefficient * damperScale * compressionSpeed;

	// The suspension can only push the chassis away from the ground, never pull it down.
	state.suspensionForce = FMath::Max(springForce + bumpStopForce + damperForce, 0.0f);
}

void UGArcadeVehicleComponent::ApplyAntiRollBars()
{
	const float antiRollRatio = GetTuningData().Suspension.AntiRollStiffnessRatio;
	if (antiRollRatio <= 0.0f)
	{
		return;
	}

	for (int32 wheelIndex = 0; wheelIndex < wheelStates.Num(); ++wheelIndex)
	{
		const int32 partnerIndex = wheelRuntimes[wheelIndex].antiRollPartnerIndex;
		FGArcadeWheelState& state = wheelStates[wheelIndex];
		const bool bHasPartnerContact = wheelStates.IsValidIndex(partnerIndex) && wheelStates[partnerIndex].bHasContact;
		if (!state.bHasContact || !bHasPartnerContact)
		{
			continue;
		}

		// The more compressed side is pushed up and its partner relieved, which resists body roll.
		const float compressionDifference = state.compression - wheelStates[partnerIndex].compression;
		const float antiRollForce = antiRollRatio * wheelRuntimes[wheelIndex].springStiffness * compressionDifference;
		state.suspensionForce = FMath::Max(state.suspensionForce + antiRollForce, 0.0f);
	}
}

void UGArcadeVehicleComponent::ApplySuspensionForce(int32 wheelIndex, const FGArcadeChassisFrame& frame)
{
	const FGArcadeTireSettings& tires = GetTuningData().Tires;
	FGArcadeWheelState& state = wheelStates[wheelIndex];
	FGArcadeWheelRuntime& runtime = wheelRuntimes[wheelIndex];

	// The suspension acts along the chassis up axis; only its part along the ground normal loads the tire.
	const float normalAlignment = FMath::Max(FVector::DotProduct(frame.up, state.contactNormal), 0.0f);
	const float maximumLoad = tires.MaxLoadRatio * runtime.cornerMass * frame.gravity.Size();
	const float targetLoad = state.bHasContact ? FMath::Min(state.suspensionForce * normalAlignment, maximumLoad) : 0.0f;
	const float smoothingAlpha = tires.LoadSmoothingTime > 0.0f
		? 1.0f - FMath::Exp(-frame.deltaTime / tires.LoadSmoothingTime)
		: 1.0f;
	runtime.smoothedLoad = FMath::Lerp(runtime.smoothedLoad, targetLoad, smoothingAlpha);
	state.normalLoad = state.bHasContact ? runtime.smoothedLoad : 0.0f;

	if (!state.bHasContact || state.suspensionForce <= 0.0f)
	{
		return;
	}
	const FVector suspensionForce = frame.up * (state.suspensionForce * GArcadeVehicleUnits::centimetersPerMeter);
	chassis->AddForceAtLocation(suspensionForce, state.contactPoint);
}

void UGArcadeVehicleComponent::UpdateDrift(const FGArcadeChassisFrame& frame)
{
	const float lateralSpeed = FVector::DotProduct(frame.velocity, frame.right);
	const bool bIsMoving = frame.velocity.Size() > slipAngleMinimumSpeed;
	bodySlipAngle = bIsMoving ? FMath::RadiansToDegrees(FMath::Atan2(lateralSpeed, FMath::Abs(frame.forwardSpeed))) : 0.0f;
	const float travelAngle = bIsMoving ? FMath::RadiansToDegrees(FMath::Atan2(lateralSpeed, frame.forwardSpeed)) : 0.0f;

	FGArcadeDriftInput input;
	input.throttle = vehicleInput.throttle;
	input.brake = vehicleInput.brake;
	input.steering = vehicleInput.steering;
	input.bHandbrake = vehicleInput.bHandbrake;
	input.forwardSpeed = frame.forwardSpeed;
	input.bodySlipAngle = bodySlipAngle;
	input.travelAngle = travelAngle;
	input.yawRate = FVector::DotProduct(frame.angularVelocity, frame.up);
	input.planarSpeed = FVector::VectorPlaneProject(frame.velocity, frame.up).Size();
	input.bIsGrounded = frame.groundedWheelCount > 0;
	driftController.Update(input, GetTuningData().Drift, frame.deltaTime);
}

void UGArcadeVehicleComponent::UpdateSteering(const FGArcadeChassisFrame& frame)
{
	const FGArcadeSteeringSettings& steering = GetTuningData().Steering;
	const float maxAngle = ComputeMaxSteerAngle(FMath::Abs(frame.forwardSpeed));
	const float targetAngle = FMath::Clamp(vehicleInput.steering * maxAngle + ComputeCounterSteerAngle(frame), -maxAngle, maxAngle);

	const bool bIsCrossingCentre = targetAngle * currentSteerAngle < 0.0f;
	const bool bIsReturning = bIsCrossingCentre || FMath::Abs(targetAngle) < FMath::Abs(currentSteerAngle);
	const float steerRate = bIsReturning ? steering.ReturnRate : steering.SteerRate;
	currentSteerAngle = FMath::FInterpConstantTo(currentSteerAngle, targetAngle, frame.deltaTime, steerRate);

	for (int32 wheelIndex = 0; wheelIndex < wheelStates.Num(); ++wheelIndex)
	{
		wheelStates[wheelIndex].steerAngle = IsRearWheel(wheelIndex) ? 0.0f : currentSteerAngle;
	}
}

float UGArcadeVehicleComponent::ComputeCounterSteerAngle(const FGArcadeChassisFrame& frame) const
{
	const FGArcadeAssistSettings& assists = GetTuningData().Assists;
	const bool bCanCounterSteer = assists.bEnableCounterSteerAssist && frame.groundedWheelCount > 0
		&& frame.forwardSpeed > counterSteerMinimumSpeed && !driftController.IsDrifting();
	if (!bCanCounterSteer)
	{
		return 0.0f;
	}

	// Steering towards the direction of travel catches a slide before it turns into a spin. Inside the
	// activation angle the slip is ordinary cornering, and steering against it would only widen the turn.
	const float excessSlipAngle = FMath::Max(FMath::Abs(bodySlipAngle) - assists.CounterSteerActivationAngle, 0.0f);
	return FMath::Sign(bodySlipAngle) * excessSlipAngle * assists.CounterSteerStrength;
}

float UGArcadeVehicleComponent::ComputeMaxSteerAngle(float speed) const
{
	const FGArcadeSteeringSettings& steering = GetTuningData().Steering;
	const float speedKilometersPerHour = speed * GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond;
	if (steering.MaxAngleBySpeedCurve)
	{
		return FMath::Max(steering.MaxAngleBySpeedCurve->GetFloatValue(speedKilometersPerHour), 0.0f);
	}

	const float speedAlpha = FMath::Clamp(speedKilometersPerHour / steering.HighSpeed, 0.0f, 1.0f);
	const float falloff = FMath::Pow(speedAlpha, steering.SpeedFalloffExponent);
	return FMath::Lerp(steering.MaxAngleAtRest, steering.MaxAngleAtHighSpeed, falloff);
}

void UGArcadeVehicleComponent::UpdateDrivetrain(const FGArcadeChassisFrame& frame)
{
	const UGArcadeVehicleTuning& tuningData = GetTuningData();

	FGArcadeDrivetrainInput input;
	input.throttle = vehicleInput.throttle;
	input.brake = vehicleInput.brake;
	input.forwardSpeed = frame.forwardSpeed;
	input.torqueMultiplier = engineTorqueMultiplier;
	input.bHasGroundContact = frame.groundedWheelCount > 0;
	float drivenWheelRadius = 0.0f;
	for (int32 wheelIndex = 0; wheelIndex < wheelStates.Num(); ++wheelIndex)
	{
		const float torqueShare = wheelRuntimes[wheelIndex].driveTorqueShare;
		input.drivenWheelAngularVelocity += torqueShare * wheelStates[wheelIndex].angularVelocity;
		drivenWheelRadius += torqueShare * GetWheelRadius(wheelIndex);
	}
	input.roadWheelAngularVelocity = frame.forwardSpeed / FMath::Max(drivenWheelRadius, UE_KINDA_SMALL_NUMBER);

	drivetrain.Update(input, tuningData.Engine, tuningData.Transmission, frame.deltaTime);
}

float UGArcadeVehicleComponent::ComputeTractionScale() const
{
	const FGArcadeAssistSettings& assists = GetTuningData().Assists;
	if (!assists.bEnableTractionLimit)
	{
		return 1.0f;
	}

	float weightedSlip = 0.0f;
	float totalWeight = 0.0f;
	for (int32 wheelIndex = 0; wheelIndex < wheelStates.Num(); ++wheelIndex)
	{
		const float torqueShare = wheelRuntimes[wheelIndex].driveTorqueShare;
		if (wheelStates[wheelIndex].bHasContact && torqueShare > 0.0f)
		{
			weightedSlip += torqueShare * FMath::Abs(wheelStates[wheelIndex].slipRatio);
			totalWeight += torqueShare;
		}
	}
	if (totalWeight <= UE_KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}

	const float averageSlip = weightedSlip / totalWeight;
	if (averageSlip <= assists.TractionLimitSlip)
	{
		return 1.0f;
	}
	return FMath::Clamp(assists.TractionLimitSlip / averageSlip, assists.TractionLimitMinimumScale, 1.0f);
}

void UGArcadeVehicleComponent::UpdateTires(const FGArcadeChassisFrame& frame)
{
	const float tractionScale = ComputeTractionScale();
	const bool bShouldHold = ShouldHoldStandstill(frame);
	const FVector holdForce = bShouldHold ? ApplyStandstillHold(frame) : FVector::ZeroVector;
	float groundedLoad = 0.0f;
	for (const FGArcadeWheelState& state : wheelStates)
	{
		groundedLoad += state.bHasContact ? state.normalLoad : 0.0f;
	}

	for (int32 wheelIndex = 0; wheelIndex < wheelStates.Num(); ++wheelIndex)
	{
		if (!wheelStates[wheelIndex].bHasContact)
		{
			UpdateAirborneWheel(wheelIndex, frame, tractionScale);
		}
		else if (bShouldHold)
		{
			const float loadShare = wheelStates[wheelIndex].normalLoad / FMath::Max(groundedLoad, UE_KINDA_SMALL_NUMBER);
			HoldWheel(wheelIndex, frame, holdForce * loadShare);
		}
		else
		{
			UpdateGroundedWheel(wheelIndex, frame, tractionScale);
		}

		FGArcadeWheelState& state = wheelStates[wheelIndex];
		state.spinAngle = FMath::Fmod(state.spinAngle + state.angularVelocity * frame.deltaTime, GArcadeVehicleUnits::fullTurnRadians);
	}
}

bool UGArcadeVehicleComponent::ShouldHoldStandstill(const FGArcadeChassisFrame& frame) const
{
	const bool bIsSlow = frame.velocity.Size() < GetTuningData().Tires.StandstillSpeed;
	const bool bHasNoDriveRequest = drivetrain.GetDriveDemand() < holdDriveDemandThreshold;
	const bool bIsSupported = frame.groundedWheelCount * 2 > wheelStates.Num();
	return bIsSlow && bHasNoDriveRequest && bIsSupported;
}

void UGArcadeVehicleComponent::UpdateGroundedWheel(int32 wheelIndex, const FGArcadeChassisFrame& frame, float tractionScale)
{
	const UGArcadeVehicleTuning& tuningData = GetTuningData();
	const FGArcadeTireSettings& tires = tuningData.Tires;
	FGArcadeWheelState& state = wheelStates[wheelIndex];
	FGArcadeWheelRuntime& runtime = wheelRuntimes[wheelIndex];
	const float radius = GetWheelRadius(wheelIndex);

	const FVector wheelForward = frame.forward.RotateAngleAxis(state.steerAngle, frame.up);
	runtime.lateralDirection = FVector::CrossProduct(state.contactNormal, wheelForward).GetSafeNormal();
	runtime.longitudinalDirection = FVector::CrossProduct(runtime.lateralDirection, state.contactNormal).GetSafeNormal();
	const float longitudinalSpeed = FVector::DotProduct(runtime.contactVelocity, runtime.longitudinalDirection);
	const float lateralSpeed = FVector::DotProduct(runtime.contactVelocity, runtime.lateralDirection);

	// Handbrake cornering loss comes from wheel slip; controlled drifts use a modest rear grip reduction.
	const float grip = tires.FrictionCoefficient * state.surfaceFriction * runtime.gripMultiplier;
	const bool bIsRear = IsRearWheel(wheelIndex);
	const bool bIsHandbrakeWheel = vehicleInput.bHandbrake && bIsRear;
	const bool bUsePhysicalSlides = tuningData.Drift.bUsePhysicalSlides;
	const float handbrakeGripScale = bIsHandbrakeWheel && !bUsePhysicalSlides ? tuningData.Brakes.HandbrakeLateralGripMultiplier : 1.0f;
	const float slideGripScale = bIsRear ? driftController.GetRearGripScale() : 1.0f;
	const float lateralGrip = grip * FMath::Min(handbrakeGripScale, slideGripScale);
	const float frictionLimit = grip * state.normalLoad;

	FGArcadeWheelSpinInput spinInput;
	spinInput.angularVelocity = state.angularVelocity;
	spinInput.inertia = GetWheelInertia(wheelIndex);
	spinInput.radius = radius;
	spinInput.driveTorque = ComputeDriveTorque(wheelIndex, tractionScale);
	spinInput.brakeTorque = ComputeBrakeTorque(wheelIndex);
	spinInput.longitudinalSpeed = longitudinalSpeed;
	spinInput.referenceSpeed = FMath::Max(FMath::Abs(longitudinalSpeed), tires.SlipReferenceSpeed);
	spinInput.deltaTime = frame.deltaTime;

	if (bUsePhysicalSlides && tuningData.Assists.bEnableTractionLimit && drivetrain.GetDriveDemand() > holdDriveDemandThreshold)
	{
		// Allow the first flare of wheelspin. As it grows, bring drive torque towards the
		// torque the sliding tire can transmit so the wheel can catch up with road speed.
		const float launchWeight = 1.0f - FMath::SmoothStep(2.0f, 6.0f, static_cast<float>(frame.velocity.Size()));
		const float driveDirection = FMath::Sign(spinInput.driveTorque);
		const float drivenSlip = state.slipRatio * driveDirection;
		const float permittedSlip = FMath::Max(0.25f, tires.LongitudinalCurve.PeakSlip * 2.0f);
		const float slipWeight = FMath::SmoothStep(permittedSlip, permittedSlip + 0.65f, drivenSlip);
		const float slidingGrip = FMath::Clamp(tires.LongitudinalCurve.FloorValue, 0.0f, 1.0f);
		const float recoveryTorque = frictionLimit * slidingGrip * radius * 0.9f;
		const float requestedTorque = FMath::Abs(spinInput.driveTorque);
		const float limitedTorque = FMath::Min(requestedTorque, recoveryTorque);
		spinInput.driveTorque = driveDirection * FMath::Lerp(requestedTorque, limitedTorque, launchWeight * slipWeight);
	}

	state.slipAngle = FGArcadeTireModel::ComputeSlipAngle(lateralSpeed, longitudinalSpeed, tires.SlipAngleReferenceSpeed);
	float lateralForce = -FGArcadeTireModel::EvaluateCurve(tires.LateralCurve, state.slipAngle) * lateralGrip * state.normalLoad;

	// Within one step a friction force must not reverse the motion it resists; slow cars would jitter otherwise.
	const float stoppingForcePerSpeed = runtime.cornerMass / frame.deltaTime;
	const float lateralStoppingForce = FMath::Abs(lateralSpeed) * stoppingForcePerSpeed;
	lateralForce = FMath::Clamp(lateralForce, -lateralStoppingForce, lateralStoppingForce);

	FGArcadeTireContactInput contactInput;
	contactInput.spin = spinInput;
	contactInput.frictionLimit = frictionLimit;
	contactInput.lateralForce = lateralForce;
	contactInput.lateralSuppression = bUsePhysicalSlides ? 0.0f : tires.LateralSuppression;
	contactInput.maxBrakingForce = FMath::Abs(longitudinalSpeed) * stoppingForcePerSpeed;
	contactInput.bUseCombinedSlip = bUsePhysicalSlides;
	contactInput.lateralSpeed = lateralSpeed;
	contactInput.maxLateralForce = lateralStoppingForce;
	const FGArcadeTireContactResult contact = FGArcadeTireModel::SolveContact(contactInput, tires.LongitudinalCurve);
	state.angularVelocity = contact.angularVelocity;
	state.slipRatio = FGArcadeTireModel::ComputeSlipRatio(state.angularVelocity * radius, longitudinalSpeed, tires.SlipReferenceSpeed);
	const FVector2D tireForce = contact.force;
	const bool bIsPastLongitudinalPeak = FMath::Abs(state.slipRatio) > tires.LongitudinalCurve.PeakSlip;
	const bool bIsPastLateralPeak = FMath::Abs(state.slipAngle) > tires.LateralCurve.PeakSlip;
	state.longitudinalForce = tireForce.X;
	state.lateralForce = tireForce.Y;
	state.bIsSliding = contact.bIsSaturated || bIsPastLongitudinalPeak || bIsPastLateralPeak;
	runtime.previousSlipRatio = state.slipRatio;

	const FVector worldForce = runtime.longitudinalDirection * tireForce.X + runtime.lateralDirection * tireForce.Y;
	chassis->AddForceAtLocation(worldForce * GArcadeVehicleUnits::centimetersPerMeter, ComputeForceApplicationPoint(wheelIndex, frame));
}

void UGArcadeVehicleComponent::UpdateAirborneWheel(int32 wheelIndex, const FGArcadeChassisFrame& frame, float tractionScale)
{
	const FGArcadeWheelSettings& wheels = GetTuningData().Wheels;
	FGArcadeWheelState& state = wheelStates[wheelIndex];

	FGArcadeWheelSpinInput spinInput;
	spinInput.angularVelocity = state.angularVelocity;
	spinInput.inertia = GetWheelInertia(wheelIndex);
	spinInput.radius = GetWheelRadius(wheelIndex);
	spinInput.driveTorque = ComputeDriveTorque(wheelIndex, tractionScale);
	spinInput.brakeTorque = ComputeBrakeTorque(wheelIndex);
	spinInput.deltaTime = frame.deltaTime;
	const float spinDecay = FMath::Exp(-wheels.AirSpinDamping * frame.deltaTime);
	state.angularVelocity = FGArcadeTireModel::IntegrateWheelSpin(spinInput) * spinDecay;

	state.slipRatio = 0.0f;
	state.slipAngle = 0.0f;
	state.longitudinalForce = 0.0f;
	state.lateralForce = 0.0f;
	state.bIsSliding = false;
	wheelRuntimes[wheelIndex].previousSlipRatio = 0.0f;
}

FVector UGArcadeVehicleComponent::ApplyStandstillHold(const FGArcadeChassisFrame& frame)
{
	const FGArcadeTireSettings& tires = GetTuningData().Tires;
	FVector groundNormal = FVector::ZeroVector;
	float frictionLimit = 0.0f;
	for (int32 wheelIndex = 0; wheelIndex < wheelStates.Num(); ++wheelIndex)
	{
		const FGArcadeWheelState& state = wheelStates[wheelIndex];
		if (state.bHasContact)
		{
			groundNormal += state.contactNormal;
			frictionLimit += tires.FrictionCoefficient * state.surfaceFriction * wheelRuntimes[wheelIndex].gripMultiplier * state.normalLoad;
		}
	}
	groundNormal = groundNormal.IsNearlyZero() ? frame.up : groundNormal.GetSafeNormal();

	// Static friction for the whole car: cancel the slope's pull and part of the creep every step. Acting on the
	// centre of mass instead of the contact patches keeps it from rocking the body, and removing only part of
	// the velocity per step keeps it stable at any frame rate.
	const float mass = chassis->GetMass();
	const FVector tangentVelocity = FVector::VectorPlaneProject(frame.velocity, groundNormal);
	const FVector tangentGravity = FVector::VectorPlaneProject(frame.gravity, groundNormal);
	const FVector requestedForce = -mass * (tangentVelocity * (standstillVelocityGain / frame.deltaTime) + tangentGravity);
	const FVector holdForce = requestedForce.GetClampedToMaxSize(frictionLimit);
	chassis->AddForce(holdForce * GArcadeVehicleUnits::centimetersPerMeter);

	const float yawRate = FVector::DotProduct(frame.angularVelocity, frame.up);
	chassis->AddTorqueInRadians(-frame.up * (yawRate * standstillVelocityGain / frame.deltaTime), NAME_None, true);
	return holdForce;
}

void UGArcadeVehicleComponent::HoldWheel(int32 wheelIndex, const FGArcadeChassisFrame& frame, const FVector& wheelHoldForce)
{
	FGArcadeWheelState& state = wheelStates[wheelIndex];
	FGArcadeWheelRuntime& runtime = wheelRuntimes[wheelIndex];
	state.angularVelocity = 0.0f;
	state.slipRatio = 0.0f;
	state.slipAngle = 0.0f;
	state.longitudinalForce = FVector::DotProduct(wheelHoldForce, frame.forward);
	state.lateralForce = FVector::DotProduct(wheelHoldForce, frame.right);
	state.bIsSliding = false;
	runtime.longitudinalDirection = frame.forward;
	runtime.lateralDirection = frame.right;
	runtime.previousSlipRatio = 0.0f;
}

float UGArcadeVehicleComponent::ComputeDriveTorque(int32 wheelIndex, float tractionScale) const
{
	// Disconnect the rear drive while the handbrake is held so engine torque cannot defeat the lock.
	if (GetTuningData().Drift.bUsePhysicalSlides && vehicleInput.bHandbrake && IsRearWheel(wheelIndex))
	{
		return 0.0f;
	}
	const float brakePriorityScale = 1.0f - drivetrain.GetBrakeDemand();
	return drivetrain.GetWheelTorque() * wheelRuntimes[wheelIndex].driveTorqueShare * tractionScale * brakePriorityScale;
}

float UGArcadeVehicleComponent::ComputeBrakeTorque(int32 wheelIndex) const
{
	const UGArcadeVehicleTuning& tuningData = GetTuningData();
	const FGArcadeBrakeSettings& brakes = tuningData.Brakes;
	// Preserve the short brake tap that initiates a controlled drift. Handbrake turns keep full braking.
	const float slideBrakeScale = tuningData.Drift.bUsePhysicalSlides && driftController.GetMode() == EGArcadeSlideMode::HandbrakeTurn
		? 1.0f
		: FMath::Lerp(1.0f, tuningData.Drift.BrakeScale, driftController.GetDriftBlend());
	// Overlapping pedals restore service braking as the drive request increases.
	const float driftBrakeScale = FMath::Lerp(slideBrakeScale, 1.0f, drivetrain.GetDriveDemand());
	const float serviceBrakeTorque = drivetrain.GetBrakeDemand() * brakes.TotalBrakeTorque * wheelRuntimes[wheelIndex].brakeTorqueShare * driftBrakeScale;
	const bool bIsHandbrakeWheel = vehicleInput.bHandbrake && IsRearWheel(wheelIndex);
	const float handbrakeTorque = bIsHandbrakeWheel ? brakes.HandbrakeTorque : 0.0f;
	return serviceBrakeTorque + handbrakeTorque;
}

FVector UGArcadeVehicleComponent::ComputeForceApplicationPoint(int32 wheelIndex, const FGArcadeChassisFrame& frame) const
{
	// Lifting the point towards the centre of mass shortens the lever that rolls the body in corners.
	const FGArcadeWheelState& state = wheelStates[wheelIndex];
	const float heightRatio = GetTuningData().Tires.ForceApplicationHeightRatio;
	const float centerOfMassHeight = FVector::DotProduct(frame.centerOfMass - state.contactPoint, frame.up);
	return state.contactPoint + frame.up * (centerOfMassHeight * heightRatio);
}

void UGArcadeVehicleComponent::ApplyAerodynamics(const FGArcadeChassisFrame& frame)
{
	const FGArcadeAeroSettings& aero = GetTuningData().Aero;
	const float speed = frame.velocity.Size();
	FVector force = -aero.DragCoefficient * speed * frame.velocity;

	// Downforce and rolling resistance only make sense while the tires touch the ground.
	if (frame.groundedWheelCount > 0)
	{
		const float dynamicPressure = 0.5f * aero.AirDensity * speed * speed;
		const float groundedFraction = static_cast<float>(frame.groundedWheelCount) / static_cast<float>(wheelStates.Num());
		const FVector planarVelocity = FVector::VectorPlaneProject(frame.velocity, frame.up);
		force -= frame.up * (dynamicPressure * aero.DownforceArea);
		force -= planarVelocity * (aero.RollingResistance * groundedFraction);
	}
	chassis->AddForce(force * GArcadeVehicleUnits::centimetersPerMeter);
}

void UGArcadeVehicleComponent::ApplyStabilization(const FGArcadeChassisFrame& frame)
{
	const bool bIsAirborne = frame.groundedWheelCount == 0;
	const FVector angularAcceleration = bIsAirborne ? ComputeAirControl(frame) : ComputeGroundYawAssists(frame);
	if (!angularAcceleration.IsNearlyZero())
	{
		chassis->AddTorqueInRadians(angularAcceleration, NAME_None, true);
	}
}

FVector UGArcadeVehicleComponent::ComputeAirControl(const FGArcadeChassisFrame& frame) const
{
	const FGArcadeAirSettings& air = GetTuningData().Air;
	if (!air.bEnableAirControl)
	{
		return FVector::ZeroVector;
	}
	// The cross product's length is the sine of the tilt, so the correction fades out near the target attitude.
	const FVector levelingAxis = FVector::CrossProduct(frame.up, ComputeAirTargetUp(frame));
	return levelingAxis * air.LevelingStrength - frame.angularVelocity * air.AngularDamping;
}

FVector UGArcadeVehicleComponent::ComputeAirTargetUp(const FGArcadeChassisFrame& frame) const
{
	const UGArcadeVehicleTuning& tuningData = GetTuningData();
	const FGArcadeAirSettings& air = tuningData.Air;
	const FVector gravityDirection = frame.gravity.GetSafeNormal();
	const FVector worldUp = gravityDirection.IsNearlyZero() ? FVector::UpVector : -gravityDirection;

	// Pitch with the flight path: the nose rises off a ramp and dips on the way down. Only pitch along the
	// car's own forward axis is used, faded out when flying sideways or slowly, so a near-vertical fall
	// cannot pick an arbitrary direction and roll the car.
	const FVector forwardOnGround = FVector::VectorPlaneProject(frame.forward, worldUp).GetSafeNormal();
	const FVector velocityOnGround = FVector::VectorPlaneProject(frame.velocity, worldUp);
	const float groundSpeed = velocityOnGround.Size();
	const float forwardAlignment = groundSpeed > UE_KINDA_SMALL_NUMBER ? FVector::DotProduct(velocityOnGround / groundSpeed, forwardOnGround) : 0.0f;
	const float speedWeight = FMath::Clamp(groundSpeed / trajectoryAlignmentFullSpeed, 0.0f, 1.0f);
	const float climbAngle = FMath::Atan2(FVector::DotProduct(frame.velocity, worldUp), FMath::Max(groundSpeed, UE_KINDA_SMALL_NUMBER));
	const float targetPitch = FMath::Clamp(climbAngle * air.TrajectoryAlignment * forwardAlignment * speedWeight,
										   -trajectoryMaxPitch, trajectoryMaxPitch);
	FVector targetUp = (worldUp * FMath::Cos(targetPitch) - forwardOnGround * FMath::Sin(targetPitch)).GetSafeNormal();

	// The closer the ground below, the more the attitude turns to its surface, so the car lands on all wheels.
	const float probeLength = air.LandingProbeDistance * GArcadeVehicleUnits::centimetersPerMeter;
	FHitResult hit;
	const FVector probeEnd = frame.centerOfMass - worldUp * probeLength;
	const bool bHasGroundBelow = probeLength > 0.0f
		&& GetWorld()->LineTraceSingleByChannel(hit, frame.centerOfMass, probeEnd, tuningData.Suspension.TraceChannel, BuildSuspensionQueryParams());
	if (bHasGroundBelow)
	{
		const float landingWeight = 1.0f - hit.Time;
		targetUp = FMath::Lerp(targetUp, FVector(hit.ImpactNormal), landingWeight).GetSafeNormal();
	}
	return targetUp.IsNearlyZero() ? worldUp : targetUp;
}

void UGArcadeVehicleComponent::ApplyExtraGravity(const FGArcadeChassisFrame& frame)
{
	airborneTime = frame.groundedWheelCount == 0 ? airborneTime + frame.deltaTime : 0.0f;
	const FGArcadeAirSettings& air = GetTuningData().Air;
	const float extraGravityScale = air.GravityScale - 1.0f;
	if (airborneTime <= 0.0f || extraGravityScale <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}
	// Ramping in keeps bumps and short hops on real gravity; a proper jump gets the full weight.
	const float rampAlpha = air.GravityRampTime > 0.0f ? FMath::Clamp(airborneTime / air.GravityRampTime, 0.0f, 1.0f) : 1.0f;
	chassis->AddForce(frame.gravity * (extraGravityScale * rampAlpha * GArcadeVehicleUnits::centimetersPerMeter), NAME_None, true);
}

FVector UGArcadeVehicleComponent::ComputeGroundYawAssists(const FGArcadeChassisFrame& frame) const
{
	const FGArcadeAssistSettings& assists = GetTuningData().Assists;
	const float yawRate = FVector::DotProduct(frame.angularVelocity, frame.up);
	const float speed = FMath::Abs(frame.forwardSpeed);
	const float corneringMinimumSpeed = assists.CorneringAssistMinimumSpeed / GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond;
	const bool bHasSteeringInput = HasSteeringInput();

	// Driving assists fade out during a slide. Scripted yaw is only supplied by the legacy drift mode.
	float yawAcceleration = driftController.GetYawAcceleration();
	const float gripScale = 1.0f - driftController.GetDriftBlend();

	const bool bCanAssistTurn = assists.bEnableTurnAssist && bHasSteeringInput && speed > corneringMinimumSpeed
		&& wheelbase > UE_KINDA_SMALL_NUMBER;
	if (bCanAssistTurn)
	{
		// At speed, the full-lock geometry asks for far more yaw than the lateral acceleration limit allows, so a
		// geometric target would saturate at a fraction of the input and steering would feel like a switch. The
		// target is proportional to the steering share instead; full steering reaches the lower of both limits.
		const float maxSteerAngle = ComputeMaxSteerAngle(speed);
		const float steeringShare = maxSteerAngle > UE_KINDA_SMALL_NUMBER ? FMath::Clamp(currentSteerAngle / maxSteerAngle, -1.0f, 1.0f) : 0.0f;
		const float fullLockYawRate = speed * FMath::Tan(FMath::DegreesToRadians(maxSteerAngle)) / wheelbase;
		const float lateralLimitYawRate = assists.TurnAssistLateralAcceleration / speed;
		const float travelDirection = FMath::Sign(frame.forwardSpeed);
		const float targetYawRate = travelDirection * steeringShare * FMath::Min(fullLockYawRate, lateralLimitYawRate);
		yawAcceleration += (targetYawRate - yawRate) * assists.TurnAssistStrength * gripScale;
	}
	if (assists.bEnableYawDamping && !bHasSteeringInput && !driftController.IsDrifting())
	{
		yawAcceleration -= yawRate * assists.YawDampingStrength;
	}

	FVector angularAcceleration = frame.up * yawAcceleration;
	if (assists.bEnableRollStabilization)
	{
		const float rollRate = FVector::DotProduct(frame.angularVelocity, frame.forward);
		angularAcceleration -= frame.forward * (rollRate * assists.RollDampingStrength);
	}
	return angularAcceleration;
}

void UGArcadeVehicleComponent::ApplyAssistForces(const FGArcadeChassisFrame& frame)
{
	if (frame.groundedWheelCount == 0)
	{
		return;
	}

	const FGArcadeAssistSettings& assists = GetTuningData().Assists;
	const float speed = FMath::Abs(frame.forwardSpeed);
	const float corneringMinimumSpeed = assists.CorneringAssistMinimumSpeed / GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond;
	FVector acceleration = FVector::ZeroVector;

	if (assists.bEnableGripAssist && speed > corneringMinimumSpeed)
	{
		// Removing part of the sideways velocity every second makes the car follow its nose instead of washing wide.
		const float lateralSpeed = FVector::DotProduct(frame.velocity, frame.right);
		const float groundedFraction = static_cast<float>(frame.groundedWheelCount) / static_cast<float>(wheelStates.Num());
		const float gripScale = (1.0f - driftController.GetDriftBlend()) * groundedFraction;
		const float lateralAcceleration = -lateralSpeed * assists.GripAssistRate * gripScale;
		acceleration += frame.right * FMath::Clamp(lateralAcceleration, -assists.GripAssistMaxAcceleration, assists.GripAssistMaxAcceleration);
	}

	// The drift channels act along and across the actual path, not the chassis axes.
	const FVector travelDirection = FVector::VectorPlaneProject(frame.velocity, frame.up).GetSafeNormal();
	const FVector rightOfTravel = FVector::CrossProduct(frame.up, travelDirection);
	acceleration += travelDirection * driftController.GetSpeedHoldAcceleration();
	acceleration += rightOfTravel * driftController.GetPathAcceleration();

	if (!acceleration.IsNearlyZero())
	{
		chassis->AddForce(acceleration * GArcadeVehicleUnits::centimetersPerMeter, NAME_None, true);
	}
}

void UGArcadeVehicleComponent::UpdateWheelVisuals(float deltaTime)
{
	const FGArcadeSuspensionSettings& suspension = GetTuningData().Suspension;
	const float travel = suspension.Travel * GArcadeVehicleUnits::centimetersPerMeter;
	const float staticCompression = travel * suspension.StaticSagRatio;
	for (int32 wheelIndex = 0; wheelIndex < wheelRuntimes.Num(); ++wheelIndex)
	{
		FGArcadeWheelRuntime& runtime = wheelRuntimes[wheelIndex];
		USceneComponent* visual = runtime.visualComponent.Get();
		if (!visual)
		{
			continue;
		}

		const FGArcadeWheelState& state = wheelStates[wheelIndex];
		const float targetCompression = FMath::Clamp(state.compression * GArcadeVehicleUnits::centimetersPerMeter, 0.0f, travel);
		runtime.visualCompression = FMath::FInterpTo(runtime.visualCompression, targetCompression, deltaTime, visualFollowSpeed);

		// Rotate about the hub: move the hub to the origin, steer and spin, then move it to its current height.
		const FQuat steerRotation(FVector::UpVector, FMath::DegreesToRadians(state.steerAngle));
		const FQuat spinRotation(FVector::RightVector, state.spinAngle);
		const FVector hubLocation = runtime.hubLocation + FVector::UpVector * (runtime.visualCompression - staticCompression);
		const FTransform hubToOrigin(FQuat::Identity, -runtime.hubLocation);
		const FTransform wheelRotation(steerRotation * spinRotation);
		const FTransform originToHub(FQuat::Identity, hubLocation);
		visual->SetRelativeTransform(runtime.visualRestTransform * hubToOrigin * wheelRotation * originToHub);
	}
}

void UGArcadeVehicleComponent::DrawDebug(const FGArcadeChassisFrame& frame) const
{
#if ENABLE_DRAW_DEBUG
	const bool bShouldDraw = bDrawDebug || arcadeVehicleDebugVariable.GetValueOnGameThread() > 0;
	if (!bShouldDraw)
	{
		return;
	}

	const UWorld* world = GetWorld();
	for (int32 wheelIndex = 0; wheelIndex < wheelStates.Num(); ++wheelIndex)
	{
		const FGArcadeWheelState& state = wheelStates[wheelIndex];
		const FGArcadeWheelRuntime& runtime = wheelRuntimes[wheelIndex];
		const FColor traceColor = state.bHasContact ? FColor::Green : FColor::Red;
		DrawDebugLine(world, runtime.traceStart, runtime.traceEnd, traceColor, false, -1.0f, 0, 1.0f);
		if (!state.bHasContact)
		{
			continue;
		}

		const FVector contact = state.contactPoint;
		const FVector suspensionArrow = frame.up * (state.suspensionForce * debugForceScale);
		const FVector longitudinalArrow = runtime.longitudinalDirection * (state.longitudinalForce * debugForceScale);
		const FVector lateralArrow = runtime.lateralDirection * (state.lateralForce * debugForceScale);
		DrawDebugPoint(world, contact, 8.0f, state.bIsSliding ? FColor::Orange : FColor::Yellow, false, -1.0f);
		DrawDebugDirectionalArrow(world, contact, contact + suspensionArrow, 10.0f, FColor::Blue, false, -1.0f, 0, 1.5f);
		DrawDebugDirectionalArrow(world, contact, contact + longitudinalArrow, 10.0f, FColor::Red, false, -1.0f, 0, 1.5f);
		DrawDebugDirectionalArrow(world, contact, contact + lateralArrow, 10.0f, FColor::Cyan, false, -1.0f, 0, 1.5f);
	}
	DrawDebugSphere(world, frame.centerOfMass, 12.0f, 12, FColor::Magenta, false, -1.0f);
#endif
}

bool UGArcadeVehicleComponent::IsRearWheel(int32 wheelIndex) const
{
	return wheelSetups.IsValidIndex(wheelIndex) && wheelSetups[wheelIndex].axle == EGArcadeAxle::Rear;
}

bool UGArcadeVehicleComponent::HasSteeringInput() const
{
	return FMath::Abs(vehicleInput.steering) > steeringDeadZone;
}

float UGArcadeVehicleComponent::GetWheelRadius(int32 wheelIndex) const
{
	const bool bHasVisualRadius = wheelRuntimes.IsValidIndex(wheelIndex) && wheelRuntimes[wheelIndex].visualRadius > UE_KINDA_SMALL_NUMBER;
	return bHasVisualRadius ? wheelRuntimes[wheelIndex].visualRadius : GetTuningData().Wheels.Radius;
}

float UGArcadeVehicleComponent::GetWheelInertia(int32 wheelIndex) const
{
	return 0.5f * GetTuningData().Wheels.Mass * FMath::Square(GetWheelRadius(wheelIndex));
}
