//#if WITH_DEV_AUTOMATION_TESTS
//
//#include "GPlayerCar.h"
//#include "GPlayerController.h"
//#include "GVehicleMovementComponent.h"
//#include "GVehicleCombatComponent.h"
//#include "GVehicleSeatingComponent.h"
//#include "GVehicleWeapon.h"
//#include "GWhosDrivingCharacter.h"
//#include "Engine/World.h"
//#include "Engine/EngineBaseTypes.h"
//#include "Engine/DamageEvents.h"
//#include "GameFramework/PlayerState.h"
//#include "Misc/AutomationTest.h"
//#include <limits>
//
//IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGVehicleCombatTest, "WhosDriving.Combat.ServerRules",
//								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
//
//bool FGVehicleCombatTest::RunTest(const FString& parameters)
//{
//	const UWorld::InitializationValues initialization = UWorld::InitializationValues()
//															.AllowAudioPlayback(false)
//															.CreatePhysicsScene(true)
//															.CreateNavigation(false)
//															.CreateAISystem(false)
//															.ShouldSimulatePhysics(false);
//	UWorld* world =
//		UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &initialization);
//	UClass* characterClass = LoadClass<AGWhosDrivingCharacter>(
//		nullptr, TEXT("/Game/FirstPerson/Blueprints/BP_GFirstPersonCharacter.BP_GFirstPersonCharacter_C"));
//	if (!TestNotNull(TEXT("Configured character"), characterClass))
//	{
//		world->DestroyWorld(false);
//		return false;
//	}
//	world->InitializeActorsForPlay(FURL());
//	world->BeginPlay();
//	// This isolated world has no GameMode to call WorldSettings::NotifyBeginPlay.
//	world->SetBegunPlay(true);
//	AGPlayerCar* car = world->SpawnActor<AGPlayerCar>();
//	AGPlayerController* driver = world->SpawnActor<AGPlayerController>();
//	driver->PlayerState = world->SpawnActor<APlayerState>();
//	FActorSpawnParameters spawn;
//	spawn.Owner = driver;
//	spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//	AGWhosDrivingCharacter* avatar =
//		world->SpawnActor<AGWhosDrivingCharacter>(characterClass, FTransform::Identity, spawn);
//	car->GetSeatingComponent()->InitializeOccupant(driver, avatar, 0);
//	UGVehicleCombatComponent* combat = avatar->GetCombatComponent();
//	AGVehicleWeapon* pistol = combat->GetWeapon(EGWeaponSlot::Secondary);
//	AGVehicleWeapon* rifle = combat->GetWeapon(EGWeaponSlot::Primary);
//	if (!TestNotNull(TEXT("Pistol"), pistol) || !TestNotNull(TEXT("Rifle"), rifle))
//	{
//		world->DestroyWorld(false);
//		return false;
//	}
//	TestEqual(TEXT("M8 pistol magazine retained"), pistol->GetBulletCount(), 15);
//	TestEqual(TEXT("M8 rifle magazine retained"), rifle->GetBulletCount(), 40);
//	TestTrue(TEXT("Driver can fire secondary inside"), combat->CanFire());
//	TestFalse(TEXT("Driver cannot select primary inside"), combat->SelectWeapon(EGWeaponSlot::Primary));
//	TestFalse(TEXT("Malformed shot direction rejected"), combat->SetTrigger(true, FVector(100.0f)));
//	TestEqual(TEXT("Malformed request consumes no ammunition"), pistol->GetBulletCount(), 15);
//	TestTrue(TEXT("Valid driver shot accepted"), combat->SetTrigger(true, FVector::ForwardVector));
//	TestEqual(TEXT("Server spends one round"), pistol->GetBulletCount(), 14);
//	combat->SetTrigger(false, FVector::ForwardVector);
//	combat->SetTrigger(true, FVector::ForwardVector);
//	TestEqual(TEXT("Repeated press cannot bypass fire interval"), pistol->GetBulletCount(), 14);
//	TestTrue(TEXT("Reload starts"), combat->RequestReload());
//	TestTrue(TEXT("Reload locks firing"), pistol->IsReloading() && !combat->CanFire());
//	TestTrue(TEXT("Driver can lean out"), combat->RequestWindow(true));
//	TestFalse(TEXT("Leaning interrupts reload without granting ammunition"), pistol->IsReloading());
//	TestEqual(TEXT("Cancelled reload preserves rounds"), pistol->GetBulletCount(), 14);
//	TestEqual(TEXT("Cancelled reload preserves reserve"), pistol->GetReserveAmmo(), 200);
//	TestTrue(TEXT("Driver retains vehicle possession"), driver->GetPawn() == car);
//	TestTrue(TEXT("Leaning locks vehicle control"),
//			 CastChecked<UGVehicleMovementComponent>(car->GetVehicleMovementComponent())->IsWindowDrivingLocked());
//	TestTrue(TEXT("Primary selectable outside"), combat->SelectWeapon(EGWeaponSlot::Primary));
//	TestFalse(TEXT("No shot during window transition"), combat->CanFire());
//	car->GetSeatingComponent()->RemovePlayer(driver, false);
//	TestFalse(TEXT("On foot fire disabled"), combat->CanFire());
//	TestFalse(TEXT("Exit clears window state"), combat->IsOutside());
//	TestFalse(TEXT("Exit releases driving lock"),
//			  CastChecked<UGVehicleMovementComponent>(car->GetVehicleMovementComponent())->IsWindowDrivingLocked());
//	TestEqual(TEXT("Car accepts friendly damage"), car->TakeDamage(25.0f, FDamageEvent(), driver, pistol), 25.0f);
//	TestEqual(TEXT("Car health updated on server"), car->health, 975.0f);
//	TestEqual(TEXT("Character rejects NaN damage"),
//			  combat->ApplyDamage(std::numeric_limits<float>::quiet_NaN(), driver), 0.0f);
//	TestEqual(TEXT("Character accepts friendly damage"), combat->ApplyDamage(25.0f, driver), 25.0f);
//	TestEqual(TEXT("Player health updated"), combat->GetHealth(), 75.0f);
//	world->DestroyWorld(false);
//	return true;
//}
//#endif
