// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "GGIWhosDriving.generated.h"

/**
 *

 */

 //event dispatcher declaration
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionsFound, const TArray<FString>&, SessionNames);


UCLASS()
class WHOS_DRIVING_API UGGIWhosDriving : public UGameInstance
{
	GENERATED_BODY()
private:
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	FDelegateHandle createSessionDelegateHandle;

	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	FDelegateHandle destroySessionDelegateHandle;

	void OnFindSessionsComplete(bool bWasSuccessful);
	FDelegateHandle findSessionsDelegateHandle;
	TArray<FString> sessionsNames;
	TArray<int32> sessionIndices;
	TSharedPtr<FOnlineSessionSearch> sessionsFound;
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	FDelegateHandle joinSessionDelegateHandle;
public:
	UPROPERTY(BlueprintAssignable, Category = "Session")
	FOnSessionsFound OnSessionsFound;
	UFUNCTION(BlueprintCallable, Category = "Session")
	void HostSession(FString sessionName);
	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();
	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions();
	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinSessionByIndex(int32 ind);
};
