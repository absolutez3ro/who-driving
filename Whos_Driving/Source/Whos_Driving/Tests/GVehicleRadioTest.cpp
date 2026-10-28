#if WITH_DEV_AUTOMATION_TESTS

#include "GVehicleRadioPlaylist.h"
#include "GVehicleRadioComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGVehicleRadioPlaylistTest, "WhosDriving.Radio.ShuffleHistory",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGVehicleRadioPlaylistTest::RunTest(const FString& parameters)
{
	const TArray<FSoftObjectPath> tracks = {
		FSoftObjectPath(TEXT("/Game/Radio/A.A")), FSoftObjectPath(TEXT("/Game/Radio/B.B")),
		FSoftObjectPath(TEXT("/Game/Radio/C.C")), FSoftObjectPath(TEXT("/Game/Radio/D.D"))};
	FGVehicleRadioPlaylist playlist;
	playlist.Reset({});
	TestTrue(TEXT("Empty playlist is safe"), playlist.Next().IsNull() && playlist.Previous().IsNull());
	playlist.Reset(tracks);
	FSoftObjectPath previous;
	for (int32 cycle = 0; cycle < 100; ++cycle)
	{
		TSet<FSoftObjectPath> played;
		for (int32 index = 0; index < tracks.Num(); ++index)
		{
			const FSoftObjectPath current = playlist.Next();
			TestTrue(TEXT("Known track"), tracks.Contains(current));
			TestTrue(TEXT("No adjacent repeats, including cycle boundary"), previous != current);
			TestFalse(TEXT("Every track appears once per cycle"), played.Contains(current));
			played.Add(current);
			previous = current;
		}
	}
	const FSoftObjectPath current = playlist.Next();
	const FSoftObjectPath next = playlist.Next();
	TestEqual(TEXT("Previous returns actual history"), playlist.Previous(), current);
	TestEqual(TEXT("Next returns forward history"), playlist.Next(), next);
	playlist.Reset(tracks, current);
	TestEqual(TEXT("Resumed track stays in history"), playlist.Previous(), current);
	TestTrue(TEXT("Resumed track not immediately repeated"), playlist.Next() != current);
	playlist.Reset({tracks[0]});
	TestEqual(TEXT("Single track works"), playlist.Next(), tracks[0]);
	TestEqual(TEXT("Single track repeats safely"), playlist.Next(), tracks[0]);
	TestEqual(TEXT("Single track history"), playlist.Previous(), tracks[0]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGVehicleRadioEmptyTest, "WhosDriving.Radio.EmptyLibrary",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGVehicleRadioEmptyTest::RunTest(const FString& parameters)
{
	const UWorld::InitializationValues initialization = UWorld::InitializationValues()
															.AllowAudioPlayback(false)
															.CreatePhysicsScene(true)
															.CreateNavigation(false)
															.CreateAISystem(false)
															.ShouldSimulatePhysics(false);
	UWorld* world =
		UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &initialization);
	world->InitializeActorsForPlay(FURL());
	world->BeginPlay();
	world->SetBegunPlay(true);
	AActor* owner = world->SpawnActor<AActor>();
	UGVehicleRadioComponent* radio = NewObject<UGVehicleRadioComponent>(owner);
	radio->trackDirectory.Path = TEXT("/Game/Radio/NonexistentTestLibrary");
	owner->AddInstanceComponent(radio);
	radio->RegisterComponent();
	TestFalse(TEXT("Cannot skip while off"), radio->ExecuteCommand(EGVehicleRadioCommand::Next));
	TestFalse(TEXT("No tracks: toggle safely rejected"), radio->ExecuteCommand(EGVehicleRadioCommand::Toggle));
	TestFalse(TEXT("Empty radio stays off"), radio->GetRadioState().bEnabled);
	TestEqual(TEXT("Stopped position"), radio->GetPlaybackPosition(), 0.0f);
	world->EndPlay(EEndPlayReason::Quit);
	world->DestroyWorld(false);
	return true;
}

#endif
