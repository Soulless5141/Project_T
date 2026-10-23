// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EOSManagerSubsystem.generated.h"

// 前方宣言（必要なポインタ型のみを事前定義）
class IOnlineIdentity;
typedef TSharedPtr<class IOnlineIdentity, ESPMode::ThreadSafe> IOnlineIdentityPtr;
class FUniqueNetId;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEOSLoginComplete, bool, bWasSuccessful, const FString&, ErrorMessage);

UCLASS()
class PROJECT_T_API UEOSManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "EOS|Auth")
	void LoginToEOS(FString AuthType = "accountportal", FString Id = "", FString Token = "");

	UPROPERTY(BlueprintAssignable, Category = "EOS|Auth")
	FOnEOSLoginComplete OnEOSLoginComplete;

	UFUNCTION(BlueprintPure, Category = "EOS|Auth")
	bool IsLoggedIn() const;

private:
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	IOnlineIdentityPtr IdentityInterface;
};
