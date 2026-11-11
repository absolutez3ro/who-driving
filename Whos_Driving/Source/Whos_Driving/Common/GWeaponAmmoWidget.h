#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GWeaponAmmoWidget.generated.h"

class UGVehicleCombatComponent;
class UTextBlock;

// Shows the selected weapon's ammunition as "magazine/reserve". The Blueprint child only owns the layout.
UCLASS(Abstract)
class WHOS_DRIVING_API UGWeaponAmmoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetCombatComponent(UGVehicleCombatComponent* newCombatComponent);

protected:
	void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ammoText;

private:
	UFUNCTION()
	void RefreshAmmo();

	TWeakObjectPtr<UGVehicleCombatComponent> combatComponent;
};
