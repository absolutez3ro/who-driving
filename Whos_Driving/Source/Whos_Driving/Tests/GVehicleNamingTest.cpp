#if WITH_DEV_AUTOMATION_TESTS

#include "GPlayerCar.h"
#include "GBoxComponent.h"
#include "GVehicleModule.h"
#include "GVehicleModuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGVehicleComponentNamesTest, "WhosDriving.Vehicle.ComponentNameBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGVehicleComponentNamesTest::RunTest(const FString& parameters)
{
	UClass* carClass = LoadClass<AGPlayerCar>(nullptr,
		TEXT("/Game/Blueprints/Vehicle/BP_GPlayerCar.BP_GPlayerCar_C"));
	if (!TestNotNull(TEXT("Vehicle Blueprint"), carClass))
	{
		return false;
	}
	const UWorld::InitializationValues initialization = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* world = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &initialization);
	FActorSpawnParameters spawn;
	spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGPlayerCar* car = world->SpawnActor<AGPlayerCar>(carClass, FTransform::Identity, spawn);
	if (!TestNotNull(TEXT("Vehicle instance"), car))
	{
		world->DestroyWorld(false);
		return false;
	}
	UGVehicleModuleComponent* modules = car->FindComponentByClass<UGVehicleModuleComponent>();
	if (!TestNotNull(TEXT("Module registry"), modules))
	{
		world->DestroyWorld(false);
		return false;
	}
	modules->RebuildModuleRegistry(car);
	TInlineComponentArray<UGBoxComponent*> hitboxes;
	car->GetComponents(hitboxes);
	TInlineComponentArray<USceneComponent*> components;
	car->GetComponents(components);
	for (int32 index = 0; index < 2; ++index)
	{
		const FName moduleId(*FString::Printf(TEXT("partDoor%d"), index));
		const FName hitboxName(*FString::Printf(TEXT("CombatDoor%d"), index));
		UGVehicleModule* module = modules->FindModule(moduleId);
		USceneComponent** visual = components.FindByPredicate([moduleId](const USceneComponent* component)
		{
			return component->GetFName() == moduleId;
		});
		UGBoxComponent** hitbox = hitboxes.FindByPredicate([hitboxName](const UGBoxComponent* component)
		{
			return component->GetFName() == hitboxName;
		});
		if (!TestNotNull(TEXT("Renamed door module"), module)
			|| !TestNotNull(TEXT("Renamed door visual"), visual)
			|| !TestNotNull(TEXT("Door hitbox"), hitbox))
		{
			continue;
		}
		TestEqual(TEXT("Hitbox targets the renamed module"), (*hitbox)->moduleId, moduleId);
		module->ApplyDamage(module->currentHP + 1.0f);
		TestFalse(TEXT("Destroyed door hides its renamed visual"), (*visual)->IsVisible());
	}
	world->DestroyWorld(false);
	return true;
}

#endif
