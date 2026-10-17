#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "G_Vehicle_Seating_Component.generated.h"

class AGPlayerCar;
class AGPlayerController;
class APlayerState;
class AWhos_DrivingCharacter;
class UGVehicleSeatComponent;

USTRUCT(BlueprintType)
struct FGVehicleSeatState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Seats")
	TObjectPtr<AWhos_DrivingCharacter> occupant;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Seats")
	TObjectPtr<APlayerState> playerState;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Seats")
	TObjectPtr<APlayerState> reservedBy;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle|Seats")
	float transitionEndTime = 0.0f;
};

struct FGVehicleCharacterRestore
{
	bool bUseControllerRotationYaw = true;
	bool bUseControllerRotationPitch = false;
	bool bUseControllerRotationRoll = false;
	uint8 movementMode = 0;
	uint8 customMovementMode = 0;
	float gravityScale = 1.0f;
	ECollisionEnabled::Type capsuleCollision = ECollisionEnabled::QueryAndPhysics;
	ECollisionEnabled::Type meshCollision = ECollisionEnabled::QueryOnly;
};

UCLASS(ClassGroup = (Vehicle), meta = (BlueprintSpawnableComponent))
class WHOS_DRIVING_API UGVehicleSeatingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGVehicleSeatingComponent();
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void TickComponent(float deltaSeconds, ELevelTick tickType, FActorComponentTickFunction* tickFunction) override;

	UFUNCTION(BlueprintPure, Category = "Vehicle|Seats")
	FGVehicleSeatState GetSeatState(int32 seatIndex) const;

	UFUNCTION(BlueprintPure, Category = "Vehicle|Seats")
	UGVehicleSeatComponent* GetSeatComponent(int32 seatIndex) const;

	UFUNCTION(BlueprintPure, Category = "Vehicle|Seats")
	int32 GetCharacterSeat(AWhos_DrivingCharacter* character) const;

	bool RequestSeat(AGPlayerController* controller, int32 seatIndex, const FVector& viewOrigin, const FVector& viewDirection);
	bool RequestExit(AGPlayerController* controller);
	bool CancelTransition(AGPlayerController* controller);
	bool InitializeDriver(AGPlayerController* controller, AWhos_DrivingCharacter* character);
	bool InitializeOccupant(AGPlayerController* controller, AWhos_DrivingCharacter* character, int32 seatIndex);
	void RemovePlayer(AGPlayerController* controller, bool bDestroyCharacter);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Seats", meta = (ClampMin = "0.1"))
	float transitionDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Seats", meta = (ClampMin = "50.0"))
	float interactionDistance = 500.0f;

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UFUNCTION()
	void OnRep_SeatStates();

	UPROPERTY(ReplicatedUsing = OnRep_SeatStates, BlueprintReadOnly, Category = "Vehicle|Seats")
	TArray<FGVehicleSeatState> seatStates;

private:
	AGPlayerCar* GetVehicle() const;
	bool IsAuthority() const;
	bool IsPlayerValid(AGPlayerController* controller) const;
	bool IsSeatTargetValid(AGPlayerController* controller, int32 seatIndex, const FVector& origin, const FVector& direction) const;
	void CompleteTransition(int32 seatIndex);
	void ClearReservation(int32 seatIndex);
	void PublishSeatStates();
	void ApplySeatStates();
	void RestoreCharacter(AWhos_DrivingCharacter* character);
	void StopVehicleControls();

	TArray<TWeakObjectPtr<AGPlayerController>> occupantControllers;
	TArray<TWeakObjectPtr<AGPlayerController>> reservationControllers;
	TMap<TWeakObjectPtr<AWhos_DrivingCharacter>, FGVehicleCharacterRestore> characterRestoreStates;
};
