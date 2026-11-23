#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GVehicleAudioProfile.h"
#include "GVehicleAudioComponent.generated.h"

class UAudioComponent;
class UGArcadeVehicleComponent;
class UChaosWheeledVehicleMovementComponent;

USTRUCT()
struct FGVehicleAudioState
{
	GENERATED_BODY()

	UPROPERTY()
	uint16 Rpm = 0;

	UPROPERTY()
	uint8 Load = 0;

	UPROPERTY()
	uint8 Skid = 0;

	UPROPERTY()
	int8 Gear = 0;

	UPROPERTY()
	bool bShifting = false;
};

UCLASS(ClassGroup = (Vehicle), Config = Game, meta = (BlueprintSpawnableComponent))
class WHOS_DRIVING_API UGVehicleAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGVehicleAudioComponent();
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Plays a named one-shot locally. Call on each listener for replicated gameplay events.
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Vehicle|Audio")
	void PlayVehicleSound(FName Event);

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Audio")
	TObjectPtr<UGVehicleAudioProfile> ProfileOverride;

	UPROPERTY(Config)
	TSoftObjectPtr<UGVehicleAudioProfile> DefaultProfile;

private:
	FGVehicleAudioState ReadPhysics() const;
	void UpdatePlayback(const FGVehicleAudioState& State, float DeltaTime);
	void UpdateEngine(float Rpm, float Load, float Limiter, float ShiftGain);
	UAudioComponent* CreateLoop(USoundWave* Sound, const FVector& Offset);

	UPROPERTY(Replicated)
	FGVehicleAudioState ReplicatedState;

	UPROPERTY(Transient)
	TObjectPtr<UGVehicleAudioProfile> Profile;

	UPROPERTY(Transient)
	TObjectPtr<UGArcadeVehicleComponent> Arcade;

	UPROPERTY(Transient)
	TObjectPtr<UChaosWheeledVehicleMovementComponent> Chaos;

	UPROPERTY(Transient)
	TArray<FGVehicleAudioEngineLayer> EngineLayers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> EngineVoices;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> LimiterVoice;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> TireVoice;

	float SnapshotTime = 0.0f;
	float RpmBlend = 0.0f;
	float LoadBlend = 0.0f;
	float SkidBlend = 0.0f;
	float ShiftGain = 1.0f;
	float LimiterBlend = 0.0f;
	int8 LastGear = 0;
};
