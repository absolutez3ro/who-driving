#include "GWeaponAmmoWidget.h"
#include "GVehicleCombatComponent.h"
#include "GVehicleWeapon.h"
#include "Components/TextBlock.h"

void UGWeaponAmmoWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RefreshAmmo();
}

void UGWeaponAmmoWidget::SetCombatComponent(UGVehicleCombatComponent* newCombatComponent)
{
	if (combatComponent == newCombatComponent)
	{
		return;
	}
	if (combatComponent.IsValid())
	{
		combatComponent->OnWeaponAmmoChanged.RemoveDynamic(this, &UGWeaponAmmoWidget::RefreshAmmo);
	}
	combatComponent = newCombatComponent;
	if (newCombatComponent)
	{
		newCombatComponent->OnWeaponAmmoChanged.AddDynamic(this, &UGWeaponAmmoWidget::RefreshAmmo);
	}
	RefreshAmmo();
}

void UGWeaponAmmoWidget::RefreshAmmo()
{
	const AGVehicleWeapon* weapon = combatComponent.IsValid() ? combatComponent->GetCurrentWeapon() : nullptr;
	if (!weapon || !weapon->IsEquipped())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	ammoText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), weapon->GetBulletCount(), weapon->GetReserveAmmo())));
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
