//#if WITH_DEV_AUTOMATION_TESTS
//
//#include "GPlayerCar.h"
//#include "Engine/World.h"
//#include "Components/StaticMeshComponent.h"
//#include "Engine/StaticMesh.h"
//#include "Misc/AutomationTest.h"
//#include <limits>
//
//IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGVehicleWheelDamageTest,
//	"WhosDriving.Vehicle.WheelDamage",
//	EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)
//
//bool FGVehicleWheelDamageTest::RunTest(const FString& parameters)
//{
//	const UWorld::InitializationValues initializationValues = UWorld::InitializationValues()
//		.AllowAudioPlayback(false)
//		.CreatePhysicsScene(false)
//		.CreateNavigation(false)
//		.CreateAISystem(false)
//		.ShouldSimulatePhysics(false);
//	UWorld* world = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
//		true, ERHIFeatureLevel::Num, &initializationValues);
//	if (!TestNotNull(TEXT("Test world"), world))
//	{
//		return false;
//	}
//
//
//	AGPlayerCar* car = world->SpawnActor<AGPlayerCar>();
//	if (!TestNotNull(TEXT("Test car"), car))
//	{
//		world->DestroyWorld(false);
//		return false;
//	}
//
//	TestTrue(TEXT("Standalone car is authoritative"), car->HasAuthority());
//	TestEqual(TEXT("FL starts healthy"), car->GetWheelDamageState(0).currentHP, 50.0f);
//	car->DamageWheel(-1, 10.0f);
//	car->DamageWheel(4, 10.0f);
//	car->DamageWheel(0, -10.0f);
//	car->DamageWheel(0, 0.0f);
//	car->DamageWheel(0, std::numeric_limits<float>::quiet_NaN());
//	car->DamageWheel(0, std::numeric_limits<float>::infinity());
//	TestEqual(TEXT("Invalid damage is ignored"), car->GetWheelDamageState(0).currentHP, 50.0f);
//
//	car->DamageWheel(0, 20.0f);
//	TestEqual(TEXT("Partial damage updates HP"), car->GetWheelDamageState(0).currentHP, 30.0f);
//	TestFalse(TEXT("Partial damage keeps intact state"), car->GetWheelDamageState(0).bDestroyed);
//	car->DamageWheel(0, 30.0f);
//	TestEqual(TEXT("Threshold clamps HP to zero"), car->GetWheelDamageState(0).currentHP, 0.0f);
//	TestTrue(TEXT("Threshold destroys FL"), car->GetWheelDamageState(0).bDestroyed);
//	car->DamageWheel(0, 100.0f);
//	TestEqual(TEXT("Repeated damage preserves destroyed state"), car->GetWheelDamageState(0).currentHP, 0.0f);
//	for (int32 wheelIndex = 1; wheelIndex < 4; ++wheelIndex)
//	{
//		TestEqual(TEXT("Other wheels stay healthy"), car->GetWheelDamageState(wheelIndex).currentHP, 50.0f);
//		TestFalse(TEXT("Other wheels are intact"), car->GetWheelDamageState(wheelIndex).bDestroyed);
//	}
//
//	world->DestroyWorld(false);
//	return true;
//}
//
//IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGVehicleWheelVisualSwapTest,
//	"WhosDriving.Vehicle.WheelVisualSwap",
//	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
//
//bool FGVehicleWheelVisualSwapTest::RunTest(const FString& parameters)
//{
//	UClass* carClass = LoadClass<AGPlayerCar>(nullptr,
//		TEXT("/Game/Assets/Viechile_Test/ModularFL/BP_GPlayerCarModularFL.BP_GPlayerCarModularFL_C"));
//	if (!TestNotNull(TEXT("Configured prototype class"), carClass))
//	{
//		return false;
//	}
//
//	const UWorld::InitializationValues initializationValues = UWorld::InitializationValues()
//		.AllowAudioPlayback(false)
//		.CreatePhysicsScene(true)
//		.CreateNavigation(false)
//		.CreateAISystem(false)
//		.ShouldSimulatePhysics(false);
//	UWorld* world = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
//		true, ERHIFeatureLevel::Num, &initializationValues);
//	if (!TestNotNull(TEXT("Visual test world"), world))
//	{
//		return false;
//	}
//
//	AGPlayerCar* car = world->SpawnActor<AGPlayerCar>(carClass);
//	if (!TestNotNull(TEXT("Prototype instance"), car))
//	{
//		world->DestroyWorld(false);
//		return false;
//	}
//
//	UStaticMeshComponent* wheelVisual = nullptr;
//	TInlineComponentArray<UStaticMeshComponent*> components;
//	car->GetComponents(components);
//	for (UStaticMeshComponent* component : components)
//	{
//		if (component && component->GetFName() == TEXT("wheelVisualFL"))
//		{
//			wheelVisual = component;
//			break;
//		}
//	}
//	if (!TestNotNull(TEXT("FL visual component"), wheelVisual))
//	{
//		world->DestroyWorld(false);
//		return false;
//	}
//
//	UStaticMesh* intactMesh = LoadObject<UStaticMesh>(nullptr,
//		TEXT("/Game/Assets/Viechile_Test/ModularFL/SM_Musclecar_Wheel_FL.SM_Musclecar_Wheel_FL"));
//	UStaticMesh* destroyedMesh = LoadObject<UStaticMesh>(nullptr,
//		TEXT("/Game/Assets/Viechile_Test/ModularFL/SM_Musclecar_Wheel_FL_DamagedPlaceholder.SM_Musclecar_Wheel_FL_DamagedPlaceholder"));
//	TestEqual(TEXT("Initial mesh is intact"), wheelVisual->GetStaticMesh().Get(), intactMesh);
//	TestEqual(TEXT("Visual uses the animated socket"), wheelVisual->GetAttachSocketName(), FName(TEXT("Wheel_FL_Visual")));
//	car->DamageWheel(0, 20.0f);
//	TestEqual(TEXT("Partial damage keeps intact mesh"), wheelVisual->GetStaticMesh().Get(), intactMesh);
//	car->DamageWheel(0, 30.0f);
//	TestEqual(TEXT("Threshold replaces the mesh asset"), wheelVisual->GetStaticMesh().Get(), destroyedMesh);
//	TestTrue(TEXT("Destroyed mesh is a distinct asset"), destroyedMesh && destroyedMesh != intactMesh);
//	car->DamageWheel(0, 100.0f);
//	TestEqual(TEXT("Repeated damage keeps the destroyed mesh"), wheelVisual->GetStaticMesh().Get(), destroyedMesh);
//
//	world->DestroyWorld(false);
//	return true;
//}
//#endif
