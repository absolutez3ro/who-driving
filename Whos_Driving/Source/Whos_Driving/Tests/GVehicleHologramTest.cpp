#if WITH_DEV_AUTOMATION_TESTS

#include "GPlayerCar.h"
#include "GVehicleHologram.h"
#include "GVehicleModule.h"
#include "GVehicleModuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

namespace
{
	UStaticMeshComponent* FindHologramMesh(AGVehicleHologram* hologram, const FName& componentName)
	{
		TInlineComponentArray<UStaticMeshComponent*> meshes;
		hologram->GetComponents(meshes);
		for (UStaticMeshComponent* mesh : meshes)
		{
			if (mesh->GetFName() == componentName)
			{
				return mesh;
			}
		}
		return nullptr;
	}

	float GetCustomDataFloat(const UStaticMeshComponent* mesh, int32 dataIndex)
	{
		const TArray<float>& data = mesh->GetCustomPrimitiveData().Data;
		if (data.IsValidIndex(dataIndex))
		{
			return data[dataIndex];
		}
		return 0.0f;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGVehicleHologramDamageTest, "WhosDriving.Vehicle.Hologram.ColourFollowsModuleHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGVehicleHologramDamageTest::RunTest(const FString& parameters)
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
	AGPlayerCar* car = world->SpawnActor<AGPlayerCar>(carClass, FTransform::Identity, spawn);

	UChildActorComponent* hologramComponent = car ? car->FindComponentByClass<UChildActorComponent>() : nullptr;
	AGVehicleHologram* hologram = hologramComponent ? Cast<AGVehicleHologram>(hologramComponent->GetChildActor()) : nullptr;
	if (!TestNotNull(TEXT("Vehicle instance"), car) || !TestNotNull(TEXT("Hologram child actor"), hologram))
	{
		world->DestroyWorld(false);
		return false;
	}

	UGVehicleModuleComponent* modules = car->GetModuleComponent();
	modules->RebuildModuleRegistry(car);
	hologram->DispatchBeginPlay();

	UStaticMeshComponent* bodyMesh = FindHologramMesh(hologram, TEXT("BodyMesh"));
	UStaticMeshComponent* wheelMesh = FindHologramMesh(hologram, TEXT("WheelFrontLeftMesh"));
	UStaticMeshComponent* doorMesh = FindHologramMesh(hologram, TEXT("DoorRightMesh"));
	UGVehicleModule* wheelModule = modules->FindModule(TEXT("Wheel_0"));
	UGVehicleModule* doorModule = modules->FindModule(TEXT("partDoor1"));
	const bool bHasMeshes = TestNotNull(TEXT("Body mesh"), bodyMesh)
		&& TestNotNull(TEXT("Front left wheel mesh"), wheelMesh)
		&& TestNotNull(TEXT("Right door mesh"), doorMesh);
	const bool bHasModules = TestNotNull(TEXT("Wheel_0 module"), wheelModule) && TestNotNull(TEXT("partDoor1 module"), doorModule);
	if (!bHasMeshes || !bHasModules)
	{
		world->DestroyWorld(false);
		return false;
	}

	TestTrue(TEXT("Every hologram part has a mesh asset"), bodyMesh->GetStaticMesh() && wheelMesh->GetStaticMesh() && doorMesh->GetStaticMesh());
	TestEqual(TEXT("Body glows dimmer"), GetCustomDataFloat(bodyMesh, 1), 0.5f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Module parts glow at full brightness"), GetCustomDataFloat(wheelMesh, 1), 1.0f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Untouched module parts glow at full brightness"), GetCustomDataFloat(doorMesh, 1), 1.0f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Intact wheel is green"), GetCustomDataFloat(wheelMesh, 0), 0.0f, KINDA_SMALL_NUMBER);

	car->DamageModule(TEXT("Wheel_0"), wheelModule->maxHP * 0.5f);
	TestEqual(TEXT("Half damaged wheel is halfway to red"), GetCustomDataFloat(wheelMesh, 0), 0.5f, KINDA_SMALL_NUMBER);

	car->DamageModule(TEXT("partDoor1"), doorModule->maxHP * 2.0f);
	TestEqual(TEXT("Destroyed door is red"), GetCustomDataFloat(doorMesh, 0), 1.0f, KINDA_SMALL_NUMBER);

	// ApplyRepair() needs a game state with repair parts, so restore the health the way replication does.
	wheelModule->ApplyReplicatedState(wheelModule->maxHP, wheelModule->maxHP, EGModuleState::Full);
	car->PublishModuleStates();
	TestEqual(TEXT("Repaired wheel is green again"), GetCustomDataFloat(wheelMesh, 0), 0.0f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Body is never damaged"), GetCustomDataFloat(bodyMesh, 0), 0.0f, KINDA_SMALL_NUMBER);

	world->DestroyWorld(false);
	return true;
}

#endif
