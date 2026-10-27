#include "G_Vehicle_Radio_Component.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/AudioComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundWave.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogVehicleRadio, Log, All);

UGVehicleRadioComponent::UGVehicleRadioComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
	InitBoxExtent(FVector(8.0f, 17.0f, 10.0f));
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	SetHiddenInGame(true);
	trackDirectory.Path = TEXT("/Game/Audio/Radio/Tracks");
}

void UGVehicleRadioComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGVehicleRadioComponent, radioState);
}

void UGVehicleRadioComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		DiscoverTracks();
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		audioComponent = NewObject<UAudioComponent>(GetOwner(), FName(*(GetName() + TEXT("_Audio"))));
		// Every replica creates this local speaker. Its attachment must have a stable network name.
		audioComponent->SetNetAddressable();
		audioComponent->bAutoActivate = false;
		audioComponent->bShouldRemainActiveIfDropped = true;
		audioComponent->SetupAttachment(this);
		audioComponent->bOverrideAttenuation = true;
		audioComponent->AttenuationOverrides.bSpatialize = true;
		audioComponent->AttenuationOverrides.bAttenuate = true;
		audioComponent->AttenuationOverrides.AttenuationShapeExtents = FVector(innerRadius, 0.0f, 0.0f);
		audioComponent->AttenuationOverrides.FalloffDistance = falloffDistance;
		audioComponent->SetVolumeMultiplier(volume);
		audioComponent->RegisterComponent();
	}
	ApplyState();
}

void UGVehicleRadioComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (UWorld* world = GetWorld())
	{
		world->GetTimerManager().ClearTimer(trackTimer);
		world->GetTimerManager().ClearTimer(playbackTimer);
		world->GetTimerManager().ClearTimer(clockRetryTimer);
	}
	if (loadHandle)
	{
		loadHandle->CancelHandle();
		loadHandle.Reset();
	}
	if (IsValid(audioComponent))
	{
		audioComponent->DestroyComponent();
		audioComponent = nullptr;
	}
	Super::EndPlay(endPlayReason);
}

void UGVehicleRadioComponent::DiscoverTracks()
{
	TArray<FSoftObjectPath> discovered;
	IAssetRegistry& registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	FARFilter filter;
	filter.PackagePaths.Add(FName(trackDirectory.Path));
	filter.ClassPaths.Add(USoundWave::StaticClass()->GetClassPathName());
	filter.bRecursivePaths = true;
	TArray<FAssetData> assets;
	registry.GetAssets(filter, assets);
	for (const FAssetData& asset : assets)
	{
		discovered.AddUnique(asset.GetSoftObjectPath());
	}
	discovered.Sort([](const FSoftObjectPath& left, const FSoftObjectPath& right)
					{ return left.ToString() < right.ToString(); });
	if (tracks != discovered)
	{
		tracks = MoveTemp(discovered);
		playlist.Reset(tracks, radioState.track.ToSoftObjectPath());
	}
}

double UGVehicleRadioComponent::GetServerTime() const
{
	const AGameStateBase* gameState = GetWorld()->GetGameState();
	return gameState ? gameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}

float UGVehicleRadioComponent::GetPlaybackPosition() const
{
	return radioState.playbackOffset +
		   (radioState.bEnabled ? FMath::Max(0.0, GetServerTime() - radioState.startedAt) : 0.0);
}

bool UGVehicleRadioComponent::ExecuteCommand(EGVehicleRadioCommand command)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	if (command == EGVehicleRadioCommand::Toggle)
	{
		if (radioState.bEnabled)
		{
			radioState.playbackOffset = GetPlaybackPosition();
			radioState.bEnabled = false;
			PublishState();
			return true;
		}
		// Refresh here too: newly imported tracks become available without an Editor restart.
		DiscoverTracks();
		if (tracks.IsEmpty())
		{
			UE_LOG(LogVehicleRadio, Display, TEXT("No SoundWave tracks in %s"), *trackDirectory.Path);
			return false;
		}
		radioState.bEnabled = true;
		if (!tracks.Contains(radioState.track.ToSoftObjectPath()))
		{
			SelectTrack(playlist.Next());
		}
		else
		{
			radioState.startedAt = GetServerTime();
			PublishState();
		}
		return true;
	}
	if (!radioState.bEnabled)
	{
		return false;
	}
	if (command == EGVehicleRadioCommand::Next || command == EGVehicleRadioCommand::Previous)
	{
		SelectTrack(command == EGVehicleRadioCommand::Next ? playlist.Next() : playlist.Previous());
		return true;
	}
	return false;
}

void UGVehicleRadioComponent::SelectTrack(const FSoftObjectPath& track)
{
	radioState.track = TSoftObjectPtr<USoundWave>(track);
	radioState.playbackOffset = 0.0f;
	radioState.startedAt = GetServerTime();
	PublishState();
}

void UGVehicleRadioComponent::PublishState()
{
	++radioState.revision;
	GetOwner()->FlushNetDormancy();
	GetOwner()->ForceNetUpdate();
	ApplyState();
}

void UGVehicleRadioComponent::OnRep_RadioState()
{
	if (HasBegunPlay())
	{
		ApplyState();
	}
}

void UGVehicleRadioComponent::ApplyState()
{
	GetWorld()->GetTimerManager().ClearTimer(trackTimer);
	GetWorld()->GetTimerManager().ClearTimer(playbackTimer);
	GetWorld()->GetTimerManager().ClearTimer(clockRetryTimer);
	if (loadHandle)
	{
		loadHandle->CancelHandle();
		loadHandle.Reset();
	}
	if (audioComponent)
	{
		audioComponent->Stop();
		audioComponent->SetSound(nullptr);
	}
	loadedTrack = nullptr;
	if (!radioState.bEnabled || radioState.track.IsNull())
	{
		return;
	}
	if (!GetOwner()->HasAuthority() && (!GetWorld()->GetGameState() || !GetWorld()->GetGameState()->HasBegunPlay()))
	{
		GetWorld()->GetTimerManager().SetTimer(clockRetryTimer, this, &ThisClass::ApplyState, 0.1f, false);
		return;
	}
	loadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		radioState.track.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &ThisClass::FinishLoading, radioState.revision));
}

void UGVehicleRadioComponent::FinishLoading(uint32 revision)
{
	if (!HasBegunPlay() || revision != radioState.revision || !radioState.bEnabled)
	{
		return;
	}
	loadedTrack = radioState.track.Get();
	const float duration = loadedTrack ? loadedTrack->GetDuration() : 0.0f;
	if (!loadedTrack || loadedTrack->bLooping || !FMath::IsFinite(duration) || duration <= 0.0f)
	{
		UE_LOG(LogVehicleRadio, Warning, TEXT("Radio track must be a finite non-looping SoundWave: %s"),
			   *radioState.track.ToString());
		if (GetOwner()->HasAuthority())
		{
			tracks.Remove(radioState.track.ToSoftObjectPath());
			playlist.Reset(tracks);
			radioState.bEnabled = !tracks.IsEmpty();
			SelectTrack(playlist.Next());
		}
		return;
	}
	const float position = GetPlaybackPosition();
	if (GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(trackTimer, this, &ThisClass::AdvanceTrack,
											   FMath::Max(0.01f, duration - position), false);
	}
	if (audioComponent)
	{
		audioComponent->SetSound(loadedTrack);
		RefreshPlayback();
		// An inaudible one-shot SoundWave may be culled. Re-enter at the shared
		// playback time when a listener approaches, without changing the imported asset.
		GetWorld()->GetTimerManager().SetTimer(playbackTimer, this, &ThisClass::RefreshPlayback, 0.5f, true);
	}
}

void UGVehicleRadioComponent::RefreshPlayback()
{
	if (radioState.bEnabled && loadedTrack && audioComponent && !audioComponent->IsPlaying() &&
		UGameplayStatics::AreAnyListenersWithinRange(this, GetComponentLocation(), innerRadius + falloffDistance))
	{
		const float position = GetPlaybackPosition();
		if (position < loadedTrack->GetDuration())
		{
			audioComponent->Play(position);
		}
	}
}

void UGVehicleRadioComponent::AdvanceTrack()
{
	ExecuteCommand(EGVehicleRadioCommand::Next);
}
