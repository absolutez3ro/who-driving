#include "GPlayerController.h"

#include "GBoxComponent.h"
#include "GInteractionFeedback.h"
#include "GPlayerCar.h"
#include "GVehicleCombatComponent.h"
#include "GVehicleSeatComponent.h"
#include "GVehicleSeatingComponent.h"
#include "Whos_DrivingCharacter.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"

#define LOCTEXT_NAMESPACE "VehicleInteractions"

namespace
{
	UMeshComponent* FindInteractionMesh(const AActor& actor, const FName name)
	{
		TInlineComponentArray<UMeshComponent*> meshes(&actor);
		for (UMeshComponent* mesh : meshes)
		{
			if (mesh->GetFName() == name && mesh->IsVisible() && mesh->GetNumMaterials() > 0)
			{
				return mesh;
			}
		}
		return nullptr;
	}

	FName GetRepairMeshName(FName moduleId)
	{
		static const TMap<FName, FName> meshNames = {
			{ TEXT("Wheel_0"), TEXT("partFrontLeftWheel") },
			{ TEXT("Wheel_1"), TEXT("partFrontRightWheel") },
			{ TEXT("Wheel_2"), TEXT("partRearLeftWheel") },
			{ TEXT("Wheel_3"), TEXT("partRearRightWheel") },
			{ TEXT("Steering_Wheel"), TEXT("partSteeringWheel") },
			{ TEXT("Engine"), TEXT("partEngine") },
			{ TEXT("Electrician"), TEXT("partElectrician") }
		};
		if (const FName* meshName = meshNames.Find(moduleId))
		{
			return *meshName;
		}
		return moduleId;
	}
}

bool AGPlayerController::TraceVehicleModule(FName& outModuleId, float& outCurrentHPInPercentage, EGModuleState& outModuleState, int32& outRepairCost)
{
	return GetModuleInfo(FindLookedAtModule(), outModuleId, outCurrentHPInPercentage, outModuleState, outRepairCost);
}

UGBoxComponent* AGPlayerController::FindLookedAtModule() const
{
	const AWhos_DrivingCharacter* character = seatedVehicle ? driverCharacter.Get() : Cast<AWhos_DrivingCharacter>(GetPawn());
	if (!GetWorld() || !IsValid(character))
	{
		return nullptr;
	}
	FVector origin;
	FRotator rotation;
	GetPlayerViewPoint(origin, rotation);
	FCollisionQueryParams query(SCENE_QUERY_STAT(VehicleModuleLook), false, character);
	if (IsValid(seatedVehicle))
	{
		query.AddIgnoredComponent(seatedVehicle->GetMesh());
	}
	FHitResult hit;
	GetWorld()->LineTraceSingleByChannel(hit, origin, origin + rotation.Vector() * 3000.0f, ECC_GameTraceChannel3, query);
	return Cast<UGBoxComponent>(hit.GetComponent());
}

bool AGPlayerController::GetModuleInfo(const UGBoxComponent* hitBox, FName& outModuleId,
	float& outCurrentHPInPercentage, EGModuleState& outModuleState, int32& outRepairCost) const
{
	outModuleId = NAME_None;
	outCurrentHPInPercentage = 0.0f;
	if (!IsValid(hitBox) || hitBox->moduleId.IsNone())
	{
		return false;
	}
	const AGPlayerCar* vehicle = Cast<AGPlayerCar>(hitBox->GetOwner());
	if (!IsValid(vehicle))
	{
		return false;
	}
	const FGVehicleModuleState state = vehicle->GetModuleDamageState(hitBox->moduleId);
	if (state.moduleId.IsNone())
	{
		return false;
	}
	outModuleId = state.moduleId;
	outCurrentHPInPercentage = state.maxHP > 0.0f ? state.currentHP / state.maxHP * 100.0f : 0.0f;
	outModuleState = state.moduleState;
	outRepairCost = state.actualRepairCost;
	return true;
}

bool AGPlayerController::TraceSeatOrRadio(FHitResult& hit, const FVector& origin, const FVector& direction) const
{
	AWhos_DrivingCharacter* character = seatedVehicle ? driverCharacter.Get() : Cast<AWhos_DrivingCharacter>(GetPawn());
	if (!IsValid(character) || !GetWorld())
	{
		return false;
	}
	FCollisionQueryParams query(SCENE_QUERY_STAT(VehicleSeatLook), false, character);
	if (seatedVehicle)
	{
		const UGVehicleSeatingComponent* seating = seatedVehicle->GetSeatingComponent();
		query.AddIgnoredComponent(seating->GetSeatComponent(seating->GetCharacterSeat(character)));
	}
	if (!GetWorld()->LineTraceSingleByChannel(hit, origin, origin + direction * 500.0f, ECC_GameTraceChannel1, query))
	{
		return false;
	}
	AGPlayerCar* vehicle = Cast<AGPlayerCar>(hit.GetActor());
	if (!vehicle || (seatedVehicle && seatedVehicle != vehicle))
	{
		return false;
	}
	if (const UGVehicleSeatComponent* seat = Cast<UGVehicleSeatComponent>(hit.GetComponent()))
	{
		const FGVehicleSeatState state = vehicle->GetSeatingComponent()->GetSeatState(static_cast<int32>(seat->seatId));
		if (state.occupant || state.reservedBy)
		{
			return false;
		}
	}
	else if (hit.GetComponent() != vehicle->GetRadioComponent())
	{
		return false;
	}
	FCollisionQueryParams visibility(SCENE_QUERY_STAT(VehicleInteractionVisibility), false, character);
	if (seatedVehicle)
	{
		visibility.AddIgnoredActor(seatedVehicle);
	}
	return !GetWorld()->LineTraceTestByChannel(origin, hit.ImpactPoint, ECC_Visibility, visibility);
}

AWhos_DrivingCharacter* AGPlayerController::FindReviveTarget() const
{
	if (!IsLocalController() || bSkillCheckActive || bKnockedOut || !IsValid(driverCharacter) || !IsValid(seatedVehicle))
	{
		return nullptr;
	}
	FVector origin;
	FRotator rotation;
	GetPlayerViewPoint(origin, rotation);
	FCollisionQueryParams query(SCENE_QUERY_STAT(PlayerLook), false, driverCharacter);
	query.AddIgnoredActor(seatedVehicle);
	FHitResult hit;
	GetWorld()->LineTraceSingleByChannel(hit, origin, origin + rotation.Vector() * 3000.0f, ECC_GameTraceChannel4, query);
	AWhos_DrivingCharacter* target = Cast<AWhos_DrivingCharacter>(hit.GetActor());
	if (!IsValid(target))
	{
		return nullptr;
	}
	const UGVehicleCombatComponent* targetCombat = target->GetCombatComponent();
	const int32 targetSeat = targetCombat->GetSeatIndex();
	const int32 ownSeat = driverCharacter->GetCombatComponent()->GetSeatIndex();
	const bool bCanRevive = targetCombat->GetHealth() <= 0.0f && ownSeat != INDEX_NONE && targetSeat != INDEX_NONE
		&& (ownSeat < 2) == (targetSeat < 2);
	return bCanRevive ? target : nullptr;
}

FText AGPlayerController::GetInteractionKey(const UInputAction* action) const
{
	const ULocalPlayer* player = GetLocalPlayer();
	const UEnhancedInputLocalPlayerSubsystem* input = player ? player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!input || !action)
	{
		return FText::GetEmpty();
	}
	const TArray<FKey> keys = input->QueryKeysMappedToAction(action);
	for (const FKey& key : keys)
	{
		if (!key.IsGamepadKey())
		{
			return key.GetDisplayName();
		}
	}
	return keys.IsEmpty() ? FText::GetEmpty() : keys[0].GetDisplayName();
}

void AGPlayerController::UpdateInteractionFeedback()
{
	if (!interactionFeedback && PlayerCameraManager)
	{
		interactionFeedback = Cast<UGInteractionFeedback>(PlayerCameraManager->AddNewCameraModifier(UGInteractionFeedback::StaticClass()));
	}
	if (!interactionFeedback)
	{
		return;
	}
	if (bSkillCheckActive || bKnockedOut || !GetPawn())
	{
		interactionFeedback->Clear();
		return;
	}

	const auto showPrompt = [this](UPrimitiveComponent* mesh, UBoxComponent* box, const UInputAction* action, const FText& label)
	{
		const FText key = GetInteractionKey(action);
		if (key.IsEmpty())
		{
			return false;
		}
		interactionFeedback->Show(mesh, box, key, label);
		return true;
	};

	if (AWhos_DrivingCharacter* target = FindReviveTarget())
	{
		if (showPrompt(target->GetMesh(), nullptr, reviveAction, LOCTEXT("Revive", "Поднять")))
		{
			return;
		}
	}

	FVector origin;
	FRotator rotation;
	GetPlayerViewPoint(origin, rotation);
	FHitResult hit;
	if (TraceSeatOrRadio(hit, origin, rotation.Vector()))
	{
		AGPlayerCar* vehicle = CastChecked<AGPlayerCar>(hit.GetActor());
		if (UGVehicleSeatComponent* seat = Cast<UGVehicleSeatComponent>(hit.GetComponent()))
		{
			UMeshComponent* mesh = nullptr;
			if (seat->seatId == EGVehicleSeat::Driver || seat->seatId == EGVehicleSeat::FrontPassenger)
			{
				const FName seatMesh = seat->seatId == EGVehicleSeat::Driver ? TEXT("partLeftSeat") : TEXT("partRightSeat");
				mesh = FindInteractionMesh(*vehicle, seatMesh);
			}
			if (showPrompt(mesh, seat, interactAction, LOCTEXT("Sit", "Сесть")))
			{
				return;
			}
		}
		else
		{
			UGVehicleRadioComponent* radio = vehicle->GetRadioComponent();
			const FText label = radio->GetRadioState().bEnabled
				? LOCTEXT("TurnRadioOff", "Выключить радио") : LOCTEXT("TurnRadioOn", "Включить радио");
			if (showPrompt(nullptr, radio, interactAction, label))
			{
				return;
			}
		}
	}

	if (CanRepairCurrentModule())
	{
		UMeshComponent* mesh = FindInteractionMesh(*seatedVehicle, GetRepairMeshName(currentModule));
		if (showPrompt(mesh, lookedAtModule.Get(), repairAction, LOCTEXT("Repair", "Починить")))
		{
			return;
		}
	}
	interactionFeedback->Clear();
}

#undef LOCTEXT_NAMESPACE
