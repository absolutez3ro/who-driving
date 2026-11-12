#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GArcadeVehicleHUD.generated.h"

class UGArcadeVehicleComponent;

// Text telemetry of the arcade vehicle the local player drives: speed, gear, rpm and per-wheel tire state.
UCLASS()
class WHOS_DRIVING_API AGArcadeVehicleHUD : public AHUD
{
	GENERATED_BODY()

public:
	void DrawHUD() override;

protected:
	UPROPERTY(EditAnywhere, Category = "HUD")
	FVector2D screenOffset = FVector2D(24.0f, 24.0f);

	UPROPERTY(EditAnywhere, Category = "HUD", meta = (ClampMin = "1.0"))
	float lineHeight = 16.0f;

	UPROPERTY(EditAnywhere, Category = "HUD", meta = (ClampMin = "0.1"))
	float textScale = 1.0f;

private:
	void DrawVehicleSummary(const UGArcadeVehicleComponent& vehicle, float& cursorY);
	void DrawWheelTable(const UGArcadeVehicleComponent& vehicle, float& cursorY);
	void DrawTextLine(const FString& text, const FLinearColor& color, float& cursorY);
	static FString FormatGear(int32 gear);
};
