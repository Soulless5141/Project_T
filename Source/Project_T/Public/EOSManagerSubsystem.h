// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EOSManagerSubsystem.generated.h"

// イベント通知用デリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEOSLoginComplete, bool, bWasSuccessful, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEOSCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEOSFindSessionsComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEOSJoinSessionComplete, bool, bWasSuccessful, const FString&, ConnectString);

UCLASS()
class PROJECT_T_API UEOSManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// --- 認証機能 ---
	UFUNCTION(BlueprintCallable, Category = "EOS|Auth")
	void LoginToEOS(FString AuthType = "accountportal", FString Id = "", FString Token = "");

	UPROPERTY(BlueprintAssignable, Category = "EOS|Auth")
	FOnEOSLoginComplete OnEOSLoginComplete;

	UFUNCTION(BlueprintPure, Category = "EOS|Auth")
	bool IsLoggedIn() const;

	// --- セッション / マッチング機能 ---
	// 4人対戦セッションを作成（Host）
	UFUNCTION(BlueprintCallable, Category = "EOS|Session")
	void CreateEOSSession(int32 MaxPublicConnections = 4, bool bIsLAN = false);

	// セッションを検索（Client）
	UFUNCTION(BlueprintCallable, Category = "EOS|Session")
	void FindEOSSessions();

	// セッションに参加（Client）
	UFUNCTION(BlueprintCallable, Category = "EOS|Session")
	void JoinEOSSession(int32 SessionIndex);

	UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
	FOnEOSCreateSessionComplete OnEOSCreateSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
	FOnEOSFindSessionsComplete OnEOSFindSessionsComplete;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
	FOnEOSJoinSessionComplete OnEOSJoinSessionComplete;

private:
	// コールバック関数
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const class FUniqueNetId& UserId, const FString& Error);
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, uint8 ResultType);

	// OSSのインターフェース保持用ポインタ（ヘッダーでの型衝突を避ける安全設計）
	void* RawIdentityInterface = nullptr;
	void* RawSessionInterface = nullptr;

	// セッション検索結果の保持
	TSharedPtr<class FOnlineSessionSearch> LastSessionSearch;
};