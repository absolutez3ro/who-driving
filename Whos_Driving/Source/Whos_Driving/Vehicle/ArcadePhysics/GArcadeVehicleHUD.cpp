#include "GArcadeVehicleHUD.h"
#include "GArcadeVehicleComponent.h"
#include "GArcadeVehicleTypes.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"

void AGArcadeVehicleHUD::DrawHUD()
{
	Super::DrawHUD();

	const APawn* pawn = GetOwningPawn();
	const UGArcadeVehicleComponent* vehicle = pawn ? pawn->FindComponentByClass<UGArcadeVehicleComponent>() : nullptr;
	if (!vehicle)
	{
		return;
	}

	float cursorY = screenOffset.Y;
	DrawVehicleSummary(*vehicle, cursorY);
	DrawWheelTable(*vehicle, cursorY);
}

void AGArcadeVehicleHUD::DrawVehicleSummary(const UGArcadeVehicleComponent& vehicle, float& cursorY)
{
	const FString gearText = vehicle.IsShifting() ? FString(TEXT("shifting")) : FormatGear(vehicle.GetCurrentGear());
	const FString airText = vehicle.IsInAir() ? FString(TEXT("  IN AIR")) : FString();
	DrawTextLine(FString::Printf(TEXT("Speed %4.0f km/h   Gear %s   %5.0f rpm%s"),
								 vehicle.GetForwardSpeedKmh(), *gearText, vehicle.GetEngineRpm(), *airText),
				 FLinearColor::White, cursorY);
	DrawTextLine(TEXT("vp.Debug 1 draws traces and forces"), FLinearColor::Gray, cursorY);
}

void AGArcadeVehicleHUD::DrawWheelTable(const UGArcadeVehicleComponent& vehicle, float& cursorY)
{
	DrawTextLine(TEXT("Wheel  Compr   Fz [N]   Slip   Angle   State"), FLinearColor::White, cursorY);

	const TArray<FGArcadeWheelState> wheelStates = vehicle.GetWheelStates();
	const TArray<FGArcadeWheelSetup> wheelSetups = vehicle.GetWheelSetups();
	for (int32 wheelIndex = 0; wheelIndex < wheelStates.Num(); ++wheelIndex)
	{
		const FGArcadeWheelState& state = wheelStates[wheelIndex];
		const FString wheelName = wheelSetups.IsValidIndex(wheelIndex) ? wheelSetups[wheelIndex].wheelName.ToString() : FString::FromInt(wheelIndex);
		const TCHAR* stateText = !state.bHasContact ? TEXT("air") : (state.bIsSliding ? TEXT("SLIDE") : TEXT("grip"));
		const FLinearColor rowColor = state.bIsSliding ? FLinearColor(1.0f, 0.55f, 0.1f) : FLinearColor::Green;
		DrawTextLine(FString::Printf(TEXT("%-5s  %4.0f%%  %7.0f  %5.2f  %5.1f   %s"),
									 *wheelName, state.compressionRatio * 100.0f, state.normalLoad, state.slipRatio,
									 state.slipAngle, stateText),
					 state.bHasContact ? rowColor : FLinearColor::Gray, cursorY);
	}
}

void AGArcadeVehicleHUD::DrawTextLine(const FString& text, const FLinearColor& color, float& cursorY)
{
	DrawText(text, color, screenOffset.X, cursorY, GEngine->GetSmallFont(), textScale);
	cursorY += lineHeight * textScale;
}

FString AGArcadeVehicleHUD::FormatGear(int32 gear)
{
	if (gear < 0)
	{
		return TEXT("R");
	}
	if (gear == 0)
	{
		return TEXT("N");
	}
	return FString::FromInt(gear);
}
