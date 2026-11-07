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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyChanged, int32, moneyCount);


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

	void PlusMoney(int32 amount);

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGOnSessionsFound OnSessionsFound;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGOnJoiningFailed OnJoiningFailed;

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

protected:
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
