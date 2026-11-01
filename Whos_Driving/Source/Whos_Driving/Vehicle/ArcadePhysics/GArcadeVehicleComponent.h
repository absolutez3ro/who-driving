#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GArcadeDrivetrain.h"
#include "GArcadeVehicleTypes.h"
#include "GArcadeVehicleComponent.generated.h"

class UGArcadeVehicleTuning;
class UPrimitiveComponent;
class USceneComponent;
struct FCollisionQueryParams;

/**
 * Raycast arcade vehicle: suspension, tires, drivetrain, steering, aerodynamics and air control.
 * Pushes the owner's root primitive, which must simulate physics, with forces every pre-physics tick.
 * The simulation runs where the owner has authority. Maths is in SI units and converted to
 * centimetres only at the engine boundary.
 */
UCLASS(ClassGroup = (Vehicle), meta = (BlueprintSpawnableComponent))
class WHOS_DRIVING_API UGArcadeVehicleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGArcadeVehicleComponent();

	void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Arcade|Input")
	void SetThrottle(float value);

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Arcade|Input")
	void SetBrake(float value);

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Arcade|Input")
	void SetSteering(float value);

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Arcade|Input")
	void SetHandbrake(bool bEngaged);

	// Puts the vehicle back on its wheels above its current position, keeping the heading.
	UFUNCTION(BlueprintCallable, Category = "Vehicle|Arcade|Input")
	void ResetVehicle();

	// Re-applies mass, centre of mass and damping after the tuning asset was edited.
	UFUNCTION(BlueprintCallable, Category = "Vehicle|Arcade")
	void ApplyChassisTuning();

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Arcade|Modifiers")
	void SetEngineTorqueMultiplier(float multiplier);

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Arcade|Modifiers")
	void SetWheelGripMultiplier(int32 wheelIndex, float multiplier);

	UFUNCTION(BlueprintPure, Category = "Vehicle|Arcade|Telemetry")
	float GetSpeedKmh() const
	{
		return speedKmh;
	}

	// Negative while moving backwards.
	UFUNCTION(BlueprintPure, Category = "Vehicle|Arcade|Telemetry")
	float GetForwardSpeedKmh() const
	{
		return forwardSpeedKmh;
	}

	// -1 is reverse, 1..N are forward gears.
	UFUNCTION(BlueprintPure, Category = "Vehicle|Arcade|Telemetry")
	int32 GetCurrentGear() const
	{
		return drivetrain.GetCurrentGear();
	}

	UFUNCTION(BlueprintPure, Category = "Vehicle|Arcade|Telemetry")
	float GetEngineRpm() const
	{
		return drivetrain.GetEngineRpm();
	}

	UFUNCTION(BlueprintPure, Category = "Vehicle|Arcade|Telemetry")
	bool IsShifting() const
	{
		return drivetrain.IsShifting();
	}

	UFUNCTION(BlueprintPure, Category = "Vehicle|Arcade|Telemetry")
	bool IsInAir() const;

	UFUNCTION(BlueprintPure, Category = "Vehicle|Arcade|Telemetry")
	TArray<FGArcadeWheelState> GetWheelStates() const
	{
		return wheelStates;
	}

	UFUNCTION(BlueprintPure, Category = "Vehicle|Arcade|Telemetry")
	TArray<FGArcadeWheelSetup> GetWheelSetups() const
	{
		return wheelSetups;
	}

	// Falls back to the class defaults when no tuning asset is assigned.
	const UGArcadeVehicleTuning& GetTuningData() const;

protected:
	void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Arcade")
	TObjectPtr<UGArcadeVehicleTuning> tuning;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Arcade")
	TArray<FGArcadeWheelSetup> wheelSetups;

	// Longest step simulated in one tick; slower frames run the simulation in slow motion instead of exploding.
	UPROPERTY(EditAnywhere, Category = "Vehicle|Arcade", meta = (ClampMin = "0.001", Units = "s"))
	float maxSimulationStep = 1.0f / 30.0f;

	// Height above the current position used by ResetVehicle.
	UPROPERTY(EditAnywhere, Category = "Vehicle|Arcade", meta = (ClampMin = "0.0", Units = "cm"))
	float resetLiftHeight = 150.0f;

	// Draws traces, contacts, forces and the centre of mass. The console variable vp.Debug does the same.
	UPROPERTY(EditAnywhere, Category = "Vehicle|Arcade|Debug")
	bool bDrawDebug = false;

	// Centimetres of arrow length per newton.
	UPROPERTY(EditAnywhere, Category = "Vehicle|Arcade|Debug", meta = (ClampMin = "0.0"))
	float debugForceScale = 0.02f;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Vehicle|Arcade|Telemetry")
	FGArcadeVehicleInput vehicleInput;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Vehicle|Arcade|Telemetry")
	TArray<FGArcadeWheelState> wheelStates;

private:
	bool CanSimulate() const;
	void InitializeWheels();
	void ResolveWheelVisual(int32 wheelIndex);
	void SynchronizeChaosVehicleMass(float mass);
	void RefreshWheelParameters(const FGArcadeChassisFrame& frame);
	FGArcadeChassisFrame CaptureChassisFrame(float deltaTime) const;
	FCollisionQueryParams BuildSuspensionQueryParams() const;

	void UpdateSuspension(FGArcadeChassisFrame& frame);
	void TraceWheel(int32 wheelIndex, const FGArcadeChassisFrame& frame, const FCollisionQueryParams& queryParams);
	void ComputeSuspensionForce(int32 wheelIndex, const FGArcadeChassisFrame& frame);
	void ApplyAntiRollBars();
	void ApplySuspensionForce(int32 wheelIndex, const FGArcadeChassisFrame& frame);

	void UpdateSteering(const FGArcadeChassisFrame& frame);
	float ComputeMaxSteerAngle(float speed) const;

	void UpdateDrivetrain(const FGArcadeChassisFrame& frame);
	float ComputeTractionScale() const;

	void UpdateTires(const FGArcadeChassisFrame& frame);
	bool ShouldHoldStandstill(const FGArcadeChassisFrame& frame) const;
	void UpdateGroundedWheel(int32 wheelIndex, const FGArcadeChassisFrame& frame, float tractionScale);
	void UpdateAirborneWheel(int32 wheelIndex, const FGArcadeChassisFrame& frame, float tractionScale);
	void ApplyStandstillHold(int32 wheelIndex, const FGArcadeChassisFrame& frame);
	float ComputeDriveTorque(int32 wheelIndex, float tractionScale) const;
	float ComputeBrakeTorque(int32 wheelIndex) const;
	FVector ComputeForceApplicationPoint(int32 wheelIndex, const FGArcadeChassisFrame& frame) const;

	void ApplyAerodynamics(const FGArcadeChassisFrame& frame);
	void ApplyStabilization(const FGArcadeChassisFrame& frame);
	void UpdateWheelVisuals(float deltaTime);
	void DrawDebug(const FGArcadeChassisFrame& frame) const;

	bool IsRearWheel(int32 wheelIndex) const;
	float GetWheelInertia() const;

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> chassis;

	TArray<FGArcadeWheelRuntime> wheelRuntimes;
	FGArcadeDrivetrain drivetrain;

	// Degrees, shared by the steered wheels.
	float currentSteerAngle = 0.0f;
	float engineTorqueMultiplier = 1.0f;
	float speedKmh = 0.0f;
	float forwardSpeedKmh = 0.0f;
};
