// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GAudioManagerSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class USoundClass;
class USoundMix;

/**
 * 
 */
UCLASS()
class WHOS_DRIVING_API UGAudioManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void SetMusicAssets(USoundBase* inMusic, USoundBase* inChaseMusic);

	UFUNCTION(BlueprintCallable, Category = "Audio|Mixer")
	void ConfigureMixer(USoundMix* inSoundMix, USoundClass* inMasterClass, USoundClass* inMusicClass, USoundClass* inSFXClass);

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void PlayMusic();

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void StopMusic();

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void PlayChaseMusic();

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void StopChaseMusic();

	UFUNCTION(BlueprintCallable, Category = "Audio|Radio")
	void SetRadioActive(UObject* Source, bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Audio|Volume")
	void SetMasterVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Audio|Volume")
	void SetMusicVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Audio|Volume")
	void SetSFXVolume(float NewVolume);

	UFUNCTION(BlueprintPure, Category = "Audio|Radio")
	bool IsAnyRadioActive() const { return activeRadios.Num() > 0; }

private:
	UAudioComponent* StartMusicComponent(USoundBase* sound, TObjectPtr<UAudioComponent>& component);
	void FadeOutComponent(UAudioComponent* component);
	void ApplyMixerOverride(USoundClass* soundClass, float volume);

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> musicAsset;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> chaseMusicAsset;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> musicComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> chaseMusicComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundMix> soundMix;

	UPROPERTY(Transient)
	TObjectPtr<USoundClass> masterSoundClass;

	UPROPERTY(Transient)
	TObjectPtr<USoundClass> musicSoundClass;

	UPROPERTY(Transient)
	TObjectPtr<USoundClass> SFXSoundClass;

	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<UObject>> activeRadios;

	float masterVolume = 1.0f;
	float musicVolume = 1.0f;
	float SFXVolume = 1.0f;
	float fadeDuration = 0.35f;
	bool bChaseMusicRequested = false;
};
