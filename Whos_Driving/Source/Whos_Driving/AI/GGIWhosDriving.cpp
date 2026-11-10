// Fill out your copyright notice in the Description page of Project Settings.


#include "GGIWhosDriving.h"
void UGGIWhosDriving::HostSessionLAN(FString sessionName)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Trying to create session..."));
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(FName(TEXT("NULL")));
	mySessionSystem = FName(TEXT("NULL"));
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
			FOnlineSessionSetting serverTypeSetting;
			sessionName += " (LAN)";
			serverNameSetting.Data.SetValue(sessionName);
			serverTypeSetting.Data.SetValue(static_cast<int32>(ESessionType::LAN));
			// MANDATORY: This makes the setting visible to clients during search
			serverNameSetting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;
			serverTypeSetting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;
			// Add to the settings map using a custom FName key
			sessionSettings.Settings.Add(FName(TEXT("CUSTOM_SERVER_NAME")), serverNameSetting);
			sessionSettings.Settings.Add(FName(TEXT("CUSTOM_SERVER_TYPE")), serverTypeSetting);
			if (!sessionInterface->CreateSession(0, NAME_GameSession, sessionSettings))
			{
				sessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(createSessionDelegateHandle);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to start session creation request!"));
			}
		}
	}
}

void UGGIWhosDriving::HostSessionSteam(FString sessionName)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Trying to create session..."));
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(FName(TEXT("Steam")));
	mySessionSystem = FName(TEXT("Steam"));
	if (onlineSubsystem)
	{
		IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
		if (sessionInterface.IsValid())
		{
			// Handle to track when the session is done creating
			createSessionDelegateHandle = sessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
				FOnCreateSessionCompleteDelegate::CreateUObject(this, &UGGIWhosDriving::OnCreateSessionComplete));

			

			FOnlineSessionSettings sessionSettings;
			sessionSettings.bIsLANMatch = false;
			sessionSettings.bShouldAdvertise = true;
			sessionSettings.bUsesPresence = true;   // Required for Steam listen servers
			sessionSettings.bUseLobbiesIfAvailable = true;   // Use Steam lobbies
			sessionSettings.bAllowJoinInProgress = true;
			sessionSettings.bAllowJoinViaPresence = true;   // Join via Steam friends list / overlay
			sessionSettings.bIsDedicated = false;
			sessionSettings.NumPublicConnections = 2;
			FOnlineSessionSetting serverNameSetting;
			FOnlineSessionSetting serverTypeSetting;
			sessionName += " (Steam)";
			serverNameSetting.Data.SetValue(sessionName);
			serverTypeSetting.Data.SetValue(static_cast<int32>(ESessionType::Steam));
			// MANDATORY: This makes the setting visible to clients during search
			serverNameSetting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;
			serverTypeSetting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;
			// Add to the settings map using a custom FName key
			sessionSettings.Settings.Add(FName(TEXT("CUSTOM_SERVER_NAME")), serverNameSetting);
			sessionSettings.Settings.Add(FName(TEXT("CUSTOM_SERVER_TYPE")), serverTypeSetting);
			
			// Steam needs the hosting player's unique net id
			bool bStarted = false;
			const ULocalPlayer* localPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
			if (!localPlayer || !localPlayer->GetPreferredUniqueNetId().IsValid())
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("No valid local player / net id (is Steam running?)"));
				bStarted = sessionInterface->CreateSession(0, NAME_GameSession, sessionSettings);

			}else
			{
				bStarted = sessionInterface->CreateSession(*localPlayer->GetPreferredUniqueNetId(), NAME_GameSession, sessionSettings);
			}

			if (!bStarted)
			{
				sessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(createSessionDelegateHandle);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to start session creation request!"));
			}
		}
	}
}

void UGGIWhosDriving::OnCreateSessionComplete(FName sessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(mySessionSystem);
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
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(mySessionSystem);
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
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(mySessionSystem);
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
	// OnComplete of find steam calls find lan and OnComplete of lan search calls OnFindSessionsComplete
	FindSteamSessions();
}

void UGGIWhosDriving::FindSteamSessions()
{
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(FName(TEXT("Steam")));
	if (onlineSubsystem)
	{
		IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
		if (sessionInterface.IsValid())
		{
			// 1. CREATE AND CONFIGURE SEARCH
			SteamSessionsFound = MakeShareable(new FOnlineSessionSearch());
			SteamSessionsFound->bIsLanQuery = false;
			SteamSessionsFound->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
			SteamSessionsFound->MaxSearchResults = 10000;
			// 2. BIND DELEGATE
			findSteamSessionsDelegateHandle = sessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
				FOnFindSessionsCompleteDelegate::CreateUObject(this, &UGGIWhosDriving::OnFindSteamSessionsComplete));

			bool bStarted = false;
			const ULocalPlayer* localPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
			if (localPlayer && localPlayer->GetPreferredUniqueNetId().IsValid())
			{
				bStarted = sessionInterface->FindSessions(*localPlayer->GetPreferredUniqueNetId(), SteamSessionsFound.ToSharedRef());
			}
			else
			{
				bStarted = sessionInterface->FindSessions(0, SteamSessionsFound.ToSharedRef());
			}
			// if starting failed unbind
			if (!bStarted)
			{
				sessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(findSteamSessionsDelegateHandle);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to start searching for sessions!"));
			}
		}
	}
}

void UGGIWhosDriving::FindLANSessions()
{
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(FName(TEXT("NULL")));
	if (onlineSubsystem)
	{
		IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
		if (sessionInterface.IsValid())
		{
			// 1. CREATE AND CONFIGURE SEARCH
			LANSessionsFound = MakeShareable(new FOnlineSessionSearch());
			LANSessionsFound->bIsLanQuery = true;
			LANSessionsFound->MaxSearchResults = 8;
			// 2. BIND DELEGATE
			findLANSessionsDelegateHandle = sessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
				FOnFindSessionsCompleteDelegate::CreateUObject(this, &UGGIWhosDriving::OnFindSessionsComplete));
			if (!sessionInterface->FindSessions(0, LANSessionsFound.ToSharedRef()))
			{
				sessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(findLANSessionsDelegateHandle);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to start searching for sessions!"));
			}
		}
	}
}

void UGGIWhosDriving::OnFindSteamSessionsComplete(bool bWasSuccessful)
{
	sessionsNames.Empty();
	sessionIndices.Empty();
	sessionTypes.Empty();
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(FName(TEXT("Steam")));
	if (onlineSubsystem)
	{
		IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
		if (sessionInterface.IsValid())
		{
			sessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(findSteamSessionsDelegateHandle);
		}
	}
	if (bWasSuccessful)
	{
		int32 amount= SteamSessionsFound->SearchResults.Num();
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::Printf(TEXT("steam sessions found: %d"), amount));
		int32 sessionIndex = 0;
		for (const FOnlineSessionSearchResult& searchResult : SteamSessionsFound->SearchResults)
		{
			FString sessionName = TEXT("random_ahh_session");
			int32 sessionType = static_cast<int32>(ESessionType::LAN);
			const FName keyName = FName(TEXT("CUSTOM_SERVER_NAME"));
			const FName keyType = FName(TEXT("CUSTOM_SERVER_TYPE"));
			// 2. Look for this key in the result's settings map
			const FOnlineSessionSetting* settingName = searchResult.Session.SessionSettings.Settings.Find(keyName);
			const FOnlineSessionSetting* settingType = searchResult.Session.SessionSettings.Settings.Find(keyType);
			if (settingName && settingType)
			{
				// 3. Extract the data into our FString variable
				settingName->Data.GetValue(sessionName);
				settingType->Data.GetValue(sessionType);
				sessionIndices.Add(sessionIndex);
				sessionsNames.Add(sessionName);
				sessionTypes.Add(sessionType);
			}
			else
			{
				bool nameOK = (settingName == nullptr);
				bool typeOK = (settingType == nullptr);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Steam Session found! settingName %d settingType %d"), nameOK, typeOK));
			}
			sessionIndex++;
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Steam Sessions NOT found!"));
	}
	FindLANSessions();
}

void UGGIWhosDriving::OnFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(FName(TEXT("NULL")));
	if (onlineSubsystem)
	{
		IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
		if (sessionInterface.IsValid())
		{
			sessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(findLANSessionsDelegateHandle);
		}
	}
	if (bWasSuccessful)
	{
		int32 amount = LANSessionsFound->SearchResults.Num();
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, FString::Printf(TEXT("lan sessions found: %d"), amount));

		int32 sessionIndex = 0;
		for (const FOnlineSessionSearchResult& searchResult : LANSessionsFound->SearchResults)
		{
			const FOnlineSession& Session = searchResult.Session;

			FString ownerName = Session.OwningUserName;

			FString ownerId = Session.OwningUserId.IsValid()
				? Session.OwningUserId->ToString()
				: TEXT("<INVALID>");

			FString sessionId = searchResult.GetSessionIdStr();
			GEngine->AddOnScreenDebugMessage(
				-1,
				10.f,
				FColor::Magenta,
				FString::Printf(
					TEXT("Using Online Subsystem: %s"),
					*onlineSubsystem->GetSubsystemName().ToString()
				)
			);
			GEngine->AddOnScreenDebugMessage(
				-1,
				10.f,
				FColor::Yellow,
				FString::Printf(
					TEXT("Session ID: %s | Owner: '%s' | Owner ID: %s | Valid: %d | Info Valid: %d | LAN: %d"),
					*sessionId,
					*ownerName,
					*ownerId,
					searchResult.IsValid(),
					searchResult.IsSessionInfoValid(),
					Session.SessionSettings.bIsLANMatch
				)
			);

			// Print every setting actually received.
			for (const TPair<FName, FOnlineSessionSetting>& Pair :
				Session.SessionSettings.Settings)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					10.f,
					FColor::Cyan,
					FString::Printf(
						TEXT("Setting: %s | Value: %s | Advertisement: %d"),
						*Pair.Key.ToString(),
						*Pair.Value.Data.ToString(),
						static_cast<int32>(Pair.Value.AdvertisementType)
					)
				);
			}
			FString sessionName = TEXT("random_ahh_session");
			int32 sessionType = static_cast<int32>(ESessionType::LAN);
			const FName keyName = FName(TEXT("CUSTOM_SERVER_NAME"));
			const FName keyType = FName(TEXT("CUSTOM_SERVER_TYPE"));
			// 2. Look for this key in the result's settings map
			const FOnlineSessionSetting* settingName = searchResult.Session.SessionSettings.Settings.Find(keyName);
			const FOnlineSessionSetting* settingType = searchResult.Session.SessionSettings.Settings.Find(keyType);
			if (settingName && settingType)
			{
				// 3. Extract the data into our FString variable
				settingName->Data.GetValue(sessionName);
				settingType->Data.GetValue(sessionType);
				sessionIndices.Add(sessionIndex);
				sessionsNames.Add(sessionName);
				sessionTypes.Add(sessionType);
			}
			else
			{
				bool nameOK = (settingName == nullptr);
				bool typeOK = (settingType == nullptr);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Lan Sessions found! settingName %d settingType %d"), nameOK, typeOK));
			}
			sessionIndex++;
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Sessions NOT found!"));
	}
		OnSessionsFound.Broadcast(sessionsNames, sessionTypes);
}


bool UGGIWhosDriving::JoinSessionByIndex(int32 sessionIndex, int32 type)
{
	TSharedPtr<FOnlineSessionSearch> sessions;
	FName subsystemToUse;
	if (sessionsNames.Num()!= sessionIndices.Num() || sessionsNames.Num() != sessionTypes.Num())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("sessionsNames, sessionsIndices and sessionsTypes do NOT have same size"));
		return false;
	}
	if (type == static_cast<int32>(ESessionType::LAN)) 
	{
		mySessionSystem = FName(TEXT("NULL"));
		sessions = LANSessionsFound;
	}
	else
	{
		mySessionSystem = FName(TEXT("Steam"));
		sessions = SteamSessionsFound;
	}
	if (sessions.IsValid())
	{
		const TArray<FOnlineSessionSearchResult>& searchResults = sessions->SearchResults;
		if (!sessionIndices.IsValidIndex(sessionIndex))
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("sessionIndex too big"));
			return false;
		}
		if (!searchResults.IsValidIndex(sessionIndices[sessionIndex]))
		{
			return false;
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("!searchResults.IsValidIndex(sessionIndices[sessionIndex])"));
		}

		IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(mySessionSystem);
		if (!onlineSubsystem)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("ONLINE SUBSYSTEM INVALID"));
			return false;
		}

		IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
		if (!sessionInterface.IsValid())
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("SESSION INTERFACE INVALID"));
			return false;
		}


		joinSessionDelegateHandle = sessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
			FOnJoinSessionCompleteDelegate::CreateUObject(this, &UGGIWhosDriving::OnJoinSessionComplete));

		bool bStarted = false;
		const ULocalPlayer* localPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
		if (!localPlayer || !localPlayer->GetPreferredUniqueNetId().IsValid())
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("No valid local player / net id (is Steam running?)"));
			bStarted = sessionInterface->JoinSession(0, NAME_GameSession, searchResults[sessionIndices[sessionIndex]]);
		}
		else
		{
			bStarted = sessionInterface->JoinSession(*localPlayer->GetPreferredUniqueNetId(), NAME_GameSession, searchResults[sessionIndices[sessionIndex]]);
		}
		if (!bStarted)
		{
			sessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(joinSessionDelegateHandle);
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Session could not be joined to!"));
			return false;
		}
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::White, TEXT("JOINING"));
		return true;
	}
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("SESSION FOUND INVALID"));
	return false;
}

void UGGIWhosDriving::OnJoinSessionComplete(FName sessionName, EOnJoinSessionCompleteResult::Type result)
{
	IOnlineSubsystem* onlineSubsystem = IOnlineSubsystem::Get(mySessionSystem);
	if (!onlineSubsystem)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("ONLINE SUBSISTEM INVALID!"));
		return;
	}


	IOnlineSessionPtr sessionInterface = onlineSubsystem->GetSessionInterface();
	if (sessionInterface.IsValid())
	{
		sessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(joinSessionDelegateHandle);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("sessionInterface INVALID!"));
		return;
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
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, TEXT("Session joined!"));
	}
	else
	{
		OnJoiningFailed.Broadcast();
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Session NOT joined!"));
	}
}
