#include "GVehicleCombatComponent.h"
#include "GVehicleWeapon.h"
#include "GVehicleSeatComponent.h"
#include "GVehicleSeatingComponent.h"
#include "GVehicleMovementComponent.h"
#include "GPlayerCar.h"
#include "GPlayerController.h"
#include "GWhosDrivingCharacter.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogVehicleCombat, Log, All);

UGVehicleCombatComponent::UGVehicleCombatComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
}

void UGVehicleCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGVehicleCombatComponent, weapons);
	DOREPLIFETIME(UGVehicleCombatComponent, selectedSlot);
	DOREPLIFETIME(UGVehicleCombatComponent, windowState);
	DOREPLIFETIME(UGVehicleCombatComponent, aimDirection);
	DOREPLIFETIME(UGVehicleCombatComponent, bAiming);
	DOREPLIFETIME(UGVehicleCombatComponent, health);
	DOREPLIFETIME(UGVehicleCombatComponent, equipStartTime);
	DOREPLIFETIME(UGVehicleCombatComponent, confirmedShots);
}

AGWhosDrivingCharacter* UGVehicleCombatComponent::GetCharacter() const
{
	return Cast<AGWhosDrivingCharacter>(GetOwner());
}

AGPlayerController* UGVehicleCombatComponent::GetPlayerController() const
{
	if (!GetCharacter())
	{
		return nullptr;
	}
	AGPlayerController* controller = Cast<AGPlayerController>(GetCharacter()->GetOwner());
	return controller ? controller : Cast<AGPlayerController>(GetCharacter()->GetController());
}

bool UGVehicleCombatComponent::IsAuthority() const
{
	return GetOwner() && GetOwner()->HasAuthority();
}

bool UGVehicleCombatComponent::IsLocallyOwned() const
{
	const AGPlayerController* controller = GetPlayerController();
	return controller && controller->IsLocalController();
}

bool UGVehicleCombatComponent::IsPlayerValid() const
{
	AGPlayerController* controller = GetPlayerController();
	AGPlayerCar* vehicle = GetVehicle();
	if (!controller || health <= 0.0f || !vehicle || controller->GetSeatedVehicle() != vehicle ||
		controller->GetDriverCharacter() != GetCharacter() || GetSeatIndex() == INDEX_NONE)
	{
		return false;
	}
	for (int32 seat = 0; seat < 4; ++seat)
	{
		if (vehicle->GetSeatingComponent()->GetSeatState(seat).reservedBy == controller->PlayerState)
		{
			return false;
		}
	}
	return true;
}

AGPlayerCar* UGVehicleCombatComponent::GetVehicle() const
{
	return GetCharacter() ? Cast<AGPlayerCar>(GetCharacter()->GetAttachParentActor()) : nullptr;
}

int32 UGVehicleCombatComponent::GetSeatIndex() const
{
	AGPlayerCar* vehicle = GetVehicle();
	return vehicle ? vehicle->GetSeatingComponent()->GetCharacterSeat(GetCharacter()) : INDEX_NONE;
}

float UGVehicleCombatComponent::GetServerTime() const
{
	const AGameStateBase* state = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return state ? state->GetServerWorldTimeSeconds() : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);
}

void UGVehicleCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetCharacter())
	{
		return;
	}
	initialCameraLocation = GetCharacter()->GetCameraRoot()->GetRelativeLocation();
	initialFirstPersonLocation = GetCharacter()->GetFirstPersonMesh()->GetRelativeLocation();
	initialCameraFov = GetCharacter()->GetFirstPersonCameraComponent()->FieldOfView;
	GetCharacter()->GetFirstPersonMesh()->HideBoneByName(TEXT("head"), EPhysBodyOp::PBO_None);
	if (IsAuthority())
	{
		health = maxHealth;
		SpawnLoadout();
	}
}

void UGVehicleCombatComponent::SpawnLoadout()
{
	for (UGWeaponDefinition* definition : {primaryDefinition.Get(), secondaryDefinition.Get()})
	{
		if (!definition)
		{
			continue;
		}
		UClass* weaponClass = definition->weaponClass ? definition->weaponClass.Get() : AGVehicleWeapon::StaticClass();
		AGVehicleWeapon* weapon = GetWorld()->SpawnActorDeferred<AGVehicleWeapon>(
			weaponClass, GetOwner()->GetActorTransform(), GetOwner(), GetCharacter(),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!weapon)
		{
			continue;
		}
		weapon->Initialize(definition);
		UGameplayStatics::FinishSpawningActor(weapon, GetOwner()->GetActorTransform());
		weapons.Add(weapon);
	}
	OnRep_Loadout();
}

void UGVehicleCombatComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(fireTimer);
	}
	if (IsAuthority())
	{
		for (AGVehicleWeapon* weapon : weapons)
		{
			if (IsValid(weapon))
			{
				weapon->Destroy();
			}
		}
	}
	Super::EndPlay(endPlayReason);
}

AGVehicleWeapon* UGVehicleCombatComponent::GetWeapon(EGWeaponSlot slot) const
{
	for (AGVehicleWeapon* weapon : weapons)
	{
		if (IsValid(weapon) && weapon->GetDefinition() && weapon->GetDefinition()->slot == slot)
		{
			return weapon;
		}
	}
	return nullptr;
}

AGVehicleWeapon* UGVehicleCombatComponent::GetCurrentWeapon() const
{
	return GetWeapon(selectedSlot);
}

float UGVehicleCombatComponent::GetWindowAlpha() const
{
	const float t =
		FMath::Clamp((GetServerTime() - windowState.startTime) / FMath::Max(0.1f, windowState.duration), 0.0f, 1.0f);
	return FMath::Lerp(windowState.startAlpha, windowState.bOutside ? 1.0f : 0.0f, FMath::SmoothStep(0.0f, 1.0f, t));
}

bool UGVehicleCombatComponent::IsDrivingLocked() const
{
	return GetSeatIndex() == 0 && (windowState.bOutside || GetWindowAlpha() > 0.01f);
}

bool UGVehicleCombatComponent::RequestWindow(bool bOutside)
{
	if (!IsAuthority() || !IsPlayerValid() || windowState.bOutside == bOutside ||
		GetServerTime() - lastWindowRequest < 0.1f)
	{
		return false;
	}
	CancelActions(false);
	lastWindowRequest = GetServerTime();
	windowState.startAlpha = GetWindowAlpha();
	windowState.startTime = GetServerTime();
	windowState.bOutside = bOutside;
	windowState.duration = GetVehicle()->GetSeatingComponent()->GetSeatComponent(GetSeatIndex())->windowDuration;
	if (!bOutside && GetSeatIndex() == 0)
	{
		SelectWeapon(EGWeaponSlot::Secondary);
	}
	RefreshDriverLock();
	GetOwner()->ForceNetUpdate();
	return true;
}

bool UGVehicleCombatComponent::SelectWeapon(EGWeaponSlot slot)
{
	if (!IsAuthority() || health <= 0.0f || !GetWeapon(slot))
	{
		return false;
	}
	if (GetSeatIndex() == 0 && !windowState.bOutside && slot == EGWeaponSlot::Primary)
	{
		return false;
	}
	if (slot == selectedSlot)
	{
		return true;
	}
	CancelActions(false);
	selectedSlot = slot;
	equipStartTime = GetServerTime();
	OnRep_Loadout();
	GetOwner()->ForceNetUpdate();
	return true;
}

void UGVehicleCombatComponent::OnRep_Loadout()
{
	if (IsAuthority())
	{
		for (AGVehicleWeapon* weapon : weapons)
		{
			if (weapon && weapon->GetDefinition())
			{
				weapon->SetEquipped(weapon->GetDefinition()->slot == selectedSlot && GetSeatIndex() != INDEX_NONE &&
									health > 0.0f);
			}
		}
	}
	if (AGVehicleWeapon* weapon = GetCurrentWeapon())
	{
		weapon->PlayEquip();
	}
}

void UGVehicleCombatComponent::OnRep_Window()
{
	RefreshPresentation();
}

void UGVehicleCombatComponent::OnRep_Health()
{
	RefreshPresentation();
}

void UGVehicleCombatComponent::CancelActions(bool bResetWindow)
{
	if (!IsAuthority())
	{
		return;
	}
	bTriggerHeld = false;
	burstRemaining = 0;
	bAiming = false;
	GetWorld()->GetTimerManager().ClearTimer(fireTimer);
	for (AGVehicleWeapon* weapon : weapons)
	{
		if (weapon)
		{
			weapon->CancelReload();
		}
	}
	if (bResetWindow)
	{
		windowState = FGVehicleWindowState();
		windowState.startTime = GetServerTime();
	}
	RefreshDriverLock();
	GetOwner()->ForceNetUpdate();
}

void UGVehicleCombatComponent::SeatChanged()
{
	if (IsAuthority())
	{
		CancelActions(true);
		if (GetSeatIndex() == 0)
		{
			selectedSlot = EGWeaponSlot::Secondary;
		}
		OnRep_Loadout();
	}
	RefreshPresentation();
}

bool UGVehicleCombatComponent::CanFire() const
{
	const AGVehicleWeapon* weapon = GetCurrentWeapon();
	if (!IsPlayerValid() || !weapon || !weapon->GetDefinition() || !weapon->IsEquipped() || weapon->IsReloading() ||
		weapon->GetBulletCount() <= 0 || GetServerTime() - equipStartTime < weapon->GetDefinition()->equipTime)
	{
		//UE_LOG(LogTemp, Warning, TEXT("cant fire IsPlayerValid %d weapon %d weapon->GetDefinition() %d weapon->IsEquipped() %d weapon->IsReloading() %d weapon->GetBulletCount() %d GetServerTime() - equipStartTime < weapon->GetDefinition()->equipTime %d"),
		//	IsPlayerValid(), weapon, weapon->GetDefinition(), weapon->IsEquipped(), weapon->IsReloading(), weapon->GetBulletCount(), GetServerTime() - equipStartTime < weapon->GetDefinition()->equipTime);
		return false;
	}
	return true;
}

void UGVehicleCombatComponent::UpdateView(const FVector& direction)
{
	if (!IsAuthority() || direction.ContainsNaN() || !direction.IsNormalized())
	{
		return;
	}
	// The client supplies a direction, never an origin, hit actor, damage or ammunition.
	if (FMath::Abs(direction.Z) > 0.99f)
	{
		return;
	}
	aimDirection = direction;
	lastViewUpdate = GetServerTime();
}

FVector UGVehicleCombatComponent::GetAimDirection() const
{
	return IsLocallyOwned() && GetCharacter() ? GetCharacter()->GetFirstPersonCameraComponent()->GetForwardVector()
											  : FVector(aimDirection);
}

bool UGVehicleCombatComponent::SetTrigger(bool bPressed, const FVector& direction)
{
	if (!IsAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("No authority to UGVehicleCombatComponent::SetTrigger"));
		return false;
	}
	if (!bPressed)
	{
		bTriggerHeld = false;
		burstRemaining = 0;
		GetWorld()->GetTimerManager().ClearTimer(fireTimer);
		return true;
	}
	UpdateView(direction);
	if (direction.ContainsNaN() || !direction.IsNormalized() || FMath::Abs(direction.Z) > 0.99f)
	{
		UE_LOG(LogTemp, Warning, TEXT("shoot direction error direction.ContainsNaN() %d direction.IsNormalized() %d FMath::Abs(direction.Z)>0.99f %d"),
			direction.ContainsNaN(), direction.IsNormalized(), FMath::Abs(direction.Z) > 0.99f);
		return false;
	}
	if (bTriggerHeld || !CanFire())
	{
		UE_LOG(LogTemp, Warning, TEXT("trigger isn't pressed or can't fire bTriggerHeld %d CanFire() %d"),
			bTriggerHeld, CanFire());
		return false;
	}
	AGVehicleWeapon* weapon = GetCurrentWeapon();
	if (GetServerTime() + 0.002f < weapon->GetNextShotTime())
	{
		UE_LOG(LogTemp, Warning, TEXT("server time bs while shooting GetServerTime() + 0.002f < weapon->GetNextShotTime() %d"),
			GetServerTime() + 0.002f < weapon->GetNextShotTime());
		return false;
	}
	bTriggerHeld = true;
	burstRemaining =
		weapon->GetFireMode() == EGWeaponFireMode::Burst ? FMath::Clamp(weapon->GetDefinition()->burstSize, 1, 10) : 0;
	TryFire();
	return true;
}

void UGVehicleCombatComponent::TryFire()
{
	if (!IsAuthority() || !bTriggerHeld || !CanFire() || GetServerTime() - lastViewUpdate > 0.5f)
	{
		UE_LOG(LogTemp, Warning, TEXT("try fire error"));
		bTriggerHeld = false;
		return;
	}
	AGVehicleWeapon* weapon = GetCurrentWeapon();
	UGWeaponDefinition* definition = weapon->GetDefinition();
	if (GetServerTime() + 0.002f < weapon->GetNextShotTime() || !weapon->ConsumeRound())
	{
		return;
	}
	const float interval = FMath::Max(0.03f, definition->fireInterval);
	weapon->SetNextShotTime(GetServerTime() + interval);
	FCollisionQueryParams query(SCENE_QUERY_STAT(VehicleWeapon), true, GetCharacter());
	for (AGVehicleWeapon* ownedWeapon : weapons)
	{
		query.AddIgnoredActor(ownedWeapon);
	}
	// Own vehicle and friendly occupants deliberately remain in the query.
	const FVector origin = GetCharacter()->GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector direction = FMath::VRandCone(
		FVector(aimDirection), FMath::DegreesToRadians(definition->spreadDegrees * (bAiming ? 0.25f : 1.0f)));
	FHitResult viewHit;
	const FVector end = origin + direction * FMath::Clamp(definition->range, 1.0f, 100000.0f);
	const bool bViewHit = GetWorld()->LineTraceSingleByChannel(viewHit, origin, end, ECC_GameTraceChannel3, query);
	const FVector target = bViewHit ? viewHit.ImpactPoint : end;
	const FVector muzzle = weapon->GetThirdPersonMesh()->GetSocketLocation(TEXT("MuzzleSocket"));
	FHitResult muzzleHit;
	const bool bMuzzleHit = GetWorld()->LineTraceSingleByChannel(muzzleHit, muzzle, target + direction * 2.0f,
																 ECC_GameTraceChannel3, query);
	const FHitResult& hit = bMuzzleHit ? muzzleHit : viewHit;
	const bool bHit = bMuzzleHit || bViewHit;
	UE_LOG(LogVehicleCombat, Verbose, TEXT("Shot %s camera=%s muzzle=%s aim=%s viewHit=%s:%s finalHit=%s:%s"),
		   *GetNameSafe(GetCharacter()), *origin.ToString(), *muzzle.ToString(), *FVector(aimDirection).ToString(),
		   *GetNameSafe(viewHit.GetActor()), *GetNameSafe(viewHit.GetComponent()), *GetNameSafe(hit.GetActor()),
		   *GetNameSafe(hit.GetComponent()));
	if (bHit && hit.GetActor())
	{
		UGameplayStatics::ApplyPointDamage(hit.GetActor(), FMath::Max(0.0f, definition->damage),
										   (target - muzzle).GetSafeNormal(), hit, GetPlayerController(), weapon,
										   UDamageType::StaticClass());
	}
	++confirmedShots;
	weapon->MulticastShot(bHit ? hit.ImpactPoint : target, bHit);
	GetOwner()->ForceNetUpdate();
	if (weapon->GetFireMode() == EGWeaponFireMode::Auto ||
		(weapon->GetFireMode() == EGWeaponFireMode::Burst && --burstRemaining > 0))
	{
		GetWorld()->GetTimerManager().SetTimer(fireTimer, this, &UGVehicleCombatComponent::TryFire, interval, false);
	}
	else if (weapon->GetFireMode() == EGWeaponFireMode::Burst)
	{
		weapon->SetNextShotTime(GetServerTime() + FMath::Max(interval, definition->burstInterval));
	}
}

bool UGVehicleCombatComponent::SetAiming(bool bNewAiming)
{
	if (!IsAuthority())
	{
		return false;
	}
	bAiming = bNewAiming && CanFire();
	GetOwner()->ForceNetUpdate();
	return bAiming == bNewAiming;
}

bool UGVehicleCombatComponent::RequestReload()
{
	if (!IsAuthority() || !IsPlayerValid() || !GetCurrentWeapon())
	{
		return false;
	}
	CancelActions(false);
	return GetCurrentWeapon()->BeginReload();
}

bool UGVehicleCombatComponent::ChangeFireMode()
{
	if (!IsAuthority() || !IsPlayerValid() || !GetCurrentWeapon())
	{
		return false;
	}
	CancelActions(false);
	return GetCurrentWeapon()->CycleFireMode();
}

void UGVehicleCombatComponent::RefreshDriverLock()
{
	if (!IsAuthority() || GetSeatIndex() != 0)
	{
		return;
	}
	if (UGVehicleMovementComponent* movement =
			Cast<UGVehicleMovementComponent>(GetVehicle()->GetVehicleMovementComponent()))
	{
		movement->SetWindowDrivingLock(IsDrivingLocked());
	}
}

void UGVehicleCombatComponent::RefreshPresentation()
{
	AGWhosDrivingCharacter* character = GetCharacter();
	if (!character)
	{
		return;
	}
	FVector offset = FVector::ZeroVector;
	FVector seatedCameraOffset = FVector::ZeroVector;
	if (AGPlayerCar* vehicle = GetVehicle())
	{
		if (UGVehicleSeatComponent* seat = vehicle->GetSeatingComponent()->GetSeatComponent(GetSeatIndex()))
		{
			offset = seat->windowOffset;
			seatedCameraOffset = seat->seatedCameraOffset;
			offset.Y *= (GetSeatIndex() == 0 || GetSeatIndex() == 2) ? -1.0f : 1.0f;
		}
	}
	character->GetCameraRoot()->SetRelativeLocation(initialCameraLocation + seatedCameraOffset +
													offset * GetWindowAlpha());
	// Offset hand targets, not the whole torso: translating Manny towards the lens
	// puts the shoulders and stock through the first-person camera.
	character->GetFirstPersonMesh()->SetRelativeLocation(initialFirstPersonLocation);
	if (!IsLocallyOwned() && GetSeatIndex() != INDEX_NONE)
	{
		const FRotator rotation =
			character->GetActorTransform().InverseTransformVectorNoScale(FVector(aimDirection)).Rotation();
		character->GetCameraRoot()->SetRelativeRotation(rotation);
	}
	character->GetFirstPersonMesh()->SetVisibility(health > 0.0f);
	const bool bSeated = GetSeatIndex() != INDEX_NONE;
	UClass* worldAnimClass = bSeated ? seatedAnimClass.Get() : walkingAnimClass.Get();
	UClass* firstAnimClass =
		bSeated ? (seatedFirstPersonAnimClass ? seatedFirstPersonAnimClass.Get() : seatedAnimClass.Get())
				: walkingFirstPersonAnimClass.Get();
	if (worldAnimClass && character->GetMesh()->GetAnimClass() != worldAnimClass)
	{
		character->GetMesh()->SetAnimInstanceClass(worldAnimClass);
	}
	if (firstAnimClass && character->GetFirstPersonMesh()->GetAnimClass() != firstAnimClass)
	{
		character->GetFirstPersonMesh()->SetAnimInstanceClass(firstAnimClass);
	}
}

void UGVehicleCombatComponent::TickComponent(float deltaSeconds, ELevelTick tickType,
											 FActorComponentTickFunction* tickFunction)
{
	Super::TickComponent(deltaSeconds, tickType, tickFunction);
	RefreshPresentation();
	RefreshDriverLock();
	if (IsLocallyOwned() && GetPlayerController())
	{
		GetPlayerController()->SendCombatView(GetAimDirection());
		UCameraComponent* camera = GetCharacter()->GetFirstPersonCameraComponent();
		const UGWeaponDefinition* definition = GetCurrentWeapon() ? GetCurrentWeapon()->GetDefinition() : nullptr;
		const float target = bAiming && definition ? definition->aimFov : initialCameraFov;
		camera->SetFieldOfView(FMath::FInterpTo(camera->FieldOfView, target, deltaSeconds,
												definition ? 3.0f / FMath::Max(0.05f, definition->aimTime) : 10.0f));
	}
}

float UGVehicleCombatComponent::ApplyDamage(float amount, AController* instigatorController)
{
	if (!IsAuthority() || !FMath::IsFinite(amount) || amount <= 0.0f || health <= 0.0f)
	{
		return 0.0f;
	}
	const float applied = FMath::Min(amount, health);
	health -= applied;
	if (health <= 0.0f)
	{
		CancelActions(true);
		if (AGPlayerController* controller = GetPlayerController())
		{
			if (AGPlayerCar* vehicle = GetVehicle())
			{
				vehicle->GetSeatingComponent()->RemovePlayer(controller, false);
				controller->UnPossess();
				controller->SetVehicleContext(nullptr, GetCharacter());
			}
		}
		GetCharacter()->GetCharacterMovement()->DisableMovement();
		OnRep_Loadout();
	}
	GetOwner()->ForceNetUpdate();
	return applied;
}

void UGVehicleCombatComponent::ApplyShotRecoil(float amount)
{
	if (IsLocallyOwned() && GetCharacter())
	{
		GetCharacter()->DoAim(FMath::FRandRange(-0.2f, 0.2f) * amount, amount);
		const UGWeaponDefinition* definition = GetCurrentWeapon() ? GetCurrentWeapon()->GetDefinition() : nullptr;
		if (definition && definition->cameraShake && GetPlayerController()->PlayerCameraManager)
		{
			GetPlayerController()->PlayerCameraManager->StartCameraShake(definition->cameraShake);
		}
	}
}

void UGVehicleCombatComponent::RecordCosmeticShot()
{
	cosmeticShotTime = GetServerTime();
}

FVector UGVehicleCombatComponent::GetFirstPersonPoseOffset() const
{
	if (GetSeatIndex() == INDEX_NONE)
	{
		return FVector::ZeroVector;
	}
	const AGVehicleWeapon* weapon = GetCurrentWeapon();
	const float reloadLift =
		weapon && weapon->IsReloading()
			? 20.0f * FMath::Clamp((weapon->GetReloadEndTime() - GetServerTime()) / 0.2f, 0.0f, 1.0f)
			: 0.0f;
	return firstPersonPoseOffset + FVector(0.0f, 0.0f, reloadLift);
}
