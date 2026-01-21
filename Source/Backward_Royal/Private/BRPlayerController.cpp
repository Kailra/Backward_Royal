// BRPlayerController.cpp
#include "BRPlayerController.h"
#include "BRCheatManager.h"
#include "BRGameSession.h"
#include "BRGameState.h"
#include "BRPlayerState.h"
#include "BRGameMode.h"
#include "GameFramework/GameModeBase.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"

ABRPlayerController::ABRPlayerController()
	: CurrentMenuWidget(nullptr)
	, MainScreenWidget(nullptr)
{
	// CheatManager 클래스 설정
	CheatClass = UBRCheatManager::StaticClass();
}

void ABRPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (CheatManager)
	{
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] CheatManager 초기화 완료 - 콘솔 명령어 사용 가능"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] CheatManager가 초기화되지 않았습니다."));
	}
	
	// 네트워크 연결 실패 델리게이트 바인딩
	if (UEngine* Engine = GEngine)
	{
		Engine->OnNetworkFailure().AddUObject(this, &ABRPlayerController::HandleNetworkFailure);
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] 네트워크 실패 감지 델리게이트 바인딩 완료"));
	}

	// 클라이언트에서만 초기 UI 표시
	if (IsLocalController())
	{
		// 약간의 지연 후 UI 표시 (모든 시스템이 초기화된 후)
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			UWorld* World = GetWorld();
			if (!World)
			{
				UE_LOG(LogTemp, Warning, TEXT("[PlayerController] World를 찾을 수 없습니다."));
				return;
			}

			ENetMode NetMode = World->GetNetMode();
			FString NetModeString = 
				NetMode == NM_Standalone ? TEXT("Standalone") :
				NetMode == NM_DedicatedServer ? TEXT("DedicatedServer") :
				NetMode == NM_ListenServer ? TEXT("ListenServer") :
				NetMode == NM_Client ? TEXT("Client") : TEXT("Unknown");
			
			UE_LOG(LogTemp, Log, TEXT("[PlayerController] BeginPlay UI 표시 결정 - NetMode: %s (%d)"), *NetModeString, (int32)NetMode);
			
			// 클라이언트로 서버에 연결된 경우
			// MainScreenWidget이 설정될 때까지 대기한 후 LobbyMenu로 전환
			if (NetMode == NM_Client)
			{
				UE_LOG(LogTemp, Log, TEXT("[PlayerController] 클라이언트 모드 감지 - MainScreenWidget 설정 대기 중..."));
				
				// MainScreenWidget이 설정될 때까지 재시도
				TSharedPtr<int32> CheckRetryCount = MakeShared<int32>(0);
				TSharedPtr<FTimerHandle> CheckTimerHandlePtr = MakeShared<FTimerHandle>();
				const int32 MaxCheckRetries = 20; // 최대 2초 대기 (0.1초 * 20)
				
				GetWorld()->GetTimerManager().SetTimer(*CheckTimerHandlePtr, [this, CheckRetryCount, CheckTimerHandlePtr, MaxCheckRetries]()
				{
					if (MainScreenWidget && IsValid(MainScreenWidget))
					{
						UE_LOG(LogTemp, Log, TEXT("[PlayerController] MainScreenWidget 발견 - LobbyMenu로 전환 시도"));
						SetMainScreenToLobbyMenu();
						
						// SetMainScreenToLobbyMenu 함수가 있는지 확인
						UFunction* Function = MainScreenWidget->FindFunction(FName("SetMainScreenToLobbyMenu"));
						if (Function)
						{
							UE_LOG(LogTemp, Log, TEXT("[PlayerController] 클라이언트 모드 - LobbyMenu로 전환 완료"));
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("[PlayerController] SetMainScreenToLobbyMenu 함수를 찾을 수 없습니다. WBP_MainScreen1에 함수가 있는지 확인하세요."));
						}
						
						if (UWorld* World = GetWorld())
						{
							World->GetTimerManager().ClearTimer(*CheckTimerHandlePtr);
						}
					}
					else
					{
						(*CheckRetryCount)++;
						if (*CheckRetryCount >= MaxCheckRetries)
						{
							UE_LOG(LogTemp, Error, TEXT("[PlayerController] MainScreenWidget을 찾을 수 없습니다. BP_HUDMain1이 WBP_MainScreen1을 생성하고 SetMainScreenWidget()을 호출하는지 확인하세요."));
							if (UWorld* World = GetWorld())
							{
								World->GetTimerManager().ClearTimer(*CheckTimerHandlePtr);
							}
						}
						else
						{
							UE_LOG(LogTemp, Log, TEXT("[PlayerController] MainScreenWidget 대기 중... (%d/%d)"), *CheckRetryCount, MaxCheckRetries);
						}
					}
				}, 0.1f, true); // 0.1초마다 확인
			}
			// 로컬 게임(Standalone) 또는 리슨 서버 → EntranceMenu 표시
			// 주의: WBP_MainScreen1이 이미 WBP_EntranceMenu1을 포함하고 있으므로
			// BeginPlay에서 ShowEntranceMenu()를 호출하지 않아야 중복 표시를 방지할 수 있습니다.
			else if (NetMode == NM_Standalone || NetMode == NM_ListenServer)
			{
				UE_LOG(LogTemp, Log, TEXT("[PlayerController] 로컬/서버 모드 감지 - WBP_MainScreen1이 UI를 관리하므로 BeginPlay에서 UI 표시를 건너뜁니다."));
				// WBP_MainScreen1이 이미 WBP_EntranceMenu1을 포함하고 있다면 주석 처리:
				// if (EntranceMenuWidgetClass)
				// {
				// 	ShowEntranceMenu();
				// 	UE_LOG(LogTemp, Log, TEXT("[PlayerController] 초기 UI (EntranceMenu) 표시 완료"));
				// }
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[PlayerController] 알 수 없는 네트워크 모드: %d (%s)"), (int32)NetMode, *NetModeString);
			}
		}, 0.1f, false);
	}
}

void ABRPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 네트워크 연결 실패 델리게이트 언바인딩
	if (UEngine* Engine = GEngine)
	{
		Engine->OnNetworkFailure().RemoveAll(this);
	}

	// 현재 위젯 정리
	if (CurrentMenuWidget)
	{
		CurrentMenuWidget->RemoveFromParent();
		CurrentMenuWidget = nullptr;
	}
	
	Super::EndPlay(EndPlayReason);
}

void ABRPlayerController::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// 현재 World가 이 PlayerController의 World인지 확인
	if (World != GetWorld())
	{
		return;
	}
	
	FString FailureTypeString;
	switch (FailureType)
	{
	case ENetworkFailure::NetDriverAlreadyExists:
		FailureTypeString = TEXT("NetDriverAlreadyExists");
		break;
	case ENetworkFailure::NetDriverCreateFailure:
		FailureTypeString = TEXT("NetDriverCreateFailure");
		break;
	case ENetworkFailure::NetDriverListenFailure:
		FailureTypeString = TEXT("NetDriverListenFailure");
		break;
	case ENetworkFailure::ConnectionLost:
		FailureTypeString = TEXT("ConnectionLost");
		break;
	case ENetworkFailure::ConnectionTimeout:
		FailureTypeString = TEXT("ConnectionTimeout");
		break;
	case ENetworkFailure::FailureReceived:
		FailureTypeString = TEXT("FailureReceived");
		break;
	case ENetworkFailure::OutdatedClient:
		FailureTypeString = TEXT("OutdatedClient");
		break;
	case ENetworkFailure::OutdatedServer:
		FailureTypeString = TEXT("OutdatedServer");
		break;
	case ENetworkFailure::PendingConnectionFailure:
		FailureTypeString = TEXT("PendingConnectionFailure");
		break;
	case ENetworkFailure::NetGuidMismatch:
		FailureTypeString = TEXT("NetGuidMismatch");
		break;
	case ENetworkFailure::NetChecksumMismatch:
		FailureTypeString = TEXT("NetChecksumMismatch");
		break;
	default:
		FailureTypeString = TEXT("Unknown");
		break;
	}
	
	UE_LOG(LogTemp, Error, TEXT("[방 참가] 네트워크 연결 실패 감지!"));
	UE_LOG(LogTemp, Error, TEXT("[방 참가] 실패 유형: %s"), *FailureTypeString);
	UE_LOG(LogTemp, Error, TEXT("[방 참가] 오류 메시지: %s"), *ErrorString);
	
	if (NetDriver)
	{
		FString ServerAddress = NetDriver->ServerConnection ? 
			NetDriver->ServerConnection->LowLevelGetRemoteAddress(true) : TEXT("Unknown");
		UE_LOG(LogTemp, Error, TEXT("[방 참가] 서버 주소: %s"), *ServerAddress);
	}
	
	// 일반적인 원인 안내
	if (FailureType == ENetworkFailure::ConnectionTimeout || FailureType == ENetworkFailure::PendingConnectionFailure)
	{
		UE_LOG(LogTemp, Error, TEXT("[방 참가] 가능한 원인:"));
		UE_LOG(LogTemp, Error, TEXT("  1. 서버가 실행 중이지 않음"));
		UE_LOG(LogTemp, Error, TEXT("  2. 방화벽이 포트를 차단함 (포트 7777 확인)"));
		UE_LOG(LogTemp, Error, TEXT("  3. 서버가 다른 맵을 로드하지 않음"));
		UE_LOG(LogTemp, Error, TEXT("  4. 네트워크 연결 문제"));
	}
}

void ABRPlayerController::CreateRoom(const FString& RoomName)
{
	UE_LOG(LogTemp, Log, TEXT("[방 생성] 명령 실행: %s"), *RoomName);
	
	if (HasAuthority())
	{
		// 서버에서 직접 실행
		if (UWorld* World = GetWorld())
		{
			if (AGameModeBase* GameMode = World->GetAuthGameMode())
			{
				if (ABRGameSession* GameSession = Cast<ABRGameSession>(GameMode->GameSession))
				{
					UE_LOG(LogTemp, Log, TEXT("[방 생성] 세션 생성 요청 중..."));
					GameSession->CreateRoomSession(RoomName);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[방 생성] 실패: GameSession을 찾을 수 없습니다."));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[방 생성] 실패: GameMode를 찾을 수 없습니다."));
			}
		}
	}
	else
	{
		// 클라이언트에서는 서버로 RPC 전송
		UE_LOG(LogTemp, Log, TEXT("[방 생성] 클라이언트에서 서버로 요청 전송..."));
		ServerCreateRoom(RoomName);
	}
}

void ABRPlayerController::FindRooms()
{
	UE_LOG(LogTemp, Log, TEXT("[방 찾기] 명령 실행"));
	
	if (HasAuthority())
	{
		// 서버에서 직접 실행
		if (UWorld* World = GetWorld())
		{
			if (AGameModeBase* GameMode = World->GetAuthGameMode())
			{
				if (ABRGameSession* GameSession = Cast<ABRGameSession>(GameMode->GameSession))
				{
					UE_LOG(LogTemp, Log, TEXT("[방 찾기] 세션 검색 시작..."));
					GameSession->FindSessions();
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[방 찾기] 실패: GameSession을 찾을 수 없습니다."));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[방 찾기] 실패: GameMode를 찾을 수 없습니다."));
			}
		}
	}
	else
	{
		// 클라이언트에서는 서버로 RPC 전송
		UE_LOG(LogTemp, Log, TEXT("[방 찾기] 클라이언트에서 서버로 요청 전송..."));
		ServerFindRooms();
	}
}

void ABRPlayerController::JoinRoom(int32 SessionIndex)
{
	UE_LOG(LogTemp, Log, TEXT("[방 참가] 명령 실행: 세션 인덱스=%d"), SessionIndex);
	
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[방 참가] 실패: World를 찾을 수 없습니다."));
		return;
	}

	// 네트워크 모드 확인
	ENetMode NetMode = World->GetNetMode();
	
	// ListenServer나 DedicatedServer는 JoinRoom을 실행할 수 없음
	// (Standalone 모드는 허용 - 다른 세션에 참가 가능)
	if (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[방 참가] 호스트는 이미 서버이므로 다른 방에 참가할 수 없습니다."));
		UE_LOG(LogTemp, Warning, TEXT("[방 참가] 클라이언트만 다른 방에 참가할 수 있습니다."));
		return;
	}

	// 클라이언트만 실행
	if (HasAuthority())
	{
		// Standalone 모드에서 실행 (로컬 게임)
		if (AGameModeBase* GameMode = World->GetAuthGameMode())
		{
			if (ABRGameSession* GameSession = Cast<ABRGameSession>(GameMode->GameSession))
			{
				UE_LOG(LogTemp, Log, TEXT("[방 참가] 세션 참가 요청 중..."));
				GameSession->JoinSessionByIndex(SessionIndex);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[방 참가] 실패: GameSession을 찾을 수 없습니다."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[방 참가] 실패: GameMode를 찾을 수 없습니다."));
		}
	}
	else
	{
		// 클라이언트에서는 서버로 RPC 전송
		UE_LOG(LogTemp, Log, TEXT("[방 참가] 클라이언트에서 서버로 요청 전송..."));
		ServerJoinRoom(SessionIndex);
	}
}

void ABRPlayerController::ToggleReady()
{
	UE_LOG(LogTemp, Log, TEXT("[준비 상태] 토글 명령 실행"));
	if (ABRPlayerState* BRPS = GetPlayerState<ABRPlayerState>())
	{
		bool bWasReady = BRPS->bIsReady;
		if (HasAuthority())
		{
			BRPS->ToggleReady();
			UE_LOG(LogTemp, Log, TEXT("[준비 상태] 변경: %s -> %s"), 
				bWasReady ? TEXT("준비 완료") : TEXT("대기 중"),
				BRPS->bIsReady ? TEXT("준비 완료") : TEXT("대기 중"));
		}
		else
		{
			// 클라이언트에서 서버로 요청
			UE_LOG(LogTemp, Log, TEXT("[준비 상태] 서버에 요청 전송..."));
			ServerToggleReady();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[준비 상태] 실패: PlayerState를 찾을 수 없습니다."));
	}
}

void ABRPlayerController::RandomTeams()
{
	UE_LOG(LogTemp, Log, TEXT("[랜덤 팀 배정] 명령 실행"));
	
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[랜덤 팀 배정] 실패: World를 찾을 수 없습니다."));
		return;
	}
	
	// 서버(호스트)인 경우 자동으로 사용 가능
	// 클라이언트인 경우 방장인지 확인
	bool bCanUse = false;
	if (HasAuthority())
	{
		// 서버인 경우 자동으로 사용 가능
		bCanUse = true;
		UE_LOG(LogTemp, Log, TEXT("[랜덤 팀 배정] 서버 권한 확인 완료, 팀 배정 시작..."));
	}
	else
	{
		// 클라이언트인 경우 방장인지 확인
		if (ABRPlayerState* BRPS = GetPlayerState<ABRPlayerState>())
		{
			if (BRPS->bIsHost)
			{
				bCanUse = true;
				UE_LOG(LogTemp, Log, TEXT("[랜덤 팀 배정] 방장 권한 확인 완료, 팀 배정 시작..."));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[랜덤 팀 배정] 실패: 방장만 사용할 수 있습니다."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[랜덤 팀 배정] 실패: PlayerState를 찾을 수 없습니다."));
		}
	}
	
	if (bCanUse)
	{
		RequestRandomTeams();
	}
}

void ABRPlayerController::ChangeTeam(int32 PlayerIndex, int32 TeamNumber)
{
	UE_LOG(LogTemp, Log, TEXT("[팀 변경] 명령 실행: PlayerIndex=%d, TeamNumber=%d"), PlayerIndex, TeamNumber);
	
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[팀 변경] 실패: World를 찾을 수 없습니다."));
		return;
	}
	
	// 서버(호스트)인 경우 자동으로 사용 가능
	// 클라이언트인 경우 방장인지 확인
	bool bCanUse = false;
	if (HasAuthority())
	{
		// 서버인 경우 자동으로 사용 가능
		bCanUse = true;
		UE_LOG(LogTemp, Log, TEXT("[팀 변경] 서버 권한 확인 완료, 팀 변경 요청 중..."));
	}
	else
	{
		// 클라이언트인 경우 방장인지 확인
		if (ABRPlayerState* BRPS = GetPlayerState<ABRPlayerState>())
		{
			if (BRPS->bIsHost)
			{
				bCanUse = true;
				UE_LOG(LogTemp, Log, TEXT("[팀 변경] 방장 권한 확인 완료, 팀 변경 요청 중..."));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[팀 변경] 실패: 방장만 사용할 수 있습니다."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[팀 변경] 실패: PlayerState를 찾을 수 없습니다."));
		}
	}
	
	if (bCanUse)
	{
		RequestChangePlayerTeam(PlayerIndex, TeamNumber);
	}
}

void ABRPlayerController::StartGame()
{
	UE_LOG(LogTemp, Log, TEXT("[게임 시작] 명령 실행"));
	
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[게임 시작] 실패: World를 찾을 수 없습니다."));
		return;
	}
	
	// 서버(호스트)인 경우 자동으로 게임 시작 가능
	// 클라이언트인 경우 방장인지 확인
	bool bCanStart = false;
	if (HasAuthority())
	{
		// 서버인 경우 자동으로 시작 가능
		bCanStart = true;
		UE_LOG(LogTemp, Log, TEXT("[게임 시작] 서버 권한 확인 완료, 게임 시작 요청 중..."));
	}
	else
	{
		// 클라이언트인 경우 방장인지 확인
		if (ABRPlayerState* BRPS = GetPlayerState<ABRPlayerState>())
		{
			if (BRPS->bIsHost)
			{
				bCanStart = true;
				UE_LOG(LogTemp, Log, TEXT("[게임 시작] 방장 권한 확인 완료, 게임 시작 요청 중..."));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[게임 시작] 실패: 방장만 사용할 수 있습니다."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[게임 시작] 실패: PlayerState를 찾을 수 없습니다."));
		}
	}
	
	if (bCanStart)
	{
		RequestStartGame();
	}
}

// 서버 RPC 함수들
void ABRPlayerController::ServerCreateRoom_Implementation(const FString& RoomName)
{
	CreateRoom(RoomName);
}

void ABRPlayerController::ServerFindRooms_Implementation()
{
	FindRooms();
}

void ABRPlayerController::ServerJoinRoom_Implementation(int32 SessionIndex)
{
	JoinRoom(SessionIndex);
}

void ABRPlayerController::ServerToggleReady_Implementation()
{
	ToggleReady();
}

void ABRPlayerController::ServerRequestRandomTeams_Implementation()
{
	// 서버에서 직접 팀 배정 실행
	if (ABRGameState* BRGameState = GetWorld()->GetGameState<ABRGameState>())
	{
		UE_LOG(LogTemp, Log, TEXT("[랜덤 팀 배정] 서버에서 직접 실행: 총 %d명의 플레이어"), BRGameState->PlayerArray.Num());
		BRGameState->AssignRandomTeams();
		UE_LOG(LogTemp, Log, TEXT("[랜덤 팀 배정] 완료"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[랜덤 팀 배정] 실패: GameState를 찾을 수 없습니다."));
	}
}

void ABRPlayerController::ServerRequestChangePlayerTeam_Implementation(int32 PlayerIndex, int32 NewTeamNumber)
{
	// 서버에서 직접 팀 변경 실행
	if (ABRGameState* BRGameState = GetWorld()->GetGameState<ABRGameState>())
	{
		if (PlayerIndex >= 0 && PlayerIndex < BRGameState->PlayerArray.Num())
		{
			if (ABRPlayerState* TargetPS = Cast<ABRPlayerState>(BRGameState->PlayerArray[PlayerIndex]))
			{
				FString PlayerName = TargetPS->GetPlayerName();
				if (PlayerName.IsEmpty())
				{
					PlayerName = FString::Printf(TEXT("Player %d"), PlayerIndex + 1);
				}
				TargetPS->SetTeamNumber(NewTeamNumber);
				UE_LOG(LogTemp, Log, TEXT("[팀 변경] 서버에서 직접 실행: %s -> 팀 %d"), *PlayerName, NewTeamNumber);
				BRGameState->OnTeamChanged.Broadcast();
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[팀 변경] 실패: PlayerState를 찾을 수 없습니다."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[팀 변경] 실패: 잘못된 PlayerIndex (%d)"), PlayerIndex);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[팀 변경] 실패: GameState를 찾을 수 없습니다."));
	}
}

void ABRPlayerController::ServerRequestStartGame_Implementation()
{
	StartGame();
}

void ABRPlayerController::ClientNotifyGameStarting_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("[게임 시작] 클라이언트: 게임 시작 알림 수신 - 맵 이동 대기 중..."));
	
	// 클라이언트에서 입력을 일시적으로 중지 (선택사항)
	// 실제로는 ServerTravel이 자동으로 클라이언트를 따라오므로 특별한 처리가 필요하지 않을 수 있습니다
	// 하지만 로그를 남겨서 알림이 도착했는지 확인할 수 있습니다
}

void ABRPlayerController::RequestRandomTeams()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[랜덤 팀 배정] 실패: World를 찾을 수 없습니다."));
		return;
	}
	
	// 서버에서 직접 실행
	if (HasAuthority())
	{
		if (ABRGameState* BRGameState = World->GetGameState<ABRGameState>())
		{
			UE_LOG(LogTemp, Log, TEXT("[랜덤 팀 배정] 서버에서 직접 실행: 총 %d명의 플레이어"), BRGameState->PlayerArray.Num());
			BRGameState->AssignRandomTeams();
			UE_LOG(LogTemp, Log, TEXT("[랜덤 팀 배정] 완료"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[랜덤 팀 배정] 실패: GameState를 찾을 수 없습니다."));
		}
	}
	else
	{
		// 클라이언트에서 서버로 요청
		UE_LOG(LogTemp, Log, TEXT("[랜덤 팀 배정] 클라이언트에서 서버로 요청 전송..."));
		ServerRequestRandomTeams();
	}
}

void ABRPlayerController::RequestChangePlayerTeam(int32 PlayerIndex, int32 NewTeamNumber)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[팀 변경] 실패: World를 찾을 수 없습니다."));
		return;
	}
	
	// 서버에서 직접 실행
	if (HasAuthority())
	{
		if (ABRGameState* BRGameState = World->GetGameState<ABRGameState>())
		{
			if (PlayerIndex >= 0 && PlayerIndex < BRGameState->PlayerArray.Num())
			{
				if (ABRPlayerState* TargetPS = Cast<ABRPlayerState>(BRGameState->PlayerArray[PlayerIndex]))
				{
					FString PlayerName = TargetPS->GetPlayerName();
					if (PlayerName.IsEmpty())
					{
						PlayerName = FString::Printf(TEXT("Player %d"), PlayerIndex + 1);
					}
					int32 OldTeam = TargetPS->TeamNumber;
					TargetPS->SetTeamNumber(NewTeamNumber);
					UE_LOG(LogTemp, Log, TEXT("[팀 변경] 서버에서 직접 실행: %s의 팀이 %d에서 %d로 변경되었습니다."), 
						*PlayerName, OldTeam, NewTeamNumber);
					BRGameState->OnTeamChanged.Broadcast();
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[팀 변경] 실패: PlayerState 캐스팅 실패"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[팀 변경] 실패: 잘못된 PlayerIndex (%d), 현재 플레이어 수: %d"), 
					PlayerIndex, BRGameState->PlayerArray.Num());
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[팀 변경] 실패: GameState를 찾을 수 없습니다."));
		}
	}
	else
	{
		// 클라이언트에서 서버로 요청
		UE_LOG(LogTemp, Log, TEXT("[팀 변경] 클라이언트에서 서버로 요청 전송..."));
		ServerRequestChangePlayerTeam(PlayerIndex, NewTeamNumber);
	}
}

void ABRPlayerController::RequestStartGame()
{
	if (ABRPlayerState* BRPS = GetPlayerState<ABRPlayerState>())
	{
		if (BRPS->bIsHost && HasAuthority())
		{
			if (ABRGameMode* BRGameMode = GetWorld()->GetAuthGameMode<ABRGameMode>())
			{
				if (ABRGameState* BRGameState = GetWorld()->GetGameState<ABRGameState>())
				{
					UE_LOG(LogTemp, Log, TEXT("[게임 시작] 조건 확인: 플레이어 수=%d/%d, 모든 플레이어 준비=%s"), 
						BRGameState->PlayerCount, BRGameState->MaxPlayers,
						BRGameState->AreAllPlayersReady() ? TEXT("예") : TEXT("아니오"));
				}
				BRGameMode->StartGame();
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[게임 시작] 실패: GameMode를 찾을 수 없습니다."));
			}
		}
		else
		{
			// 클라이언트에서 서버로 요청
			ServerRequestStartGame();
		}
	}
}

void ABRPlayerController::ShowRoomInfo()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	// 네트워크 상태 확인
	ENetMode NetMode = World->GetNetMode();
	FString NetModeString;
	switch (NetMode)
	{
	case NM_Standalone:
		NetModeString = TEXT("Standalone (로컬)");
		break;
	case NM_DedicatedServer:
		NetModeString = TEXT("Dedicated Server");
		break;
	case NM_ListenServer:
		NetModeString = TEXT("Listen Server (호스트)");
		break;
	case NM_Client:
		NetModeString = TEXT("Client (클라이언트)");
		break;
	default:
		NetModeString = TEXT("Unknown");
		break;
	}

	ABRGameState* BRGameState = World->GetGameState<ABRGameState>();
	if (!BRGameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameState를 찾을 수 없습니다."));
		return;
	}

	ABRPlayerState* LocalPS = GetPlayerState<ABRPlayerState>();
	if (!LocalPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerState를 찾을 수 없습니다."));
		return;
	}

	// 연결 상태 확인
	bool bIsConnected = (NetMode == NM_Client || NetMode == NM_ListenServer);
	FString ConnectionStatus = bIsConnected ? TEXT("연결됨") : TEXT("연결 안 됨 (Standalone)");
	
	// 서버 연결 확인 (클라이언트인 경우)
	bool bIsActuallyConnected = false;
	if (NetMode == NM_Client)
	{
		if (UNetConnection* Connection = GetNetConnection())
		{
			// Connection이 존재하면 연결된 것으로 간주
			bIsActuallyConnected = true;
			FString RemoteAddr = Connection->LowLevelGetRemoteAddress(true);
			UE_LOG(LogTemp, Log, TEXT("서버 주소: %s"), *RemoteAddr);
		}
	}
	
	// 방 정보 출력
	UE_LOG(LogTemp, Log, TEXT("=== 방 정보 ==="));
	UE_LOG(LogTemp, Log, TEXT("네트워크 모드: %s"), *NetModeString);
	UE_LOG(LogTemp, Log, TEXT("연결 상태: %s"), *ConnectionStatus);
	if (NetMode == NM_Client)
	{
		UE_LOG(LogTemp, Log, TEXT("실제 연결 상태: %s"), bIsActuallyConnected ? TEXT("연결됨") : TEXT("연결 안 됨"));
	}
	UE_LOG(LogTemp, Log, TEXT("서버 권한: %s"), HasAuthority() ? TEXT("예 (서버)") : TEXT("아니오 (클라이언트)"));
	
	// PlayerArray와 PlayerCount 비교
	int32 ActualPlayerCount = BRGameState->PlayerArray.Num();
	UE_LOG(LogTemp, Log, TEXT("플레이어 수 (PlayerCount): %d / %d (최소: %d)"), BRGameState->PlayerCount, BRGameState->MaxPlayers, BRGameState->MinPlayers);
	UE_LOG(LogTemp, Log, TEXT("플레이어 수 (PlayerArray): %d명"), ActualPlayerCount);
	
	if (ActualPlayerCount != BRGameState->PlayerCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("경고: PlayerCount(%d)와 PlayerArray(%d)가 일치하지 않습니다!"), 
			BRGameState->PlayerCount, ActualPlayerCount);
		if (HasAuthority())
		{
			UE_LOG(LogTemp, Log, TEXT("서버에서 PlayerList 업데이트 중..."));
			BRGameState->UpdatePlayerList();
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("게임 시작 가능: %s"), BRGameState->bCanStartGame ? TEXT("예") : TEXT("아니오"));
	UE_LOG(LogTemp, Log, TEXT("내 상태 - 방장: %s, 준비: %s, 팀: %d"), 
		LocalPS->bIsHost ? TEXT("예") : TEXT("아니오"),
		LocalPS->bIsReady ? TEXT("예") : TEXT("아니오"),
		LocalPS->TeamNumber);

	// 플레이어 목록 출력
	UE_LOG(LogTemp, Log, TEXT("=== 플레이어 목록 ==="));
	if (BRGameState->PlayerArray.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("플레이어 목록이 비어있습니다!"));
		if (!HasAuthority())
		{
			UE_LOG(LogTemp, Warning, TEXT("클라이언트: 서버의 GameState가 아직 복제되지 않았을 수 있습니다."));
		}
	}
	
	for (int32 i = 0; i < BRGameState->PlayerArray.Num(); i++)
	{
		if (ABRPlayerState* PS = Cast<ABRPlayerState>(BRGameState->PlayerArray[i]))
		{
			FString PlayerName = PS->GetPlayerName();
			if (PlayerName.IsEmpty())
			{
				PlayerName = FString::Printf(TEXT("Player %d"), i + 1);
			}

			FString TeamText = PS->TeamNumber == 0 ? TEXT("팀 없음") : FString::Printf(TEXT("팀 %d"), PS->TeamNumber);
			UE_LOG(LogTemp, Log, TEXT("[%d] %s - %s, 준비: %s, 방장: %s"),
				i,
				*PlayerName,
				*TeamText,
				PS->bIsReady ? TEXT("예") : TEXT("아니오"),
				PS->bIsHost ? TEXT("예") : TEXT("아니오"));
		}
	}
	UE_LOG(LogTemp, Log, TEXT("==================="));
}

void ABRPlayerController::SetupRoleInput(bool bIsLower)
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
	{
		Subsystem->ClearAllMappings();

		UInputMappingContext* TargetContext = bIsLower ? LowerBodyContext : UpperBodyContext;
		if (TargetContext)
		{
			Subsystem->AddMappingContext(TargetContext, 0);
		}
	}

	// [추가] 하체 캐릭터라면 입력 바인딩(함수 연결)을 강제로 다시 시키기
	if (bIsLower)
	{
		if (APawn* P = GetPawn())
		{
			// 클라이언트에게 입력 시스템 재시작 명령
			ClientRestart(P);
		}
	}
}

// ========== UI 관리 함수 구현 ==========

void ABRPlayerController::ShowEntranceMenu()
{
	if (EntranceMenuWidgetClass)
	{
		ShowMenuWidget(EntranceMenuWidgetClass);
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] EntranceMenu 표시"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] EntranceMenuWidgetClass가 설정되지 않았습니다."));
	}
}

void ABRPlayerController::ShowJoinMenu()
{
	if (JoinMenuWidgetClass)
	{
		ShowMenuWidget(JoinMenuWidgetClass);
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] JoinMenu 표시"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] JoinMenuWidgetClass가 설정되지 않았습니다."));
	}
}

void ABRPlayerController::ShowLobbyMenu()
{
	if (LobbyMenuWidgetClass)
	{
		ShowMenuWidget(LobbyMenuWidgetClass);
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] LobbyMenu 표시"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] LobbyMenuWidgetClass가 설정되지 않았습니다."));
	}
}

void ABRPlayerController::HideCurrentMenu()
{
	if (CurrentMenuWidget)
	{
		CurrentMenuWidget->RemoveFromParent();
		CurrentMenuWidget = nullptr;
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] 현재 메뉴 숨김"));
	}
}

void ABRPlayerController::SetMainScreenWidget(UUserWidget* Widget)
{
	MainScreenWidget = Widget;
	UE_LOG(LogTemp, Log, TEXT("[PlayerController] MainScreenWidget 설정: %s"), Widget ? *Widget->GetName() : TEXT("None"));
	
		// 클라이언트 모드이고 MainScreenWidget이 설정되면 LobbyMenu로 전환
		if (Widget && IsValid(Widget))
		{
			UWorld* World = GetWorld();
			if (World && World->GetNetMode() == NM_Client)
			{
			// 위젯이 완전히 초기화된 후 LobbyMenu로 전환
			// 여러 번 시도하여 함수가 준비될 때까지 대기
			FTimerHandle TimerHandle;
			TSharedPtr<int32> RetryCountPtr = MakeShared<int32>(0);
			TSharedPtr<FTimerHandle> TimerHandlePtr = MakeShared<FTimerHandle>();
			const int32 MaxRetries = 5;
			
			TWeakObjectPtr<UUserWidget> WeakWidget = Widget;
			GetWorld()->GetTimerManager().SetTimer(*TimerHandlePtr, [this, WeakWidget, RetryCountPtr, TimerHandlePtr, MaxRetries]()
			{
				if (!WeakWidget.IsValid())
				{
					UE_LOG(LogTemp, Warning, TEXT("[PlayerController] MainScreenWidget이 유효하지 않습니다."));
					return;
				}
				
				// SetMainScreenToLobbyMenu 함수가 있는지 확인
				UFunction* Function = WeakWidget->FindFunction(FName("SetMainScreenToLobbyMenu"));
				if (Function)
				{
					SetMainScreenToLobbyMenu();
					UE_LOG(LogTemp, Log, TEXT("[PlayerController] 클라이언트 모드 - LobbyMenu로 전환 완료"));
					if (UWorld* World = GetWorld())
					{
						World->GetTimerManager().ClearTimer(*TimerHandlePtr);
					}
				}
				else
				{
					(*RetryCountPtr)++;
					if (*RetryCountPtr >= MaxRetries)
					{
						UE_LOG(LogTemp, Error, TEXT("[PlayerController] SetMainScreenToLobbyMenu 함수를 찾을 수 없습니다. WBP_MainScreen1에 함수가 있는지 확인하세요."));
						if (UWorld* World = GetWorld())
						{
							World->GetTimerManager().ClearTimer(*TimerHandlePtr);
						}
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("[PlayerController] SetMainScreenToLobbyMenu 함수 대기 중... (%d/%d)"), *RetryCountPtr, MaxRetries);
					}
				}
			}, 0.1f, true); // 0.1초마다 재시도
			}
		}
}

void ABRPlayerController::ShowMainScreen()
{
	if (MainScreenWidget && IsValid(MainScreenWidget))
	{
		MainScreenWidget->SetVisibility(ESlateVisibility::Visible);
		
		// 현재 메뉴 위젯 제거
		if (CurrentMenuWidget)
		{
			CurrentMenuWidget->RemoveFromParent();
			CurrentMenuWidget = nullptr;
		}
		
		// WidgetSwitcher 인덱스를 EntranceMenu로 설정
		SetMainScreenToEntranceMenu();
		
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] WBP_MainScreen1 표시"));
	}
}

void ABRPlayerController::SetMainScreenToEntranceMenu()
{
	// WBP_MainScreen1 블루프린트에서 이 함수를 구현해야 합니다.
	// MainScreenWidget이 유효하면 블루프린트 함수를 호출합니다.
	if (MainScreenWidget && IsValid(MainScreenWidget))
	{
		// 블루프린트에서 구현된 함수를 호출하기 위해
		// UFunction을 찾아서 호출합니다.
		UFunction* Function = MainScreenWidget->FindFunction(FName("SetMainScreenToEntranceMenu"));
		if (Function)
		{
			MainScreenWidget->ProcessEvent(Function, nullptr);
			UE_LOG(LogTemp, Log, TEXT("[PlayerController] SetMainScreenToEntranceMenu 호출 완료"));
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[PlayerController] SetMainScreenToEntranceMenu: MainScreenWidget이 없거나 함수를 찾을 수 없습니다."));
}

void ABRPlayerController::SetMainScreenToLobbyMenu()
{
	// WBP_MainScreen1 블루프린트에서 이 함수를 구현해야 합니다.
	// MainScreenWidget이 유효하면 블루프린트 함수를 호출합니다.
	if (MainScreenWidget && IsValid(MainScreenWidget))
	{
		// 블루프린트에서 구현된 함수를 호출하기 위해
		// UFunction을 찾아서 호출합니다.
		UFunction* Function = MainScreenWidget->FindFunction(FName("SetMainScreenToLobbyMenu"));
		if (Function)
		{
			MainScreenWidget->ProcessEvent(Function, nullptr);
			UE_LOG(LogTemp, Log, TEXT("[PlayerController] SetMainScreenToLobbyMenu 호출 완료"));
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[PlayerController] SetMainScreenToLobbyMenu: MainScreenWidget이 없거나 함수를 찾을 수 없습니다."));
}

void ABRPlayerController::HideMainScreen()
{
	if (MainScreenWidget && IsValid(MainScreenWidget))
	{
		MainScreenWidget->SetVisibility(ESlateVisibility::Collapsed);
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] WBP_MainScreen1 숨김"));
	}
}

void ABRPlayerController::ShowMenuWidget(TSubclassOf<UUserWidget> WidgetClass)
{
	// 클라이언트에서만 실행
	if (!IsLocalController())
	{
		return;
	}

	// 주의: WBP_MainScreen1이 HUD에서 관리되고 있다면
	// HUD에서 위젯을 숨기거나 WidgetSwitcher를 사용하여 전환해야 합니다.
	// 현재는 PlayerController에서 별도로 위젯을 생성하고 있습니다.
	
	// WBP_MainScreen1 숨기기 (HUD에서 관리되는 경우)
	if (MainScreenWidget && IsValid(MainScreenWidget))
	{
		MainScreenWidget->SetVisibility(ESlateVisibility::Collapsed);
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] WBP_MainScreen1 숨김 (HUD에서 관리)"));
	}

	// 현재 위젯이 있으면 제거
	if (CurrentMenuWidget)
	{
		CurrentMenuWidget->RemoveFromParent();
		CurrentMenuWidget = nullptr;
	}

	// 새 위젯 생성 및 표시
	if (WidgetClass)
	{
		CurrentMenuWidget = CreateWidget<UUserWidget>(this, WidgetClass);
		if (CurrentMenuWidget)
		{
			CurrentMenuWidget->AddToViewport();
			// 입력 모드 설정 (UI에 포커스)
			FInputModeUIOnly InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputMode);
			bShowMouseCursor = true;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[PlayerController] 위젯 생성 실패"));
		}
	}
}
