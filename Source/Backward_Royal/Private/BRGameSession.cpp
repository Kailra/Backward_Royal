// BRGameSession.cpp - GitHub 예제 기반 간단한 Steam OSS 구현
#include "BRGameSession.h"
#include "BRGameInstance.h"
#include "BRGameMode.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

ABRGameSession::ABRGameSession()
	: bIsSearchingSessions(false)
	, FindSessionsRetryCount(0)
	, PendingRoomName(TEXT(""))
	, bPendingCreateSession(false)
	, bBeginPlayInitialized(false)
	, bOnlineSubsystemInitialized(false)
{
}

void ABRGameSession::BeginPlay()
{
	// 중복 호출 방지
	if (bBeginPlayInitialized)
	{
		return;
	}
	bBeginPlayInitialized = true;
	
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("[GameSession] BeginPlay - NetMode: %s"), 
		GetWorld() ? (GetWorld()->GetNetMode() == NM_Standalone ? TEXT("Standalone") :
			GetWorld()->GetNetMode() == NM_ListenServer ? TEXT("ListenServer") :
			GetWorld()->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Other")) : TEXT("NULL"));
	
	// Online Subsystem 초기화
	InitializeOnlineSubsystem();
	
	// GitHub 예제 방식: PendingRoomName이 있으면 자동 방 생성 (NetMode 체크 없음)
	UWorld* World = GetWorld();
	if (World)
	{
		if (UBRGameInstance* BRGI = Cast<UBRGameInstance>(World->GetGameInstance()))
		{
			FString RoomName = BRGI->GetPendingRoomName();
			if (!RoomName.IsEmpty() && !HasActiveSession())
			{
				UE_LOG(LogTemp, Warning, TEXT("[GameSession] PendingRoomName 발견: %s"), *RoomName);
				
				// Online Subsystem이 준비될 때까지 대기 후 방 생성
				FTimerHandle Timer;
				World->GetTimerManager().SetTimer(Timer, [this, RoomName]()
				{
					if (!IsValid(this))
					{
						return;
					}
					
					UWorld* W = GetWorld();
					if (!W)
					{
						return;
					}
					
					// SessionInterface가 준비되었는지 확인
					if (!SessionInterface.IsValid())
					{
						InitializeOnlineSubsystem();
					}
					
					// SessionInterface가 준비되었고 세션이 없으면 방 생성
					if (SessionInterface.IsValid() && !HasActiveSession())
					{
						UE_LOG(LogTemp, Warning, TEXT("[GameSession] 자동 방 생성 실행: %s"), *RoomName);
						CreateRoomSession(RoomName);
						if (UBRGameInstance* GI = Cast<UBRGameInstance>(W->GetGameInstance()))
						{
							GI->ClearPendingRoomName();
						}
					}
				}, 1.0f, false); // 1초 지연 (Online Subsystem 초기화 대기)
			}
		}
	}
}

void ABRGameSession::InitializeOnlineSubsystem()
{
	// 중복 호출 방지
	if (bOnlineSubsystemInitialized && SessionInterface.IsValid())
	{
		return;
	}
	
	if (!IsValid(this) || !GetWorld())
	{
		return;
	}
	
	// GitHub 예제 방식: 간단하게 Online Subsystem 가져오기
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (!OnlineSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameSession] Online Subsystem 초기화 실패"));
		return;
	}
	
	FString SubsystemName = OnlineSubsystem->GetSubsystemName().ToString();
	UE_LOG(LogTemp, Warning, TEXT("[GameSession] Online Subsystem: %s"), *SubsystemName);
	
	// SessionInterface 가져오기
	SessionInterface = OnlineSubsystem->GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		// 콜백 바인딩 (중복 방지)
		if (!bOnlineSubsystemInitialized)
		{
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &ABRGameSession::OnCreateSessionCompleteDelegate);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &ABRGameSession::OnDestroySessionCompleteDelegate);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &ABRGameSession::OnFindSessionsCompleteDelegate);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &ABRGameSession::OnJoinSessionCompleteDelegate);
			bOnlineSubsystemInitialized = true;
		}
		UE_LOG(LogTemp, Warning, TEXT("[GameSession] SessionInterface 초기화 완료"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GameSession] SessionInterface 초기화 실패"));
	}
}

void ABRGameSession::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	// 콜백 해제
	if (SessionInterface.IsValid())
	{
		SessionInterface->OnCreateSessionCompleteDelegates.RemoveAll(this);
		SessionInterface->OnDestroySessionCompleteDelegates.RemoveAll(this);
		SessionInterface->OnFindSessionsCompleteDelegates.RemoveAll(this);
		SessionInterface->OnJoinSessionCompleteDelegates.RemoveAll(this);
	}
	
	// 플래그 리셋
	bBeginPlayInitialized = false;
	bOnlineSubsystemInitialized = false;
	
	// 검색 중이면 취소
	if (bIsSearchingSessions && SessionInterface.IsValid())
	{
		SessionInterface->CancelFindSessions();
		bIsSearchingSessions = false;
	}
	
	// 타이머 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FindSessionsRetryHandle);
	}
}

void ABRGameSession::CreateRoomSession(const FString& RoomName)
{
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] 시작: %s"), *RoomName);
	
	if (!SessionInterface.IsValid())
	{
		InitializeOnlineSubsystem();
		if (!SessionInterface.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("[방 생성] SessionInterface 초기화 실패"));
			OnCreateSessionComplete.Broadcast(false);
			return;
		}
	}
	
	// 기존 세션이 있으면 제거
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] 기존 세션 제거 중..."));
		PendingRoomName = RoomName;
		bPendingCreateSession = true;
		SessionInterface->DestroySession(NAME_GameSession);
		return;
	}
	
	// CreateRoomSessionInternal 호출
	CreateRoomSessionInternal(RoomName);
}

void ABRGameSession::CreateRoomSessionInternal(const FString& RoomName)
{
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] Internal 시작: %s"), *RoomName);
	
	if (!SessionInterface.IsValid())
	{
		OnCreateSessionComplete.Broadcast(false);
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		OnCreateSessionComplete.Broadcast(false);
		return;
	}
	
	// GitHub 예제 방식: NetMode 체크 없이 바로 CreateSession 호출
	// Steam OSS가 자동으로 ListenServer 처리
	
	// 세션 설정 생성
	SessionSettings = MakeShareable(new FOnlineSessionSettings());
	
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	FString SubsystemName = OnlineSubsystem ? OnlineSubsystem->GetSubsystemName().ToString() : TEXT("NULL");
	
	// GitHub 예제 방식: bIsLANMatch 설정
	SessionSettings->bIsLANMatch = (SubsystemName == TEXT("NULL"));
	SessionSettings->bUsesPresence = true;
	SessionSettings->bShouldAdvertise = true;
	SessionSettings->bUseLobbiesIfAvailable = true; // GitHub 예제에서 사용
	
	// 플레이어 수 설정
	int32 MaxPlayerCount = 8;
	if (ABRGameMode* BRGM = World->GetAuthGameMode<ABRGameMode>())
	{
		MaxPlayerCount = BRGM->MaxPlayers;
	}
	SessionSettings->NumPublicConnections = MaxPlayerCount;
	
	// 세션 이름 설정
	FString SessionNameStr = RoomName.IsEmpty() ? TEXT("이름 없는 방") : RoomName;
	SessionSettings->Set(FName(TEXT("SESSION_NAME")), SessionNameStr, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	
	// GitHub 예제 방식: CreateSession 호출 (NetMode 체크 없음)
	int32 LocalUserNum = 0;
	bool bCreateResult = SessionInterface->CreateSession(LocalUserNum, NAME_GameSession, *SessionSettings);
	
	if (!bCreateResult)
	{
		UE_LOG(LogTemp, Error, TEXT("[방 생성] CreateSession 호출 실패"));
		OnCreateSessionComplete.Broadcast(false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] CreateSession 호출 성공 (비동기 처리 중)"));
	}
}

void ABRGameSession::FindSessions()
{
	UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 시작"));
	
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[방 찾기] SessionInterface가 유효하지 않습니다."));
		OnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>());
		OnFindSessionsCompleteBP.Broadcast(0);
		return;
	}
	
	// 이미 검색 중이면 무시
	if (bIsSearchingSessions)
	{
		return;
	}
	
	FindSessionsInternal(false);
}

void ABRGameSession::FindSessionsRetryCallback()
{
	FindSessionsInternal(true);
}

void ABRGameSession::FindSessionsInternal(bool bIsRetry)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}
	
	if (!bIsRetry)
	{
		FindSessionsRetryCount = 0;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(FindSessionsRetryHandle);
		}
	}
	
	bIsSearchingSessions = true;
	
	// 이전 검색 취소
	SessionInterface->CancelFindSessions();
	
	// 세션 검색 설정
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = 100;
	SessionSearch->QuerySettings.Set(FName(TEXT("PRESENCESEARCH")), true, EOnlineComparisonOp::Equals);
	
	// LAN 여부 설정
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem && OnlineSubsystem->GetSubsystemName() == "NULL")
	{
		SessionSearch->bIsLanQuery = true;
	}
	else
	{
		SessionSearch->bIsLanQuery = false;
	}
	
	// FindSessions 호출
	bool bFindSessionsResult = SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	if (!bFindSessionsResult)
	{
		UE_LOG(LogTemp, Error, TEXT("[방 찾기] FindSessions 호출 실패"));
		bIsSearchingSessions = false;
		OnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>());
		OnFindSessionsCompleteBP.Broadcast(0);
	}
}

void ABRGameSession::JoinSessionByIndex(int32 SessionIndex)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[방 참가] SessionInterface가 유효하지 않습니다."));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}
	
	if (!SessionSearch.IsValid() || SessionIndex < 0 || SessionIndex >= SessionSearch->SearchResults.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("[방 참가] 잘못된 세션 인덱스: %d"), SessionIndex);
		OnJoinSessionComplete.Broadcast(false);
		return;
	}
	
	const FOnlineSessionSearchResult& SessionResult = SessionSearch->SearchResults[SessionIndex];
	JoinSession(SessionResult);
}

void ABRGameSession::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[방 참가] SessionInterface가 유효하지 않습니다."));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}
	
	// 기존 세션이 있으면 제거
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}
	
	// JoinSession 호출
	SessionInterface->JoinSession(0, NAME_GameSession, SessionResult);
}

void ABRGameSession::OnCreateSessionCompleteDelegate(FName InSessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] 완료: %s"), bWasSuccessful ? TEXT("성공") : TEXT("실패"));
	OnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void ABRGameSession::OnDestroySessionCompleteDelegate(FName InSessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] 세션 제거 완료: %s"), bWasSuccessful ? TEXT("성공") : TEXT("실패"));
	
	if (bPendingCreateSession && !PendingRoomName.IsEmpty())
	{
		bPendingCreateSession = false;
		FString RoomName = PendingRoomName;
		PendingRoomName = TEXT("");
		CreateRoomSessionInternal(RoomName);
	}
}

void ABRGameSession::OnFindSessionsCompleteDelegate(bool bWasSuccessful)
{
	bIsSearchingSessions = false;
	
	TArray<FOnlineSessionSearchResult> Results;
	
	if (bWasSuccessful && SessionSearch.IsValid())
	{
		Results = SessionSearch->SearchResults;
		UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 완료: %d개 세션 발견"), Results.Num());
		
		// 0건일 때 Steam이면 최대 2회 자동 재검색
		if (Results.Num() == 0)
		{
			IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
			FString SubsystemName = OnlineSubsystem ? OnlineSubsystem->GetSubsystemName().ToString() : TEXT("NULL");
			
			if (SubsystemName.Equals(TEXT("Steam"), ESearchCase::IgnoreCase) &&
				FindSessionsRetryCount < MaxFindSessionsRetries)
			{
				FindSessionsRetryCount++;
				UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 세션 0건. 2초 후 재검색 (%d/%d)"), FindSessionsRetryCount, MaxFindSessionsRetries);
				if (UWorld* World = GetWorld())
				{
					World->GetTimerManager().SetTimer(FindSessionsRetryHandle, this, &ABRGameSession::FindSessionsRetryCallback, 2.0f, false);
				}
				return;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[방 찾기] 실패"));
	}
	
	OnFindSessionsComplete.Broadcast(Results);
	OnFindSessionsCompleteBP.Broadcast(Results.Num());
}

void ABRGameSession::OnJoinSessionCompleteDelegate(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bool bWasSuccessful = (Result == EOnJoinSessionCompleteResult::Success);
	
	if (!SessionInterface.IsValid())
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}
	
	// 연결 문자열 가져오기
	FString TravelURL;
	if (!SessionInterface->GetResolvedConnectString(InSessionName, TravelURL))
	{
		UE_LOG(LogTemp, Error, TEXT("[방 참가] 연결 주소를 가져올 수 없습니다."));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}
	
	// ClientTravel 호출
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			UE_LOG(LogTemp, Warning, TEXT("[방 참가] 서버로 이동: %s"), *TravelURL);
			PC->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
		}
	}
	
	OnJoinSessionComplete.Broadcast(bWasSuccessful);
}

int32 ABRGameSession::GetSessionCount() const
{
	if (SessionSearch.IsValid())
	{
		return SessionSearch->SearchResults.Num();
	}
	return 0;
}

FString ABRGameSession::GetSessionName(int32 SessionIndex) const
{
	if (!SessionSearch.IsValid() || SessionIndex < 0 || SessionIndex >= SessionSearch->SearchResults.Num())
	{
		return FString();
	}
	
	const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[SessionIndex];
	FString FoundSessionName;
	Result.Session.SessionSettings.Get(FName(TEXT("SESSION_NAME")), FoundSessionName);
	
	if (!FoundSessionName.IsEmpty())
	{
		return FoundSessionName;
	}
	return TEXT("(이름 없음)");
}

bool ABRGameSession::HasActiveSession() const
{
	if (SessionInterface.IsValid())
	{
		auto Session = SessionInterface->GetNamedSession(NAME_GameSession);
		return Session != nullptr;
	}
	return false;
}
