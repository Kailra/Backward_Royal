// BRGameSession.cpp
#include "BRGameSession.h"
#include "BRGameInstance.h"
#include "BRGameMode.h"
#include "BRPlayerState.h"
#include "BRPlayerController.h"
#include "BRGameState.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/NetDriver.h"
#include "OnlineSubsystemUtils.h"
#include "Containers/Set.h"
#include "Misc/Paths.h"

ABRGameSession::ABRGameSession()
	: bIsSearchingSessions(false)
	, FindSessionsRetryCount(0)
	, PendingRoomName(TEXT(""))
	, bPendingCreateSession(false)
{
	// 생성자에서는 초기화하지 않음 (BeginPlay에서 초기화)
	// Standalone 모드에서는 생성자 시점에 World가 준비되지 않을 수 있음
}

void ABRGameSession::CheckNetDriverStatus(UWorld* World)
{
	UE_LOG(LogTemp, Error, TEXT("========================================"));
	UE_LOG(LogTemp, Error, TEXT("[GameSession] CheckNetDriverStatus 호출됨"));
	
	if (!World || !IsValid(World))
	{
		UE_LOG(LogTemp, Error, TEXT("[GameSession] World가 NULL이거나 유효하지 않습니다!"));
		return;
	}
	
	// this 객체도 유효한지 확인 (맵 전환 중 파괴될 수 있음)
	if (!IsValid(this))
	{
		UE_LOG(LogTemp, Error, TEXT("[GameSession] this 객체가 유효하지 않습니다!"));
		return;
	}
	
	ENetMode NetMode = World->GetNetMode();
	UE_LOG(LogTemp, Error, TEXT("[GameSession] 현재 NetMode: %s"), 
		NetMode == NM_Standalone ? TEXT("Standalone") :
		NetMode == NM_ListenServer ? TEXT("ListenServer") :
		NetMode == NM_Client ? TEXT("Client") :
		NetMode == NM_DedicatedServer ? TEXT("DedicatedServer") : TEXT("Unknown"));
	
	// NetDriver 상태 확인 (안전하게 접근)
	UNetDriver* NetDriver = nullptr;
	if (World)
	{
		NetDriver = World->GetNetDriver();
	}
	
	if (NetDriver)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameSession] ✅ NetDriver 상태: 활성화됨 (NetMode: %s)"), 
			NetMode == NM_Standalone ? TEXT("Standalone") :
			NetMode == NM_ListenServer ? TEXT("ListenServer") :
			NetMode == NM_Client ? TEXT("Client") :
			NetMode == NM_DedicatedServer ? TEXT("DedicatedServer") : TEXT("Unknown"));
		
		// 포트 정보 가져오기 (LocalAddr가 유효한 경우)
		// 안전하게 접근하기 위해 유효성 검사 강화
		if (NetDriver->LocalAddr.IsValid())
		{
			TSharedPtr<FInternetAddr> LocalAddr = NetDriver->LocalAddr;
			if (LocalAddr.IsValid())
			{
				// ToString 호출 전에 포인터가 유효한지 확인
				FInternetAddr* AddrPtr = LocalAddr.Get();
				if (AddrPtr)
				{
					FString LocalAddress = AddrPtr->ToString(false);
					if (!LocalAddress.IsEmpty())
					{
						UE_LOG(LogTemp, Error, TEXT("[GameSession] ✅ NetDriver LocalAddr: %s"), *LocalAddress);
						
						if (GEngine && NetMode == NM_ListenServer)
						{
							GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
								FString::Printf(TEXT("[GameSession] ✅ NetDriver 활성화! 주소: %s"), *LocalAddress));
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[GameSession] ⚠️ LocalAddr->ToString()가 빈 문자열을 반환했습니다."));
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[GameSession] ⚠️ LocalAddr 포인터가 NULL입니다."));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[GameSession] ⚠️ NetDriver LocalAddr가 아직 초기화되지 않았습니다."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GameSession] ❌ NetDriver가 없습니다. (NetMode: %s)"), 
			NetMode == NM_Standalone ? TEXT("Standalone") :
			NetMode == NM_ListenServer ? TEXT("ListenServer") :
			NetMode == NM_Client ? TEXT("Client") :
			NetMode == NM_DedicatedServer ? TEXT("DedicatedServer") : TEXT("Unknown"));
		
		if (GEngine && NetMode == NM_ListenServer)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, 
				TEXT("[GameSession] ❌ NetDriver 없음! 리슨 서버로 재시작하세요."));
		}
	}
	
	UE_LOG(LogTemp, Error, TEXT("[GameSession] CheckNetDriverStatus 완료"));
	UE_LOG(LogTemp, Error, TEXT("========================================"));
}

void ABRGameSession::InitializeOnlineSubsystem()
{
	// 맵 전환(open ?listen) 중 등으로 this/World 무효 시 스킵 — 타이머 콜백 크래시 방지
	if (!IsValid(this) || !GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameSession] InitializeOnlineSubsystem 스킵: this 또는 World 무효"));
		return;
	}

	// Online Subsystem 초기화
	// Steam Online Subsystem 사용 (DefaultEngine.ini 설정에 따라)
	IOnlineSubsystem* OnlineSubsystem = nullptr;
	
	// Steam Online Subsystem 사용
	UE_LOG(LogTemp, Warning, TEXT("[GameSession] Steam Online Subsystem 초기화 시도 중..."));
	OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		FString SubsystemName = OnlineSubsystem->GetSubsystemName().ToString();
		UE_LOG(LogTemp, Warning, TEXT("[GameSession] IOnlineSubsystem::Get() 성공! SubsystemName: %s"), *SubsystemName);
		if (GEngine)
		{
			FString SuccessMsg = FString::Printf(TEXT("[GameSession] ✅ %s Online Subsystem 초기화 성공!"), *SubsystemName);
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, SuccessMsg);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GameSession] IOnlineSubsystem::Get() 실패 - Steam이 로드되지 않았습니다."));
		
		// 폴백: NULL Online Subsystem 시도
		UE_LOG(LogTemp, Warning, TEXT("[GameSession] NULL Online Subsystem으로 폴백 시도 중..."));
		OnlineSubsystem = IOnlineSubsystem::Get(FName("Null"));
		if (OnlineSubsystem)
		{
			FString SubsystemName = OnlineSubsystem->GetSubsystemName().ToString();
			UE_LOG(LogTemp, Log, TEXT("[GameSession] NULL Online Subsystem 폴백 성공: %s"), *SubsystemName);
			if (GEngine)
			{
				FString Message = FString::Printf(TEXT("[GameSession] ⚠️ Steam 실패, NULL Online Subsystem 사용: %s"), *SubsystemName);
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, Message);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[GameSession] Online Subsystem 초기화 완전 실패!"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("[GameSession] ❌ Online Subsystem 초기화 실패!"));
			}
		}
	}
	
	if (OnlineSubsystem)
	{
		FString SubsystemName = OnlineSubsystem->GetSubsystemName().ToString();
		// 이전 코드: UE_LOG(LogTemp, Warning, TEXT("OSS : %s is Avaliable."), *OSS->GetSubsystemName().ToString());
		UE_LOG(LogTemp, Warning, TEXT("[GameSession] OSS : %s is Available."), *SubsystemName);
		UE_LOG(LogTemp, Log, TEXT("[GameSession] Online Subsystem 초기화: %s"), *SubsystemName);
		
		// Steam인 경우 로그인 상태 확인
		if (SubsystemName.Equals(TEXT("Steam"), ESearchCase::IgnoreCase))
		{
			IOnlineIdentityPtr IdentityInterface = OnlineSubsystem->GetIdentityInterface();
			if (IdentityInterface.IsValid())
			{
				bool bFoundLoggedInUser = false;
				for (int32 UserIdx = 0; UserIdx < 4; UserIdx++)
				{
					ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(UserIdx);
					if (LoginStatus == ELoginStatus::LoggedIn)
					{
						UE_LOG(LogTemp, Warning, TEXT("[GameSession] ✅ Steam 로그인 확인: LocalUserNum=%d"), UserIdx);
						bFoundLoggedInUser = true;
						
						// 사용자 ID 가져오기
						TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(UserIdx);
						if (UserId.IsValid())
						{
							UE_LOG(LogTemp, Warning, TEXT("[GameSession] Steam 사용자 ID: %s"), *UserId->ToString());
						}
						break;
					}
				}
				if (!bFoundLoggedInUser)
				{
					UE_LOG(LogTemp, Error, TEXT("[GameSession] ⚠️ Steam에 로그인된 사용자가 없습니다!"));
					UE_LOG(LogTemp, Error, TEXT("[GameSession] Steam 클라이언트가 실행 중이고 로그인되어 있는지 확인하세요."));
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, 
							TEXT("[GameSession] ⚠️ Steam 로그인 필요!\nSteam 클라이언트를 실행하고 로그인하세요."));
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[GameSession] Identity Interface를 가져올 수 없습니다."));
			}
		}
		
		// 화면에 메시지 표시
		if (GEngine)
		{
			FString Message = FString::Printf(TEXT("[GameSession] Online Subsystem: %s"), *SubsystemName);
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, Message);
		}
		
		IOnlineSessionPtr NewInterface = OnlineSubsystem->GetSessionInterface();
		if (NewInterface.IsValid())
		{
			SessionInterface = NewInterface;
			UE_LOG(LogTemp, Log, TEXT("[GameSession] SessionInterface 초기화 완료"));
			// 콜백 바인딩
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &ABRGameSession::OnCreateSessionCompleteDelegate);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &ABRGameSession::OnDestroySessionCompleteDelegate);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &ABRGameSession::OnFindSessionsCompleteDelegate);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &ABRGameSession::OnJoinSessionCompleteDelegate);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[GameSession] SessionInterface 초기화 실패"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[GameSession] SessionInterface 초기화 실패!"));
			}
		}
	}
	else
	{
		// 이전 코드: UE_LOG(LogTemp, Warning, TEXT("Not found subsystem."));
		UE_LOG(LogTemp, Warning, TEXT("[GameSession] Not found subsystem."));
		UE_LOG(LogTemp, Error, TEXT("[GameSession] Online Subsystem 초기화 실패 - NULL"));
		if (GEngine)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				ENetMode NetMode = World->GetNetMode();
				if (NetMode == NM_Standalone)
				{
					FString WarningMsg = TEXT("[GameSession] Standalone 모드: Online Subsystem이 NULL입니다.\n");
					WarningMsg += TEXT("세션 기능을 사용하려면 Listen Server 모드를 사용하세요.\n");
					WarningMsg += TEXT("(Play 버튼 옆 드롭다운 -> Number of Players: 2+)");
					GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, WarningMsg);
					UE_LOG(LogTemp, Warning, TEXT("%s"), *WarningMsg);
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[GameSession] Online Subsystem 초기화 실패!"));
				}
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[GameSession] Online Subsystem 초기화 실패!"));
			}
		}
	}
}

void ABRGameSession::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Error, TEXT("========================================"));
	UE_LOG(LogTemp, Error, TEXT("[GameSession] BeginPlay 호출됨!"));
	UE_LOG(LogTemp, Error, TEXT("========================================"));
	
	// 리슨 서버 전환 완료 확인 (ServerTravel 후 맵 재로드 시)
	UWorld* World = GetWorld();
	if (World)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameSession] World 유효함"));
		ENetMode NetMode = World->GetNetMode();
		bool bHasActiveSession = HasActiveSession();
		
		// Standalone 모드에서 이전 세션이 남아있는 경우 처리
		// 방 생성 후 ServerTravel로 인한 재로드인지, 게임 재시작인지 구분 필요
		if (bHasActiveSession && NetMode == NM_Standalone)
		{
			// GameState를 확인하여 플레이어가 있는지 확인
			// 플레이어가 있으면 방 생성 완료 상태, 없으면 게임 재시작 상태로 간주
			bool bHasPlayers = false;
			if (ABRGameState* BRGameState = World->GetGameState<ABRGameState>())
			{
				bHasPlayers = BRGameState->PlayerArray.Num() > 0;
			}
			
			if (bHasPlayers)
			{
				// 플레이어가 있으면 방 생성 완료 상태 (ServerTravel 후 재로드)
				UE_LOG(LogTemp, Warning, TEXT("[GameSession] BeginPlay: Standalone 모드에서 활성 세션 + 플레이어 존재 - 방 생성 완료 상태"));
				UE_LOG(LogTemp, Warning, TEXT("[GameSession] ServerTravel 후 재로드로 인해 NetMode가 Standalone이지만, 세션과 플레이어가 있으면 정상 상태"));
			}
			else
			{
				// 플레이어가 없으면 게임 재시작 상태 - 이전 세션 정리
				UE_LOG(LogTemp, Warning, TEXT("[GameSession] BeginPlay: Standalone 모드에서 이전 세션이 감지되었습니다. 정리 중..."));
				UE_LOG(LogTemp, Warning, TEXT("[GameSession] 게임 재시작 시 이전 세션이 남아있어 로비로 넘어가는 문제를 방지하기 위해 세션을 정리합니다."));
				
				// Online Subsystem 초기화 후 세션 정리
				FTimerHandle CleanupTimerHandle;
				GetWorld()->GetTimerManager().SetTimer(CleanupTimerHandle, [this]()
				{
					// Online Subsystem이 초기화된 후 세션 정리
					if (SessionInterface.IsValid())
					{
						auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
						if (ExistingSession != nullptr)
						{
							UE_LOG(LogTemp, Warning, TEXT("[GameSession] 이전 세션 제거 중... (게임 재시작 시 정리)"));
							SessionInterface->DestroySession(NAME_GameSession);
							
							if (GEngine)
							{
								GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, 
									TEXT("[GameSession] 이전 세션 정리 완료"));
							}
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[GameSession] SessionInterface가 아직 초기화되지 않았습니다. 세션 정리를 건너뜁니다."));
					}
				}, 0.5f, false); // InitializeOnlineSubsystem 완료 대기 (0.1초 + 여유 시간)
			}
		}
		
		// 리슨 서버 모드 확인 (ServerTravel(?listen) 후 맵 재로드 시)
		UE_LOG(LogTemp, Error, TEXT("========================================"));
		UE_LOG(LogTemp, Error, TEXT("[GameSession] BeginPlay: NetMode 확인"));
		UE_LOG(LogTemp, Error, TEXT("[GameSession] NetMode: %s"), 
			NetMode == NM_Standalone ? TEXT("Standalone") :
			NetMode == NM_ListenServer ? TEXT("ListenServer ✅") :
			NetMode == NM_Client ? TEXT("Client") :
			NetMode == NM_DedicatedServer ? TEXT("DedicatedServer") : TEXT("Unknown"));
		UE_LOG(LogTemp, Error, TEXT("[GameSession] HasActiveSession: %s"), bHasActiveSession ? TEXT("Yes") : TEXT("No"));
		
		// NetDriver 상태 확인 (리슨 서버 작동 여부 확인)
		if (UNetDriver* NetDriver = World->GetNetDriver())
		{
			if (NetMode == NM_ListenServer)
			{
				UE_LOG(LogTemp, Error, TEXT("[GameSession] ✅ 리슨 서버 정상 작동 중! NetDriver 활성화됨"));
				if (NetDriver->LocalAddr.IsValid())
				{
					FString LocalAddress = NetDriver->LocalAddr->ToString(false);
					UE_LOG(LogTemp, Error, TEXT("[GameSession] ✅ 리슨 서버 주소: %s"), *LocalAddress);
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, 
							FString::Printf(TEXT("[GameSession] ✅ 리슨 서버 정상 작동!\n주소: %s\n클라이언트 연결 가능"), *LocalAddress));
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[GameSession] NetDriver는 있지만 NetMode가 %s입니다."),
					NetMode == NM_Standalone ? TEXT("Standalone") : TEXT("기타"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[GameSession] NetDriver가 없습니다. 리슨 서버로 작동하지 않을 수 있습니다."));
		}
		
		// NetDriver 상태 확인 (모든 NetMode에서 확인)
		// BeginPlay 시점에 NetDriver가 아직 초기화되지 않았을 수 있으므로 즉시 확인
		CheckNetDriverStatus(World);
		
		// 주의: 타이머 콜백에서 크래시가 발생할 수 있으므로 타이머는 제거
		// NetDriver는 보통 BeginPlay 시점에 이미 초기화되어 있음
		// 만약 초기화되지 않았다면, 리슨 서버 모드로 시작하지 않은 것이 원인일 수 있음
		
		UE_LOG(LogTemp, Error, TEXT("========================================"));
		
		if (NetMode == NM_ListenServer)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GameSession] ✅ 리슨 서버 모드로 정상 실행 중입니다!"));
			UE_LOG(LogTemp, Warning, TEXT("[GameSession] 클라이언트 연결을 받을 수 있는 상태입니다."));
			
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
					TEXT("[GameSession] ✅ 리슨 서버 모드 활성화!"));
			}
			
		// 리슨 서버 모드인데 세션이 없으면 자동으로 재생성 시도
		if (!bHasActiveSession)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GameSession] 리슨 서버 모드이지만 활성 세션이 없습니다."));
			
			// GameInstance에서 대기 중인 방 이름 확인
			FString GameInstancePendingRoomName;
			if (UBRGameInstance* BRGI = Cast<UBRGameInstance>(World->GetGameInstance()))
			{
				GameInstancePendingRoomName = BRGI->GetPendingRoomName();
				UE_LOG(LogTemp, Warning, TEXT("[GameSession] BeginPlay에서 PendingRoomName 확인: %s"), 
					GameInstancePendingRoomName.IsEmpty() ? TEXT("비어있음") : *GameInstancePendingRoomName);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[GameSession] BeginPlay에서 BRGameInstance를 찾을 수 없습니다."));
			}
			
			if (!GameInstancePendingRoomName.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("[GameSession] ✅ PendingRoomName 발견! 자동으로 세션 재생성 시도: %s"), *GameInstancePendingRoomName);
				
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
						FString::Printf(TEXT("[GameSession] 방 생성 중... (%s)"), *GameInstancePendingRoomName));
				}
				
				// Online Subsystem 초기화 후 세션 재생성
				FTimerHandle RecreateSessionTimerHandle;
				GetWorld()->GetTimerManager().SetTimer(RecreateSessionTimerHandle, [this, GameInstancePendingRoomName]()
				{
					if (!IsValid(this))
					{
						return;
					}
					
					// Online Subsystem이 초기화된 후 세션 재생성
					if (SessionInterface.IsValid())
					{
						UE_LOG(LogTemp, Warning, TEXT("[GameSession] ✅ 세션 자동 재생성 시작: %s"), *GameInstancePendingRoomName);
						CreateRoomSession(GameInstancePendingRoomName);
						
						// 방 이름 클리어
						if (UWorld* W = GetWorld())
						{
							if (UBRGameInstance* BRGI = Cast<UBRGameInstance>(W->GetGameInstance()))
							{
								BRGI->ClearPendingRoomName();
							}
						}
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("[GameSession] ❌ SessionInterface가 아직 초기화되지 않았습니다. 세션 재생성을 건너뜁니다."));
						UE_LOG(LogTemp, Warning, TEXT("[GameSession] Online Subsystem 초기화를 다시 시도합니다..."));
						
						// SessionInterface가 없으면 다시 초기화 시도
						InitializeOnlineSubsystem();
						
						// 초기화 후 재시도
						FTimerHandle RetryTimer;
						GetWorld()->GetTimerManager().SetTimer(RetryTimer, [this, GameInstancePendingRoomName]()
						{
							if (IsValid(this) && SessionInterface.IsValid())
							{
								UE_LOG(LogTemp, Warning, TEXT("[GameSession] ✅ 재시도: 세션 자동 재생성 시작: %s"), *GameInstancePendingRoomName);
								CreateRoomSession(GameInstancePendingRoomName);
								
								if (UWorld* W = GetWorld())
								{
									if (UBRGameInstance* BRGI = Cast<UBRGameInstance>(W->GetGameInstance()))
									{
										BRGI->ClearPendingRoomName();
									}
								}
							}
						}, 1.0f, false);
					}
				}, 1.0f, false); // InitializeOnlineSubsystem 완료 대기
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[GameSession] ⚠️ 리슨 서버 모드이지만 세션이 없고, 대기 중인 방 이름도 없습니다."));
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, 
						TEXT("[GameSession] ⚠️ 리슨 서버 모드이지만 세션이 없습니다.\n방을 다시 만들어주세요."));
				}
			}
		}
			else
			{
				if (GEngine)
				{
					FString SuccessMsg = FString::Printf(TEXT("[GameSession] ✅ 리슨 서버 모드로 정상 실행 중!\n세션: 활성"));
					GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Green, SuccessMsg);
				}
			}
		}
		else if (NetMode == NM_Standalone)
		{
			// GameInstance의 OnStart에서 자동으로 ListenServer로 전환하므로
			// 이 시점에서 Standalone 모드는 정상입니다 (전환 대기 중)
			UE_LOG(LogTemp, Warning, TEXT("[GameSession] Standalone 모드 감지 (GameInstance에서 ListenServer로 전환 예정)"));
			
			// NetDriver 상태 확인
			if (UNetDriver* NetDriver = World->GetNetDriver())
			{
				UE_LOG(LogTemp, Log, TEXT("[GameSession] NetDriver 확인: LocalAddr=%s"), 
					NetDriver->LocalAddr.IsValid() ? *NetDriver->LocalAddr->ToString(false) : TEXT("없음"));
			}
			
			// GameInstance에서 자동 전환 중이므로 경고 메시지 제거
			// (PlayerController의 BeginPlay에서 이미 ListenServer로 감지됨)
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GameSession] World가 NULL입니다!"));
		UE_LOG(LogTemp, Error, TEXT("[GameSession] BeginPlay에서 World를 가져올 수 없습니다!"));
	}
	
	UE_LOG(LogTemp, Error, TEXT("[GameSession] BeginPlay 완료 - Online Subsystem 초기화 시작"));
	
	// Online Subsystem 즉시 초기화 (타이머 사용 시 open ?listen 맵 전환 중 크래시 가능)
	InitializeOnlineSubsystem();
	
	UE_LOG(LogTemp, Error, TEXT("[GameSession] InitializeOnlineSubsystem 완료"));
}

void ABRGameSession::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	UWorld* World = GetWorld();
	bool bIsPIE = World ? World->IsPlayInEditor() : false;
	ENetMode NetMode = World ? World->GetNetMode() : NM_Standalone;
	
	UE_LOG(LogTemp, Warning, TEXT("[GameSession] EndPlay 호출됨 - 세션 정리 중..."));
	UE_LOG(LogTemp, Warning, TEXT("[GameSession] EndPlayReason: %d, PIE: %s, NetMode: %s"), 
		(int32)EndPlayReason,
		bIsPIE ? TEXT("Yes") : TEXT("No"),
		NetMode == NM_Standalone ? TEXT("Standalone") :
		NetMode == NM_ListenServer ? TEXT("ListenServer") :
		NetMode == NM_Client ? TEXT("Client") : TEXT("Other"));
	
	// 콜백 해제 (맵 전환 시 파괴된 this 참조로 인한 크래시 방지)
	if (SessionInterface.IsValid())
	{
		SessionInterface->OnCreateSessionCompleteDelegates.RemoveAll(this);
		SessionInterface->OnFindSessionsCompleteDelegates.RemoveAll(this);
		SessionInterface->OnJoinSessionCompleteDelegates.RemoveAll(this);
	}
	
	// 게임 종료 시 활성 세션이 있으면 제거 (Standalone, PIE 모드 모두)
	if (SessionInterface.IsValid())
	{
		auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
		if (ExistingSession != nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GameSession] 게임 종료 시 활성 세션 제거 중... (PIE: %s)"), 
				bIsPIE ? TEXT("Yes") : TEXT("No"));
			SessionInterface->DestroySession(NAME_GameSession);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[GameSession] 활성 세션이 없습니다. (PIE: %s)"), 
				bIsPIE ? TEXT("Yes") : TEXT("No"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameSession] SessionInterface가 유효하지 않습니다. 세션 정리를 건너뜁니다."));
	}
	
	// 검색 중이면 취소
	if (bIsSearchingSessions && SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameSession] 진행 중인 세션 검색 취소 중..."));
		SessionInterface->CancelFindSessions();
		bIsSearchingSessions = false;
	}
	
	// 타이머 정리
	if (World)
	{
		World->GetTimerManager().ClearTimer(FindSessionsRetryHandle);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[GameSession] 세션 정리 완료 (PIE: %s)"), bIsPIE ? TEXT("Yes") : TEXT("No"));
}

void ABRGameSession::CreateRoomSession(const FString& RoomName)
{
	UE_LOG(LogTemp, Log, TEXT("[방 생성] 세션 생성 시작: %s"), *RoomName);
	
	// 현재 네트워크 모드 확인 (리슨 서버 모드 진단용)
	UWorld* World = GetWorld();
	if (World)
	{
		ENetMode NetMode = World->GetNetMode();
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] 현재 네트워크 모드: %s"), 
			NetMode == NM_Standalone ? TEXT("Standalone") :
			NetMode == NM_ListenServer ? TEXT("ListenServer") :
			NetMode == NM_Client ? TEXT("Client") :
			NetMode == NM_DedicatedServer ? TEXT("DedicatedServer") : TEXT("Unknown"));
	}
	
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[방 생성] 실패: SessionInterface가 유효하지 않습니다."));
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[방 생성] 실패: SessionInterface가 유효하지 않습니다!"));
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Online Subsystem이 초기화되지 않았을 수 있습니다."));
		}
		
		// SessionInterface가 없으면 다시 초기화 시도
		InitializeOnlineSubsystem();
		
		// 초기화 후에도 유효하지 않으면 실패
		if (!SessionInterface.IsValid())
		{
			OnCreateSessionComplete.Broadcast(false);
			return;
		}
	}

	// 기존 세션이 있으면 제거 (이전 코드처럼 DestroySession 완료 후 CreateSession 호출)
	// 이전 코드: if (AlreadyExsistingSession) { UE_LOG(LogTemp, Warning, TEXT("%s is already exsist. re-createSession."),*SESSION_NAME.ToString()); SessionInterface->DestroySession(SESSION_NAME); } else { CreateSession(); }
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] %s is already exist. re-createSession."), *FName(NAME_GameSession).ToString());
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] 기존 세션이 발견되었습니다. 제거 중..."));
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] 기존 세션 정보: 최대 인원=%d, 현재 인원=%d"), 
			ExistingSession->SessionSettings.NumPublicConnections,
			ExistingSession->NumOpenPublicConnections);
		
		// DestroySession 완료 후 CreateSession을 호출하기 위해 방 이름 저장
		PendingRoomName = RoomName;
		bPendingCreateSession = true;
		
		// 이전 코드처럼 DestroySession만 호출하고, OnDestroySessionCompleteDelegate에서 CreateSession 호출
		bool bDestroyResult = SessionInterface->DestroySession(NAME_GameSession);
		if (bDestroyResult)
		{
			UE_LOG(LogTemp, Warning, TEXT("[방 생성] 기존 세션 제거 요청 전송됨 (비동기 처리 중... DestroySession 완료 후 CreateSession 호출 예정)"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[방 생성] 기존 세션 제거 실패! 세션이 이미 제거되었을 수 있습니다. CreateSession을 바로 시도합니다."));
			// DestroySession이 실패했으면 바로 CreateSession 시도
			bPendingCreateSession = false;
		}
		
		// DestroySession이 성공하면 OnDestroySessionCompleteDelegate에서 CreateSession 호출
		if (bDestroyResult)
		{
			return;
		}
	}
	else
	{
		// 기존 세션이 없으면 바로 CreateSession 진행
		bPendingCreateSession = false;
	}

	// CreateRoomSessionInternal 호출 (기존 세션 체크 제외)
	CreateRoomSessionInternal(RoomName);
}

void ABRGameSession::CreateRoomSessionInternal(const FString& RoomName)
{
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] CreateRoomSessionInternal 시작: %s"), *RoomName);
	
	// SessionInterface 유효성 재확인
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[방 생성] CreateRoomSessionInternal: SessionInterface가 유효하지 않습니다!"));
		OnCreateSessionComplete.Broadcast(false);
		return;
	}
	
	// 리슨 서버가 완전히 준비되었는지 확인 (openListenServer 명령어 사용 시 성공하는 이유)
	UWorld* World = GetWorld();
	if (World)
	{
		ENetMode NetMode = World->GetNetMode();
		UNetDriver* NetDriver = World->GetNetDriver();
		
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] 현재 상태 확인: NetMode=%d (0=Standalone, 3=ListenServer), NetDriver=%s"), 
			(int32)NetMode, NetDriver ? TEXT("유효") : TEXT("NULL"));
		
		// Standalone 모드이지만 NetDriver가 있으면 ListenServer로 전환 중일 수 있음
		if (NetMode == NM_Standalone && NetDriver)
		{
			UE_LOG(LogTemp, Warning, TEXT("[방 생성] ⚠️ Standalone 모드이지만 NetDriver가 존재합니다. ListenServer 전환 중일 수 있습니다."));
			UE_LOG(LogTemp, Warning, TEXT("[방 생성] NetDriver 초기화 완료 대기 후 재시도..."));
			
			// NetDriver가 완전히 준비될 때까지 대기 후 재시도
			FTimerHandle RetryTimer;
			World->GetTimerManager().SetTimer(RetryTimer, [this, RoomName]()
			{
				if (IsValid(this))
				{
					UWorld* W = GetWorld();
					if (W)
					{
						ENetMode NM = W->GetNetMode();
						UE_LOG(LogTemp, Warning, TEXT("[방 생성] 재시도: NetMode=%d, 재시도: %s"), (int32)NM, *RoomName);
						CreateRoomSessionInternal(RoomName);
					}
				}
			}, 1.0f, false); // 1초 후 재시도
			return;
		}
		
		if (NetMode == NM_ListenServer)
		{
			// 리슨 서버 모드인 경우 NetDriver가 활성화되었는지 확인
			if (!NetDriver)
			{
				UE_LOG(LogTemp, Warning, TEXT("[방 생성] ⚠️ 리슨 서버 모드이지만 NetDriver가 아직 초기화되지 않았습니다."));
				UE_LOG(LogTemp, Warning, TEXT("[방 생성] NetDriver 초기화 대기 후 재시도..."));
				
				// NetDriver가 준비될 때까지 대기 후 재시도
				FTimerHandle RetryTimer;
				World->GetTimerManager().SetTimer(RetryTimer, [this, RoomName]()
				{
					if (IsValid(this))
					{
						UE_LOG(LogTemp, Warning, TEXT("[방 생성] NetDriver 초기화 대기 후 재시도: %s"), *RoomName);
						CreateRoomSessionInternal(RoomName);
					}
				}, 0.5f, false); // 0.5초 후 재시도
				return;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[방 생성] ✅ 리슨 서버 NetDriver 확인됨 - CreateSession 진행"));
			}
		}
		else if (NetMode == NM_Standalone && !NetDriver)
		{
			UE_LOG(LogTemp, Warning, TEXT("[방 생성] ⚠️ Standalone 모드이고 NetDriver가 없습니다."));
			UE_LOG(LogTemp, Warning, TEXT("[방 생성] ListenServer 모드로 자동 전환 후 방 생성을 진행합니다..."));
			
			// GameInstance에 방 이름 저장 (ListenServer 전환 후 자동 방 생성용)
			if (UBRGameInstance* BRGI = Cast<UBRGameInstance>(World->GetGameInstance()))
			{
				BRGI->SetPendingRoomName(RoomName);
				UE_LOG(LogTemp, Warning, TEXT("[방 생성] PendingRoomName 설정: %s"), *RoomName);
			}
			
			// 현재 맵 경로 가져오기
			FString CurrentMapPath = UGameplayStatics::GetCurrentLevelName(World, true);
			if (CurrentMapPath.IsEmpty())
			{
				CurrentMapPath = World->GetMapName();
				CurrentMapPath.RemoveFromStart(World->StreamingLevelsPrefix);
			}
			
			// 맵 경로를 /Game/.../MapName.MapName 형식으로 변환
			if (!CurrentMapPath.Contains(TEXT("/")))
			{
				CurrentMapPath = FString::Printf(TEXT("/Game/Main/Level/%s.%s"), *CurrentMapPath, *CurrentMapPath);
			}
			else if (!CurrentMapPath.Contains(TEXT(".")))
			{
				FString MapName = FPaths::GetBaseFilename(CurrentMapPath);
				CurrentMapPath = FString::Printf(TEXT("%s.%s"), *CurrentMapPath, *MapName);
			}
			
			// 맵 경로가 유효한지 확인
			if (CurrentMapPath.IsEmpty())
			{
				UE_LOG(LogTemp, Error, TEXT("[방 생성] 맵 경로를 가져올 수 없습니다. ListenServer 전환을 건너뜁니다."));
				OnCreateSessionComplete.Broadcast(false);
				return;
			}
			
			FString ListenURL = FString::Printf(TEXT("%s?listen"), *CurrentMapPath);
			FString OpenCommand = FString::Printf(TEXT("open %s"), *ListenURL);
			
			UE_LOG(LogTemp, Warning, TEXT("[방 생성] ListenServer 모드로 자동 전환: %s"), *OpenCommand);
			
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
					FString::Printf(TEXT("[방 생성] ListenServer 모드로 전환 중...\n맵 로드 후 자동으로 방 생성됩니다.")));
			}
			
			// PlayerController를 통한 ConsoleCommand 실행
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				PC->ConsoleCommand(OpenCommand, /*bExecInEditor=*/false);
				UE_LOG(LogTemp, Warning, TEXT("[방 생성] ✅ openListenServer 명령어 실행 완료: %s"), *OpenCommand);
			}
			else
			{
				// PlayerController가 없으면 GEngine->Exec 사용
				if (GEngine)
				{
					bool bExecResult = GEngine->Exec(World, *OpenCommand);
					UE_LOG(LogTemp, Warning, TEXT("[방 생성] GEngine->Exec 결과: %s"), bExecResult ? TEXT("성공") : TEXT("실패"));
				}
			}
			
			// openListenServer 실행 후 맵이 다시 로드되면 OnStart()에서 PendingRoomName으로 자동 방 생성됨
			// 여기서는 OnCreateSessionComplete를 호출하지 않음 (맵 로드 후 자동 방 생성에서 호출됨)
			return;
		}
	}
	
	// GitHub 예제 참고: 기존 세션이 있으면 제거 (간단하게 처리)
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] 기존 세션이 발견되었습니다. 제거 중..."));
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] 기존 세션 정보: 최대 인원=%d, 현재 인원=%d"), 
			ExistingSession->SessionSettings.NumPublicConnections,
			ExistingSession->NumOpenPublicConnections);
		
		// GitHub 예제처럼 간단하게 처리: DestroySession 호출하고 대기
		PendingRoomName = RoomName;
		bPendingCreateSession = true;
		SessionInterface->DestroySession(NAME_GameSession);
		// DestroySession은 비동기이므로 OnDestroySessionCompleteDelegate에서 CreateSession 호출
		return;
	}
	
	// GitHub 예제 참고: 세션 설정 생성 (간단하게)
	SessionSettings = MakeShareable(new FOnlineSessionSettings());
	
	// GitHub 예제: SessionSettings.bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	FString SubsystemName = OnlineSubsystem ? OnlineSubsystem->GetSubsystemName().ToString() : TEXT("NULL");
	
	// GitHub 예제처럼 간단하게 처리
	SessionSettings->bIsLANMatch = (SubsystemName == TEXT("NULL"));
	
	if (SubsystemName.Equals(TEXT("NULL"), ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] NULL 서브시스템 사용 중 (LAN 모드)"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] %s 서브시스템 사용 중 (인터넷 모드)"), *SubsystemName);
	}
	
	// GameMode에서 최소/최대 플레이어 수 가져오기
	int32 MinPlayerCount = 4; // 기본값
	int32 MaxPlayerCount = 8; // 기본값
	
	World = GetWorld();
	if (World)
	{
		if (ABRGameMode* BRGameMode = World->GetAuthGameMode<ABRGameMode>())
		{
			MinPlayerCount = BRGameMode->MinPlayers;
			MaxPlayerCount = BRGameMode->MaxPlayers;
			UE_LOG(LogTemp, Warning, TEXT("[방 생성] GameMode에서 플레이어 수 설정 가져옴: MinPlayers=%d, MaxPlayers=%d"), MinPlayerCount, MaxPlayerCount);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[방 생성] GameMode를 찾을 수 없어 기본값 사용: MinPlayers=%d, MaxPlayers=%d"), MinPlayerCount, MaxPlayerCount);
		}
	}
	
	// 이전 코드: SessionSettings.NumPublicConnections = 24;
	SessionSettings->NumPublicConnections = MaxPlayerCount; // 최대 플레이어 수 설정
	// 최소 플레이어 수를 세션 설정에 추가 (검색 시 필터링에 사용)
	SessionSettings->Set(FName(TEXT("MIN_PLAYERS")), MinPlayerCount, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	
	// GitHub 예제 참고: Steam 세션 설정
	// bUsesPresence = true는 Steam Lobby에 필수
	SessionSettings->bUsesPresence = true;
	SessionSettings->bShouldAdvertise = true; // Steam에서 세션을 검색 가능하게 함
	
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] Steam 세션 설정: bUsesPresence=%s, bShouldAdvertise=%s"), 
		SessionSettings->bUsesPresence ? TEXT("true") : TEXT("false"),
		SessionSettings->bShouldAdvertise ? TEXT("true") : TEXT("false"));
	
	// Steam을 사용할 때 추가 설정 (GitHub 예제는 설정하지 않지만, 필요시 추가)
	// SessionSettings->bAllowInvites = true;
	// SessionSettings->bAllowJoinInProgress = true;
	// 이전 코드: SessionSettings.Set(SESSION_SETTINGS_KEY, DesiredServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	// 세션 이름 설정
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] 세션 설정: Subsystem=%s, bIsLANMatch=%s"), 
		*SubsystemName,
		SessionSettings->bIsLANMatch ? TEXT("true (LAN)") : TEXT("false (인터넷)"));

	// 이전 코드: SessionSettings->Set(SESSION_SETTINGS_KEY, DesiredServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	// 이전 코드의 SESSION_SETTINGS_KEY는 "FREE"였지만, 현재는 "SESSION_NAME" 사용
	// RoomName이 비어있으면 기본값 사용
	FString EffectiveRoomName = RoomName;
	if (EffectiveRoomName.IsEmpty())
	{
		EffectiveRoomName = TEXT("이름 없는 방");
	}
	SessionSettings->Set(FName(TEXT("SESSION_NAME")), EffectiveRoomName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// GitHub 예제 참고: Steam은 LocalUserNum=0을 사용 (단순하게 처리)
	// 복잡한 로그인 확인 로직 제거하고 GitHub 예제처럼 단순하게 처리
	int32 LocalUserNum = 0;
	
	// BuildUniqueId는 GitHub 예제에서 설정하지 않음 (설정하면 다른 빌드와 호환되지 않을 수 있음)
	// 주석 처리: if (!SubsystemName.Equals(TEXT("NULL"), ESearchCase::IgnoreCase)) { SessionSettings->BuildUniqueId = 1; }
	
	// 최소 플레이어 수 가져오기 (로그용)
	int32 LogMinPlayers = 4;
	SessionSettings->Get(FName(TEXT("MIN_PLAYERS")), LogMinPlayers);
	
	// GitHub 예제 참고: CreateSession 호출 전 기존 세션 최종 확인
	auto FinalCheckSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (FinalCheckSession != nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[방 생성] ⚠️ CreateSession 호출 직전에 기존 세션이 발견되었습니다!"));
		UE_LOG(LogTemp, Error, TEXT("[방 생성] 이것은 비정상적인 상황입니다. CreateSession을 시도하지 않습니다."));
		OnCreateSessionComplete.Broadcast(false);
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] CreateSession 호출 중... (Subsystem: %s, LocalUserNum: %d)"), 
		*SubsystemName, LocalUserNum);
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] 세션 설정: MinPlayers=%d, MaxPlayers=%d, bUsesPresence=%s, bShouldAdvertise=%s, bIsLANMatch=%s"), 
		LogMinPlayers,
		SessionSettings->NumPublicConnections,
		SessionSettings->bUsesPresence ? TEXT("true") : TEXT("false"),
		SessionSettings->bShouldAdvertise ? TEXT("true") : TEXT("false"),
		SessionSettings->bIsLANMatch ? TEXT("true") : TEXT("false"));
	
	// GitHub 예제 참고: CreateSession 호출 (단순하게)
	// GitHub 예제: SessionInterface->CreateSession(0, k_SessionName, SessionSettings);
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] CreateSession 호출 직전 최종 확인..."));
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] - SessionInterface 유효: %s"), SessionInterface.IsValid() ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] - LocalUserNum: %d"), LocalUserNum);
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] - SessionName: %s"), *FName(NAME_GameSession).ToString());
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] - NumPublicConnections: %d"), SessionSettings->NumPublicConnections);
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] - bUsesPresence: %s"), SessionSettings->bUsesPresence ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] - bShouldAdvertise: %s"), SessionSettings->bShouldAdvertise ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] - bIsLANMatch: %s"), SessionSettings->bIsLANMatch ? TEXT("true") : TEXT("false"));
	
	// 기존 세션 최종 재확인 (CreateSession 직전)
	auto FinalCheckBeforeCreate = SessionInterface->GetNamedSession(NAME_GameSession);
	if (FinalCheckBeforeCreate != nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[방 생성] ❌ CreateSession 호출 직전에 기존 세션이 발견되었습니다!"));
		UE_LOG(LogTemp, Error, TEXT("[방 생성] 이것이 CreateSession 실패의 주요 원인입니다."));
		UE_LOG(LogTemp, Error, TEXT("[방 생성] 기존 세션: 최대 인원=%d, 현재 인원=%d"), 
			FinalCheckBeforeCreate->SessionSettings.NumPublicConnections,
			FinalCheckBeforeCreate->NumOpenPublicConnections);
		
		// 기존 세션 강제 제거 시도
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] 기존 세션 강제 제거 시도..."));
		PendingRoomName = RoomName;
		bPendingCreateSession = true;
		SessionInterface->DestroySession(NAME_GameSession);
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Orange, 
				TEXT("[방 생성] 기존 세션 발견! 제거 후 재시도 중..."));
		}
		return; // DestroySession 완료 후 OnDestroySessionCompleteDelegate에서 재시도
	}
	
	// GitHub 예제 참고: CreateSession 호출 전 Steam 로그인 상태 확인
	// Steam인 경우 로그인 상태를 먼저 확인하고, 로그인되지 않았으면 CreateSession을 호출하지 않음
	if (SubsystemName.Equals(TEXT("Steam"), ESearchCase::IgnoreCase))
	{
		if (OnlineSubsystem)
		{
			IOnlineIdentityPtr IdentityInterface = OnlineSubsystem->GetIdentityInterface();
			if (IdentityInterface.IsValid())
			{
				bool bHasLoggedInUser = false;
				for (int32 UserIdx = 0; UserIdx < 4; UserIdx++)
				{
					ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(UserIdx);
					UE_LOG(LogTemp, Warning, TEXT("[방 생성] CreateSession 전 확인: UserIdx=%d, LoginStatus=%d"), UserIdx, (int32)LoginStatus);
					if (LoginStatus == ELoginStatus::LoggedIn)
					{
						bHasLoggedInUser = true;
						UE_LOG(LogTemp, Warning, TEXT("[방 생성] ✅ Steam 로그인 확인됨 (UserIdx=%d) - CreateSession 진행"), UserIdx);
						break;
					}
				}
				if (!bHasLoggedInUser)
				{
					UE_LOG(LogTemp, Error, TEXT("[방 생성] ❌ Steam에 로그인된 사용자가 없습니다!"));
					UE_LOG(LogTemp, Error, TEXT("[방 생성] CreateSession을 호출하지 않습니다."));
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, 
							TEXT("[방 생성] ❌ Steam 로그인 필요!\nSteam 클라이언트를 실행하고 로그인하세요."));
					}
					OnCreateSessionComplete.Broadcast(false);
					return;
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[방 생성] ⚠️ Identity Interface를 가져올 수 없지만 CreateSession을 시도합니다."));
			}
		}
	}
	
	// GitHub 예제 참고: CreateSession 호출
	// GitHub 예제: SessionInterface->CreateSession(0, k_SessionName, SessionSettings);
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] CreateSession 호출: LocalUserNum=%d, SessionName=%s"), 
		LocalUserNum, *FName(NAME_GameSession).ToString());
	
	bool bCreateResult = SessionInterface->CreateSession(LocalUserNum, NAME_GameSession, *SessionSettings);
	
	if (!bCreateResult)
	{
		UE_LOG(LogTemp, Error, TEXT("========================================"));
		UE_LOG(LogTemp, Error, TEXT("[방 생성] ❌ CreateSession 호출이 즉시 실패했습니다!"));
		UE_LOG(LogTemp, Error, TEXT("[방 생성] Subsystem: %s"), *SubsystemName);
		
		// 기존 세션 재확인
		auto CheckSession = SessionInterface->GetNamedSession(NAME_GameSession);
		if (CheckSession != nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("[방 생성] ⚠️ 기존 세션이 아직 존재합니다! 이것이 실패 원인일 가능성이 높습니다."));
			UE_LOG(LogTemp, Error, TEXT("[방 생성] 기존 세션: 최대 인원=%d, 현재 인원=%d"), 
				CheckSession->SessionSettings.NumPublicConnections,
				CheckSession->NumOpenPublicConnections);
			
			// 기존 세션 강제 제거 후 재시도
			UE_LOG(LogTemp, Warning, TEXT("[방 생성] 기존 세션 강제 제거 후 재시도..."));
			PendingRoomName = RoomName;
			bPendingCreateSession = true;
			SessionInterface->DestroySession(NAME_GameSession);
			return; // OnDestroySessionCompleteDelegate에서 재시도
		}
		
		// Steam 특정 진단
		if (SubsystemName.Equals(TEXT("Steam"), ESearchCase::IgnoreCase))
		{
			UE_LOG(LogTemp, Error, TEXT("[방 생성] Steam 세션 생성 실패 - 상세 진단:"));
			
			// Steam 로그인 상태 확인
			if (OnlineSubsystem)
			{
				IOnlineIdentityPtr IdentityInterface = OnlineSubsystem->GetIdentityInterface();
				if (IdentityInterface.IsValid())
				{
					bool bFoundLoggedIn = false;
					for (int32 UserIdx = 0; UserIdx < 4; UserIdx++)
					{
						ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(UserIdx);
						UE_LOG(LogTemp, Error, TEXT("[방 생성] UserIdx=%d, LoginStatus=%d (%s)"), 
							UserIdx, (int32)LoginStatus,
							LoginStatus == ELoginStatus::LoggedIn ? TEXT("LoggedIn") :
							LoginStatus == ELoginStatus::NotLoggedIn ? TEXT("NotLoggedIn") :
							LoginStatus == ELoginStatus::UsingLocalProfile ? TEXT("UsingLocalProfile") :
							TEXT("Unknown"));
						if (LoginStatus == ELoginStatus::LoggedIn)
						{
							bFoundLoggedIn = true;
							UE_LOG(LogTemp, Warning, TEXT("[방 생성] ✅ Steam 로그인 확인: UserIdx=%d"), UserIdx);
							
							// 사용자 ID 확인
							TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(UserIdx);
							if (UserId.IsValid())
							{
								UE_LOG(LogTemp, Warning, TEXT("[방 생성] Steam 사용자 ID: %s"), *UserId->ToString());
							}
						}
					}
					if (!bFoundLoggedIn)
					{
						UE_LOG(LogTemp, Error, TEXT("[방 생성] ❌ Steam에 로그인된 사용자가 없습니다!"));
						UE_LOG(LogTemp, Error, TEXT("[방 생성] → Steam 클라이언트를 실행하고 로그인하세요."));
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[방 생성] ❌ Identity Interface를 가져올 수 없습니다."));
					UE_LOG(LogTemp, Error, TEXT("[방 생성] → Steam Online Subsystem 초기화 문제일 수 있습니다."));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[방 생성] ❌ OnlineSubsystem이 NULL입니다!"));
			}
		}
		
		UE_LOG(LogTemp, Error, TEXT("[방 생성] 가능한 원인:"));
		UE_LOG(LogTemp, Error, TEXT("  1. 기존 세션이 아직 제거되지 않음 (위에서 확인됨)"));
		UE_LOG(LogTemp, Error, TEXT("  2. Steam 클라이언트가 실행되지 않음"));
		UE_LOG(LogTemp, Error, TEXT("  3. Steam에 로그인되지 않음 (위에서 확인됨)"));
		UE_LOG(LogTemp, Error, TEXT("  4. Steam 네트워크 연결 문제"));
		UE_LOG(LogTemp, Error, TEXT("  5. Steam App ID 설정 문제 (현재: 480)"));
		UE_LOG(LogTemp, Error, TEXT("========================================"));
		
		if (GEngine)
		{
			FString ErrorMsg = TEXT("[방 생성] CreateSession 즉시 실패!\n");
			ErrorMsg += FString::Printf(TEXT("Subsystem: %s\n"), *SubsystemName);
			if (CheckSession != nullptr)
			{
				ErrorMsg += TEXT("⚠️ 기존 세션이 아직 존재합니다!\n");
			}
			if (SubsystemName.Equals(TEXT("Steam"), ESearchCase::IgnoreCase))
			{
				ErrorMsg += TEXT("Steam 클라이언트 확인 필요\n");
				ErrorMsg += TEXT("1. Steam 실행 확인\n");
				ErrorMsg += TEXT("2. Steam 로그인 확인");
			}
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, ErrorMsg);
		}
		
		OnCreateSessionComplete.Broadcast(false);
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] CreateSession 호출 성공 (비동기 처리 대기 중...)"));
}

void ABRGameSession::FindSessions()
{
	UE_LOG(LogTemp, Log, TEXT("[방 찾기] 세션 검색 시작"));
	
	// 화면에 디버그 메시지 표시
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("[방 찾기] 세션 검색 시작..."));
	}
	
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[방 찾기] 실패: SessionInterface가 유효하지 않습니다."));
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[방 찾기] 실패: SessionInterface가 유효하지 않습니다!"));
		}
		
		OnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>());
		return;
	}

	// 이미 검색 중이면 무시
	if (bIsSearchingSessions)
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 이미 검색이 진행 중입니다. 기다려주세요."));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("[방 찾기] 이미 검색이 진행 중입니다. 기다려주세요."));
		}
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
		GetWorld()->GetTimerManager().ClearTimer(FindSessionsRetryHandle);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 자동 재검색 실행 (%d/%d)"), FindSessionsRetryCount, MaxFindSessionsRetries);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("[방 찾기] 재검색 중..."));
		}
	}

	// 검색 시작 플래그 설정
	bIsSearchingSessions = true;

	// 이전 검색 취소 (중복/스태일 결과 방지)
	SessionInterface->CancelFindSessions();

	// 이전 코드(OSS251030last)처럼 간단하게 설정
	// 이전 코드: SessionSearch = MakeShareable(new FOnlineSessionSearch());
	// 이전 코드: SessionSearch->MaxSearchResults = 100;
	// 이전 코드: SessionSearch->QuerySettings.Set(FName(TEXT("PRESENCESEARCH")),true, EOnlineComparisonOp::Equals);
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = 100;
	SessionSearch->QuerySettings.Set(FName(TEXT("PRESENCESEARCH")), true, EOnlineComparisonOp::Equals);
	
	// LAN 여부는 세션 생성 시와 동일하게 처리
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem && OnlineSubsystem->GetSubsystemName() == "NULL")
	{
		SessionSearch->bIsLanQuery = true;
	}
	else
	{
		SessionSearch->bIsLanQuery = false;
	}
	
	// 이전 코드: UE_LOG(LogTemp, Warning, TEXT("Finding Session"));
	// 이전 코드: SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 검색 요청 중..."));
	
	bool bFindSessionsResult = SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	if (!bFindSessionsResult)
	{
		UE_LOG(LogTemp, Error, TEXT("[방 찾기] FindSessions 호출이 즉시 실패했습니다!"));
		UE_LOG(LogTemp, Error, TEXT("[방 찾기] 가능한 원인:"));
		UE_LOG(LogTemp, Error, TEXT("  1. SessionInterface가 유효하지 않음"));
		UE_LOG(LogTemp, Error, TEXT("  2. Steam 네트워크 연결 문제"));
		UE_LOG(LogTemp, Error, TEXT("  3. 이미 검색이 진행 중"));
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			// 위에서 선언한 OnlineSubsystem 변수 재사용
			FString SubsystemName = OnlineSubsystem ? OnlineSubsystem->GetSubsystemName().ToString() : TEXT("Unknown");
			FString ErrorMsg = TEXT("[방 찾기] FindSessions 호출 실패!\n");
			ErrorMsg += FString::Printf(TEXT("Subsystem: %s\n"), *SubsystemName);
			ErrorMsg += TEXT("Steam 네트워크 연결 확인 필요");
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, ErrorMsg);
		}
		
		bIsSearchingSessions = false;
		OnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>());
		OnFindSessionsCompleteBP.Broadcast(0);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 찾기] FindSessions 호출 성공 (비동기 처리 대기 중...)"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("[방 찾기] 검색 중... 잠시만 기다려주세요."));
		}
	}
}

void ABRGameSession::JoinSessionByIndex(int32 SessionIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("[방 참가] JoinSessionByIndex 호출됨: 세션 인덱스=%d"), SessionIndex);
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	// SessionInterface 유효성 확인 (Standalone 모드에서 NULL일 수 있음)
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[방 참가] 실패: SessionInterface가 NULL입니다. Online Subsystem이 초기화되지 않았습니다."));
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			FString ErrorMsg = TEXT("[방 참가] 실패: Online Subsystem이 NULL입니다!\n");
			ErrorMsg += TEXT("Standalone 모드에서는 세션 기능을 사용할 수 없습니다.\n");
			ErrorMsg += TEXT("Listen Server 모드를 사용하세요 (Number of Players: 2+)");
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, ErrorMsg);
		}
		
		OnJoinSessionComplete.Broadcast(false);
		return;
	}
	
	if (!SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[방 참가] 실패: 검색 결과가 없습니다. 먼저 FindRooms를 실행하세요."));
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[방 참가] 실패: 검색 결과가 없습니다! 먼저 방 찾기를 실행하세요."));
		}
		
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	// 이전 코드: if(SessionSearch->SearchResults.Num() > (int32)Index) SessionInterface->JoinSession(...); else UE_LOG(LogTemp, Warning, TEXT("Empty Session"));
	if (SessionIndex < 0 || SessionIndex >= SessionSearch->SearchResults.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 참가] Empty Session"));
		UE_LOG(LogTemp, Error, TEXT("[방 참가] 실패: 잘못된 세션 인덱스 (%d). 사용 가능한 범위: 0-%d"), 
			SessionIndex, SessionSearch->SearchResults.Num() - 1);
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			FString ErrorMsg = FString::Printf(TEXT("[방 참가] 실패: 잘못된 세션 인덱스 (%d). 범위: 0-%d"), 
				SessionIndex, SessionSearch->SearchResults.Num() - 1);
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, ErrorMsg);
		}
		
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
		UE_LOG(LogTemp, Error, TEXT("[방 참가] 실패: SessionInterface가 유효하지 않습니다."));
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[방 참가] 실패: SessionInterface가 유효하지 않습니다!"));
		}
		
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	// 기존 세션이 있으면 제거 (다른 세션에 참가하기 전에)
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("[방 참가] 기존 세션 제거 중... (다른 세션에 참가하기 위해)"));
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("[방 참가] 기존 세션 제거 중..."));
		}
		
		SessionInterface->DestroySession(NAME_GameSession);
		// DestroySession은 비동기이므로 잠시 대기 후 참가 시도
		// 실제로는 DestroySession 완료 콜백을 기다려야 하지만, 간단한 테스트를 위해 바로 시도
	}

	FString FoundSessionName;
	if (SessionResult.Session.SessionSettings.Get(FName(TEXT("SESSION_NAME")), FoundSessionName))
	{
		UE_LOG(LogTemp, Log, TEXT("[방 참가] 세션 참가 시도: %s"), *FoundSessionName);
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			FString Message = FString::Printf(TEXT("[방 참가] 세션 참가 시도: %s"), *FoundSessionName);
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, Message);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[방 참가] 세션 참가 시도 중..."));
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("[방 참가] 세션 참가 시도 중..."));
		}
	}

	// 세션 정보 상세 로깅
	UE_LOG(LogTemp, Warning, TEXT("[방 참가] 세션 정보: Ping=%dms, NumOpenPublicConnections=%d/%d"), 
		SessionResult.PingInMs,
		SessionResult.Session.SessionSettings.NumPublicConnections - SessionResult.Session.NumOpenPublicConnections,
		SessionResult.Session.SessionSettings.NumPublicConnections);
	
	// Ping 9999(또는 -9999)는 호스트 연결 불가를 의미 → 참가 실패 가능성 큼
	if (SessionResult.PingInMs >= 9999 || SessionResult.PingInMs <= -9999)
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 참가] 경고: Ping=%d → 호스트에 연결되지 않을 수 있습니다. (방화벽/포트 7777, 같은 LAN 확인)"), SessionResult.PingInMs);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Orange, 
				TEXT("[방 참가] 이 방은 Ping 비정상입니다.\n방화벽·같은 LAN 확인 후 다시 시도하세요."));
		}
	}

	// 세션 참가
	UE_LOG(LogTemp, Warning, TEXT("[방 참가] SessionInterface->JoinSession() 호출 중..."));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("[방 참가] SessionInterface->JoinSession() 호출 중..."));
	}
	
	SessionInterface->JoinSession(0, NAME_GameSession, SessionResult);
	
	UE_LOG(LogTemp, Warning, TEXT("[방 참가] SessionInterface->JoinSession() 호출 완료 (비동기 처리 중)"));
}

void ABRGameSession::OnDestroySessionCompleteDelegate(FName InSessionName, bool bWasSuccessful)
{
	// 이전 코드: if (IsSuccess == true) CreateSession();
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] OnDestroySessionCompleteDelegate 호출됨: InSessionName=%s, bWasSuccessful=%s"), 
		*InSessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"));
	
	if (bWasSuccessful && bPendingCreateSession)
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] 기존 세션 제거 완료! 이제 CreateSession을 호출합니다."));
		// DestroySession 완료 후 CreateSession 호출
		FString RoomNameToUse = PendingRoomName;
		bPendingCreateSession = false;
		PendingRoomName.Empty();
		
		// CreateRoomSession의 CreateSession 부분만 호출 (기존 세션 체크 제외)
		CreateRoomSessionInternal(RoomNameToUse);
	}
	else if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("[방 생성] 기존 세션 제거 실패! CreateSession을 시도합니다."));
		// DestroySession 실패해도 CreateSession 시도
		if (bPendingCreateSession)
		{
			FString RoomNameToUse = PendingRoomName;
			bPendingCreateSession = false;
			PendingRoomName.Empty();
			CreateRoomSessionInternal(RoomNameToUse);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 생성] OnDestroySessionCompleteDelegate: bPendingCreateSession=false이므로 CreateSession 호출하지 않음"));
	}
}

void ABRGameSession::OnCreateSessionCompleteDelegate(FName InSessionName, bool bWasSuccessful)
{
	// 이전 코드(OSS251030last)처럼 간단하게 처리
	// 이전 코드: if (!IsSuccess) { UE_LOG(LogTemp, Error, TEXT("Could not Createsession")); return; }
	if (!bWasSuccessful)
	{
		// 이전 코드: UE_LOG(LogTemp, Error, TEXT("Could not Createsession"));
		UE_LOG(LogTemp, Error, TEXT("[방 생성] Could not Createsession"));
		UE_LOG(LogTemp, Error, TEXT("========================================"));
		UE_LOG(LogTemp, Error, TEXT("[방 생성] 실패: 세션 생성에 실패했습니다."));
		UE_LOG(LogTemp, Error, TEXT("세션 이름: %s"), *InSessionName.ToString());
		
		// 실패 원인 확인
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		FString SubsystemName = OnlineSubsystem ? OnlineSubsystem->GetSubsystemName().ToString() : TEXT("Unknown");
		UE_LOG(LogTemp, Error, TEXT("Online Subsystem: %s"), *SubsystemName);
		
		if (SubsystemName.Equals(TEXT("Steam"), ESearchCase::IgnoreCase))
		{
			UE_LOG(LogTemp, Error, TEXT("[방 생성] Steam 세션 생성 실패 가능 원인:"));
			
			// Steam 로그인 상태 확인
			if (OnlineSubsystem)
			{
				IOnlineIdentityPtr IdentityInterface = OnlineSubsystem->GetIdentityInterface();
				if (IdentityInterface.IsValid())
				{
					bool bFoundLoggedInUser = false;
					for (int32 UserIdx = 0; UserIdx < 4; UserIdx++)
					{
						ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(UserIdx);
						if (LoginStatus == ELoginStatus::LoggedIn)
						{
							UE_LOG(LogTemp, Warning, TEXT("[방 생성] ✅ Steam 로그인 확인됨: LocalUserNum=%d"), UserIdx);
							bFoundLoggedInUser = true;
							break;
						}
					}
					if (!bFoundLoggedInUser)
					{
						UE_LOG(LogTemp, Error, TEXT("  ❌ Steam에 로그인된 사용자가 없습니다!"));
						UE_LOG(LogTemp, Error, TEXT("     → Steam 클라이언트를 실행하고 로그인하세요."));
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("  ⚠️ Identity Interface를 가져올 수 없습니다."));
				}
			}
			
			UE_LOG(LogTemp, Error, TEXT("  1. Steam 클라이언트가 실행되지 않음"));
			UE_LOG(LogTemp, Error, TEXT("  2. Steam에 로그인되지 않음 (위에서 확인됨)"));
			UE_LOG(LogTemp, Error, TEXT("  3. Steam 네트워크 연결 문제"));
			UE_LOG(LogTemp, Error, TEXT("  4. Steam App ID 설정 문제 (현재: 480)"));
			UE_LOG(LogTemp, Error, TEXT("  5. 기존 세션이 아직 제거되지 않음"));
			
			// 기존 세션 확인
			if (SessionInterface.IsValid())
			{
				auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
				if (ExistingSession != nullptr)
				{
					UE_LOG(LogTemp, Error, TEXT("  ⚠️ 기존 세션이 아직 존재합니다!"));
					UE_LOG(LogTemp, Error, TEXT("     → 세션을 제거하고 다시 시도하세요."));
				}
			}
		}
		else if (SubsystemName.Equals(TEXT("NULL"), ESearchCase::IgnoreCase))
		{
			UE_LOG(LogTemp, Error, TEXT("[방 생성] NULL Subsystem - LAN 모드 사용 중"));
			UE_LOG(LogTemp, Error, TEXT("  1. 같은 LAN에 연결되어 있는지 확인"));
			UE_LOG(LogTemp, Error, TEXT("  2. 방화벽에서 포트 7777 허용 확인"));
		}
		
		UE_LOG(LogTemp, Error, TEXT("========================================"));
		
		if (GEngine)
		{
			FString ErrorMsg = TEXT("[방 생성] 세션 생성 실패!\n");
			ErrorMsg += FString::Printf(TEXT("Subsystem: %s\n"), *SubsystemName);
			if (SubsystemName.Equals(TEXT("Steam"), ESearchCase::IgnoreCase))
			{
				ErrorMsg += TEXT("Steam 클라이언트 확인 필요");
			}
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, ErrorMsg);
		}
		
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	// 이전 코드: UE_LOG(LogTemp, Warning, TEXT("Session name is %s"), *InSessionName.ToString());
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] 성공: 세션 이름 = %s"), *InSessionName.ToString());

	// 이전 코드: Engine->AddOnScreenDebugMessage(0, 2, FColor::Green, TEXT("Host Complate!"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("[방 생성] 완료!"));
	}

	// 이전 코드: World->ServerTravel("/Game/Maps/Lobby?listen");
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[방 생성] World를 찾을 수 없습니다!"));
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	// 로비 맵 경로 결정: GameMode의 LobbyMapPath 사용, 없으면 이전 코드처럼 "/Game/Maps/Lobby"
	FString LobbyMapPath = TEXT("/Game/Maps/Lobby"); // 이전 코드 기본값
	if (AGameModeBase* GM = World->GetAuthGameMode())
	{
		if (ABRGameMode* BRGM = Cast<ABRGameMode>(GM))
		{
			if (!BRGM->LobbyMapPath.IsEmpty())
			{
				LobbyMapPath = BRGM->LobbyMapPath;
			}
		}
	}

	// 처음부터 ListenServer 모드로 시작하므로 ServerTravel만 사용
	// (GameInstance의 OnStart에서 Standalone 모드를 자동으로 ListenServer로 전환)
	FString ListenURL = FString::Printf(TEXT("%s?listen"), *LobbyMapPath);
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] 리슨 서버로 전환: %s"), *ListenURL);
	
	// ListenServer 모드에서 ServerTravel 사용
	World->ServerTravel(ListenURL, true);
	UE_LOG(LogTemp, Warning, TEXT("[방 생성] ✅ ListenServer 모드에서 ServerTravel 실행"));
	
	OnCreateSessionComplete.Broadcast(true);
}

void ABRGameSession::OnFindSessionsCompleteDelegate(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("[방 찾기] OnFindSessionsCompleteDelegate 호출됨"));
	UE_LOG(LogTemp, Warning, TEXT("  - bWasSuccessful: %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("  - bIsSearchingSessions: %s"), bIsSearchingSessions ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("  - SessionSearch.IsValid(): %s"), SessionSearch.IsValid() ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	// 검색이 진행 중이 아니면 무시 (중복 콜백 방지)
	// 단, 첫 번째 콜백이 이미 처리되었을 수 있으므로 조용히 무시
	if (!bIsSearchingSessions)
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 이미 처리된 콜백이므로 무시합니다."));
		// 이미 처리된 콜백이므로 조용히 무시 (로그 출력 안 함)
		return;
	}

	// 검색 완료 플래그 해제
	bIsSearchingSessions = false;

	// OnlineSubsystem을 함수 상단에서 한 번만 가져옴 (중복 선언 방지)
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	FString SubsystemName = OnlineSubsystem ? OnlineSubsystem->GetSubsystemName().ToString() : TEXT("Unknown");

	TArray<FOnlineSessionSearchResult> Results;
	
	if (bWasSuccessful)
	{
		if (SessionSearch.IsValid())
		{
			Results = SessionSearch->SearchResults;
			UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 성공: 찾은 세션 수(중복 제거 전) = %d"), Results.Num());

			// SessionId 기준 중복 제거 (같은 방이 여러 번 나오는 현상 방지)
			const int32 RawCount = Results.Num();
			TSet<FString> SeenIds;
			TArray<FOnlineSessionSearchResult> Deduplicated;
			for (const FOnlineSessionSearchResult& R : Results)
			{
				FString Sid = R.GetSessionIdStr();
				if (Sid.IsEmpty()) Sid = FString::Printf(TEXT("row-%d"), Deduplicated.Num());
				if (SeenIds.Contains(Sid)) continue;
				SeenIds.Add(Sid);
				Deduplicated.Add(R);
			}
			Results = MoveTemp(Deduplicated);
			SessionSearch->SearchResults = Results;
			if (RawCount != Results.Num())
			{
				UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 중복 제거: %d개 → %d개"), RawCount, Results.Num());
			}
			UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 성공: 찾은 세션 수(중복 제거 후) = %d"), Results.Num());
			
			// Steam 세션 검색 시 추가 정보
			UE_LOG(LogTemp, Warning, TEXT("[방 찾기] Online Subsystem: %s"), *SubsystemName);
			
			if (SubsystemName.Equals(TEXT("Steam"), ESearchCase::IgnoreCase))
			{
				if (Results.Num() == 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("========================================"));
					UE_LOG(LogTemp, Warning, TEXT("[방 찾기] Steam 세션을 찾지 못했습니다."));
					UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 가능한 원인:"));
					UE_LOG(LogTemp, Warning, TEXT("  1. 서버 PC에서 방이 아직 생성되지 않음"));
					UE_LOG(LogTemp, Warning, TEXT("     → 서버 로그에서 '[방 생성] CreateSession 호출 성공' 확인"));
					UE_LOG(LogTemp, Warning, TEXT("     → 서버 로그에서 '[방 생성] 성공: 세션 이름 = ...' 확인"));
					UE_LOG(LogTemp, Warning, TEXT("  2. 서버 PC에서 bShouldAdvertise=false로 설정됨"));
					UE_LOG(LogTemp, Warning, TEXT("  3. 서버와 클라이언트 모두 Steam에 로그인되어 있는지 확인"));
					UE_LOG(LogTemp, Warning, TEXT("  4. Steam 네트워크 문제 (방화벽, NAT 등)"));
					UE_LOG(LogTemp, Warning, TEXT("  5. 방 생성과 방 찾기의 세션 설정이 일치하지 않음"));
					UE_LOG(LogTemp, Warning, TEXT("  6. Steam App ID가 서버와 클라이언트에서 동일한지 확인 (현재: 480)"));
					UE_LOG(LogTemp, Warning, TEXT("========================================"));
					
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Orange,
							TEXT("[방 찾기] 방을 찾지 못했습니다.\n서버에서 방이 생성되었는지 확인하세요."));
					}
				}
			}
			
			// 화면에 디버그 메시지 표시
			if (GEngine)
			{
				FString Message = FString::Printf(TEXT("[방 찾기] 완료: %d개의 방을 찾았습니다."), Results.Num());
				FColor MessageColor = Results.Num() > 0 ? FColor::Green : FColor::Yellow;
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, MessageColor, Message);
			}
			
			if (Results.Num() > 0)
			{
				UE_LOG(LogTemp, Log, TEXT("[방 찾기] 참가하려면 'JoinRoom [인덱스]' 명령어를 사용하세요."));
				for (int32 i = 0; i < Results.Num(); i++)
				{
					const FOnlineSessionSearchResult& Result = Results[i];
					
					// 이전 코드: UE_LOG(LogTemp, Display, TEXT("Found Session name : %s"), *SearchResult.GetSessionIdStr());
					// 이전 코드: UE_LOG(LogTemp, Display, TEXT("Ping : %d"), SearchResult.PingInMs);
					UE_LOG(LogTemp, Display, TEXT("[방 찾기] Found Session name : %s"), *Result.GetSessionIdStr());
					UE_LOG(LogTemp, Display, TEXT("[방 찾기] Ping : %d"), Result.PingInMs);
					
					FString FoundSessionName;
					FString FoundHostName;
					FString FoundMapName;
					Result.Session.SessionSettings.Get(FName(TEXT("SESSION_NAME")), FoundSessionName);
					Result.Session.SessionSettings.Get(FName(TEXT("HOST_NAME")), FoundHostName);
					
					// 이전 코드: if (SearchResult.Session.SessionSettings.Get(SESSION_SETTINGS_KEY, ServerName)) ServerData.Name = ServerName; else UE_LOG(LogTemp, Warning, TEXT("Session Name Not Found"));
					if (FoundSessionName.IsEmpty())
					{
						UE_LOG(LogTemp, Warning, TEXT("[방 찾기] Session Name Not Found"));
					}
					
					// 표시 이름: SESSION_NAME → HOST_NAME → "(이름 없음)"
					FString DisplayName = !FoundSessionName.IsEmpty() ? FoundSessionName : (!FoundHostName.IsEmpty() ? FoundHostName : TEXT("(이름 없음)"));

					int32 CurrentPlayerCount = Result.Session.NumOpenPublicConnections + Result.Session.NumOpenPrivateConnections;
					int32 MaxPlayerCount = Result.Session.SessionSettings.NumPublicConnections + Result.Session.SessionSettings.NumPrivateConnections;
					int32 ActualPlayerCount = FMath::Max(0, MaxPlayerCount - CurrentPlayerCount);

					UE_LOG(LogTemp, Log, TEXT("[방 찾기] 세션 [%d]: 이름=%s, Ping=%dms"), i, *DisplayName, Result.PingInMs);
					if (GEngine)
					{
						FString SessionInfo = FString::Printf(TEXT("  [%d] %s - 플레이어: %d/%d, Ping: %dms"),
							i, *DisplayName, ActualPlayerCount, MaxPlayerCount, Result.PingInMs);
						GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, SessionInfo);
					}
					
					if (Result.Session.SessionSettings.Get(FName(TEXT("MAPNAME")), FoundMapName))
					{
						UE_LOG(LogTemp, Log, TEXT("[방 찾기] 세션 [%d]: 맵=%s"), i, *FoundMapName);
					}
					UE_LOG(LogTemp, Log, TEXT("[방 찾기] 세션 [%d]: 플레이어 수=%d/%d"), i, ActualPlayerCount, MaxPlayerCount);
					UE_LOG(LogTemp, Log, TEXT("[방 찾기] 세션 [%d] 참가 명령: JoinRoom %d"), i, i);
				}
			}
			else
			{
				// 0건일 때 Steam이면 최대 2회 자동 재검색 (간헐적 실패 완화)
				if (SubsystemName.Equals(TEXT("Steam"), ESearchCase::IgnoreCase) &&
					FindSessionsRetryCount < MaxFindSessionsRetries)
				{
					FindSessionsRetryCount++;
					UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 세션 0건. %d초 후 재검색 (%d/%d)"), 2, FindSessionsRetryCount, MaxFindSessionsRetries);
					if (GEngine)
					{
						FString RetryMsg = FString::Printf(TEXT("[방 찾기] 방 없음 → 2초 후 재검색 (%d/%d)"), FindSessionsRetryCount, MaxFindSessionsRetries);
						GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Orange, RetryMsg);
					}
					GetWorld()->GetTimerManager().SetTimer(FindSessionsRetryHandle, this, &ABRGameSession::FindSessionsRetryCallback, 2.0f, false);
					return; // 브로드캐스트하지 않음, 재검색 후 콜백에서 처리
				}

				UE_LOG(LogTemp, Warning, TEXT("========================================"));
				UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 사용 가능한 세션이 없습니다."));
				
				UE_LOG(LogTemp, Warning, TEXT("Online Subsystem: %s"), *SubsystemName);
				
				if (SubsystemName.Equals(TEXT("Steam"), ESearchCase::IgnoreCase))
				{
					UE_LOG(LogTemp, Warning, TEXT("Steam 세션을 찾지 못한 가능한 원인:"));
					UE_LOG(LogTemp, Warning, TEXT("  1. 호스트 PC에서 먼저 방 생성 → '[방 생성] CreateSession 호출 성공' 로그 확인"));
					UE_LOG(LogTemp, Warning, TEXT("  2. 두 PC가 같은 LAN(와이파이/유선)에 연결되어 있는지 확인"));
					UE_LOG(LogTemp, Warning, TEXT("  3. Steam 양쪽 실행·로그인, 방화벽에서 게임/Steam 허용"));
					UE_LOG(LogTemp, Warning, TEXT("  4. 방 찾기 버튼 한 번 더 눌러 재검색"));
				}
				UE_LOG(LogTemp, Warning, TEXT("========================================"));
				
				if (GEngine)
				{
					FString WarningMsg = TEXT("[방 찾기] 사용 가능한 세션이 없습니다.\n");
					WarningMsg += TEXT("1. 호스트에서 먼저 방 만들기 → 생성 성공 로그 확인\n");
					WarningMsg += TEXT("2. 두 PC 같은 LAN 연결 확인\n");
					WarningMsg += TEXT("3. 방 찾기 다시 눌러 재검색");
					GEngine->AddOnScreenDebugMessage(-1, 12.0f, FColor::Yellow, WarningMsg);
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[방 찾기] 실패: SessionSearch가 유효하지 않습니다. (bWasSuccessful=true이지만)"));
			UE_LOG(LogTemp, Warning, TEXT("[방 찾기] 참고: 이는 비동기 타이밍 문제일 수 있습니다."));
			
			// 화면에 디버그 메시지 표시
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[방 찾기] 실패: SessionSearch가 유효하지 않습니다!"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[방 찾기] 실패: 세션 찾기에 실패했습니다. (bWasSuccessful=false)"));
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[방 찾기] 실패: 세션 찾기에 실패했습니다!"));
		}
	}

	// 이전 코드: UE_LOG(LogTemp, Warning, TEXT("Finished Finding Session"));
	UE_LOG(LogTemp, Warning, TEXT("[방 찾기] Finished Finding Session"));
	
	OnFindSessionsComplete.Broadcast(Results);
	
	// 블루프린트용 이벤트도 브로드캐스트 (세션 개수 전달)
	OnFindSessionsCompleteBP.Broadcast(Results.Num());
}

void ABRGameSession::OnJoinSessionCompleteDelegate(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	// 이전 코드(OSS251030last)처럼 Result를 체크하지 않고 바로 GetResolvedConnectString 시도
	// 성공/실패 여부와 관계없이 연결 문자열을 가져올 수 있으면 ClientTravel 시도
	bool bWasSuccessful = (Result == EOnJoinSessionCompleteResult::Success);
	
	// 이전 코드처럼 SessionInterface 유효성만 확인하고 바로 진행
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[방 참가] SessionInterface가 유효하지 않습니다."));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}
	
	// 세션 상태 확인 (GetResolvedConnectString 전에)
	UE_LOG(LogTemp, Warning, TEXT("[방 참가] ========================================"));
	UE_LOG(LogTemp, Warning, TEXT("[방 참가] OnJoinSessionCompleteDelegate 호출됨"));
	UE_LOG(LogTemp, Warning, TEXT("[방 참가] InSessionName: %s"), *InSessionName.ToString());
	UE_LOG(LogTemp, Warning, TEXT("[방 참가] Result 코드: %d"), (int32)Result);
	
	// 세션이 등록되었는지 확인
	auto Session = SessionInterface->GetNamedSession(InSessionName);
	if (Session)
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 참가] 세션 등록됨: NumOpenPublicConnections=%d/%d"), 
			Session->NumOpenPublicConnections, 
			Session->SessionSettings.NumPublicConnections);
		UE_LOG(LogTemp, Warning, TEXT("[방 참가] 세션 설정: bIsLANMatch=%s, bUsesPresence=%s"), 
			Session->SessionSettings.bIsLANMatch ? TEXT("true") : TEXT("false"),
			Session->SessionSettings.bUsesPresence ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[방 참가] ⚠️ 세션이 등록되지 않았습니다! GetNamedSession(%s) = NULL"), *InSessionName.ToString());
		UE_LOG(LogTemp, Error, TEXT("[방 참가] ⚠️ 이것이 GetResolvedConnectString 실패의 원인일 수 있습니다!"));
	}
	
	// 이전 코드: if (!SessionInterface->GetResolvedConnectString(InSessionName, Address)) { return; }
	// 이전 코드: if (!SessionInterface->GetResolvedConnectString(InSessionName, Address)) { UE_LOG(LogTemp, Error, TEXT("Could not convert IP Address")); return; }
	// 성공/실패 여부와 관계없이 연결 문자열 가져오기 시도
	FString TravelURL;
	bool bGotConnectString = SessionInterface->GetResolvedConnectString(InSessionName, TravelURL);
	
	UE_LOG(LogTemp, Warning, TEXT("[방 참가] GetResolvedConnectString 결과: %s"), bGotConnectString ? TEXT("성공") : TEXT("실패"));
	if (bGotConnectString)
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 참가] TravelURL: %s"), *TravelURL);
	}
	
	if (!bGotConnectString)
	{
		// 이전 코드: UE_LOG(LogTemp, Error, TEXT("Could not convert IP Address"));
		UE_LOG(LogTemp, Error, TEXT("[방 참가] Could not convert IP Address"));
		UE_LOG(LogTemp, Error, TEXT("[방 참가] ========================================"));
		UE_LOG(LogTemp, Error, TEXT("[방 참가] GetResolvedConnectString 실패! 연결 주소를 가져올 수 없습니다."));
		UE_LOG(LogTemp, Error, TEXT("[방 참가] 가능한 원인: 서버가 ListenServer 모드가 아니거나 Steam 연결 문제"));
		UE_LOG(LogTemp, Error, TEXT("[방 참가] Result 코드: %d"), (int32)Result);
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, 
				TEXT("[방 참가] 연결 주소를 가져올 수 없습니다.\n서버가 ListenServer 모드인지 확인하세요."));
		}
		
		OnJoinSessionComplete.Broadcast(false);
		return; // 이전 코드처럼 실패 시 바로 return
	}
	
	// 연결 문자열을 가져왔으면 성공/실패 여부와 관계없이 ClientTravel 시도
	// (이전 코드는 Result를 체크하지 않고 바로 ClientTravel 호출)
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("[방 참가] 성공: 세션 참가 완료 - %s"), *InSessionName.ToString());
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			FString Message = FString::Printf(TEXT("[방 참가] 성공! 세션: %s"), *InSessionName.ToString());
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, Message);
		}
		
		UE_LOG(LogTemp, Log, TEXT("[방 참가] 성공: 세션 참가 완료 - %s"), *InSessionName.ToString());
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			FString Message = FString::Printf(TEXT("[방 참가] 성공! 세션: %s"), *InSessionName.ToString());
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, Message);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 참가] Result가 Success가 아니지만 연결 문자열을 가져왔으므로 시도합니다. (Result: %d)"), (int32)Result);
	}
	
	// 이전 코드: Engine->AddOnScreenDebugMessage(0,5,FColor::Green,FString::Printf(TEXT("Joining To %s"),*Address));
	// 이전 코드: Engine->AddOnScreenDebugMessage(0,5,FColor::Green,FString::Printf(TEXT("Joining To %s"),*Address));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
			FString::Printf(TEXT("Joining To %s"), *TravelURL));
	}
	
	// 이전 코드: PC->ClientTravel(Address,ETravelType::TRAVEL_Absolute);
	// 이전 코드처럼 바로 ClientTravel 호출 (NetMode 체크 없이)
	UWorld* World = GetWorld();
	if (World)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			UE_LOG(LogTemp, Warning, TEXT("[방 참가] 서버로 이동: %s"), *TravelURL);
			PC->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[방 참가] PlayerController를 찾을 수 없습니다!"));
			OnJoinSessionComplete.Broadcast(false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[방 참가] World를 찾을 수 없습니다!"));
		OnJoinSessionComplete.Broadcast(false);
	}
	
	// 성공한 경우에만 Broadcast
	if (bWasSuccessful)
	{
		OnJoinSessionComplete.Broadcast(true);
	}
	else
	{
		// 실패한 경우 상세 진단 정보 출력
		UE_LOG(LogTemp, Error, TEXT("[방 참가] ========================================"));
		UE_LOG(LogTemp, Error, TEXT("[방 참가] JoinSession 실패 상세 진단"));
		UE_LOG(LogTemp, Error, TEXT("[방 참가] ========================================"));
		
		if (SessionInterface.IsValid())
		{
			auto FailedSession = SessionInterface->GetNamedSession(NAME_GameSession);
			if (FailedSession)
			{
				UE_LOG(LogTemp, Warning, TEXT("[방 참가] 실패 시 세션 상태: NumOpenPublicConnections=%d/%d"), 
					FailedSession->NumOpenPublicConnections, 
					FailedSession->SessionSettings.NumPublicConnections);
				UE_LOG(LogTemp, Warning, TEXT("[방 참가] 세션 설정: bIsLANMatch=%s, bUsesPresence=%s, bUseLobbiesIfAvailable=%s"),
					FailedSession->SessionSettings.bIsLANMatch ? TEXT("true") : TEXT("false"),
					FailedSession->SessionSettings.bUsesPresence ? TEXT("true") : TEXT("false"),
					FailedSession->SessionSettings.bUseLobbiesIfAvailable ? TEXT("true") : TEXT("false"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[방 참가] 실패 시 세션 상태: GetNamedSession(NAME_GameSession) = NULL"));
				UE_LOG(LogTemp, Error, TEXT("[방 참가] → 세션이 제대로 등록되지 않았습니다!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[방 참가] SessionInterface가 유효하지 않습니다!"));
		}
		
		// Online Subsystem 정보
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem)
		{
			FString SubsystemName = OnlineSubsystem->GetSubsystemName().ToString();
			UE_LOG(LogTemp, Warning, TEXT("[방 참가] 현재 Online Subsystem: %s"), *SubsystemName);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[방 참가] Online Subsystem이 NULL입니다!"));
		}
		
		FString ErrorMessage;
		FString DetailedErrorMsg;
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::Success:
			ErrorMessage = TEXT("성공");
			break;
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
			ErrorMessage = TEXT("주소를 가져올 수 없음");
			DetailedErrorMsg = TEXT("서버의 연결 주소를 가져올 수 없습니다.\n가능한 원인:\n1. 서버가 ListenServer 모드가 아님\n2. Steam 네트워크 연결 문제\n3. 서버의 NetDriver가 초기화되지 않음");
			break;
		case EOnJoinSessionCompleteResult::AlreadyInSession:
			ErrorMessage = TEXT("이미 세션에 참가 중입니다. 기존 세션을 먼저 떠나야 합니다.");
			break;
		case EOnJoinSessionCompleteResult::SessionIsFull:
			ErrorMessage = TEXT("세션이 가득 찼습니다");
			break;
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			ErrorMessage = TEXT("세션이 존재하지 않습니다");
			DetailedErrorMsg = TEXT("찾은 세션이 더 이상 존재하지 않습니다.\n가능한 원인:\n1. 서버가 세션을 종료함\n2. Steam 세션 동기화 지연\n3. 방 찾기 후 시간이 지나 세션이 만료됨");
			break;
		case EOnJoinSessionCompleteResult::UnknownError:
		default:
			ErrorMessage = TEXT("알 수 없는 오류 (방화벽/네트워크 가능성)");
			DetailedErrorMsg = TEXT("알 수 없는 오류가 발생했습니다.\n가능한 원인:\n1. 서버가 ListenServer 모드가 아님 (가장 가능성 높음)\n2. 방화벽이 연결을 차단\n3. 라우터 포트 포워딩 필요 (인터넷 모드)\n4. Steam 네트워크 문제\n5. 서버의 NetDriver가 없음");
			break;
		}
		UE_LOG(LogTemp, Error, TEXT("[방 참가] 실패: 세션 참가에 실패했습니다. (결과 코드: %d, %s)"), (int32)Result, *ErrorMessage);
		
		// 추가 진단 정보
		UE_LOG(LogTemp, Error, TEXT("[방 참가] ========================================"));
		UE_LOG(LogTemp, Error, TEXT("[방 참가] 실패 원인 상세 진단"));
		UE_LOG(LogTemp, Error, TEXT("[방 참가] ========================================"));
		
		// Online Subsystem 정보 (위에서 이미 선언된 변수 재사용)
		OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem)
		{
			FString SubsystemName = OnlineSubsystem->GetSubsystemName().ToString();
			UE_LOG(LogTemp, Error, TEXT("[방 참가] Online Subsystem: %s"), *SubsystemName);
			
			// Steam인 경우 추가 정보
			if (SubsystemName.Equals(TEXT("Steam"), ESearchCase::IgnoreCase))
			{
				UE_LOG(LogTemp, Error, TEXT("[방 참가] Steam 모드 진단:"));
				UE_LOG(LogTemp, Error, TEXT("  1. Steam 클라이언트 실행 중인지 확인"));
				UE_LOG(LogTemp, Error, TEXT("  2. Steam에 로그인되어 있는지 확인"));
				UE_LOG(LogTemp, Error, TEXT("  3. 서버와 클라이언트 모두 Steam 실행 중인지 확인"));
				UE_LOG(LogTemp, Error, TEXT("  4. 인터넷 모드인 경우 라우터 포트 포워딩 필요"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[방 참가] Online Subsystem이 NULL입니다!"));
		}
		
		// 현재 NetMode 확인 (위에서 선언한 World 변수 재사용)
		World = GetWorld();
		if (World)
		{
			ENetMode NetMode = World->GetNetMode();
			UE_LOG(LogTemp, Error, TEXT("[방 참가] 클라이언트 NetMode: %s"), 
				NetMode == NM_Standalone ? TEXT("Standalone") :
				NetMode == NM_ListenServer ? TEXT("ListenServer") :
				NetMode == NM_Client ? TEXT("Client") :
				NetMode == NM_DedicatedServer ? TEXT("DedicatedServer") : TEXT("Unknown"));
		}
		
		UE_LOG(LogTemp, Error, TEXT("[방 참가] ========================================"));
		
		// 화면에 디버그 메시지 표시
		if (GEngine)
		{
			FString ErrorMsg = FString::Printf(TEXT("[방 참가] 실패: %s"), *ErrorMessage);
			GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, ErrorMsg);
			
			// 상세 오류 메시지 표시
			if (!DetailedErrorMsg.IsEmpty())
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, DetailedErrorMsg);
			}
			
			// 추가 해결 방법 안내
			if (Result == EOnJoinSessionCompleteResult::UnknownError ||
				Result == EOnJoinSessionCompleteResult::CouldNotRetrieveAddress)
			{
				FString Hint = TEXT("========================================\n");
				Hint += TEXT("연결 실패 해결 방법:\n");
				Hint += TEXT("========================================\n");
				Hint += TEXT("1. 서버가 ListenServer 모드인지 확인\n");
				Hint += TEXT("   (서버 로그에서 'NetMode: ListenServer' 확인)\n");
				Hint += TEXT("2. 호스트·클라이언트 방화벽에서 게임·Steam 허용\n");
				Hint += TEXT("3. 호스트에서 포트 7777 인바운드 허용\n");
				Hint += TEXT("4. 인터넷 모드인 경우 라우터 포트 포워딩 필요\n");
				Hint += TEXT("5. Steam 클라이언트 양쪽 모두 실행 중인지 확인\n");
				Hint += TEXT("6. 같은 LAN 연결 확인 (LAN 모드인 경우)\n");
				Hint += TEXT("7. 방 찾기 후 재시도");
				GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Yellow, Hint);
			}
		}
		
		if (Result == EOnJoinSessionCompleteResult::AlreadyInSession)
		{
			UE_LOG(LogTemp, Warning, TEXT("[방 참가] 참고: 같은 프로세스에서 생성한 방에 참가할 수 없습니다."));
			UE_LOG(LogTemp, Warning, TEXT("[방 참가] 참고: 다른 게임 인스턴스를 실행하여 테스트해보세요."));
			
			// 화면에 디버그 메시지 표시
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("다른 게임 인스턴스를 실행하여 테스트해보세요."));
			}
		}
		
		// 실패한 경우 Broadcast
		OnJoinSessionComplete.Broadcast(false);
	}
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
	if (!SessionSearch.IsValid())
	{
		return FString();
	}

	if (SessionIndex < 0 || SessionIndex >= SessionSearch->SearchResults.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GetSessionName] 잘못된 세션 인덱스: %d (범위: 0~%d)"), 
			SessionIndex, SessionSearch->SearchResults.Num() - 1);
		return FString();
	}

	const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[SessionIndex];
	FString FoundSessionName;
	FString FoundHostName;
	Result.Session.SessionSettings.Get(FName(TEXT("SESSION_NAME")), FoundSessionName);
	Result.Session.SessionSettings.Get(FName(TEXT("HOST_NAME")), FoundHostName);
	// SESSION_NAME → HOST_NAME → "(이름 없음)"
	if (!FoundSessionName.IsEmpty()) return FoundSessionName;
	if (!FoundHostName.IsEmpty()) return FoundHostName;
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
