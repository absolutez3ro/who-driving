#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "G_Vehicle_Seat_Component.generated.h"

UENUM(BlueprintType)
enum class EGVehicleSeat : uint8
{
	Driver,
	FrontPassenger,
	RearLeft,
	RearRight
};

UCLASS(ClassGroup = (Vehicle), meta = (BlueprintSpawnableComponent))
class WHOS_DRIVING_API UGVehicleSeatComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UGVehicleSeatComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Seat")
	EGVehicleSeat seatId = EGVehicleSeat::Driver;

	// Character root relative to the interaction box; camera and sitting pose can be tuned independently.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Seat")
	FTransform characterOffset = FTransform(FVector(0.0f, 0.0f, 50.0f));

	// Exit point in vehicle mesh space, outside the matching side of the chassis.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Seat")
	FVector exitLocation = FVector(20.0f, -230.0f, 110.0f);

	// Positive lateral distance is mirrored for the left seats.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Window")
	FVector windowOffset = FVector(0.0f, 85.0f, 5.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Window")
	FVector seatedCameraOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Window", meta = (ClampMin = "0.1"))
	float windowDuration = 0.45f;
};
