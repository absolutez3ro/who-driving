#include "GVehicleCombatComponent.h"
#include "GVehicleWeapon.h"
#include "GVehicleSeatComponent.h"
#include "GVehicleSeatingComponent.h"
#include "GVehicleMovementComponent.h"
#include "GPlayerCar.h"
#include "GPlayerController.h"
#include "Whos_DrivingCharacter.h"
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
	bWantsInitializeComponent = true;
}

void UGVehicleCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGVehicleCombatComponent, weapons);
	DOREPLIFETIME(UGVehicleCombatComponent, selectedWeaponIndex);
	DOREPLIFETIME(UGVehicleCombatComponent, windowState);
	DOREPLIFETIME(UGVehicleCombatComponent, aimDirection);
	DOREPLIFETIME(UGVehicleCombatComponent, bAiming);
	DOREPLIFETIME(UGVehicleCombatComponent, health);
	DOREPLIFETIME(UGVehicleCombatComponent, equipStartTime);
	DOREPLIFETIME(UGVehicleCombatComponent, confirmedShots);
}

AWhos_DrivingCharacter* UGVehicleCombatComponent::GetCharacter() const
{
	return Cast<AWhos_DrivingCharacter>(GetOwner());
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

void UGVehicleCombatComponent::InitializeComponent()
{
	Super::InitializeComponent();
	if (!GetCharacter())
	{
		return;
	}
	// Captured during spawn, before anyone can seat the character. The game mode seats a freshly spawned
	// character before the world begins play, so a BeginPlay capture would store the seated pose and
	// RefreshPresentation would then apply the seat camera offset twice.
	initialCameraLocation = GetCharacter()->GetCameraRoot()->GetRelativeLocation();
	initialFirstPersonLocation = GetCharacter()->GetFirstPersonMesh()->GetRelativeLocation();
	initialCameraFov = GetCharacter()->GetFirstPersonCameraComponent()->FieldOfView;
}

void UGVehicleCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetCharacter())
	{
		return;
	}
	GetCharacter()->GetFirstPersonMesh()->HideBoneByName(TEXT("head"), EPhysBodyOp::PBO_None);
	if (IsAuthority())
	{
		SpawnLoadout();
	}
	health = maxHealth;
}

void UGVehicleCombatComponent::SpawnLoadout()
{
	for (UGWeaponDefinition* definition : loadout)
	{
		if (!definition)
		{
			continue;
		}
		UClass* weaponClass = definition->WeaponClass ? definition->WeaponClass.Get() : AGVehicleWeapon::StaticClass();
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
	selectedWeaponIndex = FMath::Max(0, FindDriverCabinWeaponIndex());
	OnRep_Loadout();
}

int32 UGVehicleCombatComponent::FindDriverCabinWeaponIndex() const
{
	const AGVehicleWeapon* currentWeapon = GetCurrentWeapon();
	if (currentWeapon && currentWeapon->GetDefinition()->bUsableByDriverInCabin)
	{
		return selectedWeaponIndex;
	}
	for (int32 weaponIndex = 0; weaponIndex < weapons.Num(); ++weaponIndex)
	{
		const AGVehicleWeapon* weapon = GetWeapon(weaponIndex);
		if (weapon && weapon->GetDefinition()->bUsableByDriverInCabin)
		{
			return weaponIndex;
		}
	}
	return selectedWeaponIndex;
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

AGVehicleWeapon* UGVehicleCombatComponent::GetWeapon(int32 weaponIndex) const
{
	AGVehicleWeapon* weapon = weapons.IsValidIndex(weaponIndex) ? weapons[weaponIndex].Get() : nullptr;
	return IsValid(weapon) && weapon->GetDefinition() ? weapon : nullptr;
}

AGVehicleWeapon* UGVehicleCombatComponent::GetCurrentWeapon() const
{
	return GetWeapon(selectedWeaponIndex);
}

bool UGVehicleCombatComponent::GetCurrentWeaponAmmo(int32& outMagazineAmmo, int32& outReserveAmmo) const
{
	const AGVehicleWeapon* weapon = GetCurrentWeapon();
	const bool bHasWeapon = weapon && weapon->IsEquipped();
	outMagazineAmmo = bHasWeapon ? weapon->GetBulletCount() : 0;
	outReserveAmmo = bHasWeapon ? weapon->GetReserveAmmo() : 0;
	return bHasWeapon;
}

void UGVehicleCombatComponent::NotifyWeaponStateChanged(const AGVehicleWeapon* weapon)
{
	if (weapon == GetCurrentWeapon())
	{
		OnWeaponAmmoChanged.Broadcast();
	}
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
	// Leaning only interrupts firing; a reload in progress continues through the transition.
	StopFiring();
	lastWindowRequest = GetServerTime();
	windowState.startAlpha = GetWindowAlpha();
	windowState.startTime = GetServerTime();
	windowState.bOutside = bOutside;
	windowState.duration = GetVehicle()->GetSeatingComponent()->GetSeatComponent(GetSeatIndex())->windowDuration;
	if (!bOutside && GetSeatIndex() == 0)
	{
		SelectWeapon(FindDriverCabinWeaponIndex());
	}
	RefreshDriverLock();
	GetOwner()->ForceNetUpdate();
	return true;
}

bool UGVehicleCombatComponent::RequestWindowSide(bool bLeftSide, const FVector& viewDirection)
{
	AGPlayerCar* vehicle = GetVehicle();
	const int32 seatIndex = GetSeatIndex();
	if (!vehicle || seatIndex == INDEX_NONE || viewDirection.ContainsNaN())
	{
		return false;
	}
	// Sides are taken from the camera: looking towards the rear of the car swaps them.
	const bool bLeftSeat = seatIndex == 0 || seatIndex == 2;
	const bool bLookingBackward = FVector::DotProduct(viewDirection, vehicle->GetActorForwardVector()) < 0.0f;
	const bool bWindowOnLeft = bLeftSeat != bLookingBackward;
	return RequestWindow(bLeftSide == bWindowOnLeft);
}

bool UGVehicleCombatComponent::SelectWeapon(int32 weaponIndex)
{
	const AGVehicleWeapon* weapon = GetWeapon(weaponIndex);
	if (!IsAuthority() || health <= 0.0f || !weapon)
	{
		return false;
	}
	if (GetSeatIndex() == 0 && !windowState.bOutside && !weapon->GetDefinition()->bUsableByDriverInCabin)
	{
		return false;
	}
	if (weaponIndex == selectedWeaponIndex)
	{
		return true;
	}
	CancelActions(false);
	selectedWeaponIndex = weaponIndex;
	equipStartTime = GetServerTime();
	OnRep_Loadout();
	GetOwner()->ForceNetUpdate();
	return true;
}

void UGVehicleCombatComponent::OnRep_Loadout()
{
	if (IsAuthority())
	{
		for (int32 weaponIndex = 0; weaponIndex < weapons.Num(); ++weaponIndex)
		{
			if (AGVehicleWeapon* weapon = GetWeapon(weaponIndex))
			{
				weapon->SetEquipped(weaponIndex == selectedWeaponIndex && GetSeatIndex() != INDEX_NONE && health > 0.0f);
			}
		}
	}
	if (AGVehicleWeapon* weapon = GetCurrentWeapon())
	{
		weapon->PlayEquip();
	}
	OnWeaponAmmoChanged.Broadcast();
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
	StopFiring();
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

void UGVehicleCombatComponent::StopFiring()
{
	bTriggerHeld = false;
	burstRemaining = 0;
	bAiming = false;
	GetWorld()->GetTimerManager().ClearTimer(fireTimer);
	GetOwner()->ForceNetUpdate();
}

void UGVehicleCombatComponent::SeatChanged()
{
	if (IsAuthority())
	{
		CancelActions(true);
		if (GetSeatIndex() == 0)
		{
			selectedWeaponIndex = FindDriverCabinWeaponIndex();
		}
		OnRep_Loadout();
	}
	RefreshPresentation();
}

bool UGVehicleCombatComponent::CanFire() const
{
	const AGVehicleWeapon* weapon = GetCurrentWeapon();
	if (!IsPlayerValid() || !weapon || !weapon->GetDefinition() || !weapon->IsEquipped() || weapon->IsReloading() ||
		weapon->GetBulletCount() <= 0 || GetServerTime() - equipStartTime < weapon->GetDefinition()->EquipTime)
	{
		//UE_LOG(LogTemp, Warning, TEXT("cant fire IsPlayerValid %d weapon %d weapon->GetDefinition() %d weapon->IsEquipped() %d weapon->IsReloading() %d weapon->GetBulletCount() %d GetServerTime() - equipStartTime < weapon->GetDefinition()->EquipTime %d"),
		//	IsPlayerValid(), weapon, weapon->GetDefinition(), weapon->IsEquipped(), weapon->IsReloading(), weapon->GetBulletCount(), GetServerTime() - equipStartTime < weapon->GetDefinition()->EquipTime);
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

FRotator UGVehicleCombatComponent::GetAimRotation() const
{
	// The replicated direction carries no roll. The camera never rolls relative to the character,
	// and the seated character shares the vehicle's up vector, so the view frame stays tied to the car.
	const FVector up = GetCharacter() ? GetCharacter()->GetActorUpVector() : FVector::UpVector;
	return FRotationMatrix::MakeFromXZ(GetAimDirection(), up).Rotator();
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
		weapon->GetFireMode() == EGWeaponFireMode::Burst ? FMath::Clamp(weapon->GetDefinition()->BurstSize, 1, 10) : 0;
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
	const float interval = FMath::Max(0.03f, definition->FireInterval);
	weapon->SetNextShotTime(GetServerTime() + interval);
	FCollisionQueryParams query(SCENE_QUERY_STAT(VehicleWeapon), true, GetCharacter());
	for (AGVehicleWeapon* ownedWeapon : weapons)
	{
		query.AddIgnoredActor(ownedWeapon);
	}
	// Own vehicle and friendly occupants deliberately remain in the query.
	const FVector origin = GetCharacter()->GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector direction = FMath::VRandCone(
		FVector(aimDirection),
		FMath::DegreesToRadians(definition->SpreadDegrees * (bAiming ? definition->AimSpreadMultiplier : 1.0f)));
	FHitResult viewHit;
	const FVector end = origin + direction * FMath::Clamp(definition->Range, 1.0f, 100000.0f);
	const bool bViewHit = GetWorld()->LineTraceSingleByChannel(viewHit, origin, end, ECC_GameTraceChannel3, query);
	const FVector target = bViewHit ? viewHit.ImpactPoint : end;
	const FVector muzzle = weapon->GetShotOrigin();
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
		UGameplayStatics::ApplyPointDamage(hit.GetActor(), FMath::Max(0.0f, definition->Damage),
										   (target - muzzle).GetSafeNormal(), hit, GetPlayerController(), weapon,
										   definition->DamageType);
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
		weapon->SetNextShotTime(GetServerTime() + FMath::Max(interval, definition->BurstInterval));
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
	AGVehicleWeapon* weapon = GetCurrentWeapon();
	// A repeated press must not restart the reload that is already running.
	if (!IsAuthority() || !IsPlayerValid() || !weapon || weapon->IsReloading())
	{
		return false;
	}
	StopFiring();
	return weapon->BeginReload();
}

bool UGVehicleCombatComponent::ChangeFireMode()
{
	if (!IsAuthority() || !IsPlayerValid() || !GetCurrentWeapon())
	{
		return false;
	}
	// The weapon refuses to switch modes while reloading, so the reload is kept.
	StopFiring();
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
	AWhos_DrivingCharacter* character = GetCharacter();
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
	const bool bSeated = GetSeatIndex() != INDEX_NONE;
	const FVector seatedCameraLocation = initialCameraLocation + seatedCameraOffset;
	character->GetCameraRoot()->SetRelativeLocation(seatedCameraLocation + offset * GetWindowAlpha());
	if (!IsLocallyOwned() && bSeated)
	{
		const FRotator rotation =
			character->GetActorTransform().InverseTransformVectorNoScale(FVector(aimDirection)).Rotation();
		character->GetCameraRoot()->SetRelativeRotation(rotation);
	}
	RefreshFirstPersonMesh(seatedCameraLocation);
	character->GetFirstPersonMesh()->SetVisibility(health > 0.0f);
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

void UGVehicleCombatComponent::RefreshFirstPersonMesh(const FVector& seatedCameraLocation)
{
	AWhos_DrivingCharacter* character = GetCharacter();
	USkeletalMeshComponent* firstPersonMesh = character->GetFirstPersonMesh();
	const FTransform restTransform(initialFirstPersonLocation);
	if (GetSeatIndex() == INDEX_NONE)
	{
		firstPersonMesh->SetRelativeTransform(restTransform);
		return;
	}
	// The visible arms only copy bone rotations, so they cannot stretch towards a hand target. While seated, the
	// whole mesh moves and turns with the camera instead, which keeps the weapon in the same place on the screen.
	const FTransform parentInCharacter = character->GetMesh()->GetRelativeTransform();
	const FTransform restInCharacter = restTransform * parentInCharacter;
	const USceneComponent* cameraRoot = character->GetCameraRoot();
	const FQuat viewRotation = cameraRoot->GetRelativeRotation().Quaternion();
	const FVector fromCamera = restInCharacter.GetLocation() + seatedFirstPersonOffset - seatedCameraLocation;
	const FTransform meshInCharacter(viewRotation * restInCharacter.GetRotation(),
									 cameraRoot->GetRelativeLocation() + viewRotation.RotateVector(fromCamera));
	firstPersonMesh->SetRelativeTransform(meshInCharacter.GetRelativeTransform(parentInCharacter));
}

void UGVehicleCombatComponent::TickComponent(float deltaSeconds, ELevelTick tickType,
											 FActorComponentTickFunction* tickFunction)
{
	Super::TickComponent(deltaSeconds, tickType, tickFunction);
	RegenHealth(deltaSeconds);
	RefreshPresentation();
	RefreshDriverLock();
	if (IsLocallyOwned() && GetPlayerController())
	{
		GetPlayerController()->SendCombatView(GetAimDirection());
		UCameraComponent* camera = GetCharacter()->GetFirstPersonCameraComponent();
		const UGWeaponDefinition* definition = GetCurrentWeapon() ? GetCurrentWeapon()->GetDefinition() : nullptr;
		const float target = bAiming && definition ? definition->AimFov : initialCameraFov;
		camera->SetFieldOfView(FMath::FInterpTo(camera->FieldOfView, target, deltaSeconds,
												definition ? 3.0f / FMath::Max(0.05f, definition->AimTime) : 10.0f));
	}
}

void UGVehicleCombatComponent::RegenHealth(float deltaSeconds)
{
	if (IsAuthority() && health > 0.0f && health < maxHealth &&
		GetServerTime() - lastDamageTime >= regenCooldown)
	{
		const float previous = health;
		health = FMath::Min(health + healthRegenRate * deltaSeconds, maxHealth);
		OnHealthUpdate.Broadcast(health, maxHealth);
		if (!FMath::IsNearlyEqual(previous, health))
		{
			if (GetNetMode() != NM_DedicatedServer)
			{
				RefreshPresentation();
			}
		}
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
	OnHealthUpdate.Broadcast(health, maxHealth);
	lastDamageTime = GetServerTime();
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
	const UGWeaponDefinition* definition = GetCurrentWeapon() ? GetCurrentWeapon()->GetDefinition() : nullptr;
	if (IsLocallyOwned() && GetCharacter() && definition)
	{
		GetCharacter()->DoAim(FMath::FRandRange(-definition->RecoilYawRange, definition->RecoilYawRange) * amount, amount);
		if (definition->CameraShake && GetPlayerController()->PlayerCameraManager)
		{
			GetPlayerController()->PlayerCameraManager->StartCameraShake(definition->CameraShake);
		}
	}
}

void UGVehicleCombatComponent::RecordCosmeticShot()
{
	cosmeticShotTime = GetServerTime();
}

FVector UGVehicleCombatComponent::GetFirstPersonPoseOffset() const
{
	const AGVehicleWeapon* weapon = GetCurrentWeapon();
	if (GetSeatIndex() == INDEX_NONE || !weapon)
	{
		return FVector::ZeroVector;
	}
	const UGWeaponDefinition* definition = weapon->GetDefinition();
	const float liftAlpha = FMath::Clamp((weapon->GetReloadEndTime() - GetServerTime()) /
											 FMath::Max(0.01f, definition->ReloadHandLiftTime),
										 0.0f, 1.0f);
	const float reloadLift = weapon->IsReloading() ? definition->ReloadHandLift * liftAlpha : 0.0f;
	return definition->ViewmodelOffset + FVector(0.0f, 0.0f, reloadLift);
}
