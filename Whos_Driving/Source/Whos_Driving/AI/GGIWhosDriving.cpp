// Fill out your copyright notice in the Description page of Project Settings.


#include "GGIWhosDriving.h"

void UGGIWhosDriving::HostSession(FString sessionName)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Trying to create session..."));
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get();
	if (onlineSubsystem)
	{
		IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
		if (sessionInterface.IsValid())
		{
			// Handle to track when the session is done creating
			createSessionDelegateHandle = sessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
				FOnCreateSessionCompleteDelegate::CreateUObject(this, &UGGIWhosDriving::OnCreateSessionComplete));

			FOnlineSessionSettings sessionSettings;
			sessionSettings.bIsLANMatch = true;
			sessionSettings.bShouldAdvertise = true;
			sessionSettings.NumPublicConnections = 2;
			FOnlineSessionSetting serverNameSetting;
			serverNameSetting.Data.SetValue(sessionName);
			// MANDATORY: This makes the setting visible to clients during search
			serverNameSetting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;
			// Add to the settings map using a custom FName key
			sessionSettings.Settings.Add(FName(TEXT("CUSTOM_SERVER_NAME")), serverNameSetting);
			if (!sessionInterface->CreateSession(0, NAME_GameSession, sessionSettings))
			{
				sessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(createSessionDelegateHandle);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to start session creation request!"));
			}
		}
	}
}

void UGGIWhosDriving::OnCreateSessionComplete(FName sessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get();
	if (onlineSubsystem)
	{
		IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
		if (sessionInterface.IsValid())
		{
			// 1. CLEAR THE DELEGATE (Crucial for cleanup)
			sessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(createSessionDelegateHandle);
		}
	}

	// 2. CHECK THE RESULT
	if (bWasSuccessful)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Session created!"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Session failed to create!"));
	}
}


void UGGIWhosDriving::DestroySession()
{
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get();
	if (onlineSubsystem)
	{
		IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
		if (sessionInterface.IsValid())
		{
			destroySessionDelegateHandle = sessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
				FOnDestroySessionCompleteDelegate::CreateUObject(this, &UGGIWhosDriving::OnDestroySessionComplete));

			if (!sessionInterface->DestroySession(NAME_GameSession))
			{
				sessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(destroySessionDelegateHandle);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("No session to destroy!"));
			}
		}
	}
}

void UGGIWhosDriving::OnDestroySessionComplete(FName sessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get();
	if (onlineSubsystem)
	{
		IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
		if (sessionInterface.IsValid())
		{
			sessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(destroySessionDelegateHandle);
		}
	}

	if (bWasSuccessful)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Session eliminated!"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Session NOT eliminated!"));
	}
}


void UGGIWhosDriving::FindSessions()
{
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get();
	if (onlineSubsystem)
	{
		IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
		if (sessionInterface.IsValid())
		{
			// 1. CREATE AND CONFIGURE SEARCH
			sessionsFound = MakeShareable(new FOnlineSessionSearch());
			sessionsFound->bIsLanQuery = true;
			sessionsFound->MaxSearchResults = 8;
			// 2. BIND DELEGATE
			findSessionsDelegateHandle = sessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
				FOnFindSessionsCompleteDelegate::CreateUObject(this, &UGGIWhosDriving::OnFindSessionsComplete));
			if (!sessionInterface->FindSessions(0, sessionsFound.ToSharedRef()))
			{
				sessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(findSessionsDelegateHandle);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to start searching for sessions!"));
			}
		}
	}
}

void UGGIWhosDriving::OnFindSessionsComplete(bool bWasSuccessful)
{
	sessionsNames.Empty();
	sessionIndices.Empty();
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get();
	if (onlineSubsystem)
	{
		IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
		if (sessionInterface.IsValid())
		{
			sessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(findSessionsDelegateHandle);
		}
	}
	if (bWasSuccessful)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Sessions found!"));
		int32 sessionIndex = 0;
		for (const FOnlineSessionSearchResult& searchResult : sessionsFound->SearchResults)
		{
			FString sessionName = TEXT("random_ahh_session");
			const FName keyName = FName(TEXT("CUSTOM_SERVER_NAME"));
			// 2. Look for this key in the result's settings map
			const FOnlineSessionSetting* setting = searchResult.Session.SessionSettings.Settings.Find(keyName);
			if (setting)
			{
				// 3. Extract the data into our FString variable
				setting->Data.GetValue(sessionName);
			}
			sessionIndices.Add(sessionIndex);
			sessionIndex++;
			sessionsNames.Add(sessionName);
		}


		OnSessionsFound.Broadcast(sessionsNames);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Sessions NOT found!"));
	}

}


bool UGGIWhosDriving::JoinSessionByIndex(int32 sessionIndex)
{
	if (sessionsFound.IsValid())
	{
		const TArray<FOnlineSessionSearchResult>& searchResults = sessionsFound->SearchResults;

		if (!searchResults.IsValidIndex(sessionIndex))
		{
			return false;
		}

		IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get();
		if (!onlineSubsystem)
		{
			return false;
		}

		IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
		if (!sessionInterface.IsValid())
		{
			return false;
		}


		joinSessionDelegateHandle = sessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
			FOnJoinSessionCompleteDelegate::CreateUObject(this, &UGGIWhosDriving::OnJoinSessionComplete));
		if (!sessionInterface->JoinSession(0, NAME_GameSession, searchResults[sessionIndex]))
		{
			sessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(joinSessionDelegateHandle);
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Session could not be joined to!"));
			return false;
		}
		return true;
	}
	return false;
}

void UGGIWhosDriving::OnJoinSessionComplete(FName sessionName, EOnJoinSessionCompleteResult::Type result)
{
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get();
	IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
	if (onlineSubsystem)
	{
		if (sessionInterface.IsValid())
		{
			sessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(joinSessionDelegateHandle);
		}
	}
	if (result == EOnJoinSessionCompleteResult::Success)
	{
		FString connectString;

		if (sessionInterface->GetResolvedConnectString(sessionName, connectString))
		{
			APlayerController* playerController = GetFirstLocalPlayerController();

			if (playerController)
			{
				playerController->ClientTravel(connectString, TRAVEL_Absolute);
			}
		}
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Session joined!"));
	}
	else
	{
		OnJoiningFailed.Broadcast();
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Session NOT joined!"));
	}
}
