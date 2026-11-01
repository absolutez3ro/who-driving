#if WITH_DEV_AUTOMATION_TESTS

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
	input.drivenWheelAngularVelocity = (transmission.UpshiftRpm + 100.0f) / firstGearRatio / GArcadeVehicleUnits::rpmPerRadianPerSecond;
	input.forwardSpeed = 20.0f;
	drivetrain.Update(input, engine, transmission, 1.0f / 60.0f);
	TestTrue(TEXT("High rpm starts an upshift"), drivetrain.IsShifting());
	TestEqual(TEXT("Torque is cut while shifting"), drivetrain.GetWheelTorque(), 0.0f);

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

	drivetrain.Reset();
	input.forwardSpeed = 15.0f;
	for (int32 step = 0; step < 10; ++step)
	{
		drivetrain.Update(input, engine, transmission, 0.1f);
	}
	TestEqual(TEXT("Reverse never engages while moving"), drivetrain.GetCurrentGear(), 1);
	return true;
}

#endif
