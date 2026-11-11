#if WITH_DEV_AUTOMATION_TESTS

#include "GGameState.h"
#include "GPlayerCar.h"
#include "GVehicleModule.h"
#include "GVehicleModuleComponent.h"
#include "GVehicleSeatComponent.h"
#include "GVehicleSeatingComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGVehicleSeatRepairRuleTest, "WhosDriving.Vehicle.RepairSeat.SeatRule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGVehicleSeatRepairRuleTest::RunTest(const FString& parameters)
{
	UGVehicleSeatComponent* seat = NewObject<UGVehicleSeatComponent>();
	TestFalse(TEXT("A seat without data repairs nothing"), seat->CanRepairModule(TEXT("Engine")));

	seat->repairableModuleIds.Add(TEXT("Engine"));
	TestTrue(TEXT("Listed module is repairable"), seat->CanRepairModule(TEXT("Engine")));
	TestFalse(TEXT("Unlisted module is not repairable"), seat->CanRepairModule(TEXT("Wheel_0")));
	TestFalse(TEXT("Empty module id is not repairable"), seat->CanRepairModule(NAME_None));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGVehicleServerRepairSeatTest, "WhosDriving.Vehicle.RepairSeat.ServerRepair",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGVehicleServerRepairSeatTest::RunTest(const FString& parameters)
{
	UClass* carClass = LoadClass<AGPlayerCar>(nullptr, TEXT("/Game/Blueprints/Vehicle/BP_GPlayerCar.BP_GPlayerCar_C"));
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
	// Repairs spend parts from the game state, so the test world needs one.
	AGGameState* gameState = world->SpawnActor<AGGameState>(spawn);
	AGPlayerCar* car = world->SpawnActor<AGPlayerCar>(carClass, FTransform::Identity, spawn);
	if (!TestNotNull(TEXT("Game state"), gameState) || !TestNotNull(TEXT("Vehicle instance"), car))
	{
		world->DestroyWorld(false);
		return false;
	}
	world->SetGameState(gameState);
	gameState->PlusRepairParts(1000);
	UGVehicleModuleComponent* modules = car->GetModuleComponent();
	UGVehicleSeatingComponent* seating = car->GetSeatingComponent();
	modules->RebuildModuleRegistry(car);

	TInlineComponentArray<UGVehicleSeatComponent*> seats;
	car->GetComponents(seats);
	TestTrue(TEXT("Vehicle has seats"), seats.Num() > 0);
	for (const UGVehicleSeatComponent* seat : seats)
	{
		for (const FName& moduleId : seat->repairableModuleIds)
		{
			TestNotNull(*FString::Printf(TEXT("%s lists existing module %s"), *seat->GetName(), *moduleId.ToString()),
				modules->FindModule(moduleId));
		}
	}

	TestFalse(TEXT("No seat means no repair"), car->RepairModuleFromSeat(INDEX_NONE, TEXT("Engine")));
	for (UGVehicleModule* module : modules->GetModules())
	{
		const FName moduleId = module->moduleId;
		int32 allowedSeat = INDEX_NONE;
		int32 deniedSeat = INDEX_NONE;
		for (const UGVehicleSeatComponent* seat : seats)
		{
			const int32 seatIndex = static_cast<int32>(seat->seatId);
			if (seating->CanRepairModuleFromSeat(seatIndex, moduleId))
			{
				allowedSeat = seatIndex;
			}
			else
			{
				deniedSeat = seatIndex;
			}
		}
		if (!TestTrue(*FString::Printf(TEXT("%s is repairable from some seat"), *moduleId.ToString()), allowedSeat != INDEX_NONE))
		{
			continue;
		}
		car->DamageModule(moduleId, module->maxHP);
		TestTrue(*FString::Printf(TEXT("%s is destroyed"), *moduleId.ToString()), module->IsDestroyed());
		if (deniedSeat != INDEX_NONE)
		{
			const int32 partsBefore = gameState->GetRepairParts();
			TestFalse(*FString::Printf(TEXT("%s rejects seat %d"), *moduleId.ToString(), deniedSeat),
				car->RepairModuleFromSeat(deniedSeat, moduleId));
			TestTrue(*FString::Printf(TEXT("%s stays destroyed after a rejected repair"), *moduleId.ToString()), module->IsDestroyed());
			TestEqual(TEXT("A rejected repair spends no parts"), gameState->GetRepairParts(), partsBefore);
		}
		TestTrue(*FString::Printf(TEXT("%s accepts seat %d"), *moduleId.ToString(), allowedSeat),
			car->RepairModuleFromSeat(allowedSeat, moduleId));
		TestTrue(*FString::Printf(TEXT("%s is repaired"), *moduleId.ToString()), module->IsFull());
	}
	world->DestroyWorld(false);
	return true;
}

#endif
