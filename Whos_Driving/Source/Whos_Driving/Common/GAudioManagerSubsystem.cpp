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

	UE_LOG(LogTemp, Warning,
		TEXT("[AudioManager] MenuMusic = %s | ChaseMusic = %s"),
		*GetNameSafe(musicAsset),
		*GetNameSafe(chaseMusicAsset));
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

	if (!IsValid(soundMix))
	{
		return;
	}

	masterVolume = GetVolumeAdjuster(soundMix, masterSoundClass);
	musicVolume = GetVolumeAdjuster(soundMix, musicSoundClass);
	SFXVolume = GetVolumeAdjuster(soundMix, SFXSoundClass);

	UGameplayStatics::PushSoundMixModifier(this, soundMix);

	ApplyMixerOverride(masterSoundClass, masterVolume);
	ApplyMixerOverride(musicSoundClass, musicVolume);
	ApplyMixerOverride(SFXSoundClass, SFXVolume);

	UE_LOG(LogTemp, Warning,
		TEXT("[Mixer] configured. MusicClass=%s, ChaseAssetClass=%s, MusicVolume=%.2f"),
		*GetNameSafe(musicSoundClass),
		*GetNameSafe(IsValid(chaseMusicAsset) ? chaseMusicAsset->GetSoundClass() : nullptr),
		musicVolume);
}

float UGAudioManagerSubsystem::GetVolumeAdjuster(USoundMix* InSoundMix, USoundClass* InSoundClass)
{
	if (!IsValid(InSoundMix) || !IsValid(InSoundClass))
	{
		return 1.0f;
	}

	for (const FSoundClassAdjuster& Adjuster : InSoundMix->SoundClassEffects)
	{
		if (Adjuster.SoundClassObject == InSoundClass)
		{
			return Adjuster.VolumeAdjuster;
		}
	}

	return 1.0f;
}


UAudioComponent* UGAudioManagerSubsystem::StartMusicComponent(USoundBase* sound, TObjectPtr<UAudioComponent>& component)
{
	if (!IsValid(sound))
	{
		UE_LOG(LogTemp, Error, TEXT("[AudioManager] StartMusicComponent: Sound is invalid!"));
		return nullptr;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[AudioManager] Starting sound: %s | SoundClass: %s"),
		*GetNameSafe(sound),
		*GetNameSafe(sound->GetSoundClass()));

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

	component = UGameplayStatics::SpawnSound2D(this, sound, 1.0f, 1.0f, 0.0f, nullptr, true, false);

	if (IsValid(component))
	{
		component->bAutoDestroy = false;
		//component->SetVolumeMultiplier(musicVolume);
		component->FadeIn(fadeDuration, 1.0f);
		UE_LOG(LogTemp, Warning,
			TEXT("[AudioManager] Audio component created: %s | IsPlaying: %s"),
			*GetNameSafe(component),
			component->IsPlaying() ? TEXT("YES") : TEXT("NO"));
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
	UE_LOG(LogTemp, Warning,
		TEXT("[AudioManager] PlayChaseMusic called. Asset: %s | Radios: %d"),
		*GetNameSafe(chaseMusicAsset),
		activeRadios.Num());
	if (activeRadios.Num() > 0)
	{
		return;
	}

	if (IsValid(chaseMusicAsset))
	{
		StartMusicComponent(chaseMusicAsset, chaseMusicComponent);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AudioManager] ChaseMusicAsset is INVALID!"));
	}

	USoundBase* Sound = chaseMusicComponent->Sound;

	UE_LOG(LogTemp, Warning,
		TEXT("[ChaseMusic] Asset=%s | AssetClass=%s | Component=%s"),
		*GetNameSafe(Sound),
		*GetNameSafe(Sound ? Sound->GetSoundClass() : nullptr),
		*GetNameSafe(chaseMusicComponent));

	if (Sound)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ChaseMusic] SoundClass is SC_Music: %s"),
			Sound->GetSoundClass() == musicSoundClass
			? TEXT("YES")
			: TEXT("NO"));
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
		if (activeRadios.Contains(source))
		{
			return;
		}
		const bool bWasRadioActive = activeRadios.Num() > 0;
		activeRadios.Add(source);
		if (!bWasRadioActive && IsValid(chaseMusicComponent))
		{
			chaseMusicComponent->SetPaused(true);
		}
	}
	else
	{
		activeRadios.Remove(source);
		if (activeRadios.Num() == 0 && bChaseMusicRequested)
		{
			chaseMusicComponent->SetPaused(false);
		}
	}
}

void UGAudioManagerSubsystem::ApplyMixerOverride(USoundClass* soundClass, float volume)
{
	if (IsValid(soundMix) && IsValid(soundClass))
	{
		UGameplayStatics::SetSoundMixClassOverride(
			this, soundMix, soundClass, FMath::Clamp(volume, 0.0f, 4.0f),
			1.0f, 0.0f, true);

		UE_LOG(LogTemp, Warning,
			TEXT("[Mixer] Applying SoundMix override: Class=%s Volume=%.2f"),
			*GetNameSafe(soundClass),
			volume);

		UGameplayStatics::PushSoundMixModifier(this, soundMix);
	}
}


void UGAudioManagerSubsystem::SetMasterVolume(float newVolume)
{
	masterVolume = FMath::Clamp(newVolume, 0.0f, 4.0f);
	ApplyMixerOverride(masterSoundClass, masterVolume);
}

void UGAudioManagerSubsystem::SetMusicVolume(float NewVolume)
{
	musicVolume = FMath::Clamp(NewVolume, 0.0f, 4.0f);
	UE_LOG(LogTemp, Warning,
		TEXT("[Mixer] SetMusicVolume called: %.2f | MusicClass: %s"),
		musicVolume,
		*GetNameSafe(musicSoundClass));
	ApplyMixerOverride(musicSoundClass, musicVolume);\
}

void UGAudioManagerSubsystem::SetSFXVolume(float newVolume)
{
	SFXVolume = FMath::Clamp(newVolume, 0.0f, 4.0f);
	ApplyMixerOverride(SFXSoundClass, SFXVolume);
}

//void UGAudioManagerSubsystem::ExecSetMasterVolume(float newVolume)
//{
//	SetMasterVolume(newVolume);
//}
//
//void UGAudioManagerSubsystem::ExecSetMusicVolume(float newVolume)
//{
//	SetMusicVolume(newVolume);
//}
//
//void UGAudioManagerSubsystem::ExecSetSFXVolume(float newVolume)
//{
//	SetSFXVolume(newVolume);
//}

