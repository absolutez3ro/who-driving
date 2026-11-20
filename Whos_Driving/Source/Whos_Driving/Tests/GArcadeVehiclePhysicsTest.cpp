#if WITH_DEV_AUTOMATION_TESTS

#include "Vehicle/ArcadePhysics/GArcadeDriftController.h"
#include "Vehicle/ArcadePhysics/GArcadeDrivetrain.h"
#include "Vehicle/ArcadePhysics/GArcadeTireModel.h"
#include "Vehicle/ArcadePhysics/GArcadeVehicleSettings.h"
#include "Vehicle/ArcadePhysics/GArcadeVehicleTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGArcadeTireCurveTest, "WhosDriving.ArcadeVehicle.TireCurve",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGArcadeTireCurveTest::RunTest(const FString& parameters)
{
	const FGArcadeTireCurve curve(0.1f, 1.0f, 0.5f, 0.7f);
	TestEqual(TEXT("No slip gives no force"), FGArcadeTireModel::EvaluateCurve(curve, 0.0f), 0.0f);
	TestEqual(TEXT("Linear rise below the peak"), FGArcadeTireModel::EvaluateCurve(curve, 0.05f), 0.5f, 1.0e-4f);
	TestEqual(TEXT("Peak value at the peak slip"), FGArcadeTireModel::EvaluateCurve(curve, 0.1f), 1.0f, 1.0e-4f);
	TestEqual(TEXT("Linear fall towards the floor"), FGArcadeTireModel::EvaluateCurve(curve, 0.3f), 0.85f, 1.0e-4f);
	TestEqual(TEXT("Flat floor past the floor slip"), FGArcadeTireModel::EvaluateCurve(curve, 3.0f), 0.7f, 1.0e-4f);
	TestEqual(TEXT("Curve is odd"), FGArcadeTireModel::EvaluateCurve(curve, -0.3f), -0.85f, 1.0e-4f);

	const float frictionLimit = 1000.0f;
	TestEqual(TEXT("Secant equals the initial slope below the peak"),
			  FGArcadeTireModel::ComputeSecantStiffness(curve, 0.05f, frictionLimit), 10000.0f, 1.0e-2f);
	TestTrue(TEXT("Secant softens past the peak"),
			 FGArcadeTireModel::ComputeSecantStiffness(curve, 1.0f, frictionLimit) < 1000.0f);
	TestEqual(TEXT("Slip ratio uses the reference speed at standstill"),
			  FGArcadeTireModel::ComputeSlipRatio(0.5f, 0.0f, 1.0f), 0.5f, 1.0e-4f);
	TestEqual(TEXT("Slip angle of a pure side slide"), FGArcadeTireModel::ComputeSlipAngle(10.0f, 10.0f, 1.0f), 45.0f, 1.0e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGArcadeFrictionCircleTest, "WhosDriving.ArcadeVehicle.FrictionCircle",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGArcadeFrictionCircleTest::RunTest(const FString& parameters)
{
	const FVector2D insideForce(300.0f, 400.0f);
	TestTrue(TEXT("Forces inside the circle pass through"),
			 FGArcadeTireModel::LimitToFrictionCircle(insideForce, 1000.0f, 0.3f).Equals(insideForce));

	const FVector2D outsideForce(1000.0f, 1000.0f);
	const FVector2D limitedForce = FGArcadeTireModel::LimitToFrictionCircle(outsideForce, 1000.0f, 0.3f);
	TestTrue(TEXT("Saturated force lies on the circle"), FMath::IsNearlyEqual(limitedForce.Size(), 1000.0, 1.0e-2));
	TestTrue(TEXT("Lateral force is suppressed more than longitudinal"), limitedForce.Y < limitedForce.X);
	TestTrue(TEXT("No grip without load"),
			 FGArcadeTireModel::LimitToFrictionCircle(outsideForce, 0.0f, 0.3f).IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGArcadeWheelSpinTest, "WhosDriving.ArcadeVehicle.WheelSpin",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGArcadeWheelSpinTest::RunTest(const FString& parameters)
{
	FGArcadeWheelSpinInput input;
	input.inertia = 1.9f;
	input.radius = 0.436f;
	input.referenceSpeed = 1.0f;
	input.deltaTime = 1.0f / 30.0f;

	input.angularVelocity = 2.0f;
	input.brakeTorque = 5000.0f;
	TestEqual(TEXT("Brake stops the wheel without reversing it"), FGArcadeTireModel::IntegrateWheelSpin(input), 0.0f);

	// A stiff tire on a slow car makes an explicit step explode; the implicit step must settle on rolling speed.
	input.brakeTorque = 0.0f;
	input.angularVelocity = 0.0f;
	input.longitudinalSpeed = 2.0f;
	input.tireStiffness = 90000.0f;
	const float rollingVelocity = input.longitudinalSpeed / input.radius;
	for (int32 step = 0; step < 10; ++step)
	{
		input.angularVelocity = FGArcadeTireModel::IntegrateWheelSpin(input);
		TestTrue(TEXT("Never overshoots the rolling speed"), input.angularVelocity <= rollingVelocity + 1.0e-3f);
	}
	TestEqual(TEXT("Converges to rolling speed"), input.angularVelocity, rollingVelocity, 1.0e-2f);

	input.angularVelocity = 0.0f;
	input.tireStiffness = 0.0f;
	input.driveTorque = 100.0f;
	TestTrue(TEXT("Drive torque spins a wheel in the air"), FGArcadeTireModel::IntegrateWheelSpin(input) > 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGArcadeDrivetrainTest, "WhosDriving.ArcadeVehicle.Drivetrain",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGArcadeDrivetrainTest::RunTest(const FString& parameters)
{
	const FGArcadeEngineSettings engine;
	const FGArcadeTransmissionSettings transmission;
	const float midRangeTorque = FGArcadeDrivetrain::EvaluateEngineTorque(engine, 0.5f * (engine.IdleRpm + engine.RedlineRpm));
	TestTrue(TEXT("Torque peaks in the middle of the range"),
			 midRangeTorque > FGArcadeDrivetrain::EvaluateEngineTorque(engine, engine.IdleRpm));
	TestTrue(TEXT("Torque stays near the configured peak"), midRangeTorque <= engine.PeakTorque * 1.05f);
	TestTrue(TEXT("Reverse ratio is negative"), FGArcadeDrivetrain::GetGearRatio(transmission, -1) < 0.0f);

	FGArcadeDrivetrain drivetrain;
	drivetrain.Reset();
	FGArcadeDrivetrainInput input;
	input.bHasGroundContact = true;
	input.throttle = 1.0f;
	drivetrain.Update(input, engine, transmission, 1.0f / 60.0f);
	TestTrue(TEXT("Throttle drives the wheels forward"), drivetrain.GetWheelTorque() > 0.0f);

	const float firstGearRatio = FGArcadeDrivetrain::GetGearRatio(transmission, 1);
	const float upshiftVelocity = (transmission.UpshiftRpm + 100.0f) / firstGearRatio / GArcadeVehicleUnits::rpmPerRadianPerSecond;
	input.drivenWheelAngularVelocity = upshiftVelocity;
	input.roadWheelAngularVelocity = 0.1f * upshiftVelocity;
	input.forwardSpeed = 3.0f;
	drivetrain.Update(input, engine, transmission, 1.0f / 60.0f);
	TestFalse(TEXT("Wheelspin alone does not upshift"), drivetrain.IsShifting());

	input.roadWheelAngularVelocity = upshiftVelocity;
	input.forwardSpeed = 20.0f;
	drivetrain.Update(input, engine, transmission, 1.0f / 60.0f);
	TestTrue(TEXT("High road speed starts an upshift"), drivetrain.IsShifting());
	TestEqual(TEXT("Torque is cut while shifting"), drivetrain.GetWheelTorque(), 0.0f);

	// Climb to the top gear, then lose almost all speed: the gearbox has to drop straight to first.
	const int32 topGear = transmission.ForwardGearRatios.Num();
	for (int32 step = 0; step < 200; ++step)
	{
		const float gearRatio = FGArcadeDrivetrain::GetGearRatio(transmission, drivetrain.GetCurrentGear());
		input.drivenWheelAngularVelocity = (transmission.UpshiftRpm + 100.0f) / gearRatio / GArcadeVehicleUnits::rpmPerRadianPerSecond;
		input.roadWheelAngularVelocity = input.drivenWheelAngularVelocity;
		drivetrain.Update(input, engine, transmission, 0.05f);
	}
	TestEqual(TEXT("Upshifts reach the top gear"), drivetrain.GetCurrentGear(), topGear);
	input.drivenWheelAngularVelocity = 5.0f;
	input.roadWheelAngularVelocity = 5.0f;
	input.forwardSpeed = 2.0f;
	bool bSkippedGears = true;
	for (int32 step = 0; step < 10; ++step)
	{
		drivetrain.Update(input, engine, transmission, 0.05f);
		const int32 gear = drivetrain.GetCurrentGear();
		bSkippedGears = bSkippedGears && (gear == topGear || gear == 1);
	}
	TestEqual(TEXT("Low road speed selects first gear"), drivetrain.GetCurrentGear(), 1);
	TestTrue(TEXT("Downshift skips the gears in between"), bSkippedGears);

	drivetrain.Reset();
	input = FGArcadeDrivetrainInput();
	input.bHasGroundContact = true;
	input.brake = 1.0f;
	drivetrain.Update(input, engine, transmission, 0.1f);
	TestEqual(TEXT("Braking to a stop does not engage reverse at once"), drivetrain.GetCurrentGear(), 1);
	TestEqual(TEXT("Brake pedal brakes in a forward gear"), drivetrain.GetBrakeDemand(), 1.0f);
	for (int32 step = 0; step < 10; ++step)
	{
		drivetrain.Update(input, engine, transmission, 0.1f);
	}
	TestEqual(TEXT("Holding brake at a standstill engages reverse"), drivetrain.GetCurrentGear(), -1);
	TestEqual(TEXT("Brake pedal drives in reverse"), drivetrain.GetDriveDemand(), 1.0f);

	input.brake = 0.0f;
	input.throttle = 1.0f;
	drivetrain.Update(input, engine, transmission, 1.0f / 60.0f);
	TestEqual(TEXT("Throttle leaves reverse at once"), drivetrain.GetCurrentGear(), 1);
	TestTrue(TEXT("Throttle drives forward right after leaving reverse"), drivetrain.GetWheelTorque() > 0.0f);

	drivetrain.Reset();
	input = FGArcadeDrivetrainInput();
	input.bHasGroundContact = true;
	input.brake = 1.0f;
	input.forwardSpeed = 15.0f;
	for (int32 step = 0; step < 10; ++step)
	{
		drivetrain.Update(input, engine, transmission, 0.1f);
	}
	TestEqual(TEXT("Reverse never engages while moving"), drivetrain.GetCurrentGear(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGArcadeDriftControllerTest, "WhosDriving.ArcadeVehicle.Drift",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGArcadeDriftControllerTest::RunTest(const FString& parameters)
{
	FGArcadeDriftSettings settings;
	settings.bUsePhysicalSlides = false;
	TestEqual(TEXT("Neutral steering holds the base angle"), FGArcadeDriftController::ComputeTargetAngle(settings, 0.0f), settings.BaseAngle);
	TestEqual(TEXT("Steering into the drift opens it up"), FGArcadeDriftController::ComputeTargetAngle(settings, 1.0f), settings.MaxAngle);
	TestEqual(TEXT("Full counter steer asks to straighten"), FGArcadeDriftController::ComputeTargetAngle(settings, -1.0f), 0.0f);

	FGArcadeDriftInput input;
	input.bIsGrounded = true;
	input.forwardSpeed = 8.0f;
	input.planarSpeed = 8.0f;
	input.steering = 0.8f;
	input.brake = 1.0f;
	FGArcadeDriftController slowController;
	slowController.Update(input, settings, 1.0f / 60.0f);
	TestFalse(TEXT("A brake tap at low speed only brakes"), slowController.IsDrifting());

	FGArcadeDriftController controller;
	input.forwardSpeed = 25.0f;
	input.planarSpeed = 25.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	TestTrue(TEXT("Lifting, steering and tapping the brake at speed starts a drift"), controller.IsDrifting());
	TestEqual(TEXT("The drift follows the steering side"), controller.GetDriftDirection(), 1.0f);
	TestTrue(TEXT("A right drift yaws the nose right"), controller.GetYawAcceleration() > 0.0f);
	TestTrue(TEXT("A right drift aims for a negative slip angle"), controller.GetTargetSlipAngle() < 0.0f);

	input.forwardSpeed = 20.0f;
	input.planarSpeed = 20.0f;
	input.brake = 0.0f;
	input.throttle = 1.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	TestTrue(TEXT("Lost speed is held up"), controller.GetSpeedHoldAcceleration() > 0.0f);

	input.bodySlipAngle = -2.0f;
	for (int32 step = 0; step < 60; ++step)
	{
		controller.Update(input, settings, 1.0f / 60.0f);
	}
	TestFalse(TEXT("The drift ends once the car straightens"), controller.IsDrifting());
	controller.Update(input, settings, 1.0f / 60.0f);
	TestFalse(TEXT("Throttle alone does not start a second drift"), controller.IsDrifting());

	input.brake = 0.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	input.brake = 1.0f;
	input.throttle = 0.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	TestTrue(TEXT("A new brake tap starts the next drift"), controller.IsDrifting());

	FGArcadeDriftController handbrakeController;
	FGArcadeDriftInput handbrakeInput;
	handbrakeInput.bIsGrounded = true;
	handbrakeInput.forwardSpeed = 10.0f;
	handbrakeInput.planarSpeed = 10.0f;
	handbrakeInput.steering = -0.5f;
	handbrakeInput.throttle = 1.0f;
	handbrakeInput.bHandbrake = true;
	handbrakeController.Update(handbrakeInput, settings, 1.0f / 60.0f);
	TestTrue(TEXT("The handbrake starts a drift even on throttle"), handbrakeController.IsDrifting());
	TestEqual(TEXT("The handbrake drift follows the steering side"), handbrakeController.GetDriftDirection(), -1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGArcadeDriftPathTest, "WhosDriving.ArcadeVehicle.DriftPath",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGArcadeDriftPathTest::RunTest(const FString& parameters)
{
	FGArcadeDriftSettings settings;
	settings.bUsePhysicalSlides = false;
	TestEqual(TEXT("Neutral steering bends the path at the base rate"),
			  FGArcadeDriftController::ComputePathAcceleration(settings, 0.0f), settings.PathAcceleration);
	TestTrue(TEXT("Steering into the drift tightens the path"),
			 FGArcadeDriftController::ComputePathAcceleration(settings, 1.0f) > settings.PathAcceleration);
	TestTrue(TEXT("Counter steer opens the path"),
			 FGArcadeDriftController::ComputePathAcceleration(settings, -1.0f) < settings.PathAcceleration);

	FGArcadeDriftController controller;
	FGArcadeDriftInput input;
	input.bIsGrounded = true;
	input.forwardSpeed = 30.0f;
	input.planarSpeed = 30.0f;
	input.steering = -0.6f;
	input.brake = 1.0f;
	for (int32 step = 0; step < 20; ++step)
	{
		controller.Update(input, settings, 1.0f / 60.0f);
	}
	TestTrue(TEXT("A brake tap starts a left drift"), controller.IsDrifting() && controller.GetDriftDirection() < 0.0f);
	TestTrue(TEXT("A left drift bends the path to the left"), controller.GetPathAcceleration() < 0.0f);
	TestTrue(TEXT("A left drift loosens the rear axle"), controller.GetRearGripScale() < 1.0f);

	// A steady slide close to the target angle: what is left is following the turning path.
	input.bodySlipAngle = 28.0f;
	input.yawRate = 0.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	controller.Update(input, settings, 1.0f / 60.0f);
	TestTrue(TEXT("The nose is turned into a left corner the path is turning through"), controller.GetYawAcceleration() < 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGArcadeHandbrakeTurnTest, "WhosDriving.ArcadeVehicle.HandbrakeTurn",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGArcadeHandbrakeTurnTest::RunTest(const FString& parameters)
{
	FGArcadeDriftSettings settings;
	settings.bUsePhysicalSlides = false;
	const float stepTime = 1.0f / 60.0f;

	FGArcadeDriftController controller;
	FGArcadeDriftInput input;
	input.bIsGrounded = true;
	input.forwardSpeed = 20.0f;
	input.planarSpeed = 20.0f;
	input.steering = 1.0f;
	input.throttle = 1.0f;
	input.bHandbrake = true;
	controller.Update(input, settings, stepTime);
	TestEqual(TEXT("Handbrake and steering after a run-up start a handbrake turn"),
			  static_cast<int32>(controller.GetMode()), static_cast<int32>(EGArcadeSlideMode::HandbrakeTurn));
	TestTrue(TEXT("The car swings round towards the steering side"), controller.GetYawAcceleration() > 0.0f);
	TestEqual(TEXT("A handbrake turn does not bend the path"), controller.GetPathAcceleration(), 0.0f);

	for (int32 step = 0; step < 30; ++step)
	{
		controller.Update(input, settings, stepTime);
	}
	TestEqual(TEXT("The rear axle lets go during the turn"), controller.GetRearGripScale(), settings.HandbrakeTurnRearGripMultiplier, 1.0e-3f);

	input.steering = 0.0f;
	input.yawRate = 2.0f;
	controller.Update(input, settings, stepTime);
	TestTrue(TEXT("Releasing the steering stops the rotation"), controller.GetYawAcceleration() < 0.0f);

	input.steering = 1.0f;
	input.travelAngle = -175.0f;
	controller.Update(input, settings, stepTime);
	TestTrue(TEXT("The rotation stops once the car faces back the way it came"), controller.GetYawAcceleration() < 0.0f);

	input.bHandbrake = false;
	input.forwardSpeed = -10.0f;
	controller.Update(input, settings, stepTime);
	TestFalse(TEXT("Releasing the handbrake after turning round grips again"), controller.IsDrifting());

	FGArcadeDriftController shallowController;
	FGArcadeDriftInput shallowInput;
	shallowInput.bIsGrounded = true;
	shallowInput.forwardSpeed = 20.0f;
	shallowInput.planarSpeed = 20.0f;
	shallowInput.steering = 1.0f;
	shallowInput.bHandbrake = true;
	shallowController.Update(shallowInput, settings, stepTime);
	shallowInput.bHandbrake = false;
	shallowInput.travelAngle = -30.0f;
	shallowInput.bodySlipAngle = -30.0f;
	shallowController.Update(shallowInput, settings, stepTime);
	TestEqual(TEXT("Releasing the handbrake in a shallow slide carries on as a drift"),
			  static_cast<int32>(shallowController.GetMode()), static_cast<int32>(EGArcadeSlideMode::Drift));
	TestEqual(TEXT("The drift continues to the side the tail swung out"), shallowController.GetDriftDirection(), 1.0f);

	FGArcadeDriftController slowController;
	FGArcadeDriftInput slowInput = shallowInput;
	slowInput.bHandbrake = true;
	slowInput.forwardSpeed = 3.0f;
	slowInput.planarSpeed = 3.0f;
	slowController.Update(slowInput, settings, stepTime);
	TestFalse(TEXT("The handbrake at walking pace only brakes"), slowController.IsDrifting());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGArcadeContactBalanceTest, "WhosDriving.ArcadeVehicle.ContactBalance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGArcadeContactBalanceTest::RunTest(const FString& parameters)
{
	const FGArcadeTireSettings tires;
	const float stepTimes[] = { 1.0f / 120.0f, 1.0f / 60.0f, 1.0f / 30.0f, 1.0f / 15.0f };
	const float travelSpeeds[] = { -20.0f, -0.2f, 0.0f, 0.2f, 20.0f };
	const float driveTorques[] = { -3000.0f, 0.0f, 3000.0f };
	const float brakeTorques[] = { 0.0f, 5000.0f };
	const float lateralForces[] = { 0.0f, 4000.0f, 9000.0f };
	for (const float stepTime : stepTimes)
	{
		for (const float travelSpeed : travelSpeeds)
		{
			for (const float driveTorque : driveTorques)
			{
				for (const float brakeTorque : brakeTorques)
				{
					for (const float lateralForce : lateralForces)
					{
						FGArcadeTireContactInput input;
						input.spin.radius = 0.38f;
						input.spin.inertia = 1.444f;
						input.spin.longitudinalSpeed = travelSpeed;
						input.spin.angularVelocity = travelSpeed / input.spin.radius;
						input.spin.referenceSpeed = FMath::Max(FMath::Abs(travelSpeed), 1.0f);
						input.spin.driveTorque = driveTorque;
						input.spin.brakeTorque = brakeTorque;
						input.spin.deltaTime = stepTime;
						input.frictionLimit = 3433.5f * 1.65f;
						input.lateralForce = lateralForce;
						input.lateralSuppression = tires.LateralSuppression;
						input.maxBrakingForce = FMath::Abs(travelSpeed) * 350.0f / stepTime;
						const FGArcadeTireContactResult result = FGArcadeTireModel::SolveContact(input, tires.LongitudinalCurve);
						const double inertiaTorque = input.spin.inertia * (result.angularVelocity - input.spin.angularVelocity) / stepTime;
						const double externalTorque = driveTorque - result.signedBrakeTorque - input.spin.radius * result.force.X;
						TestTrue(TEXT("Final body force closes the wheel torque balance"), FMath::Abs(inertiaTorque - externalTorque) < 0.1);
						TestTrue(TEXT("Combined force stays within available grip"), result.force.Size() <= input.frictionLimit + 0.01);
						TestTrue(TEXT("Brake respects its torque budget"), FMath::Abs(result.signedBrakeTorque) <= brakeTorque + 0.01f);
						TestTrue(TEXT("Brake never adds rotational energy"), result.signedBrakeTorque * result.angularVelocity >= -0.01f);

						const float slip = FGArcadeTireModel::ComputeSlipRatio(result.angularVelocity * input.spin.radius, travelSpeed, input.spin.referenceSpeed);
						float expectedLongitudinalForce = FGArcadeTireModel::EvaluateCurve(tires.LongitudinalCurve, slip) * input.frictionLimit;
						if (expectedLongitudinalForce * travelSpeed < 0.0f)
						{
							expectedLongitudinalForce = FMath::Clamp(expectedLongitudinalForce, -input.maxBrakingForce, input.maxBrakingForce);
						}
						const FVector2D expectedForce = FGArcadeTireModel::LimitToFrictionCircle(
							FVector2D(expectedLongitudinalForce, lateralForce), input.frictionLimit, input.lateralSuppression);
						TestTrue(TEXT("Solved rotation agrees with the nonlinear tire curve"), result.force.Equals(expectedForce, 1.0));
					}
				}
			}
		}
	}

	const FVector2D boundaryForce(600.0f, 800.0f);
	const FVector2D outsideForce = FGArcadeTireModel::LimitToFrictionCircle(boundaryForce * 1.000001, 1000.0f, 0.8f);
	TestTrue(TEXT("Crossing the grip boundary does not jump the force direction"), outsideForce.Equals(boundaryForce, 0.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGArcadeDriftSpeedHoldTest, "WhosDriving.ArcadeVehicle.DriftSpeedHold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGArcadeDriftSpeedHoldTest::RunTest(const FString& parameters)
{
	FGArcadeDriftSettings settings;
	settings.bUsePhysicalSlides = false;
	FGArcadeDriftController controller;
	FGArcadeDriftInput input;
	input.bIsGrounded = true;
	input.forwardSpeed = 20.0f;
	input.planarSpeed = 20.0f;
	input.steering = 0.8f;
	input.brake = 1.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	TestTrue(TEXT("Test entered the drift state"), controller.IsDrifting());
	input.brake = 0.0f;
	input.throttle = 1.0f;
	input.bodySlipAngle = -28.0f;
	input.forwardSpeed = 20.0f * FMath::Cos(FMath::DegreesToRadians(28.0f));
	controller.Update(input, settings, 1.0f / 60.0f);
	TestEqual(TEXT("Turning the chassis at constant speed adds no acceleration"), controller.GetSpeedHoldAcceleration(), 0.0f);
	input.planarSpeed = 18.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	TestEqual(TEXT("Actual speed loss is compensated on throttle"), controller.GetSpeedHoldAcceleration(), 2.0f * settings.SpeedHoldGain);
	input.brake = 1.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	TestEqual(TEXT("Speed hold does not fight the brake"), controller.GetSpeedHoldAcceleration(), 0.0f);
	input.brake = 0.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	TestEqual(TEXT("Braking creates no deferred acceleration"), controller.GetSpeedHoldAcceleration(), 0.0f);
	input.throttle = 0.0f;
	input.planarSpeed = 16.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	TestEqual(TEXT("Coasting adds no acceleration"), controller.GetSpeedHoldAcceleration(), 0.0f);
	input.throttle = 1.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	TestEqual(TEXT("Coasting creates no deferred acceleration"), controller.GetSpeedHoldAcceleration(), 0.0f);
	input.bHandbrake = true;
	input.planarSpeed = 14.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	TestEqual(TEXT("Speed hold does not fight the handbrake"), controller.GetSpeedHoldAcceleration(), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGArcadePhysicalSlideTest, "WhosDriving.ArcadeVehicle.PhysicalSlide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGArcadePhysicalSlideTest::RunTest(const FString& parameters)
{
	const FGArcadeDriftSettings settings;
	TestTrue(TEXT("Improved tire grip is enabled by default"), settings.bUsePhysicalSlides);
	const float steeringDirections[] = { -1.0f, 1.0f };
	for (const float steeringDirection : steeringDirections)
	{
		FGArcadeDriftController controller;
		FGArcadeDriftInput input;
		input.bIsGrounded = true;
		input.forwardSpeed = 25.0f;
		input.planarSpeed = 25.0f;
		input.steering = steeringDirection;
		input.brake = 1.0f;
		const float stepTime = 1.0f / 60.0f;
		controller.Update(input, settings, stepTime);
		const float initialYawAcceleration = FMath::Abs(controller.GetYawAcceleration());
		const float initialPathAcceleration = FMath::Abs(controller.GetPathAcceleration());
		TestTrue(TEXT("The brake tap starts with a gentle yaw response"), initialYawAcceleration < settings.MaxYawAcceleration * 0.1f);
		for (int32 stepIndex = 0; stepIndex < 16; ++stepIndex)
		{
			controller.Update(input, settings, stepTime);
		}
		TestTrue(TEXT("Yaw authority grows as the car enters the drift"), FMath::Abs(controller.GetYawAcceleration()) > initialYawAcceleration);
		TestTrue(TEXT("Path authority grows as the car enters the drift"), FMath::Abs(controller.GetPathAcceleration()) > initialPathAcceleration);
		TestEqual(TEXT("Steering and S at speed enter controlled drift with default settings"),
			static_cast<int32>(controller.GetMode()), static_cast<int32>(EGArcadeSlideMode::Drift));
		TestTrue(TEXT("Brake entry actively swings the nose into the corner"), controller.GetYawAcceleration() * steeringDirection > 0.0f);
		TestTrue(TEXT("Brake entry retains the controlled drift angle"), controller.GetTargetSlipAngle() * steeringDirection < -settings.ExitAngle);
		TestTrue(TEXT("Drift steering bends the path through the corner"), controller.GetPathAcceleration() * steeringDirection > 0.0f);
		TestTrue(TEXT("Drift retains more rear grip than the original slippery mode"), controller.GetRearGripScale() > settings.RearGripMultiplier);
		TestTrue(TEXT("Rear axle keeps a modest slip allowance"), controller.GetRearGripScale() >= 0.75f && controller.GetRearGripScale() < 1.0f);

		input.brake = 0.0f;
		input.throttle = 1.0f;
		input.bodySlipAngle = -25.0f * steeringDirection;
		input.travelAngle = input.bodySlipAngle;
		input.planarSpeed = 20.0f;
		controller.Update(input, settings, 0.2f);
		TestTrue(TEXT("Releasing S keeps an established drift controllable"), controller.IsDrifting());
		TestEqual(TEXT("Lost speed is not artificially repaid during the slide"), controller.GetSpeedHoldAcceleration(), 0.0f);

		input.steering = -steeringDirection;
		controller.Update(input, settings, 0.1f);
		TestEqual(TEXT("Counter steer requests a straightened car"), controller.GetTargetSlipAngle(), 0.0f);
		TestTrue(TEXT("Counter steer actively reduces the slide angle"), controller.GetYawAcceleration() * steeringDirection < 0.0f);
		input.bodySlipAngle = -2.0f * steeringDirection;
		controller.Update(input, settings, 0.2f);
		TestFalse(TEXT("Straightening exits controlled drift"), controller.IsDrifting());
		TestEqual(TEXT("Rear grip fully returns after exit"), controller.GetRearGripScale(), 1.0f);
		controller.Update(input, settings, 0.1f);
		TestFalse(TEXT("Throttle alone does not re-enter drift"), controller.IsDrifting());

		input.steering = steeringDirection;
		input.bHandbrake = true;
		controller.Update(input, settings, 0.2f);
		TestEqual(TEXT("Held handbrake relies on tire forces instead of scripted spin"), controller.GetYawAcceleration(), 0.0f);
		TestEqual(TEXT("Locked tire receives no additional fixed grip penalty"), controller.GetRearGripScale(), 1.0f);
		input.bHandbrake = false;
		input.bodySlipAngle = -25.0f * steeringDirection;
		input.travelAngle = input.bodySlipAngle;
		controller.Update(input, settings, 0.1f);
		TestEqual(TEXT("A shallow handbrake slide hands over to controlled drift"),
			static_cast<int32>(controller.GetMode()), static_cast<int32>(EGArcadeSlideMode::Drift));
		TestTrue(TEXT("Handbrake handover preserves steering control"), controller.GetPathAcceleration() * steeringDirection > 0.0f);

		FGArcadeDriftController slowController;
		input.bHandbrake = false;
		input.throttle = 0.0f;
		input.brake = 1.0f;
		input.forwardSpeed = 5.0f;
		input.planarSpeed = 5.0f;
		slowController.Update(input, settings, 0.1f);
		TestFalse(TEXT("S at low speed still only brakes"), slowController.IsDrifting());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGArcadeLowSpeedDriftTest, "WhosDriving.ArcadeVehicle.LowSpeedDrift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGArcadeLowSpeedDriftTest::RunTest(const FString& parameters)
{
	const FGArcadeDriftSettings settings;
	FGArcadeDriftController controller;
	FGArcadeDriftInput input;
	input.bIsGrounded = true;
	input.steering = 1.0f;
	input.brake = 1.0f;
	input.forwardSpeed = 20.0f;
	input.planarSpeed = 20.0f;
	controller.Update(input, settings, 0.2f);
	input.brake = 0.0f;
	input.bodySlipAngle = -34.5f;
	controller.Update(input, settings, 0.2f);
	const float fastPathAcceleration = controller.GetPathAcceleration();
	TestTrue(TEXT("A brake tap establishes a controlled drift at speed"), fastPathAcceleration > 0.0f);

	input.throttle = 1.0f;
	input.brake = 1.0f;
	input.forwardSpeed = 10.0f;
	input.planarSpeed = 12.1f;
	controller.Update(input, settings, 1.0f / 60.0f);
	TestTrue(TEXT("Drift assistance fades before the low speed exit"), controller.GetPathAcceleration() < fastPathAcceleration);

	input.forwardSpeed = 25.0f / 3.6f;
	input.planarSpeed = 8.4f;
	controller.Update(input, settings, 0.2f);
	TestFalse(TEXT("Holding W and S at 25 km/h cannot sustain a controlled drift"), controller.IsDrifting());
	TestEqual(TEXT("Low speed exit restores rear grip"), controller.GetRearGripScale(), 1.0f);
	TestEqual(TEXT("Low speed exit removes scripted path acceleration"), controller.GetPathAcceleration(), 0.0f);
	TestEqual(TEXT("Low speed exit removes scripted yaw acceleration"), controller.GetYawAcceleration(), 0.0f);

	input.throttle = 0.0f;
	input.forwardSpeed = 20.0f;
	input.planarSpeed = 20.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	TestFalse(TEXT("Holding S after exit cannot re-enter drift"), controller.IsDrifting());
	input.brake = 0.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	input.brake = 1.0f;
	controller.Update(input, settings, 1.0f / 60.0f);
	TestTrue(TEXT("A fresh tap of S can start the next drift"), controller.IsDrifting());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGArcadeLockedTireTest, "WhosDriving.ArcadeVehicle.LockedTire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGArcadeLockedTireTest::RunTest(const FString& parameters)
{
	const FGArcadeTireSettings tires;
	const float stepTimes[] = { 1.0f / 120.0f, 1.0f / 60.0f, 1.0f / 30.0f, 1.0f / 15.0f };
	const float longitudinalSpeeds[] = { -20.0f, 20.0f };
	const float lateralSpeeds[] = { -20.0f, -5.0f, 0.0f, 5.0f, 20.0f };
	for (const float stepTime : stepTimes)
	{
		for (const float longitudinalSpeed : longitudinalSpeeds)
		{
			for (const float lateralSpeed : lateralSpeeds)
			{
				FGArcadeTireContactInput input;
				input.bUseCombinedSlip = true;
				input.spin.radius = 0.38f;
				input.spin.inertia = 1.444f;
				input.spin.longitudinalSpeed = longitudinalSpeed;
				input.spin.referenceSpeed = FMath::Abs(longitudinalSpeed);
				input.spin.brakeTorque = 5000.0f;
				input.spin.deltaTime = stepTime;
				input.frictionLimit = 5000.0f;
				input.lateralSpeed = lateralSpeed;
				const float slipAngle = FGArcadeTireModel::ComputeSlipAngle(lateralSpeed, longitudinalSpeed, tires.SlipAngleReferenceSpeed);
				input.lateralForce = -FGArcadeTireModel::EvaluateCurve(tires.LateralCurve, slipAngle) * input.frictionLimit;
				const FGArcadeTireContactResult lockedContact = FGArcadeTireModel::SolveContact(input, tires.LongitudinalCurve);
				const FVector2D travelVelocity(longitudinalSpeed, lateralSpeed);
				TestEqual(TEXT("Held handbrake keeps the tire locked"), lockedContact.angularVelocity, 0.0f);
				TestTrue(TEXT("Locked tire friction opposes the actual skid direction"),
					lockedContact.force.GetSafeNormal().Equals(-travelVelocity.GetSafeNormal(), 0.001));
				TestTrue(TEXT("Locked tire removes translational energy"), FVector2D::DotProduct(lockedContact.force, travelVelocity) < 0.0);
				TestTrue(TEXT("Large sideways skids retain sliding friction"),
					FMath::IsNearlyEqual(lockedContact.force.Size(), static_cast<double>(input.frictionLimit * tires.LongitudinalCurve.FloorValue), 0.1));

				input.spin.brakeTorque = 0.0f;
				for (int32 stepIndex = 0; stepIndex < 120; ++stepIndex)
				{
					const FGArcadeTireContactResult contact = FGArcadeTireModel::SolveContact(input, tires.LongitudinalCurve);
					const float inertiaTorque = input.spin.inertia * (contact.angularVelocity - input.spin.angularVelocity) / stepTime;
					TestTrue(TEXT("Released tire balances body force and wheel reaction"),
						FMath::Abs(inertiaTorque + input.spin.radius * contact.force.X) < 0.1f);
					const FVector2D slipVelocity(input.spin.radius * contact.angularVelocity - longitudinalSpeed, -lateralSpeed);
					TestTrue(TEXT("Contact friction cannot add energy to body and wheel together"),
						FVector2D::DotProduct(contact.force, slipVelocity) >= -0.01);
					TestTrue(TEXT("Combined slip respects the friction budget"), contact.force.Size() <= input.frictionLimit + 0.01);
					TestTrue(TEXT("Wheel resumes rolling in the direction of travel"), contact.angularVelocity * longitudinalSpeed > 0.0f);
					input.spin.angularVelocity = contact.angularVelocity;
				}
				TestTrue(TEXT("Released tire returns to rolling speed"),
					FMath::Abs(input.spin.angularVelocity * input.spin.radius - longitudinalSpeed) < 0.01f);
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGArcadeCombinedSlipBalanceTest, "WhosDriving.ArcadeVehicle.CombinedSlipBalance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGArcadeCombinedSlipBalanceTest::RunTest(const FString& parameters)
{
	const FGArcadeTireSettings tires;
	const float stepTimes[] = { 1.0f / 120.0f, 1.0f / 30.0f, 1.0f / 15.0f };
	const float travelSpeeds[] = { -20.0f, -0.2f, 0.0f, 0.2f, 20.0f };
	const float driveTorques[] = { -3000.0f, 0.0f, 3000.0f };
	const float brakeTorques[] = { 0.0f, 5000.0f };
	const float lateralSpeeds[] = { -10.0f, 0.0f, 10.0f };
	for (const float stepTime : stepTimes)
	{
		for (const float travelSpeed : travelSpeeds)
		{
			for (const float driveTorque : driveTorques)
			{
				for (const float brakeTorque : brakeTorques)
				{
					for (const float lateralSpeed : lateralSpeeds)
					{
						FGArcadeTireContactInput input;
						input.bUseCombinedSlip = true;
						input.spin.radius = 0.38f;
						input.spin.inertia = 1.444f;
						input.spin.longitudinalSpeed = travelSpeed;
						input.spin.angularVelocity = travelSpeed / input.spin.radius;
						input.spin.referenceSpeed = FMath::Max(FMath::Abs(travelSpeed), tires.SlipReferenceSpeed);
						input.spin.driveTorque = driveTorque;
						input.spin.brakeTorque = brakeTorque;
						input.spin.deltaTime = stepTime;
						input.frictionLimit = 5000.0f;
						input.lateralSpeed = lateralSpeed;
						input.maxBrakingForce = FMath::Abs(travelSpeed) * 350.0f / stepTime;
						input.maxLateralForce = FMath::Abs(lateralSpeed) * 350.0f / stepTime;
						const float slipAngle = FGArcadeTireModel::ComputeSlipAngle(lateralSpeed, travelSpeed, tires.SlipAngleReferenceSpeed);
						input.lateralForce = -FGArcadeTireModel::EvaluateCurve(tires.LateralCurve, slipAngle) * input.frictionLimit;
						const FGArcadeTireContactResult contact = FGArcadeTireModel::SolveContact(input, tires.LongitudinalCurve);
						const double inertiaTorque = input.spin.inertia * (contact.angularVelocity - input.spin.angularVelocity) / stepTime;
						const double externalTorque = driveTorque - contact.signedBrakeTorque - input.spin.radius * contact.force.X;
						TestTrue(TEXT("Combined slip closes the wheel torque balance"), FMath::Abs(inertiaTorque - externalTorque) < 0.1);
						TestTrue(TEXT("Combined slip respects the grip budget"), contact.force.Size() <= input.frictionLimit + 0.01);
						TestTrue(TEXT("Brake remains within its torque budget"), FMath::Abs(contact.signedBrakeTorque) <= brakeTorque + 0.01f);
						TestTrue(TEXT("Brake does not reverse wheel rotation"), contact.signedBrakeTorque * contact.angularVelocity >= -0.01f);
						TestTrue(TEXT("Lateral force cannot reverse the skid within a step"), FMath::Abs(contact.force.Y) <= input.maxLateralForce + 0.01);
						if (contact.force.X * travelSpeed < 0.0f)
						{
							TestTrue(TEXT("Braking force cannot reverse travel within a step"), FMath::Abs(contact.force.X) <= input.maxBrakingForce + 0.01);
						}
						const FVector2D slipVelocity(input.spin.radius * contact.angularVelocity - travelSpeed, -lateralSpeed);
						TestTrue(TEXT("Sliding contact dissipates energy even under power"), FVector2D::DotProduct(contact.force, slipVelocity) >= -0.01);
					}
				}
			}
		}
	}
	return true;
}

#endif
