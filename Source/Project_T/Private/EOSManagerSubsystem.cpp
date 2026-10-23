// Fill out your copyright notice in the Description page of Project Settings.


#include "EOSManagerSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h" // ← .cpp 側でインクルードする！

void UEOSManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld(), EOS_SUBSYSTEM);
	if (Subsystem)
	{
		IdentityInterface = Subsystem->GetIdentityInterface();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EOSManager] Failed to get EOS OnlineSubsystem!"));
	}
}

void UEOSManagerSubsystem::LoginToEOS(FString AuthType, FString Id, FString Token)
{
	if (!IdentityInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[EOSManager] IdentityInterface is invalid!"));
		OnEOSLoginComplete.Broadcast(false, TEXT("IdentityInterface is invalid"));
		return;
	}

	if (IsLoggedIn())
	{
		UE_LOG(LogTemp, Log, TEXT("[EOSManager] Already logged in."));
		OnEOSLoginComplete.Broadcast(true, TEXT(""));
		return;
	}

	IdentityInterface->AddOnLoginCompleteDelegate_Handle(
		0,
		FOnLoginCompleteDelegate::CreateUObject(this, &UEOSManagerSubsystem::OnLoginComplete)
	);

	FOnlineAccountCredentials Credentials;
	Credentials.Type = AuthType;
	Credentials.Id = Id;
	Credentials.Token = Token;

	UE_LOG(LogTemp, Log, TEXT("[EOSManager] Starting EOS Login with Type: %s..."), *AuthType);

	IdentityInterface->Login(0, Credentials);
}

void UEOSManagerSubsystem::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("[EOSManager] EOS Login Successful! User ID: %s"), *UserId.ToString());
		OnEOSLoginComplete.Broadcast(true, TEXT(""));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EOSManager] EOS Login Failed! Error: %s"), *Error);
		OnEOSLoginComplete.Broadcast(false, Error);
	}
}

bool UEOSManagerSubsystem::IsLoggedIn() const
{
	if (IdentityInterface.IsValid())
	{
		ELoginStatus::Type Status = IdentityInterface->GetLoginStatus(0);
		return Status == ELoginStatus::LoggedIn;
	}
	return false;
}

