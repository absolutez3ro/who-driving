#include "G_Vehicle_Seating_Component.h"
#include "G_Vehicle_Seat_Component.h"
#include "GPlayerCar.h"
#include "GPlayerController.h"
#include "Whos_DrivingCharacter.h"
#include "GVehicleMovementComponent.h"
#include "G_Vehicle_Combat_Component.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

UGVehicleSeatingComponent::UGVehicleSeatingComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	seatStates.SetNum(4);
	occupantControllers.SetNum(4);
	reservationControllers.SetNum(4);
}

void UGVehicleSeatingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGVehicleSeatingComponent, seatStates);
}

AGPlayerCar* UGVehicleSeatingComponent::GetVehicle() const
{
	return Cast<AGPlayerCar>(GetOwner());
}

bool UGVehicleSeatingComponent::IsAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}

void UGVehicleSeatingComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplySeatStates();
}

void UGVehicleSeatingComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (IsAuthority() && endPlayReason == EEndPlayReason::Destroyed)
	{
		for (int32 seatIndex = 0; seatIndex < seatStates.Num(); ++seatIndex)
		{
			AGPlayerController* controller = occupantControllers[seatIndex].Get();
			AWhos_DrivingCharacter* character = seatStates[seatIndex].occupant;
			if (IsValid(controller) && IsValid(character))
			{
				controller->Possess(character);
				controller->SetVehicleContext(nullptr, character);
			}
			seatStates[seatIndex] = FGVehicleSeatState();
		}
	}
	TArray<TWeakObjectPtr<AWhos_DrivingCharacter>> characters;
	characterRestoreStates.GetKeys(characters);
	for (const TWeakObjectPtr<AWhos_DrivingCharacter>& character : characters)
	{
		RestoreCharacter(character.Get());
	}
	Super::EndPlay(endPlayReason);
}

FGVehicleSeatState UGVehicleSeatingComponent::GetSeatState(int32 seatIndex) const
{
	return seatStates.IsValidIndex(seatIndex) ? seatStates[seatIndex] : FGVehicleSeatState();
}

UGVehicleSeatComponent* UGVehicleSeatingComponent::GetSeatComponent(int32 seatIndex) const
{
	if (!seatStates.IsValidIndex(seatIndex) || !GetOwner())
	{
		return nullptr;
	}
	TInlineComponentArray<UGVehicleSeatComponent*> seats;
	GetOwner()->GetComponents(seats);
	for (UGVehicleSeatComponent* seat : seats)
	{
		if (static_cast<int32>(seat->seatId) == seatIndex)
		{
			return seat;
		}
	}
	return nullptr;
}

int32 UGVehicleSeatingComponent::GetCharacterSeat(AWhos_DrivingCharacter* character) const
{
	if (!character)
	{
		return INDEX_NONE;
	}
	for (int32 seatIndex = 0; seatIndex < seatStates.Num(); ++seatIndex)
	{
		if (seatStates[seatIndex].occupant == character)
		{
			return seatIndex;
		}
	}
	return INDEX_NONE;
}

bool UGVehicleSeatingComponent::IsPlayerValid(AGPlayerController* controller) const
{
	if (!IsValid(controller) || !controller->GetPlayerState<APlayerState>())
	{
		return false;
	}
	AWhos_DrivingCharacter* character = controller->GetDriverCharacter();
	if (controller->GetSeatedVehicle() == GetVehicle())
	{
		const int32 currentSeat = GetCharacterSeat(character);
		return currentSeat != INDEX_NONE && occupantControllers[currentSeat] == controller
			&& controller->GetPawn() == (currentSeat == 0 ? static_cast<APawn*>(GetVehicle()) : character);
	}
	return controller->GetSeatedVehicle() == nullptr
		&& Cast<AWhos_DrivingCharacter>(controller->GetPawn()) != nullptr;
}

bool UGVehicleSeatingComponent::IsSeatTargetValid(AGPlayerController* controller, int32 seatIndex,
	const FVector& origin, const FVector& direction) const
{
	UGVehicleSeatComponent* targetSeat = GetSeatComponent(seatIndex);
	AWhos_DrivingCharacter* character = controller->GetSeatedVehicle()
		? controller->GetDriverCharacter() : Cast<AWhos_DrivingCharacter>(controller->GetPawn());
	if (!targetSeat || !IsValid(character) || origin.ContainsNaN() || direction.ContainsNaN()
		|| !direction.IsNormalized() || FVector::DistSquared(origin, character->GetActorLocation()) > FMath::Square(150.0f))
	{
		UE_LOG(LogTemp, Display, TEXT("Seat request invalid view or avatar: normalized=%d distance=%f"), direction.IsNormalized(),
			character ? FVector::Dist(origin, character->GetActorLocation()) : -1.0f);
		return false;
	}
	FHitResult selection;
	FCollisionQueryParams selectionParams(SCENE_QUERY_STAT(VehicleSeatSelection), false, character);
	if (controller->GetSeatedVehicle() == GetVehicle())
	{
		selectionParams.AddIgnoredComponent(GetSeatComponent(GetCharacterSeat(character)));
	}
	const FVector end = origin + direction * interactionDistance;
	if (!GetWorld()->LineTraceSingleByChannel(selection, origin, end, ECC_GameTraceChannel1, selectionParams)
		|| selection.GetComponent() != targetSeat)
	{
		UE_LOG(LogTemp, Display, TEXT("Seat request trace mismatch: hit=%s target=%s"), *GetNameSafe(selection.GetComponent()), *GetNameSafe(targetSeat));
		return false;
	}
	FCollisionQueryParams visibilityParams(SCENE_QUERY_STAT(VehicleSeatVisibility), false, character);
	if (controller->GetSeatedVehicle() == GetVehicle())
	{
		visibilityParams.AddIgnoredActor(GetOwner());
	}
	FHitResult obstruction;
	const bool bObstructed = GetWorld()->LineTraceSingleByChannel(obstruction, origin, selection.ImpactPoint, ECC_Visibility, visibilityParams);
	if (bObstructed)
	{
		UE_LOG(LogTemp, Display, TEXT("Seat request obstructed: actor=%s component=%s"), *GetNameSafe(obstruction.GetActor()), *GetNameSafe(obstruction.GetComponent()));
	}
	return !bObstructed;
}




bool UGVehicleSeatingComponent::RequestSeat(AGPlayerController* controller, int32 seatIndex,
	const FVector& viewOrigin, const FVector& viewDirection)
{
	if (!IsAuthority() || !IsPlayerValid(controller) || !seatStates.IsValidIndex(seatIndex)
		|| !GetSeatComponent(seatIndex) || seatStates[seatIndex].occupant || seatStates[seatIndex].reservedBy)
	{
		UE_LOG(LogTemp, Display, TEXT("Seat request invalid player or unavailable slot: authority=%d validPlayer=%d index=%d"), IsAuthority(), IsPlayerValid(controller), seatIndex);
		return false;
	}
	APlayerState* playerState = controller->GetPlayerState<APlayerState>();
	for (const FGVehicleSeatState& state : seatStates)
	{
		if (state.reservedBy == playerState)
		{
			return false;
		}
	}
	if (!IsSeatTargetValid(controller, seatIndex, viewOrigin, viewDirection))
	{
		return false;
	}
	seatStates[seatIndex].reservedBy = playerState;
	AWhos_DrivingCharacter* character = controller->GetSeatedVehicle() ? controller->GetDriverCharacter() : Cast<AWhos_DrivingCharacter>(controller->GetPawn());
	if (character) { character->GetCombatComponent()->CancelActions(true); }
	seatStates[seatIndex].transitionEndTime = GetWorld()->GetTimeSeconds() + FMath::Max(transitionDuration, 0.1f);
	reservationControllers[seatIndex] = controller;
	PublishSeatStates();
	return true;
}

void UGVehicleSeatingComponent::ClearReservation(int32 seatIndex)
{
	seatStates[seatIndex].reservedBy = nullptr;
	seatStates[seatIndex].transitionEndTime = 0.0f;
	reservationControllers[seatIndex].Reset();
}

void UGVehicleSeatingComponent::CompleteTransition(int32 seatIndex)
{
	AGPlayerController* controller = reservationControllers[seatIndex].Get();
	if (!IsPlayerValid(controller) || seatStates[seatIndex].occupant
		|| seatStates[seatIndex].reservedBy != controller->GetPlayerState<APlayerState>())
	{
		ClearReservation(seatIndex);
		PublishSeatStates();
		return;
	}
	AWhos_DrivingCharacter* character = controller->GetSeatedVehicle()
		? controller->GetDriverCharacter() : Cast<AWhos_DrivingCharacter>(controller->GetPawn());
	UGVehicleSeatComponent* targetSeat = GetSeatComponent(seatIndex);
	if (!IsValid(character) || !targetSeat || (controller->GetSeatedVehicle() == nullptr
		&& FVector::DistSquared(character->GetActorLocation(), targetSeat->GetComponentLocation()) > FMath::Square(interactionDistance)))
	{
		ClearReservation(seatIndex);
		PublishSeatStates();
		return;
	}
	const int32 previousSeat = GetCharacterSeat(character);
	if (previousSeat != INDEX_NONE)
	{
		seatStates[previousSeat] = FGVehicleSeatState();
		occupantControllers[previousSeat].Reset();
		if (previousSeat == 0)
		{
			StopVehicleControls();
		}
	}
	FGVehicleSeatState& state = seatStates[seatIndex];
	state.occupant = character;
	state.playerState = controller->GetPlayerState<APlayerState>();
	ClearReservation(seatIndex);
	occupantControllers[seatIndex] = controller;
	ApplySeatStates();
	controller->Possess(seatIndex == 0 ? static_cast<APawn*>(GetVehicle()) : character);
	controller->SetVehicleContext(GetVehicle(), character);
	PublishSeatStates();
}

bool UGVehicleSeatingComponent::InitializeDriver(AGPlayerController* controller, AWhos_DrivingCharacter* character, int32 seatIndex)
{
	return InitializeOccupant(controller, character, seatIndex);
}

bool UGVehicleSeatingComponent::InitializeOccupant(AGPlayerController* controller, AWhos_DrivingCharacter* character, int32 seatIndex)
{
	if (!IsAuthority() || !IsValid(controller) || !IsValid(character) || !seatStates.IsValidIndex(seatIndex)
		|| !GetSeatComponent(seatIndex) || seatStates[seatIndex].occupant || seatStates[seatIndex].reservedBy)
	{
		UE_LOG(LogTemp, Error, TEXT("occupant init problem: authority=%d validCharacter=%d validIndex=%d validSeatComponent=%d SeatOccupied=%d SeatReserved=%d Seatindex=%d"),
			IsAuthority(), IsValid(character), seatStates.IsValidIndex(seatIndex), IsValid(GetSeatComponent(seatIndex)), IsValid(seatStates[seatIndex].occupant), IsValid(seatStates[seatIndex].reservedBy), seatIndex) ;
		return false;
	}
	seatStates[seatIndex].occupant = character;
	seatStates[seatIndex].playerState = controller->GetPlayerState<APlayerState>();
	occupantControllers[seatIndex] = controller;
	ApplySeatStates();
	UE_LOG(LogTemp, Log, TEXT("POSESSING"));
	controller->Possess(seatIndex == 0 ? static_cast<APawn*>(GetVehicle()) : character);
	controller->SetVehicleContext(GetVehicle(), character);
	PublishSeatStates();
	return true;
}

void UGVehicleSeatingComponent::StopVehicleControls()
{
	if (AGPlayerCar* vehicle = GetVehicle())
	{
		if (UGVehicleMovementComponent* movement = Cast<UGVehicleMovementComponent>(vehicle->GetVehicleMovementComponent()))
		{
			movement->StopDrivingInput();
		}
	}
}

bool UGVehicleSeatingComponent::CancelTransition(AGPlayerController* controller)
{
	if (!IsAuthority() || !IsValid(controller))
	{
		return false;
	}
	bool bCancelled = false;
	for (int32 seatIndex = 0; seatIndex < seatStates.Num(); ++seatIndex)
	{
		if (reservationControllers[seatIndex] == controller)
		{
			ClearReservation(seatIndex);
			bCancelled = true;
		}
	}
	if (bCancelled)
	{
		PublishSeatStates();
	}
	return bCancelled;
}

bool UGVehicleSeatingComponent::RequestExit(AGPlayerController* controller)
{
	if (!IsAuthority() || !IsPlayerValid(controller) || controller->GetSeatedVehicle() != GetVehicle())
	{
		UE_LOG(LogTemp, Warning, TEXT("Vehicle exit rejected: controller=%s avatar=%s pawn=%s seat=%d valid=%d"),
			*GetNameSafe(controller), *GetNameSafe(controller ? controller->GetDriverCharacter() : nullptr),
			*GetNameSafe(controller ? controller->GetPawn() : nullptr),
			GetCharacterSeat(controller ? controller->GetDriverCharacter() : nullptr), IsPlayerValid(controller));
		return false;
	}
	AWhos_DrivingCharacter* character = controller->GetDriverCharacter();
	const int32 currentSeat = GetCharacterSeat(character);
	UGVehicleSeatComponent* seat = GetSeatComponent(currentSeat);
	if (!seat)
	{
		return false;
	}
	const FVector exitPosition = GetVehicle()->GetMesh()->GetComponentTransform().TransformPosition(seat->exitLocation);
	FCollisionQueryParams query(SCENE_QUERY_STAT(VehicleSeatExit), false, character);
	query.AddIgnoredActor(GetOwner());
	const UCapsuleComponent* capsule = character->GetCapsuleComponent();
	const FCollisionShape shape = FCollisionShape::MakeCapsule(capsule->GetScaledCapsuleRadius(), capsule->GetScaledCapsuleHalfHeight());
	if (GetWorld()->OverlapBlockingTestByChannel(exitPosition, FQuat::Identity, ECC_Pawn, shape, query))
	{
		UE_LOG(LogTemp, Warning, TEXT("Vehicle exit blocked: position=%s radius=%f halfHeight=%f"),
			*exitPosition.ToString(), capsule->GetScaledCapsuleRadius(), capsule->GetScaledCapsuleHalfHeight());
		return false;
	}
	for (int32 seatIndex = 0; seatIndex < seatStates.Num(); ++seatIndex)
	{
		if (reservationControllers[seatIndex] == controller)
		{
			ClearReservation(seatIndex);
		}
	}
	seatStates[currentSeat] = FGVehicleSeatState();
	occupantControllers[currentSeat].Reset();
	if (currentSeat == 0)
	{
		StopVehicleControls();
	}
	RestoreCharacter(character);
	character->SetActorLocationAndRotation(exitPosition, GetOwner()->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	controller->Possess(character);
	controller->SetVehicleContext(nullptr, character);
	PublishSeatStates();
	return true;
}

void UGVehicleSeatingComponent::RemovePlayer(AGPlayerController* controller, bool bDestroyCharacter)
{
	if (!IsAuthority())
	{
		return;
	}
	for (int32 seatIndex = 0; seatIndex < seatStates.Num(); ++seatIndex)
	{
		if (reservationControllers[seatIndex] == controller)
		{
			ClearReservation(seatIndex);
		}
		if (occupantControllers[seatIndex] == controller)
		{
			AWhos_DrivingCharacter* character = seatStates[seatIndex].occupant;
			seatStates[seatIndex] = FGVehicleSeatState();
			occupantControllers[seatIndex].Reset();
			RestoreCharacter(character);
			if (seatIndex == 0)
			{
				StopVehicleControls();
			}
			if (bDestroyCharacter && IsValid(character))
			{
				character->Destroy();
			}
		}
	}
	PublishSeatStates();
}

void UGVehicleSeatingComponent::TickComponent(float deltaSeconds, ELevelTick tickType, FActorComponentTickFunction* tickFunction)
{
	Super::TickComponent(deltaSeconds, tickType, tickFunction);
	if (!IsAuthority())
	{
		return;
	}
	for (int32 seatIndex = 0; seatIndex < seatStates.Num(); ++seatIndex)
	{
		FGVehicleSeatState& state = seatStates[seatIndex];
		if (state.occupant && (!occupantControllers[seatIndex].IsValid() || !IsValid(state.occupant)))
		{
			AWhos_DrivingCharacter* character = state.occupant;
			state = FGVehicleSeatState();
			occupantControllers[seatIndex].Reset();
			RestoreCharacter(character);
			if (IsValid(character))
			{
				character->Destroy();
			}
			if (seatIndex == 0)
			{
				StopVehicleControls();
			}
			PublishSeatStates();
		}
		if (state.reservedBy && (!reservationControllers[seatIndex].IsValid()
			|| GetWorld()->GetTimeSeconds() >= state.transitionEndTime))
		{
			CompleteTransition(seatIndex);
		}
	}
}

void UGVehicleSeatingComponent::PublishSeatStates()
{
	ApplySeatStates();
	GetOwner()->ForceNetUpdate();
}

void UGVehicleSeatingComponent::OnRep_SeatStates()
{
	ApplySeatStates();
}

void UGVehicleSeatingComponent::RestoreCharacter(AWhos_DrivingCharacter* character)
{
	if (!character)
	{
		return;
	}
	FGVehicleCharacterRestore saved;
	if (!characterRestoreStates.RemoveAndCopyValue(character, saved))
	{
		return;
	}
	if (character->GetAttachParentActor() == GetOwner())
	{
		character->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	character->bUseControllerRotationYaw = saved.bUseControllerRotationYaw;
	character->bUseControllerRotationPitch = saved.bUseControllerRotationPitch;
	character->bUseControllerRotationRoll = saved.bUseControllerRotationRoll;
	character->GetCapsuleComponent()->SetCollisionEnabled(saved.capsuleCollision);
	character->GetMesh()->SetCollisionEnabled(saved.meshCollision);
	character->GetCharacterMovement()->GravityScale = saved.gravityScale;
	const EMovementMode movementMode = !IsAuthority() && saved.movementMode == MOVE_None
		? MOVE_Walking : static_cast<EMovementMode>(saved.movementMode);
	character->GetCharacterMovement()->SetMovementMode(movementMode, saved.customMovementMode);
	character->ResetVehicleLook();
	character->GetCombatComponent()->SeatChanged();
}

void UGVehicleSeatingComponent::ApplySeatStates()
{
	TArray<TWeakObjectPtr<AWhos_DrivingCharacter>> previousCharacters;
	characterRestoreStates.GetKeys(previousCharacters);
	for (const TWeakObjectPtr<AWhos_DrivingCharacter>& character : previousCharacters)
	{
		if (!character.IsValid())
		{
			characterRestoreStates.Remove(character);
		}
		else if (GetCharacterSeat(character.Get()) == INDEX_NONE)
		{
			RestoreCharacter(character.Get());
		}
	}
	for (int32 seatIndex = 0; seatIndex < seatStates.Num(); ++seatIndex)
	{
		AWhos_DrivingCharacter* character = seatStates[seatIndex].occupant;
		UGVehicleSeatComponent* seat = GetSeatComponent(seatIndex);
		if (!IsValid(character) || !seat)
		{
			continue;
		}
		UCharacterMovementComponent* movement = character->GetCharacterMovement();
		if (!characterRestoreStates.Contains(character))
		{
			FGVehicleCharacterRestore saved;
			saved.bUseControllerRotationYaw = character->bUseControllerRotationYaw;
			saved.bUseControllerRotationPitch = character->bUseControllerRotationPitch;
			saved.bUseControllerRotationRoll = character->bUseControllerRotationRoll;
			saved.movementMode = movement->MovementMode;
			saved.customMovementMode = movement->CustomMovementMode;
			saved.gravityScale = movement->GravityScale;
			saved.capsuleCollision = character->GetCapsuleComponent()->GetCollisionEnabled();
			saved.meshCollision = character->GetMesh()->GetCollisionEnabled();
			characterRestoreStates.Add(character, saved);
		}
		character->bUseControllerRotationYaw = false;
		character->bUseControllerRotationPitch = false;
		character->bUseControllerRotationRoll = false;
		movement->StopMovementImmediately();
		movement->DisableMovement();
		movement->GravityScale = 0.0f;
		character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		character->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (character->GetRootComponent()->GetAttachParent() != seat)
		{
			character->AttachToComponent(seat, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			character->GetRootComponent()->SetRelativeTransform(seat->characterOffset);
			character->ResetVehicleLook();
			character->GetCombatComponent()->SeatChanged();
		}
	}
}
