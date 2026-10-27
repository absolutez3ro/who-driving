#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "G_Vehicle_Radio_Playlist.h"
#include "G_Vehicle_Radio_Component.generated.h"

class UAudioComponent;
class USoundWave;
struct FStreamableHandle;

UENUM()
enum class EGVehicleRadioCommand : uint8
{
	Toggle,
	Previous,
	Next
};

USTRUCT(BlueprintType)
struct FGVehicleRadioState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Radio")
	bool bEnabled = false;
	UPROPERTY(BlueprintReadOnly, Category = "Radio")
	TSoftObjectPtr<USoundWave> track;
	UPROPERTY(BlueprintReadOnly, Category = "Radio")
	double startedAt = 0.0;
	UPROPERTY(BlueprintReadOnly, Category = "Radio")
	float playbackOffset = 0.0f;
	UPROPERTY()
	uint32 revision = 0;
};

// Query-only dashboard target. Gameplay requests arrive through the owning controller.
UCLASS(ClassGroup = (Vehicle), meta = (BlueprintSpawnableComponent))
class WHOS_DRIVING_API UGVehicleRadioComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UGVehicleRadioComponent();
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Vehicle|Radio")
	FGVehicleRadioState GetRadioState() const { return radioState; }
	UFUNCTION(BlueprintPure, Category = "Vehicle|Radio")
	float GetPlaybackPosition() const;

	// Authority-only entry point; clients use validated controller RPCs.
	bool ExecuteCommand(EGVehicleRadioCommand command);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Radio",
			  meta = (RelativeToGameContentDir, LongPackageName))
	FDirectoryPath trackDirectory;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Radio", meta = (ClampMin = "0", ClampMax = "1"))
	float volume = 0.35f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Radio", meta = (ClampMin = "0"))
	float innerRadius = 200.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Radio", meta = (ClampMin = "1"))
	float falloffDistance = 1800.0f;

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type endPlayReason) override;

private:
	void DiscoverTracks();
	void SelectTrack(const FSoftObjectPath& track);
	void PublishState();
	void ApplyState();
	void FinishLoading(uint32 revision);
	void AdvanceTrack();
	void RefreshPlayback();
	double GetServerTime() const;
	UFUNCTION()
	void OnRep_RadioState();

	UPROPERTY(ReplicatedUsing = OnRep_RadioState)
	FGVehicleRadioState radioState;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> audioComponent;
	UPROPERTY(Transient)
	TObjectPtr<USoundWave> loadedTrack;
	TSharedPtr<FStreamableHandle> loadHandle;
	TArray<FSoftObjectPath> tracks;
	FGVehicleRadioPlaylist playlist;
	FTimerHandle trackTimer;
	FTimerHandle playbackTimer;
	FTimerHandle clockRetryTimer;
};
