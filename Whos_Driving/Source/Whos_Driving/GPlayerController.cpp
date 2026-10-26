#include "GPlayerController.h"
#include "GPlayerCar.h"
#include "G_Vehicle_Seat_Component.h"
#include "G_Vehicle_Seating_Component.h"
#include "G_Vehicle_Combat_Component.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Whos_DrivingCharacter.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GBoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"

AGPlayerController::AGPlayerController()
{
	bAutoManageActiveCameraTarget = false;
}

void AGPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGPlayerController, driverCharacter);
	DOREPLIFETIME(AGPlayerController, seatedVehicle);
}

void AGPlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	if (ULocalPlayer* localPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* subsystem = localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (vehicleMappingContext)
			{
				subsystem->AddMappingContext(vehicleMappingContext, 0);
			}
			if (mouseLookMappingContext)
			{
				subsystem->AddMappingContext(mouseLookMappingContext, 1);
			}
		}
	}
	RefreshVehicleView();
}

void AGPlayerController::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (HasAuthority() && IsValid(seatedVehicle))
	{
		seatedVehicle->GetSeatingComponent()->RemovePlayer(this, true);
	}
	Super::EndPlay(endPlayReason);
}

void AGPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FName moduleId = NAME_None;
	float currentHpInPercentage = 0.0f;
	EGModuleState moduleState = EGModuleState::Full;
	const bool bFoundModule = TraceVehicleModule(moduleId, currentHpInPercentage, moduleState);
	if (bFoundModule)
	{
		OnModuleInfoUpdated.Broadcast(moduleId, currentHpInPercentage, moduleState);
		currentModule = moduleId;
	}
	if (bIsLookingOnModule != bFoundModule)
	{
		bIsLookingOnModule = bFoundModule;
		OnLookingModuleChanged(bFoundModule);
	}
}

void AGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	UEnhancedInputComponent* enhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (enhancedInput)
	{
		enhancedInput->BindAction(lookAction.Get(), ETriggerEvent::Triggered, this, &AGPlayerController::LookInput);
		enhancedInput->BindAction(repairAction.Get(), ETriggerEvent::Triggered, this, &AGPlayerController::TryToSendDelegate);
	}
	if (InputComponent)
	{
		InputComponent->BindKey(seatInteractionKey, IE_Pressed, this, &AGPlayerController::RequestLookedAtSeat);
		InputComponent->BindKey(vehicleExitKey, IE_Pressed, this, &AGPlayerController::RequestVehicleExit);
		InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AGPlayerController::RequestPrimaryWeapon);
		InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AGPlayerController::RequestSecondaryWeapon);
		InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AGPlayerController::RequestWindowLeft);
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AGPlayerController::RequestWindowRight);
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AGPlayerController::RequestFireStart);
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AGPlayerController::RequestFireStop);
		InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AGPlayerController::RequestAimStart);
		InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AGPlayerController::RequestAimStop);
		InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AGPlayerController::RequestReload);
		InputComponent->BindKey(EKeys::V, IE_Pressed, this, &AGPlayerController::RequestFireMode);
	}
}

void AGPlayerController::LookInput(const FInputActionValue& value)
{
	AWhos_DrivingCharacter* character = seatedVehicle ? driverCharacter.Get() : Cast<AWhos_DrivingCharacter>(GetPawn());
	if (character)
	{
		const FVector2D lookAxis = value.Get<FVector2D>();
		character->DoAim(lookAxis.X, lookAxis.Y);
	}
}

void AGPlayerController::TryToSendDelegate()
{
	if (bIsLookingOnModule)
	{
		TryToRepair.Execute(currentModule);
	}
}

void AGPlayerController::SetDriverCharacter(AWhos_DrivingCharacter* newDriver)
{
	if (HasAuthority())
	{
		driverCharacter = newDriver;
		ForceNetUpdate();
	}

	RefreshVehicleView();
}

void AGPlayerController::SetVehicleContext(AGPlayerCar* vehicle, AWhos_DrivingCharacter* character)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("don't have authority to AGPlayerController::SetVehicleContext"));
		return;
	}
	seatedVehicle = vehicle;
	driverCharacter = character;
	if (IsValid(character))
	{
		character->SetOwner(this);
		character->ForceNetUpdate();
	}else
	{
		UE_LOG(LogTemp, Warning, TEXT("character invalid in AGPlayerController::SetVehicleContext"));
	}

	ForceNetUpdate();
	RefreshVehicleView();
}

AWhos_DrivingCharacter* AGPlayerController::GetDriverCharacter() const
{
	return driverCharacter;
}

AGPlayerCar* AGPlayerController::GetSeatedVehicle() const
{
	return seatedVehicle;
}

void AGPlayerController::OnRep_VehicleContext()
{
	RefreshVehicleView();
}

void AGPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	RefreshVehicleView();
}

void AGPlayerController::RefreshVehicleView()
{
	if (IsLocalController())
	{
		if (ULocalPlayer* localPlayer = GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* input = localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				UInputMappingContext* activeContext = seatedVehicle ? vehicleMappingContext.Get() : walkingMappingContext.Get();
				UInputMappingContext* inactiveContext = seatedVehicle ? walkingMappingContext.Get() : vehicleMappingContext.Get();
				if (inactiveContext)
				{
					input->RemoveMappingContext(inactiveContext);
				}
				if (activeContext)
				{
					input->AddMappingContext(activeContext, 0);
				}
			}
		}
		AActor* target = seatedVehicle && driverCharacter ? static_cast<AActor*>(driverCharacter) : GetPawn();
		if (IsValid(target))
		{
			SetViewTarget(target);
		}
	}
}

void AGPlayerController::RequestLookedAtSeat()
{
	if (!IsLocalController() || !GetWorld())
	{
		return;
	}
	FVector origin;
	FRotator rotation;
	GetPlayerViewPoint(origin, rotation);
	FCollisionQueryParams query(SCENE_QUERY_STAT(VehicleSeatLook), false);
	query.AddIgnoredActor(driverCharacter);
	if (GetPawn() != seatedVehicle)
	{
		query.AddIgnoredActor(GetPawn());
	}
	if (seatedVehicle && driverCharacter)
	{
		UGVehicleSeatingComponent* seating = seatedVehicle->GetSeatingComponent();
		query.AddIgnoredComponent(seating->GetSeatComponent(seating->GetCharacterSeat(driverCharacter)));
	}
	FHitResult hit;
	if (GetWorld()->LineTraceSingleByChannel(hit, origin, origin + rotation.Vector() * 500.0f, ECC_GameTraceChannel1, query))
	{
		UGVehicleSeatComponent* seat = Cast<UGVehicleSeatComponent>(hit.GetComponent());
		AGPlayerCar* vehicle = Cast<AGPlayerCar>(hit.GetActor());
		UE_LOG(LogTemp, Display, TEXT("Seat selection hit: actor=%s component=%s"), *GetNameSafe(hit.GetActor()), *GetNameSafe(hit.GetComponent()));
		if (seat && vehicle)
		{
			ServerRequestVehicleSeat(vehicle, static_cast<int32>(seat->seatId), origin, rotation.Vector());
		}
	}
}

void AGPlayerController::ServerRequestVehicleSeat_Implementation(AGPlayerCar* vehicle, int32 seatIndex,
	FVector_NetQuantize viewOrigin, FVector_NetQuantizeNormal viewDirection)
{
	const float now = GetWorld()->GetTimeSeconds();
	if (now - lastSeatRequestTime < 0.15f || !IsValid(vehicle))
	{
		return;
	}
	lastSeatRequestTime = now;
	const bool bAccepted = vehicle->GetSeatingComponent()->RequestSeat(this, seatIndex, viewOrigin, viewDirection);
	UE_LOG(LogTemp, Display, TEXT("Seat request: authority=%d controller=%s seat=%d accepted=%d origin=%s"),
		HasAuthority(), *GetName(), seatIndex, bAccepted, *viewOrigin.ToString());
}

void AGPlayerController::RequestVehicleExit()
{
	if (IsLocalController())
	{
		ServerRequestVehicleExit();
	}
}

void AGPlayerController::ServerRequestVehicleExit_Implementation()
{
	for (TActorIterator<AGPlayerCar> iterator(GetWorld()); iterator; ++iterator)
	{
		if (iterator->GetSeatingComponent()->CancelTransition(this))
		{
			return;
		}
	}
	if (IsValid(seatedVehicle))
	{
		seatedVehicle->GetSeatingComponent()->RequestExit(this);
	}
}

UGVehicleCombatComponent* AGPlayerController::GetOwnedCombat() const
{
	AWhos_DrivingCharacter* character = seatedVehicle ? driverCharacter.Get() : Cast<AWhos_DrivingCharacter>(GetPawn());
	if (!IsValid(character) || character->GetOwner() != this)
	{
		return nullptr;
	}
	if (seatedVehicle && GetPawn() != seatedVehicle && GetPawn() != character)
	{
		return nullptr;
	}
	return character->GetCombatComponent();
}

void AGPlayerController::RequestPrimaryWeapon()
{
	if (IsLocalController())
	{
		ServerCombatAction(0, FVector::ForwardVector);
	}
}
void AGPlayerController::RequestSecondaryWeapon()
{
	if (IsLocalController())
	{
		ServerCombatAction(1, FVector::ForwardVector);
	}
}
void AGPlayerController::RequestWindowLeft()
{
	if (IsLocalController())
	{
		ServerCombatAction(2, FVector::ForwardVector);
	}
}
void AGPlayerController::RequestWindowRight()
{
	if (IsLocalController())
	{
		ServerCombatAction(3, FVector::ForwardVector);
	}
}
void AGPlayerController::RequestFireStart()
{
	if (IsLocalController())
	{
		if (UGVehicleCombatComponent* combat = GetOwnedCombat())
		{
			// Use the same current camera direction as continuous aim updates.
			// PlayerCameraManager can still contain the preceding frame during input.
			ServerCombatAction(4, combat->GetAimDirection());
		}
	}
}
void AGPlayerController::RequestFireStop()
{
	if (IsLocalController())
	{
		ServerCombatAction(5, FVector::ForwardVector);
	}
}
void AGPlayerController::RequestAimStart()
{
	if (IsLocalController())
	{
		ServerCombatAction(6, FVector::ForwardVector);
	}
}
void AGPlayerController::RequestAimStop()
{
	if (IsLocalController())
	{
		ServerCombatAction(7, FVector::ForwardVector);
	}
}
void AGPlayerController::RequestReload()
{
	if (IsLocalController())
	{
		ServerCombatAction(8, FVector::ForwardVector);
	}
}
void AGPlayerController::RequestFireMode()
{
	if (IsLocalController())
	{
		ServerCombatAction(9, FVector::ForwardVector);
	}
}

void AGPlayerController::ServerCombatAction_Implementation(uint8 action, FVector_NetQuantizeNormal direction)
{
	UGVehicleCombatComponent* combat = GetOwnedCombat();
	if (!combat)
	{
		return;
	}
	const int32 seat = combat->GetSeatIndex();
	const bool bLeftSeat = seat == 0 || seat == 2;
	switch (action)
	{
	case 0:
		combat->SelectWeapon(EGWeaponSlot::Primary);
		break;
	case 1:
		combat->SelectWeapon(EGWeaponSlot::Secondary);
		break;
	case 2:
		if (seat != INDEX_NONE)
		{
			combat->RequestWindow(bLeftSeat);
		}
		break;
	case 3:
		if (seat != INDEX_NONE)
		{
			combat->RequestWindow(!bLeftSeat);
		}
		break;
	case 4:
		combat->SetTrigger(true, direction);
		break;
	case 5:
		combat->SetTrigger(false, direction);
		break;
	case 6:
		combat->SetAiming(true);
		break;
	case 7:
		combat->SetAiming(false);
		break;
	case 8:
		combat->RequestReload();
		break;
	case 9:
		combat->ChangeFireMode();
		break;
	default:
		break;
	}
}

void AGPlayerController::SendCombatView(const FVector& direction)
{
	if (IsLocalController() && GetWorld() && GetWorld()->GetTimeSeconds() - lastCombatViewTime >= 0.05f)
	{
		lastCombatViewTime = GetWorld()->GetTimeSeconds();
		ServerCombatView(direction);
	}
}

void AGPlayerController::ServerCombatView_Implementation(FVector_NetQuantizeNormal direction)
{
	if (UGVehicleCombatComponent* combat = GetOwnedCombat())
	{
		combat->UpdateView(direction);
	}
}


void AGPlayerController::Client_SetupLobbyInput_Implementation()
{
	//FInputModeGameAndUI InputMode;
	FInputModeGameOnly InputMode;
	this->bShowMouseCursor = false;
	this->SetInputMode(InputMode);
}

bool AGPlayerController::TraceVehicleModule(FName& outModuleId, float& outCurrentHPInPercentage, EGModuleState& outModuleState)
{
	outModuleId = NAME_None;
	outCurrentHPInPercentage = 0.0f;

	if (!GetWorld())
	{
		return false;
	}

	FVector origin;
	FRotator rotation;
	GetPlayerViewPoint(origin, rotation);

	const FVector end = origin + rotation.Vector() * 3000.0f;

	FCollisionQueryParams query(
		SCENE_QUERY_STAT(VehicleModuleLook),
		false);

	if (IsValid(driverCharacter))
	{
		query.AddIgnoredActor(driverCharacter);
	}

	if (IsValid(seatedVehicle))
	{
		if (USkeletalMeshComponent* vehicleMesh = seatedVehicle->GetMesh())
		{
			query.AddIgnoredComponent(vehicleMesh);
		}
	}

	FHitResult hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(hit, origin, end, ECC_GameTraceChannel3, query);

	if (!bHit)
	{
		return false;
	}

	const UGBoxComponent* hitBox = Cast<UGBoxComponent>(hit.GetComponent());

	if (!IsValid(hitBox) || hitBox->moduleId.IsNone())
	{
		return false;
	}

	AGPlayerCar* vehicle = Cast<AGPlayerCar>(hitBox->GetOwner());

	if (!IsValid(vehicle))
	{
		return false;
	}

	const FGVehicleModuleState state =
		vehicle->GetModuleDamageState(hitBox->moduleId);

	if (state.moduleId.IsNone())
	{
		return false;
	}

	outModuleId = state.moduleId;
	outCurrentHPInPercentage = state.currentHP / state.maxHP * 100.0f;
	outModuleState = state.moduleState;

	return true;
}