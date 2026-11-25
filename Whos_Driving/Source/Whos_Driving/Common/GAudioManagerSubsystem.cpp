// Fill out your copyright notice in the Description page of Project Settings.


#include "GAudioManagerSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

void UGAudioManagerSubsystem::Deinitialize()
{
	activeRadios.Empty();
	if (IsValid(musicComponent))
	{
		musicComponent->Stop();
		musicComponent->DestroyComponent();
		musicComponent = nullptr;
	}

	if (IsValid(chaseMusicComponent))
	{
		chaseMusicComponent->Stop();
		chaseMusicComponent->DestroyComponent();
		chaseMusicComponent = nullptr;
	}

	if (IsValid(soundMix))
	{
		UGameplayStatics::PopSoundMixModifier(this, soundMix);
	}
	Super::Deinitialize();
}

void UGAudioManagerSubsystem::SetMusicAssets(USoundBase* inMusic, USoundBase* inChaseMusic)
{
	musicAsset = inMusic;
	chaseMusicAsset = inChaseMusic;
}

void UGAudioManagerSubsystem::ConfigureMixer(USoundMix* inSoundMix, USoundClass* inMasterClass, USoundClass* inMusicClass, USoundClass* inSFXClass)
{
	if (IsValid(soundMix))
	{
		UGameplayStatics::PopSoundMixModifier(this, soundMix);
	}

	soundMix = inSoundMix;
	masterSoundClass = inMasterClass;
	musicSoundClass = inMusicClass;
	SFXSoundClass = inSFXClass;

	if (IsValid(soundMix))
	{
		UGameplayStatics::PushSoundMixModifier(this, soundMix);
		ApplyMixerOverride(masterSoundClass, masterVolume);
		ApplyMixerOverride(musicSoundClass, musicVolume);
		ApplyMixerOverride(SFXSoundClass, SFXVolume);
	}
}

UAudioComponent* UGAudioManagerSubsystem::StartMusicComponent(USoundBase* sound, TObjectPtr<UAudioComponent>& component)
{
	if (!IsValid(sound))
	{
		return nullptr;
	}

	if (IsValid(component))
	{
		if (component->Sound == sound)
		{
			if (!component->IsPlaying())
			{
				component->FadeIn(fadeDuration, 1.0f);
			}
			return component;
		}
		component->FadeOut(fadeDuration, 0.0f);
		component = nullptr;
	}

	component = UGameplayStatics::SpawnSound2D(this, sound, 0.0f, 1.0f, 0.0f, nullptr, true, false);
	if (IsValid(component))
	{
		component->bAutoDestroy = false;
		if (!IsValid(musicSoundClass))
		{
			component->SetVolumeMultiplier(musicVolume);
		}
		component->FadeIn(fadeDuration, 1.0f);
	}
	return component;
}

void UGAudioManagerSubsystem::FadeOutComponent(UAudioComponent* component)
{
	if (IsValid(component) && component->IsPlaying())
	{
		component->FadeOut(fadeDuration, 0.0f);
	}
}

void UGAudioManagerSubsystem::PlayMusic()
{
	if (IsValid(musicAsset))
	{
		StartMusicComponent(musicAsset, musicComponent);
	}
}

void UGAudioManagerSubsystem::StopMusic()
{
	FadeOutComponent(musicComponent);
}

void UGAudioManagerSubsystem::PlayChaseMusic()
{
	bChaseMusicRequested = true;
	if (activeRadios.Num() > 0)
	{
		return;
	}

	if (IsValid(chaseMusicAsset))
	{
		StartMusicComponent(chaseMusicAsset, chaseMusicComponent);
	}
}

void UGAudioManagerSubsystem::StopChaseMusic()
{
	bChaseMusicRequested = false;
	FadeOutComponent(chaseMusicComponent);
}

void UGAudioManagerSubsystem::SetRadioActive(UObject* source, bool bActive)
{
	if (!IsValid(source))
	{
		return;
	}

	if (bActive)
	{
		activeRadios.Add(source);
		StopChaseMusic();
	}
	else
	{
		activeRadios.Remove(source);
		if (activeRadios.Num() == 0 && bChaseMusicRequested)
		{
			PlayChaseMusic();
		}
	}
}

void UGAudioManagerSubsystem::ApplyMixerOverride(USoundClass* soundClass, float volume)
{
	if (IsValid(soundMix) && IsValid(soundClass))
	{
		UGameplayStatics::SetSoundMixClassOverride(
			this, soundMix, soundClass, FMath::Clamp(volume, 0.0f, 1.0f),
			1.0f, 0.1f, true);
	}
}


void UGAudioManagerSubsystem::SetMasterVolume(float newVolume)
{
	masterVolume = FMath::Clamp(newVolume, 0.0f, 1.0f);
	ApplyMixerOverride(masterSoundClass, masterVolume);
}

void UGAudioManagerSubsystem::SetMusicVolume(float NewVolume)
{
	musicVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	ApplyMixerOverride(musicSoundClass, musicVolume);

	if (!IsValid(musicSoundClass))
	{
		if (IsValid(musicComponent))
		{
			musicComponent->SetVolumeMultiplier(musicVolume);
		}
		if (IsValid(chaseMusicComponent))
		{
			chaseMusicComponent->SetVolumeMultiplier(musicVolume);
		}
	}
}


void UGAudioManagerSubsystem::SetSFXVolume(float newVolume)
{
	SFXVolume = FMath::Clamp(newVolume, 0.0f, 1.0f);
	ApplyMixerOverride(SFXSoundClass, SFXVolume);
}
