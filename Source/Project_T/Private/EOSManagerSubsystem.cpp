// Fill out your copyright notice in the Description page of Project Settings.

#include "EOSManagerSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"

void UEOSManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld(), EOS_SUBSYSTEM);
	if (Subsystem)
	{
		IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface();
		RawIdentityInterface = IdentityInterface.Get();

		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		RawSessionInterface = SessionInterface.Get();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EOSManager] Failed to get EOS OnlineSubsystem!"));
	}
}

// --- ログイン処理 ---
void UEOSManagerSubsystem::LoginToEOS(FString AuthType, FString Id, FString Token)
{
	IOnlineIdentity* Identity = static_cast<IOnlineIdentity*>(RawIdentityInterface);
	if (!Identity)
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

	Identity->AddOnLoginCompleteDelegate_Handle(
		0,
		FOnLoginCompleteDelegate::CreateUObject(this, &UEOSManagerSubsystem::OnLoginComplete)
	);

	FOnlineAccountCredentials Credentials;
	Credentials.Type = AuthType;
	Credentials.Id = Id;
	Credentials.Token = Token;

	UE_LOG(LogTemp, Log, TEXT("[EOSManager] Starting EOS Login with Type: %s..."), *AuthType);
	Identity->Login(0, Credentials);
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
	IOnlineIdentity* Identity = static_cast<IOnlineIdentity*>(RawIdentityInterface);
	if (Identity)
	{
		ELoginStatus::Type Status = Identity->GetLoginStatus(0);
		return Status == ELoginStatus::LoggedIn;
	}
	return false;
}

// --- セッション作成（Host） ---
void UEOSManagerSubsystem::CreateEOSSession(int32 MaxPublicConnections, bool bIsLAN)
{
	IOnlineSession* Session = static_cast<IOnlineSession*>(RawSessionInterface);
	if (!Session)
	{
		UE_LOG(LogTemp, Error, TEXT("[EOSManager] SessionInterface is invalid!"));
		OnEOSCreateSessionComplete.Broadcast(false);
		return;
	}

	Session->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UEOSManagerSubsystem::OnCreateSessionComplete)
	);

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = bIsLAN;
	SessionSettings.NumPublicConnections = MaxPublicConnections;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = true; // EOS Lobby を使用

	UE_LOG(LogTemp, Log, TEXT("[EOSManager] Creating EOS Session for %d players..."), MaxPublicConnections);
	Session->CreateSession(0, NAME_GameSession, SessionSettings);
}

void UEOSManagerSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("[EOSManager] CreateSession Complete: %s (Success: %d)"), *SessionName.ToString(), bWasSuccessful);
	OnEOSCreateSessionComplete.Broadcast(bWasSuccessful);
}

// --- セッション検索（Client） ---
void UEOSManagerSubsystem::FindEOSSessions()
{
	IOnlineSession* Session = static_cast<IOnlineSession*>(RawSessionInterface);
	if (!Session)
	{
		UE_LOG(LogTemp, Error, TEXT("[EOSManager] SessionInterface is invalid!"));
		OnEOSFindSessionsComplete.Broadcast(false);
		return;
	}

	Session->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UEOSManagerSubsystem::OnFindSessionsComplete)
	);

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = 20;
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	UE_LOG(LogTemp, Log, TEXT("[EOSManager] Finding EOS Sessions..."));
	Session->FindSessions(0, LastSessionSearch.ToSharedRef());
}

void UEOSManagerSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	int32 SearchResultCount = LastSessionSearch.IsValid() ? LastSessionSearch->SearchResults.Num() : 0;
	UE_LOG(LogTemp, Log, TEXT("[EOSManager] FindSessions Complete. Found %d sessions."), SearchResultCount);
	OnEOSFindSessionsComplete.Broadcast(bWasSuccessful && SearchResultCount > 0);
}

// --- セッション参加（Client） ---
void UEOSManagerSubsystem::JoinEOSSession(int32 SessionIndex)
{
	IOnlineSession* Session = static_cast<IOnlineSession*>(RawSessionInterface);
	if (!Session || !LastSessionSearch.IsValid() || LastSessionSearch->SearchResults.Num() <= SessionIndex)
	{
		UE_LOG(LogTemp, Error, TEXT("[EOSManager] Invalid Session Index or Search Result!"));
		OnEOSJoinSessionComplete.Broadcast(false, TEXT(""));
		return;
	}

	Session->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateLambda([this](FName SessionName, EOnJoinSessionCompleteResult::Type Result)
			{
				this->OnJoinSessionComplete(SessionName, static_cast<uint8>(Result));
			})
	);

	UE_LOG(LogTemp, Log, TEXT("[EOSManager] Joining Session at index %d..."), SessionIndex);
	Session->JoinSession(0, NAME_GameSession, LastSessionSearch->SearchResults[SessionIndex]);
}

void UEOSManagerSubsystem::OnJoinSessionComplete(FName SessionName, uint8 ResultType)
{
	IOnlineSession* Session = static_cast<IOnlineSession*>(RawSessionInterface);
	EOnJoinSessionCompleteResult::Type Result = static_cast<EOnJoinSessionCompleteResult::Type>(ResultType);

	if (Result == EOnJoinSessionCompleteResult::Success && Session)
	{
		FString ConnectString;
		if (Session->GetResolvedConnectString(NAME_GameSession, ConnectString))
		{
			UE_LOG(LogTemp, Log, TEXT("[EOSManager] Join Successful! Connect String: %s"), *ConnectString);
			OnEOSJoinSessionComplete.Broadcast(true, ConnectString);
			return;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("[EOSManager] Join Session Failed. Result: %d"), (int32)Result);
	OnEOSJoinSessionComplete.Broadcast(false, TEXT(""));
}