#include "GVehicleAudioComponent.h"

#include "Components/AudioComponent.h"
#include "GameFramework/Actor.h"
#include "GVehicleMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"
#include "Vehicle/ArcadePhysics/GArcadeVehicleComponent.h"
#include "Vehicle/ArcadePhysics/GArcadeVehicleTuning.h"

DEFINE_LOG_CATEGORY_STATIC(LogVehicleAudio, Log, All);

namespace
{
	constexpr float snapshotInterval = 1.0f / 20.0f;
	constexpr float silentVolume = 0.001f;

	float SmoothTowards(float Current, float Target, float Rate, float DeltaTime)
	{
		return FMath::Lerp(Current, Target, 1.0f - FMath::Exp(-Rate * DeltaTime));
	}

	void SetLoopMix(UAudioComponent* Voice, float Volume, float Pitch)
	{
		if (!Voice)
		{
			return;
		}
		if (Volume <= silentVolume)
		{
			if (Voice->IsPlaying())
			{
				Voice->Stop();
			}
			return;
		}

		Voice->SetVolumeMultiplier(Volume);
		Voice->SetPitchMultiplier(Pitch);
		if (!Voice->IsPlaying())
		{
			Voice->Play();
		}
	}
}

UGVehicleAudioComponent::UGVehicleAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	SetIsReplicatedByDefault(true);
}

void UGVehicleAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	Profile = ProfileOverride ? ProfileOverride.Get() : DefaultProfile.LoadSynchronous();
	Arcade = GetOwner()->FindComponentByClass<UGArcadeVehicleComponent>();
	Chaos = GetOwner()->FindComponentByClass<UChaosWheeledVehicleMovementComponent>();
	if (!Profile || (!Arcade && !Chaos))
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (GetNetMode() == NM_DedicatedServer)
	{
		// Dedicated servers only publish telemetry.
		PrimaryComponentTick.TickInterval = snapshotInterval;
		return;
	}

	EngineLayers = Profile->EngineLayers.FilterByPredicate([](const FGVehicleAudioEngineLayer& Layer)
	{
		return Layer.Loop && Layer.RecordedRpm > 0.0f;
	});
	EngineLayers.Sort([](const FGVehicleAudioEngineLayer& A, const FGVehicleAudioEngineLayer& B)
	{
		return A.RecordedRpm < B.RecordedRpm;
	});
	for (const FGVehicleAudioEngineLayer& Layer : EngineLayers)
	{
		EngineVoices.Add(CreateLoop(Layer.Loop, Profile->EngineOffset));
	}
	LimiterVoice = CreateLoop(Profile->LimiterLayer.Loop, Profile->EngineOffset);
	TireVoice = CreateLoop(Profile->TireLoop, Profile->TireOffset);
}

void UGVehicleAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (UAudioComponent* Voice : EngineVoices)
	{
		if (Voice)
		{
			Voice->DestroyComponent();
		}
	}
	if (LimiterVoice)
	{
		LimiterVoice->DestroyComponent();
	}
	if (TireVoice)
	{
		TireVoice->DestroyComponent();
	}

	Super::EndPlay(EndPlayReason);
}

void UGVehicleAudioComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGVehicleAudioComponent, ReplicatedState);
}

void UGVehicleAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FGVehicleAudioState State = ReplicatedState;
	if (GetOwner()->HasAuthority())
	{
		State = ReadPhysics();
		if (GetNetMode() != NM_Standalone)
		{
			SnapshotTime -= DeltaTime;
			if (SnapshotTime <= UE_KINDA_SMALL_NUMBER)
			{
				ReplicatedState = State;
				SnapshotTime = snapshotInterval;
			}
		}
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		UpdatePlayback(State, DeltaTime);
	}
}

FGVehicleAudioState UGVehicleAudioComponent::ReadPhysics() const
{
	float Idle = 0.0f;
	float Redline = 0.0f;
	float Rpm = 0.0f;
	float Load = 0.0f;
	float SlipKmh = 0.0f;
	float DriftFloor = 0.0f;
	int32 Gear = 0;
	bool bShifting = false;

	if (Arcade)
	{
		Idle = Arcade->GetTuningData().Engine.IdleRpm;
		Redline = Arcade->GetEngineRedlineRpm();
		Rpm = Arcade->GetEngineRpm();
		Load = Arcade->GetDriveDemand();
		Gear = Arcade->GetCurrentGear();
		bShifting = Arcade->IsShifting();

		// The tire model also reports slip under normal cornering load.
		if (Arcade->IsDrifting())
		{
			for (const FGArcadeWheelState& Wheel : Arcade->GetWheelTelemetry())
			{
				if (Wheel.bHasContact)
				{
					SlipKmh = FMath::Max(SlipKmh, Wheel.slipSpeed * GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond);
					DriftFloor = Profile->SustainedDriftLevel;
				}
			}
		}
	}
	else
	{
		Idle = Chaos->EngineSetup.EngineIdleRPM;
		Redline = Chaos->GetEngineMaxRotationSpeed();
		Rpm = Chaos->GetEngineRotationSpeed();
		const UGVehicleMovementComponent* PlayerMovement = Cast<UGVehicleMovementComponent>(Chaos);
		Load = PlayerMovement ? PlayerMovement->GetAppliedThrottle() : FMath::Abs(Chaos->GetThrottleInput());
		Gear = Chaos->GetCurrentGear();
		bShifting = Chaos->GetTargetGear() != Gear;

		// Chaos has no drift mode. Require the chassis to move sideways and a contacting tire to skid.
		const FVector LocalVelocity = GetOwner()->GetActorQuat().UnrotateVector(GetOwner()->GetVelocity());
		const float BodySlipAngle = FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(LocalVelocity.Y), FMath::Abs(LocalVelocity.X)));
		if (BodySlipAngle >= Profile->ChaosDriftMinimumAngle)
		{
			constexpr float kmhPerCentimeterPerSecond = GArcadeVehicleUnits::metersPerCentimeter
				* GArcadeVehicleUnits::kilometersPerHourPerMeterPerSecond;
			for (int32 Index = 0; Index < Chaos->GetNumWheels(); ++Index)
			{
				const FWheelStatus& Wheel = Chaos->GetWheelState(Index);
				if (Wheel.bInContact && Wheel.bIsSkidding)
				{
					SlipKmh = FMath::Max(SlipKmh, FMath::Abs(Wheel.SkidMagnitude) * kmhPerCentimeterPerSecond);
				}
			}
		}
	}

	const float RpmFraction = FMath::Clamp((Rpm - Idle) / FMath::Max(Redline - Idle, 1.0f), 0.0f, 1.0f);
	const float FullSlipKmh = FMath::Max(Profile->TireFullSlipKmh, Profile->TireMinimumSlipKmh + 1.0f);
	const float Skid = FMath::Max(DriftFloor, FMath::SmoothStep(Profile->TireMinimumSlipKmh, FullSlipKmh, SlipKmh));

	FGVehicleAudioState State;
	State.Rpm = FMath::RoundToInt(RpmFraction * MAX_uint16);
	State.Load = FMath::RoundToInt(FMath::Clamp(Load, 0.0f, 1.0f) * MAX_uint8);
	State.Skid = FMath::RoundToInt(FMath::Clamp(Skid, 0.0f, 1.0f) * MAX_uint8);
	State.Gear = FMath::Clamp(Gear, -1, 127);
	State.bShifting = bShifting;
	return State;
}

void UGVehicleAudioComponent::UpdatePlayback(const FGVehicleAudioState& State, float DeltaTime)
{
	const float Rpm = State.Rpm / float(MAX_uint16);
	const float Load = State.Load / float(MAX_uint8);
	const float Skid = State.Skid / float(MAX_uint8);
	const bool bShift = State.bShifting || State.Gear != LastGear;
	LastGear = State.Gear;

	RpmBlend = SmoothTowards(RpmBlend, Rpm, bShift ? Profile->ShiftResponse : Profile->EngineResponse, DeltaTime);
	LoadBlend = SmoothTowards(LoadBlend, bShift ? 0.0f : Load, Profile->ShiftResponse, DeltaTime);
	ShiftGain = SmoothTowards(ShiftGain, bShift ? Profile->ShiftVolume : 1.0f, Profile->EngineResponse, DeltaTime);
	const float LimiterStart = 1.0f - FMath::Max(Profile->LimiterBlendRange, UE_KINDA_SMALL_NUMBER);
	const float LimiterTarget = LimiterVoice && !bShift && Load >= Profile->MinimumLimiterLoad
		? FMath::SmoothStep(LimiterStart, 1.0f, Rpm)
		: 0.0f;
	LimiterBlend = SmoothTowards(LimiterBlend, LimiterTarget, Profile->ShiftResponse, DeltaTime);
	UpdateEngine(RpmBlend, LoadBlend, LimiterBlend, ShiftGain);

	SkidBlend = SmoothTowards(SkidBlend, Skid, Skid > SkidBlend ? Profile->TireAttack : Profile->TireRelease, DeltaTime);
	SetLoopMix(TireVoice, SkidBlend * Profile->TireVolume, FMath::Lerp(Profile->TireLowPitch, Profile->TireHighPitch, SkidBlend));
}

void UGVehicleAudioComponent::UpdateEngine(float Rpm, float Load, float Limiter, float ShiftVolume)
{
	if (EngineLayers.IsEmpty())
	{
		return;
	}

	const float ReferenceRpm = FMath::Lerp(EngineLayers[0].RecordedRpm, EngineLayers.Last().RecordedRpm, Rpm);
	int32 Upper = 0;
	while (Upper < EngineLayers.Num() - 1 && ReferenceRpm > EngineLayers[Upper].RecordedRpm)
	{
		++Upper;
	}
	const int32 Lower = FMath::Max(0, Upper - 1);
	const float Band = FMath::SmoothStep(EngineLayers[Lower].RecordedRpm,
		FMath::Max(EngineLayers[Upper].RecordedRpm, EngineLayers[Lower].RecordedRpm + UE_KINDA_SMALL_NUMBER), ReferenceRpm);
	const float Volume = Profile->EngineVolume * FMath::Lerp(Profile->CoastVolume, 1.0f, Load) * ShiftVolume;
	const float LowPass = FMath::Lerp(Profile->CoastLowPass, Profile->LoadLowPass, Load);

	for (int32 Index = 0; Index < EngineVoices.Num(); ++Index)
	{
		const float Weight = Index == Lower ? FMath::Sqrt(1.0f - Band) : (Index == Upper ? FMath::Sqrt(Band) : 0.0f);
		const FGVehicleAudioEngineLayer& Layer = EngineLayers[Index];
		if (UAudioComponent* Voice = EngineVoices[Index])
		{
			Voice->SetLowPassFilterFrequency(LowPass);
			SetLoopMix(Voice, Volume * Layer.Volume * Weight * FMath::Sqrt(1.0f - Limiter), ReferenceRpm / Layer.RecordedRpm);
		}
	}
	if (LimiterVoice)
	{
		LimiterVoice->SetLowPassFilterFrequency(LowPass);
		const FGVehicleAudioEngineLayer& Layer = Profile->LimiterLayer;
		SetLoopMix(LimiterVoice, Volume * Layer.Volume * FMath::Sqrt(Limiter), ReferenceRpm / FMath::Max(Layer.RecordedRpm, 1.0f));
	}
}

UAudioComponent* UGVehicleAudioComponent::CreateLoop(USoundWave* Sound, const FVector& Offset)
{
	if (!Sound || !GetOwner()->GetRootComponent())
	{
		return nullptr;
	}
	if (!Sound->bLooping)
	{
		UE_LOG(LogVehicleAudio, Warning, TEXT("Enable Looping on vehicle sound %s"), *Sound->GetPathName());
		return nullptr;
	}

	UAudioComponent* Voice = NewObject<UAudioComponent>(GetOwner());
	Voice->bAutoActivate = false;
	Voice->SetupAttachment(GetOwner()->GetRootComponent());
	Voice->SetRelativeLocation(Offset);
	Voice->AttenuationSettings = Profile->Attenuation;
	Voice->SetSound(Sound);
	Voice->SetLowPassFilterEnabled(true);
	Voice->RegisterComponent();
	return Voice;
}

void UGVehicleAudioComponent::PlayVehicleSound(FName Event)
{
	if (GetNetMode() == NM_DedicatedServer || !Profile)
	{
		return;
	}
	const FGVehicleAudioEvent* Entry = Profile->Events.Find(Event);
	if (!Entry || !Entry->Sound)
	{
		return;
	}

	USceneComponent* AttachTo = GetOwner()->GetRootComponent();
	// Named sockets can live on any mesh component, including a mesh separate from the physics root.
	if (!Entry->Socket.IsNone())
	{
		TInlineComponentArray<USceneComponent*> Components(GetOwner());
		for (USceneComponent* Component : Components)
		{
			if (Component->DoesSocketExist(Entry->Socket))
			{
				AttachTo = Component;
				break;
			}
		}
	}
	if (AttachTo)
	{
		UGameplayStatics::SpawnSoundAttached(Entry->Sound, AttachTo, Entry->Socket, Entry->Offset,
			EAttachLocation::KeepRelativeOffset, true, Entry->Volume, 1.0f, 0.0f, Profile->Attenuation);
	}
}
