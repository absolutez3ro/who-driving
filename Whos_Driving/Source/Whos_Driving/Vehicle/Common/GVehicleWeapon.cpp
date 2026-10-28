#include "GVehicleWeapon.h"
#include "GVehicleCombatComponent.h"
#include "Whos_DrivingCharacter.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AGVehicleWeapon::AGVehicleWeapon()
{
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
	SetReplicateMovement(false);
	firstMagazine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("First Magazine"));
	worldMagazine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("World Magazine"));
	firstMagazine->SetupAttachment(GetFirstPersonMesh());
	worldMagazine->SetupAttachment(GetThirdPersonMesh());
	firstMagazine->SetOnlyOwnerSee(true);
	worldMagazine->SetOwnerNoSee(true);
	firstMagazine->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	worldMagazine->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	firstMagazine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	worldMagazine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGVehicleWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGVehicleWeapon, definition);
	DOREPLIFETIME(AGVehicleWeapon, reserveAmmo);
	DOREPLIFETIME(AGVehicleWeapon, fireMode);
	DOREPLIFETIME(AGVehicleWeapon, bEquipped);
	DOREPLIFETIME(AGVehicleWeapon, bReloading);
	DOREPLIFETIME(AGVehicleWeapon, bEmptyReload);
	DOREPLIFETIME(AGVehicleWeapon, actionStartTime);
	DOREPLIFETIME(AGVehicleWeapon, reloadEndTime);
}

void AGVehicleWeapon::Initialize(UGWeaponDefinition* newDefinition)
{
	if (!HasAuthority() || !newDefinition)
	{
		return;
	}
	definition = newDefinition;
	MagazineSize = FMath::Max(1, definition->MagazineSize);
	CurrentBullets = MagazineSize;
	reserveAmmo = FMath::Max(0, definition->InitialReserve);
	fireMode = definition->FireModes.IsEmpty() ? EGWeaponFireMode::Single : definition->FireModes[0];
	ConfigureMeshes();
	ForceNetUpdate();
}

AWhos_DrivingCharacter* AGVehicleWeapon::GetCharacter() const
{
	return Cast<AWhos_DrivingCharacter>(GetOwner());
}

void AGVehicleWeapon::BeginPlay()
{
	Super::BeginPlay();
	ConfigureMeshes();
}

void AGVehicleWeapon::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	GetWorldTimerManager().ClearTimer(reloadTimer);
	Super::EndPlay(endPlayReason);
}

void AGVehicleWeapon::ConfigureMeshes()
{
	if (!definition)
	{
		return;
	}
	MagazineSize = FMath::Max(1, definition->MagazineSize);
	GetFirstPersonMesh()->SetSkeletalMeshAsset(definition->Mesh);
	GetThirdPersonMesh()->SetSkeletalMeshAsset(definition->Mesh);
	for (USkeletalMeshComponent* mesh : {GetFirstPersonMesh(), GetThirdPersonMesh()})
	{
		mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
	firstMagazine->SetStaticMesh(definition->MagazineMesh);
	worldMagazine->SetStaticMesh(definition->MagazineMesh);
	SetMagazineInHand(false, true, true);
	SetMagazineInHand(false, true, false);
	for (UStaticMeshComponent* mesh : attachmentMeshes)
	{
		if (mesh)
		{
			mesh->DestroyComponent();
		}
	}
	attachmentMeshes.Reset();
	for (const FGWeaponAttachment& attachment : definition->Attachments)
	{
		if (!attachment.Mesh)
		{
			continue;
		}
		for (int32 view = 0; view < 2; ++view)
		{
			UStaticMeshComponent* mesh = NewObject<UStaticMeshComponent>(this);
			mesh->SetStaticMesh(attachment.Mesh);
			mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			mesh->SetOnlyOwnerSee(view == 0);
			mesh->SetOwnerNoSee(view == 1);
			mesh->SetFirstPersonPrimitiveType(view == 0 ? EFirstPersonPrimitiveType::FirstPerson
														: EFirstPersonPrimitiveType::WorldSpaceRepresentation);
			mesh->SetupAttachment(view == 0 ? GetFirstPersonMesh() : GetThirdPersonMesh(), attachment.Socket);
			mesh->SetRelativeTransform(attachment.Offset);
			mesh->RegisterComponent();
			attachmentMeshes.Add(mesh);
		}
	}
	AttachMeshes();
	OnRep_Presentation();
	OnRep_Reload();
}

void AGVehicleWeapon::AttachMeshes()
{
	AWhos_DrivingCharacter* character = GetCharacter();
	if (!character || !definition)
	{
		return;
	}
	AttachToActor(character, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	GetFirstPersonMesh()->AttachToComponent(character->GetFirstPersonMesh(),
											FAttachmentTransformRules::SnapToTargetNotIncludingScale,
											definition->GripSocket);
	GetThirdPersonMesh()->AttachToComponent(
		character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, definition->GripSocket);
	GetFirstPersonMesh()->SetRelativeTransform(definition->GripOffset);
	GetThirdPersonMesh()->SetRelativeTransform(definition->GripOffset);
}

void AGVehicleWeapon::OnRep_Definition()
{
	ConfigureMeshes();
}

void AGVehicleWeapon::SetEquipped(bool bNewEquipped)
{
	if (!HasAuthority() || bEquipped == bNewEquipped)
	{
		return;
	}
	bEquipped = bNewEquipped;
	if (!bEquipped)
	{
		CancelReload();
	}
	OnRep_Presentation();
	ForceNetUpdate();
}

void AGVehicleWeapon::OnRep_Presentation()
{
	SetActorHiddenInGame(!bEquipped);
	if (bEquipped)
	{
		AttachMeshes();
	}
}

bool AGVehicleWeapon::ConsumeRound()
{
	if (!HasAuthority() || !bEquipped || bReloading || CurrentBullets <= 0)
	{
		return false;
	}
	--CurrentBullets;
	ForceNetUpdate();
	return true;
}

bool AGVehicleWeapon::BeginReload()
{
	if (!HasAuthority() || !definition || !bEquipped || bReloading || reserveAmmo <= 0 ||
		CurrentBullets >= MagazineSize)
	{
		return false;
	}
	bEmptyReload = CurrentBullets == 0;
	bReloading = true;
	actionStartTime = GetCharacter()->GetCombatComponent()->GetServerTime();
	const float duration = FMath::Max(0.1f, bEmptyReload ? definition->EmptyReloadTime : definition->ReloadTime);
	reloadEndTime = actionStartTime + duration;
	GetWorldTimerManager().SetTimer(reloadTimer, this, &AGVehicleWeapon::FinishReload, duration, false);
	OnRep_Reload();
	ForceNetUpdate();
	return true;
}

void AGVehicleWeapon::FinishReload()
{
	if (!HasAuthority() || !bReloading || !bEquipped)
	{
		return;
	}
	const int32 count = FMath::Min(MagazineSize - CurrentBullets, reserveAmmo);
	CurrentBullets += count;
	reserveAmmo -= count;
	bReloading = false;
	OnRep_Reload();
	ForceNetUpdate();
}

void AGVehicleWeapon::CancelReload()
{
	if (!HasAuthority())
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(reloadTimer);
	bReloading = false;
	OnRep_Reload();
	ForceNetUpdate();
}

void AGVehicleWeapon::OnRep_Reload()
{
	if (!definition)
	{
		return;
	}
	UAnimSequence* sequence = bEmptyReload ? definition->WeaponEmptyReload : definition->WeaponReload;
	for (USkeletalMeshComponent* mesh : {GetFirstPersonMesh(), GetThirdPersonMesh()})
	{
		if (bReloading && sequence)
		{
			mesh->PlayAnimation(sequence, false);
		}
	}
	if (!bReloading)
	{
		SetMagazineInHand(false, true, true);
		SetMagazineInHand(false, true, false);
	}
}

bool AGVehicleWeapon::CycleFireMode()
{
	if (!HasAuthority() || !definition || definition->FireModes.Num() < 2 || bReloading)
	{
		return false;
	}
	const int32 index = definition->FireModes.IndexOfByKey(fireMode);
	fireMode = definition->FireModes[(index + 1) % definition->FireModes.Num()];
	ForceNetUpdate();
	return true;
}

void AGVehicleWeapon::PlayEquip()
{
	if (!definition || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	for (USkeletalMeshComponent* mesh : {GetFirstPersonMesh(), GetThirdPersonMesh()})
	{
		if (definition->WeaponEquip)
		{
			mesh->PlayAnimation(definition->WeaponEquip, false);
		}
	}
	if (definition->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, definition->EquipSound, GetActorLocation());
	}
}

void AGVehicleWeapon::SetMagazineInHand(bool bInHand, bool bLeftHand, bool bFirstPerson)
{
	AWhos_DrivingCharacter* character = GetCharacter();
	if (!character)
	{
		return;
	}
	UStaticMeshComponent* magazine = bFirstPerson ? firstMagazine.Get() : worldMagazine.Get();
	USceneComponent* target = bInHand ? (bFirstPerson ? character->GetFirstPersonMesh() : character->GetMesh())
									  : (bFirstPerson ? GetFirstPersonMesh() : GetThirdPersonMesh());
	magazine->AttachToComponent(target, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
								bInHand ? FName(bLeftHand ? TEXT("M8_Magazine_L") : TEXT("M8_Magazine_R"))
										: FName(TEXT("MagazineSocket")));
}

void AGVehicleWeapon::MulticastShot_Implementation(FVector_NetQuantize impact, bool bHit)
{
	if (!definition || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	AWhos_DrivingCharacter* character = GetCharacter();
	if (!character)
	{
		return;
	}
	UGVehicleCombatComponent* combat = character->GetCombatComponent();
	USkeletalMeshComponent* mesh = combat->IsLocallyOwned() ? GetFirstPersonMesh() : GetThirdPersonMesh();
	lastShotTime = combat->GetServerTime();
	combat->RecordCosmeticShot();
	if (definition->WeaponFire)
	{
		GetFirstPersonMesh()->PlayAnimation(definition->WeaponFire, false);
		GetThirdPersonMesh()->PlayAnimation(definition->WeaponFire, false);
	}
	UGameplayStatics::SpawnEmitterAttached(definition->MuzzleEffect, mesh, TEXT("MuzzleSocket"));
	UGameplayStatics::SpawnEmitterAttached(definition->ShellEffect, mesh, TEXT("ShellSocket"));
	UGameplayStatics::PlaySoundAtLocation(this, definition->FireSound, mesh->GetSocketLocation(TEXT("MuzzleSocket")));
	if (bHit && definition->ImpactEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), definition->ImpactEffect, impact);
	}
	if (combat->IsLocallyOwned())
	{
		combat->ApplyShotRecoil(definition->Recoil);
	}
}

void AGVehicleWeapon::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);
	if (bReloading && definition && GetCharacter())
	{
		UAnimSequence* sequence = bEmptyReload ? definition->WeaponEmptyReload : definition->WeaponReload;
		if (sequence)
		{
			const float phase = FMath::Clamp((GetCharacter()->GetCombatComponent()->GetServerTime() - actionStartTime) /
												 FMath::Max(0.1f, reloadEndTime - actionStartTime),
											 0.0f, 1.0f);
			for (USkeletalMeshComponent* mesh : {GetFirstPersonMesh(), GetThirdPersonMesh()})
			{
				if (UAnimSingleNodeInstance* instance = mesh->GetSingleNodeInstance())
				{
					instance->SetPosition(sequence->GetPlayLength() * phase, false);
				}
			}
		}
	}
	// Owner/definition replication can arrive in either order, including during late join.
	if (bEquipped && GetCharacter() && GetFirstPersonMesh()->GetAttachParent() != GetCharacter()->GetFirstPersonMesh())
	{
		AttachMeshes();
	}
}
