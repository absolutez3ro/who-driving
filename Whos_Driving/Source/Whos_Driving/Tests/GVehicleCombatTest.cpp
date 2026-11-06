#if WITH_DEV_AUTOMATION_TESTS

#include "GPlayerCar.h"
#include "GPlayerController.h"
#include "GVehicleCombatComponent.h"
#include "GVehicleSeatingComponent.h"
#include "GVehicleWeapon.h"
#include "GWeaponAmmoWidget.h"
#include "GWeaponDefinition.h"
#include "Whos_DrivingCharacter.h"
#include "Components/TextBlock.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "TimerManager.h"

namespace
{
	// Loadout order of BP_GFirstPersonCharacter, which also matches the keys 1 and 2.
	constexpr int32 rifleIndex = 0;
	constexpr int32 pistolIndex = 1;

	// An isolated world with one player seated as the driver of a car.
	struct FGCombatTestScene
	{
		UWorld* world = nullptr;
		AGPlayerController* driver = nullptr;
		AWhos_DrivingCharacter* avatar = nullptr;
		UGVehicleCombatComponent* combat = nullptr;

		~FGCombatTestScene()
		{
			if (world)
			{
				world->DestroyWorld(false);
			}
		}

		bool Create(FAutomationTestBase& test)
		{
			const UWorld::InitializationValues initialization = UWorld::InitializationValues()
																	.AllowAudioPlayback(false)
																	.CreatePhysicsScene(true)
																	.CreateNavigation(false)
																	.CreateAISystem(false)
																	.ShouldSimulatePhysics(false);
			UClass* characterClass = LoadClass<AWhos_DrivingCharacter>(
				nullptr, TEXT("/Game/FirstPerson/Blueprints/BP_GFirstPersonCharacter.BP_GFirstPersonCharacter_C"));
			UClass* carClass =
				LoadClass<AGPlayerCar>(nullptr, TEXT("/Game/Blueprints/Vehicle/BP_GPlayerCar.BP_GPlayerCar_C"));
			if (!test.TestNotNull(TEXT("Configured character"), characterClass) ||
				!test.TestNotNull(TEXT("Configured car"), carClass))
			{
				return false;
			}
			world = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num,
										&initialization);
			world->InitializeActorsForPlay(FURL());
			world->BeginPlay();
			// This isolated world has no GameMode to call WorldSettings::NotifyBeginPlay.
			world->SetBegunPlay(true);
			AGPlayerCar* car = world->SpawnActor<AGPlayerCar>(carClass);
			driver = world->SpawnActor<AGPlayerController>();
			driver->PlayerState = world->SpawnActor<APlayerState>();
			FActorSpawnParameters spawn;
			spawn.Owner = driver;
			spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			avatar = world->SpawnActor<AWhos_DrivingCharacter>(characterClass, FTransform::Identity, spawn);
			combat = avatar->GetCombatComponent();
			return test.TestTrue(TEXT("Driver seated"), car->GetSeatingComponent()->InitializeOccupant(driver, avatar, 0));
		}

		void AdvanceTime(float seconds)
		{
			world->TimeSeconds += seconds;
			// The timer manager only ticks once per engine frame and activates new timers at the end of a tick.
			++GFrameCounter;
			world->GetTimerManager().Tick(0.0f);
			++GFrameCounter;
			world->GetTimerManager().Tick(seconds);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGWeaponAmmoTest, "WhosDriving.Combat.WeaponAmmo",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGWeaponAmmoTest::RunTest(const FString& parameters)
{
	FGCombatTestScene scene;
	if (!scene.Create(*this))
	{
		return false;
	}
	AGVehicleWeapon* pistol = scene.combat->GetWeapon(pistolIndex);
	AGVehicleWeapon* rifle = scene.combat->GetWeapon(rifleIndex);
	if (!TestNotNull(TEXT("Pistol"), pistol) || !TestNotNull(TEXT("Rifle"), rifle))
	{
		return false;
	}
	const int32 pistolMagazine = pistol->GetDefinition()->MagazineSize;
	const int32 pistolReserve = pistol->GetDefinition()->InitialReserve;
	TestEqual(TEXT("Magazine comes from the data asset"), pistol->GetBulletCount(), pistolMagazine);
	TestEqual(TEXT("Reserve comes from the data asset"), pistol->GetReserveAmmo(), pistolReserve);
	TestEqual(TEXT("Rifle has its own magazine"), rifle->GetBulletCount(), rifle->GetDefinition()->MagazineSize);
	TestEqual(TEXT("Rifle has its own reserve"), rifle->GetReserveAmmo(), rifle->GetDefinition()->InitialReserve);

	TestFalse(TEXT("Malformed shot direction rejected"), scene.combat->SetTrigger(true, FVector(100.0f)));
	TestEqual(TEXT("Malformed request consumes no ammunition"), pistol->GetBulletCount(), pistolMagazine);
	TestTrue(TEXT("Valid shot accepted"), scene.combat->SetTrigger(true, FVector::ForwardVector));
	scene.combat->SetTrigger(false, FVector::ForwardVector);
	TestEqual(TEXT("A shot spends one round"), pistol->GetBulletCount(), pistolMagazine - 1);
	TestEqual(TEXT("A shot leaves the reserve alone"), pistol->GetReserveAmmo(), pistolReserve);

	TestTrue(TEXT("Reload starts"), scene.combat->RequestReload());
	TestTrue(TEXT("Reload locks firing"), pistol->IsReloading() && !scene.combat->CanFire());
	TestEqual(TEXT("Rounds move only when the reload ends"), pistol->GetBulletCount(), pistolMagazine - 1);
	scene.AdvanceTime(pistol->GetDefinition()->ReloadTime + 0.1f);
	TestFalse(TEXT("Reload ends"), pistol->IsReloading());
	TestEqual(TEXT("Reload fills the magazine"), pistol->GetBulletCount(), pistolMagazine);
	TestEqual(TEXT("Reload takes rounds from the reserve"), pistol->GetReserveAmmo(), pistolReserve - 1);
	TestFalse(TEXT("A full magazine does not reload"), scene.combat->RequestReload());

	TestTrue(TEXT("Round spent before cancelled reload"), pistol->ConsumeRound());
	TestTrue(TEXT("Second reload starts"), scene.combat->RequestReload());
	pistol->CancelReload();
	scene.AdvanceTime(pistol->GetDefinition()->ReloadTime + 0.1f);
	TestEqual(TEXT("Cancelled reload keeps the magazine"), pistol->GetBulletCount(), pistolMagazine - 1);
	TestEqual(TEXT("Cancelled reload keeps the reserve"), pistol->GetReserveAmmo(), pistolReserve - 1);

	UGWeaponDefinition* lowReserveDefinition = NewObject<UGWeaponDefinition>();
	lowReserveDefinition->Mesh = pistol->GetDefinition()->Mesh;
	lowReserveDefinition->MagazineSize = 10;
	lowReserveDefinition->InitialReserve = 3;
	AGVehicleWeapon* lowReserveWeapon = scene.world->SpawnActorDeferred<AGVehicleWeapon>(
		AGVehicleWeapon::StaticClass(), FTransform::Identity, scene.avatar, scene.avatar,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	lowReserveWeapon->Initialize(lowReserveDefinition);
	UGameplayStatics::FinishSpawningActor(lowReserveWeapon, FTransform::Identity);
	lowReserveWeapon->SetEquipped(true);
	for (int32 shot = 0; shot < 5; ++shot)
	{
		lowReserveWeapon->ConsumeRound();
	}
	TestTrue(TEXT("Reload with a small reserve starts"), lowReserveWeapon->BeginReload());
	scene.AdvanceTime(lowReserveDefinition->ReloadTime + 0.1f);
	TestEqual(TEXT("Reload moves only what the reserve holds"), lowReserveWeapon->GetBulletCount(), 8);
	TestEqual(TEXT("Reload empties the small reserve"), lowReserveWeapon->GetReserveAmmo(), 0);
	TestFalse(TEXT("No reload without a reserve"), lowReserveWeapon->BeginReload());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGWeaponSelectionTest, "WhosDriving.Combat.WeaponSelection",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGWeaponSelectionTest::RunTest(const FString& parameters)
{
	FGCombatTestScene scene;
	if (!scene.Create(*this))
	{
		return false;
	}
	AGVehicleWeapon* pistol = scene.combat->GetWeapon(pistolIndex);
	AGVehicleWeapon* rifle = scene.combat->GetWeapon(rifleIndex);
	if (!TestNotNull(TEXT("Pistol"), pistol) || !TestNotNull(TEXT("Rifle"), rifle))
	{
		return false;
	}
	TestNull(TEXT("Index past the loadout"), scene.combat->GetWeapon(rifleIndex + 2));
	TestNull(TEXT("Negative index"), scene.combat->GetWeapon(-1));
	TestFalse(TEXT("Index past the loadout rejected"), scene.combat->SelectWeapon(rifleIndex + 2));
	TestEqual(TEXT("The driver starts with the cabin weapon"), scene.combat->GetSelectedWeaponIndex(), pistolIndex);
	TestTrue(TEXT("Only the selected weapon is equipped"), pistol->IsEquipped() && !rifle->IsEquipped());
	const USkeletalMeshComponent* firstPersonParent = Cast<USkeletalMeshComponent>(pistol->GetFirstPersonMesh()->GetAttachParent());
	const USkeletalMeshComponent* thirdPersonParent = Cast<USkeletalMeshComponent>(pistol->GetThirdPersonMesh()->GetAttachParent());
	if (TestNotNull(TEXT("First-person weapon parent"), firstPersonParent) && TestNotNull(TEXT("Third-person weapon parent"), thirdPersonParent))
	{
		TestTrue(TEXT("First-person weapon hangs on a rendered mesh"), firstPersonParent->bRenderInMainPass);
		TestTrue(TEXT("Third-person weapon hangs on a rendered mesh"), thirdPersonParent->bRenderInMainPass);
		TestTrue(TEXT("Third-person weapon stays out of the first-person mesh"), !thirdPersonParent->bOnlyOwnerSee);
	}
	TestFalse(TEXT("Driver cannot take the rifle inside"), scene.combat->SelectWeapon(rifleIndex));
	TestEqual(TEXT("Refused selection changes nothing"), scene.combat->GetSelectedWeaponIndex(), pistolIndex);

	TestTrue(TEXT("Driver can lean out"), scene.combat->RequestWindow(true));
	TestTrue(TEXT("Rifle selectable outside"), scene.combat->SelectWeapon(rifleIndex));
	TestTrue(TEXT("Selection swaps the equipped weapon"), rifle->IsEquipped() && !pistol->IsEquipped());
	TestEqual(TEXT("Current weapon follows the selection"), scene.combat->GetCurrentWeapon(), rifle);
	scene.AdvanceTime(1.0f);
	TestTrue(TEXT("Driver can return inside"), scene.combat->RequestWindow(false));
	TestEqual(TEXT("Returning inside restores the cabin weapon"), scene.combat->GetSelectedWeaponIndex(), pistolIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGWeaponAmmoWidgetTest, "WhosDriving.Combat.WeaponAmmoWidget",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGWeaponAmmoWidgetTest::RunTest(const FString& parameters)
{
	UClass* widgetClass =
		LoadClass<UGWeaponAmmoWidget>(nullptr, TEXT("/Game/Widgets/WBP_GWeaponAmmo.WBP_GWeaponAmmo_C"));
	FGCombatTestScene scene;
	if (!TestNotNull(TEXT("Configured widget"), widgetClass) || !scene.Create(*this))
	{
		return false;
	}
	UGWeaponAmmoWidget* widget = CreateWidget<UGWeaponAmmoWidget>(scene.world, widgetClass);
	const UTextBlock* text = Cast<UTextBlock>(widget->GetWidgetFromName(TEXT("ammoText")));
	AGVehicleWeapon* pistol = scene.combat->GetWeapon(pistolIndex);
	AGVehicleWeapon* rifle = scene.combat->GetWeapon(rifleIndex);
	if (!TestNotNull(TEXT("Ammo text"), text) || !TestNotNull(TEXT("Pistol"), pistol) || !TestNotNull(TEXT("Rifle"), rifle))
	{
		return false;
	}
	const int32 magazine = pistol->GetDefinition()->MagazineSize;
	const int32 reserve = pistol->GetDefinition()->InitialReserve;
	const FString rifleAmmo =
		FString::Printf(TEXT("%d/%d"), rifle->GetDefinition()->MagazineSize, rifle->GetDefinition()->InitialReserve);
	TestEqual(TEXT("Hidden without a combat component"), widget->GetVisibility(), ESlateVisibility::Collapsed);
	widget->SetCombatComponent(scene.combat);
	TestEqual(TEXT("Visible with an equipped weapon"), widget->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Shows magazine/reserve"), text->GetText().ToString(), FString::Printf(TEXT("%d/%d"), magazine, reserve));

	TestTrue(TEXT("Round spent"), pistol->ConsumeRound());
	TestEqual(TEXT("Counter follows a shot"), text->GetText().ToString(), FString::Printf(TEXT("%d/%d"), magazine - 1, reserve));

	TestTrue(TEXT("Reload starts"), scene.combat->RequestReload());
	scene.AdvanceTime(pistol->GetDefinition()->ReloadTime + 0.1f);
	TestEqual(TEXT("Counter follows a reload"), text->GetText().ToString(), FString::Printf(TEXT("%d/%d"), magazine, reserve - 1));

	TestTrue(TEXT("Driver can lean out"), scene.combat->RequestWindow(true));
	TestTrue(TEXT("Rifle selected"), scene.combat->SelectWeapon(rifleIndex));
	TestEqual(TEXT("Counter shows the selected weapon"), text->GetText().ToString(), rifleAmmo);
	pistol->SetEquipped(true);
	pistol->ConsumeRound();
	pistol->SetEquipped(false);
	TestEqual(TEXT("Another weapon does not touch the counter"), text->GetText().ToString(), rifleAmmo);

	widget->SetCombatComponent(nullptr);
	TestEqual(TEXT("Hidden after losing the combat component"), widget->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGWeaponDefinitionValidationTest, "WhosDriving.Combat.WeaponDefinitionValidation",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGWeaponDefinitionValidationTest::RunTest(const FString& parameters)
{
	for (const TCHAR* path : {TEXT("/Game/M8Shooter/Weapons/DA_GM8Pistol.DA_GM8Pistol"),
							  TEXT("/Game/M8Shooter/Weapons/DA_GM8Rifle.DA_GM8Rifle")})
	{
		const UGWeaponDefinition* definition = LoadObject<UGWeaponDefinition>(nullptr, path);
		if (TestNotNull(path, definition))
		{
			FDataValidationContext context;
			TestEqual(path, definition->IsDataValid(context), EDataValidationResult::Valid);
		}
	}
	const UGWeaponDefinition* emptyDefinition = NewObject<UGWeaponDefinition>();
	FDataValidationContext context;
	TestEqual(TEXT("An empty definition is invalid"), emptyDefinition->IsDataValid(context),
			  EDataValidationResult::Invalid);
	TestTrue(TEXT("Each problem is reported"), context.GetNumErrors() >= 3);
	return true;
}
#endif

#endif
