#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GArcadeCameraSwitcherComponent.generated.h"

class UCameraComponent;

// One selectable view: a camera component of the owner, picked in the Blueprint editor.
USTRUCT(BlueprintType)
struct FGArcadeCameraView
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	FName viewName;

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.CameraComponent"))
	FComponentReference camera;
};

/**
 * Switches the owner's view between its camera components, for example a chase camera and a cockpit camera.
 * Exactly one listed camera is active at a time; the player camera manager uses the active one while the
 * owner is the view target. Purely cosmetic and local, so nothing is replicated.
 */
UCLASS(ClassGroup = (Vehicle), meta = (BlueprintSpawnableComponent))
class WHOS_DRIVING_API UGArcadeCameraSwitcherComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGArcadeCameraSwitcherComponent();

	// Moves to the next view and wraps around after the last one.
	UFUNCTION(BlueprintCallable, Category = "Vehicle|Arcade|Camera")
	void CycleView();

	UFUNCTION(BlueprintCallable, Category = "Vehicle|Arcade|Camera")
	void SetView(int32 viewIndex);

	UFUNCTION(BlueprintPure, Category = "Vehicle|Arcade|Camera")
	int32 GetViewIndex() const
	{
		return currentViewIndex;
	}

	UFUNCTION(BlueprintPure, Category = "Vehicle|Arcade|Camera")
	FName GetViewName() const;

protected:
	void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Arcade|Camera")
	TArray<FGArcadeCameraView> views;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle|Arcade|Camera", meta = (ClampMin = "0"))
	int32 initialViewIndex = 0;

private:
	UCameraComponent* ResolveCamera(int32 viewIndex) const;

	int32 currentViewIndex = INDEX_NONE;
};
