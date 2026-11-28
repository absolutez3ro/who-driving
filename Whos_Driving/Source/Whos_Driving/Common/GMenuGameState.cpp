
#include "Common/GMenuGameState.h"
#include "GGIWhosDriving.h"
#include "GAudioManagerSubsystem.h"

void AGMenuGameState::BeginPlay()
{
	Super::BeginPlay();

	if (UGAudioManagerSubsystem* audioManager = GetGameInstance()->GetSubsystem<UGAudioManagerSubsystem>())
	{
		audioManager->PlayMusic();
	}
}

void AGMenuGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (UGAudioManagerSubsystem* audioManager = GetGameInstance()->GetSubsystem<UGAudioManagerSubsystem>())
	{
		audioManager->StopMusic();
	}
}
