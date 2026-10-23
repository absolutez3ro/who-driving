// Fill out your copyright notice in the Description page of Project Settings.


#include "GGIWhosDriving.h"

void UGGIWhosDriving::HostSession(FString sessionName)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Trying to create session..."));
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
    if (Subsystem)
    {
        IOnlineSessionPtr sessionInterface = Subsystem->GetSessionInterface();
        if (sessionInterface.IsValid())
        {
            //handle to track when session is done creating
            createSessionDelegateHandle = sessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
                FOnCreateSessionCompleteDelegate::CreateUObject(this, &UGGIWhosDriving::OnCreateSessionComplete));

            FOnlineSessionSettings SessionSettings;
            SessionSettings.bIsLANMatch = true;
            SessionSettings.bShouldAdvertise = true;
            SessionSettings.NumPublicConnections = 2;
            FOnlineSessionSetting ServerNameSetting;
            ServerNameSetting.Data.SetValue(sessionName);
            // MANDATORY: This makes the setting visible to clients during search
            ServerNameSetting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;
            // Add to the settings map using a custom FName key
            SessionSettings.Settings.Add(FName("CUSTOM_SERVER_NAME"), ServerNameSetting);
            if (!sessionInterface->CreateSession(0, NAME_GameSession, SessionSettings))
            {
                sessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(createSessionDelegateHandle);
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to start session creation request!"));
            }
        }
    }
}
void UGGIWhosDriving::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (OSS)
    {
        IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
        if (Sessions.IsValid())
        {
            // 1. CLEAR THE DELEGATE (Crucial for cleanup)
            Sessions->ClearOnCreateSessionCompleteDelegate_Handle(createSessionDelegateHandle);
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
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (OSS)
    {
        IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
        if (Sessions.IsValid())
        {
            destroySessionDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
                FOnDestroySessionCompleteDelegate::CreateUObject(this, &UGGIWhosDriving::OnDestroySessionComplete));

            if (!Sessions->DestroySession(NAME_GameSession))
            {
                Sessions->ClearOnDestroySessionCompleteDelegate_Handle(destroySessionDelegateHandle);
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("No session to destroy!"));
            }
        }
    }
}
void UGGIWhosDriving::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (OSS)
    {
        IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
        if (Sessions.IsValid())
        {
            Sessions->ClearOnDestroySessionCompleteDelegate_Handle(destroySessionDelegateHandle);
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
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (OSS)
    {
        IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
        if (Sessions.IsValid())
        {
            // 1. CREATE AND CONFIGURE SEARCH
            sessionsFound = MakeShareable(new FOnlineSessionSearch());
            sessionsFound->bIsLanQuery = true;
            sessionsFound->MaxSearchResults = 8;
            // 2. BIND DELEGATE
            findSessionsDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
                FOnFindSessionsCompleteDelegate::CreateUObject(this, &UGGIWhosDriving::OnFindSessionsComplete));
            if (!Sessions->FindSessions(0, sessionsFound.ToSharedRef()))
            {
                Sessions->ClearOnFindSessionsCompleteDelegate_Handle(findSessionsDelegateHandle);
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to start searching for sessions!"));
            }
        }
    }
}
void UGGIWhosDriving::OnFindSessionsComplete(bool bWasSuccessful)
{
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (OSS)
    {
        IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
        if (Sessions.IsValid())
        {
            Sessions->ClearOnFindSessionsCompleteDelegate_Handle(findSessionsDelegateHandle);
        }
    }
    if (bWasSuccessful) {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Sessions found!"));
        int32 ind = 0;
        for (const FOnlineSessionSearchResult& Result : sessionsFound->SearchResults)
        {
            FString sessionName = "random_ahh_session";
            FName KeyName = FName("CUSTOM_SERVER_NAME");
            // 2. Look for this key in the Result's settings map
            const FOnlineSessionSetting* Setting = Result.Session.SessionSettings.Settings.Find(KeyName);
            if (Setting)
            {
                // 3. Extract the data into our FString variable
                Setting->Data.GetValue(sessionName);
            }
            sessionIndices.Add(ind);
            ind++;
            sessionsNames.Add(sessionName);
        }


        OnSessionsFound.Broadcast(sessionsNames);
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Sessions NOT found!"));
    }

}


bool UGGIWhosDriving::JoinSessionByIndex(int32 ind)
{
    if (sessionsFound.IsValid()) {
        const auto& Results = sessionsFound->SearchResults;

        if (!Results.IsValidIndex(ind))
            return false;

        IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
        if (!OSS)
            return false;

        IOnlineSessionPtr sessionInterface = OSS->GetSessionInterface();
        if (!sessionInterface.IsValid())
            return false;


        joinSessionDelegateHandle = sessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
            FOnJoinSessionCompleteDelegate::CreateUObject(this, &UGGIWhosDriving::OnJoinSessionComplete));
        if (!sessionInterface->JoinSession(0, NAME_GameSession, Results[ind]))
        {
            sessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(joinSessionDelegateHandle);
            return false;
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Session could not be joined to!"));
        }
        return true;
    }
    return false;
}
void UGGIWhosDriving::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    IOnlineSessionPtr SessionInterface = OSS->GetSessionInterface();
    if (OSS)
    {
        if (SessionInterface.IsValid())
        {
            SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(joinSessionDelegateHandle);
        }
    }
    if (Result == EOnJoinSessionCompleteResult::Success)
    {
        /**/
        FString ConnectString;

        if (SessionInterface->GetResolvedConnectString(
            SessionName, ConnectString))
        {
            
            APlayerController* PC = GetFirstLocalPlayerController();

            if (PC)
            {
                PC->ClientTravel(ConnectString, TRAVEL_Absolute);
            }
        }
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Session joined!"));
    }
    else
    {
        FOnJoiningFailed.Broadcast();
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Session NOT joined!"));
    }
}