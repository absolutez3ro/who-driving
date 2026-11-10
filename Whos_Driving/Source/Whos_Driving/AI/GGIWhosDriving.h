// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/OnlineSessionNames.h"
#include "GGIWhosDriving.generated.h"

// Event dispatcher declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGOnSessionsFound, const TArray<FString>&, sessionNames, const TArray<int32>&, sessionTypes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGOnJoiningFailed);

UENUM(BlueprintType)
enum class ESessionType : uint8
{
	LAN    UMETA(DisplayName = "LAN"),
	Steam  UMETA(DisplayName = "Steam")
};

UCLASS()
class WHOS_DRIVING_API UGGIWhosDriving : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Session")
	void HostSessionLAN(FString sessionName);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void HostSessionSteam(FString sessionName);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions();

	UFUNCTION(BlueprintCallable, Category = "Session")
	bool JoinSessionByIndex(int32 sessionIndex, int32 type);

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGOnSessionsFound OnSessionsFound;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGOnJoiningFailed OnJoiningFailed;

	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void PrintNetDriver();
protected:
	void HandleNetworkFailure(UWorld* world, UNetDriver* netDriver, ENetworkFailure::Type failureType, const FString& errorString);
	void HandleTravelFailure(UWorld* world, ETravelFailure::Type failureType, const FString& errorString);
private:
	FString GetNetDriverInfo() const;
	void FindLANSessions();
	void FindSteamSessions();
	void OnCreateSessionComplete(FName sessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName sessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnFindSteamSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName sessionName, EOnJoinSessionCompleteResult::Type result);
	FName mySessionSystem= FName(TEXT("NULL"));
	FDelegateHandle createSessionDelegateHandle;
	FDelegateHandle destroySessionDelegateHandle;
	FDelegateHandle findLANSessionsDelegateHandle;
	FDelegateHandle findSteamSessionsDelegateHandle;
	FDelegateHandle joinSessionDelegateHandle;
	TArray<FString> sessionsNames;
	TArray<int32> sessionIndices;
	TArray<int32> sessionTypes;
	TSharedPtr<FOnlineSessionSearch> LANSessionsFound;
	TSharedPtr<FOnlineSessionSearch> SteamSessionsFound;
};
