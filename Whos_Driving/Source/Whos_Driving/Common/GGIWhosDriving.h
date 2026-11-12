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
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInvitationAccepted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyChanged, int32, moneyCount);

class UGEconomicDataAsset;

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
	void Init() override;

	UFUNCTION(BlueprintCallable, Category = "Session")
	void HostSessionLAN(FString sessionName);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void HostSessionSteam(FString sessionName, bool openUIAfter);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions();

	UFUNCTION(BlueprintCallable, Category = "Session")
	bool JoinSessionByIndex(int32 sessionIndex, int32 type);

	void PlusMoney(int32 amount);

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGOnSessionsFound OnSessionsFound;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGOnJoiningFailed OnJoiningFailed;

	UFUNCTION(BlueprintCallable, Category = "Session")
	void ShowSteamInviteUI();

	virtual void Init() override;
	virtual void Shutdown() override;
	FDelegateHandle inviteAcceptedDelegateHandle;
	FOnInvitationAccepted OnInvitationAccepted;
	void OnSessionUserInviteAccepted(const bool bWasSuccessful, const int32 controllerId, FUniqueNetIdPtr userId, const FOnlineSessionSearchResult& inviteResult);

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void PrintNetDriver();
	UPROPERTY(BlueprintAssignable, Category = "Resources")
	FOnMoneyChanged OnMoneyChanged;

	UFUNCTION(BlueprintCallable, Category = "Economic")
	void CalculateMoneyOnFinish();

	UFUNCTION()
	void HandleRaceFinish();

	int32 GetMoneyFromTotal() const;
	int32 GetMoneyFromKills() const;
	int32 GetMoneyFromRepairParts() const;
	int32 GetMoneyFromTime() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TObjectPtr<UGEconomicDataAsset> economicData;

protected:
	void HandleNetworkFailure(UWorld* world, UNetDriver* netDriver, ENetworkFailure::Type failureType, const FString& errorString);
	void HandleTravelFailure(UWorld* world, ETravelFailure::Type failureType, const FString& errorString);
	void LoadEconomicData();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economic")
	int32 moneyCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economic")
	int32 enemiesKillsToMoneyConversionRate = 200;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economic")
	int32 partsToMoneyConversionRate = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economic")
	float timeBonusMultiplier = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Economic")
	int32 moneyFromKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Economic")
	int32 moneyFromRepairParts = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Economic")
	int32 moneyFromTime = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Economic")
	int32 moneyFromTotal = 0;
private:
	//destroy func needs to know if player destroy while joining by invitation or on his own
	bool currentlyJoining = false;
	bool openUIAfterHosting = false;
	FOnlineSessionSearchResult pendingInviteResult;
	FString GetNetDriverInfo() const;
	void FindLANSessions();
	void FindSteamSessions();
	void OnCreateSessionComplete(FName sessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName sessionName, bool bWasSuccessful);
	void OnDestroySessionBeforJoiningComplete(FName sessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnFindSteamSessionsComplete(bool bWasSuccessful);
	bool JoinSession(const FOnlineSessionSearchResult& sessionToJoin);
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
