// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "GGIWhosDriving.generated.h"

// Event dispatcher declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGOnSessionsFound, const TArray<FString>&, sessionNames);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGOnJoiningFailed);


UCLASS()
class WHOS_DRIVING_API UGGIWhosDriving : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Session")
	void HostSession(FString sessionName);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions();

	UFUNCTION(BlueprintCallable, Category = "Session")
	bool JoinSessionByIndex(int32 sessionIndex);

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGOnSessionsFound OnSessionsFound;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGOnJoiningFailed OnJoiningFailed;

private:
	void OnCreateSessionComplete(FName sessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName sessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName sessionName, EOnJoinSessionCompleteResult::Type result);

	FDelegateHandle createSessionDelegateHandle;
	FDelegateHandle destroySessionDelegateHandle;
	FDelegateHandle findSessionsDelegateHandle;
	FDelegateHandle joinSessionDelegateHandle;
	TArray<FString> sessionsNames;
	TArray<int32> sessionIndices;
	TSharedPtr<FOnlineSessionSearch> sessionsFound;
};
