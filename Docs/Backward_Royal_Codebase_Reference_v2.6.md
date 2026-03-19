# Backward Royal 코드 베이스 분석서 (Codebase Reference v2.6)

## 1. 개요 (Overview)
본 문서는 `Source/Backward_Royal` 디렉토리 내의 핵심 소스 코드 파일(헤더)을 **전면 재조사(Exhaustive Inventory)**하여 작성된 상세 분석서입니다. 누락되었던 가상 함수(`PreLogin`, `Logout`), 각종 타이머 핸들(`FTimerHandle`), 내부 검증용 변수까지 모두 포함하여 **단 하나의 함수나 변수도 빠짐없이 명세**하는 것을 원칙으로 합니다.

*   **Version**: v2.6 (Exhaustive Full Inventory + Crash/Sync Fixes, UI/BGM Updates)
*   **Coverage**: Core Framework, Characters, Items, Components, UI Library
*   **Legend**:
    *   `[Prop]`: `UPROPERTY` (변수)
    *   `[Func]`: `UFUNCTION` (함수 / 블루프린트 노출)
    *   `[Virtual]`: 가상 함수 오버라이드 (C++ 내부 구현)
    *   `[RPC]`: 네트워크 통신용 함수 (Server/Client/NetMulticast)
    *   `(Internal)`: UPROPERTY/UFUNCTION 매크로가 없는 순수 C++ 멤버

---

## 2. Core Framework (게임 로직)

### 2.1. `ABRGameMode`
*   **파일**: `BRGameMode.h` / **상속**: `AGameModeBase`
*   **설명**: 게임 규칙, 스폰, 역할 배정, 접속 처리 및 상태 전이 관리.

#### Member Variables
*   `[Prop] MinPlayers` (int32): 최소 시작 인원.
*   `[Prop] MaxPlayers` (int32): 최대 인원.
*   `[Prop] LobbyMapPath` (FString): 로비 레벨 경로.
*   `[Prop] GameMapPath` (FString): 게임 맵 경로 (Legacy).
*   `[Prop] StageFolderPath` (FString): 랜덤 맵 생성용 스테이지 폴더.
*   `[Prop] bUseRandomMap` (bool): 랜덤 스테이지 기능 활성화 여부.
*   `[Prop] bBlockJoinWhenGameStarted` (bool): 게임 진행 중 난입(Join) 차단 여부.
*   `[Prop] SpawnDelayBetweenTeams` (float): 팀별 스폰 간격 (기본 1.2초).
*   `[Prop] DelayBeforeReturnToLobby` (float): 승리 후 로비 귀환 전 대기 시간.
*   `[Prop] bUseStunInsteadOfDeath` (bool): 사망 대신 기절(Stun) 체제 사용 여부.
*   `[Prop] UpperBodyClass` (TSubclassOf<AUpperBodyPawn>): 상체 폰 클래스 캐싱.
*   `(Internal) StageMapPathsFallback` (TArray<FString>): 스테이지 폴더 목록 로드 실패 시 사용하는 하드코딩된 예비 맵 경로.
*   `(Internal) bMatchEnded` (bool): 매치 종료 중복 실행 방지 플래그.
*   `(Internal) ReturnToLobbyTimerHandle` (FTimerHandle): 로비 자동 복귀용 타이머.
*   `(Internal) StagedSortedByTeam` (TArray<ABRPlayerState*>): 순차 스폰용 팀원 목록 캐시.
*   `(Internal) StagedNumTeams`, `StagedCurrentTeamIndex`, `StagedUpperBodiesSpawnedCount`, `StagedAllLowerReadyRetries`: 스폰 페이즈 검증용 변수들.
*   `(Internal) StagedAllLowerReadyHandle`, `StagedApplyTimerHandle`, `InitialRoleApplyTimerHandle`, `DirectStartRoleApplyTimerHandle`: 스폰 동기화 및 딜레이 처리 타이머 핸들.
*   `(Internal) PendingSortedByTeamSnapshot` (TArray<ABRPlayerState*>): 팀 상태 백업용.
*   `(Internal) StagedPawnWaitRetriesForTeam`, `StagedControllerWaitRetriesForTeam`: 스폰 무한 대기 방지용 시도 횟수.
*   `(Internal) SpecTimerHandle_DeathSpectator`: 사망 후 관전 전환용 2초 딜레이 타이머.
*   `(Internal) InitialPlayerWaitRetries`, `MinPlayerWaitRetries`, `ExpectedZeroWaitRetries`: 대기실 초기 접속 지연 및 재시도 확인용.
*   `(Internal) bHasScheduledInitialRoleApply` (bool): OnPossess 중복 스케줄링 방지 플래그.
*   `(Internal) MaxStagedPawnWaitRetriesPerTeam`, `MaxStagedControllerWaitRetriesForTeam`, `MaxInitialPlayerWaitRetries`, `MaxMinPlayerWaitRetries`, `MaxExpectedZeroWaitRetries` (static constexpr): 최대 재시도 횟수 상수상.

#### Functions
*   `[Func] StartGame()`: 호스트가 게임 시작을 눌렀을 때 실행.
*   `[Func] CheckMatchWinner()`: 생존 팀 확인 후 최후의 1팀 도출 및 `MulticastMatchEnded` 기폭.
*   `[Virtual] BeginPlay()`, `EndPlay(EndPlayReason)`: 게임 모드 생명주기.
*   `[Virtual] PreLogin(...)`: 접속 시도 시 유효성(진행 중 난입 등) 검사 및 거부 처리.
*   `[Virtual] PostLogin(APlayerController*)`: 유저 입장 시 PlayerState 할당 및 검증.
*   `[Virtual] Logout(AController*)`: 유저 탈주/종료 시 세션 및 팀 데이터 정리.
*   `(Internal) ClearGameSessionForPIEExit()`: PIE 모드 종료 시 세계 참조 찌꺼기를 명시적으로 차단.
*   `(Internal) ApplyRoleChangesForRandomTeams()`: 저장된 팀 정보를 바탕으로 하체/상체 폰 배정.
*   `(Internal) ApplyRoleChangesForRandomTeams_ApplyOneTeam()`: 한 번에 한 팀씩 스폰 처리 (병목 및 버그 방지).
*   `(Internal) GetExpectedLowerBodyCount()`, `GetExpectedUpperBodyCount()`: 인원수 비례 기대 폰 숫자 산출 정적 함수.
*   `(Internal) OnPlayerDied(ABaseCharacter*)`: 체력 0 된 캐릭터 감지 시 진입 구간.
*   `(Internal) CheckAndEndGameIfWinner()`: OnPlayerDied 도중 남은 팀이 1팀인지 검사 (구버전 우승 확인용).
*   `(Internal) SwitchTeamToSpectator(...)`: 파트너와 함께 관전 모드로 강제 지정 (WeakPtr 기반).
*   `(Internal) SwitchTeamToSpectatorByPlayerIndices(...)`: 인덱스 배열 기반 관전 전환.
*   `(Internal) SwitchEliminatedTeamToSpectator(...)`: 팀 전체 관전 전환.
*   `(Internal) TravelToLobby()`, `ReturnToLobby()`: 로비맵 ServerTravel 실행.
*   `(Internal) GetAvailableStageMapPaths()`: 스테이지 폴더 순회 및 맵 목록 수집.
*   `(Internal) TryApplyDirectStartRolesFallback()`: 로비를 거치지 않고 강제 시작 시 예비 랜던 팀 배정 체제 가동.
*   `(Internal) ScheduleInitialRoleApplyIfNeeded()`: 게임 진입 극초기 빙의 및 역할 정보 스케줄링.
*   `(Internal) HasScheduledInitialRoleApply()`: 타이머 진입 중복 방지 getter.

---

### 2.2. `ABRGameState`
*   **파일**: `BRGameState.h` / **상속**: `AGameStateBase`
*   **설명**: 방 정보, 대기열, 팀 슬롯 상태, 접속자 목록 및 주요 이벤트(Delegate) 멀티캐스트 동기화.

#### Member Variables
*   `[Prop] MinPlayers` (int32): 최소 시작 인원 (단순 UI 표기용).
*   `[Prop] MaxPlayers` (int32): 최대 허용 인원.
*   `[Prop] PlayerCount` (int32, Rep): 현재 접속자 수.
*   `[Prop] PlayerListForDisplay` (TArray<FBRUserInfo>, Rep): UMG에서 읽을 전체 유저 정보 캐시.
*   `[Prop] LobbyEntrySlots`, `LobbyTeamSlots` (TArray<int32>, Rep): 대기열 및 팀 배정 슬롯.
*   `[Prop] bCanStartGame` (bool, Rep): 게임 시작 가능 상태.
*   `[Prop] WinningTeamNumber` (int32, Rep): 매치 종료 시 최종 우승 팀 번호.
*   `[Prop] RoomTitle` (FString, Rep): 방 제목 캐시.
*   `[Prop] bSkipLoadingScreen` (bool, Rep): 개발 편의를 위한 로딩 스킵 플래그.
*   `[Prop] OnPlayerListChanged` (Delegate): 플레이어 목록 업데이트 시 UI에 통보.
*   `[Prop] OnTeamChanged` (Delegate): 팀 번호 바뀌었을 때 발생.
*   `[Prop] OnGameEndedWithWinner` (Delegate): 우승자 확정 시 게임 종료 브로드캐스트.
*   `[Prop] OnMatchEnded` (Delegate): 최신판 결산 창 정보 (승리자 이름, 위치) 알림.
*   `[Prop] OnBodyAssignmentComplete` (Delegate): 빙의 및 파트너 동기화 완료 시점 통보.
*   `[Prop] OnAllClientsSpawnReady` (Delegate): 모두 스폰을 마쳐 게임이 진짜로 시작될 때 호출.
*   `(Internal) ExpectedSpawnReadyCount` (int32): 서버 입장에서 받아야 할 스폰 '완료' 패킷 총개수.
*   `(Internal) SpawnReadyControllers` (TSet<APlayerController*>): 스폰 완료 패킷을 쏜 컨트롤러 목록 (중복 방지).
*   `(Internal) SpawnReadyTimeoutHandle` (FTimerHandle): 비정상 클라이언트로 인한 무한 대기 방지 타이머.

#### Functions
*   `[Func] NotifyWidgetIfSpawnReady(UObject*, FName)`: 위젯 객체에 스폰 준비 상태를 Custom Event 명으로 강제 리플렉션 호출.
*   `[Func] UpdatePlayerList()`: PlayerArray 배열을 FBRUserInfo 배열로 변환 구축.
*   `[Func] GetAllPlayerUserInfo()`: 전체 유니크 접속자 정보 반환.
*   `[Func] GetPlayerUserInfo(int32)`: 단일 인덱스 정보 반환.
*   `[Func] GetLobbyEntryDisplayList()`: 대기열 UI(가장 우측) 표기용 슬롯 변환.
*   `[Func] GetLobbyTeamSlotInfo(...)`, `GetLobbyTeamSlotInfoByTeamIDAndPlayerIndex(...)`: 각 팀 박스(하체/상체/관전) UI 데이터 반환.
*   `[Func] GetHostPlayerName()`: 방장 이름 반환 (Ex: XXX's Game).
*   `[Func] GetRoomTitleDisplay()`: UI에서 방 이름 출력할 때 RoomTitle 포맷팅 변환.
*   `[Func] CheckCanStartGame()`: 서버에서 현재 인원과 배정 상태를 보고 Start 버튼 허용 여부를 검증.
*   `[Func] AssignRandomTeams()`: Entry에 있는 인원을 랜덤한 확률로 Team Slot에 밀어넣음.
*   `[Func] AreAllPlayersReady()`, `AreAllNonHostPlayersReady()`: 모두 레디를 박았는지 확인.
*   `[Func] OnRep_...`: `PlayerCount`, `PlayerListForDisplay`, `LobbySlots`, `CanStartGame`, `RoomTitle`, `WinningTeamNumber`, `BodyAssignmentComplete`, `AllClientsSpawnReady` 관련 RepNotify.
*   `[RPC] MulticastMatchEnded(...)`: 최종 승자를 클라이언트에 전파하고 결산 처리.
*   `(Internal) AssignPlayerToLobbyTeam(...)`: 지정 슬롯에 플레이어를 강제 배치 (Server 로직).
*   `(Internal) MovePlayerToLobbyEntry(...)`: 팀 슬롯의 플레이어를 하차시키고 엔트리로 돌려보냄.
*   `(Internal) SetRoomTitle(...)`: 맵 입장 시 호스트 권한으로 서버에 방장 타이틀 명명.
*   `(Internal) SetExpectedSpawnReadyCount(int32)`: 대기해야 할 총 스폰 카운트 세팅.
*   `(Internal) ReportClientSpawnReady(APlayerController*)`: 특정 PC가 스폰 완료를 고발하면 Array에 담고 꽉차면 다같이 시작 통보.
*   `(Internal) EndGameWithWinner(int32)`: 구버전 승자 기록 전용 함수.
*   `(Internal) CompactLobbyEntrySlots()`: 대기열 사이에 빈 공간(퇴장한 유저)이 생기면 당겨서 정렬.
*   `(Virtual) GetLifetimeReplicatedProps(...)`: Replicated 변수 등록부.
*   `(Virtual) BeginPlay()`: 타이머 등 초기설정.
*   `(Internal) OnSpawnReadyTimeout()`: 스폰 서버 동기화가 너무 길어지면 강제로 무시하고 시작 통보.

---

### 2.3. `UBRGameInstance`
*   **파일**: `BRGameInstance.h` / **상속**: `UGameInstance`
*   **설명**: 맵 전환 시 소실되지 않는 영구 캐시. 환경 설정 파일 및 Steam/LAN 세션 생성 통제.

#### Member Variables
*   `[Prop] ConfigDataMap` (TMap): 무기, 체력 등 엑셀(JSON) 데이터 테이블 연동 구조체 맵.
*   `[Prop] PlayerName`, `UserUID` (FString): 로컬 게이머 이름 및 고유 식별자 (SteamID 등).
*   `[Prop] bUseLANOnly` (bool): 랜선 전용 멀티 활성화 플래그.
*   `[Prop] LocalCustomizationData` (FBRCustomizationData): 저장된 스킨 및 치장 정보 메모리.
*   `[Prop] OnRoomTitleReceived` (Delegate): 방 검색 및 조인 시 세션 이름을 띄워주기 위한 델리게이트 알림.
*   `(Internal) bDidCreateRoomThenTravel` (bool): 룸 생성 직후 로비로 넘어가는 과도기 판단 플래그.
*   `(Internal) bExcludeOwnSessionFromSearch` (bool): 자기가 만든 방이 검색 리스트에 뜨는 것 방지.
*   `(Internal) PendingRoomName`, `CachedRoomTitle` (FString): 방 정보 보조 메모 공간.
*   `(Internal) bPendingApplyRandomTeamRoles` (bool): 로비에서 게임 이동 시 롤 배정이 필요한 예약 상태.
*   `(Internal) PendingRoleRestoreByName` (TMap): SteamID 기준 팀 번호 및 역할 임시 보존통 (끊김 복구 및 트래블 보존).
*   `(Internal) PendingRoleRestoreByIndex` (TArray): 인덱스 기준 임시 보존.
*   `(Internal) ListenServerTimerHandle`, `SessionRecreateTimerHandle`: 세션 관련 비동기 타이머.
*   `(Internal) OnWorldCleanupHandle`, `PrePIEEndedHandle`: 에디터 내 종료 시 좀비 프로세스 방지 핸들러.

#### Functions
*   `[Func] CreateRoom(FName)`, `FindRooms()`, `JoinRoom(int32)`, `LeaveRoom()`: 기본 세션 랩퍼 컴포넌트 호출.
*   `[Func] RequestAssignToLobbyTeam(...)`, `RequestMoveMyPlayerToLobbyEntry()`, `ToggleReady()`, `RandomTeams()`, `ChangeTeam(...)`, `StartGame()`: 로비 버튼 RPC를 컨트롤러에 중계.
*   `[Func] ShowRoomInfo()`, `ReloadAllConfigs()`, `UpdateDataTableFromJson()`: 개발 편의성 함수 (JSON 파싱).
*   `[Func] Get/SetPlayerName()`, `Get/SetUserUID()`: 게이머 신분 상호작용.
*   `[Func] SetUseLANOnly(bool)`, `GetUseLANOnly()`: 랜서버 토글.
*   `[Func] SetLANOnly(int32)`: 콘솔 명령어용 래퍼 (SetLANOnly 1).
*   `[Func] SetExculdeOwnSessionFromSearch(...)`: 룸 검색 옵션 켜고 끄기.
*   `[Func] SetPendingRoomName()`, `SetCachedRoomTitle()` 등 방 이름, 아이디 임시 세터.
*   `[Func] SaveCustomization()`, `GetLocalCustomization()`: 개인화 설정 반환/저장.
*   `(Internal) LoadPlayerNameFromUserInfo()`, `SavePlayerSettingsToSlot()`, `LoadPlayerSettingsFromSlot()`: 로컬 SaveGame 세이브/로드 기능.
*   `(Internal) SavePendingRolesForTravel(...)`, `RestorePendingRolesFromTravel(...)`: 트래블 후 사라지는 역할을 보존/복원.
*   `(Internal) ClearPendingRoleRestoreData()`, `HasPendingRoleRestore()`, `GetPendingRoleRestoreCount()`, `HasPendingUserInfoForIndex(...)`, `RestoreUserInfoToPlayerStateForPostLogin(...)`: 유저 인벤토리 및 팀 정보 복원 체크.
*   `(Internal) ApplyGlobalMultipliers()`: 공격력/체력 글로벌 계수 불러오기.
*   `(Internal) LoadConfigFromJson()`, `GetConfigDirectory()`: JSON 설정 파싱 엔진 연결기.
*   `(Internal) DoPIEExitCleanup(UWorld*)`: 에디터 클리너.

---

### 2.4. `ABRPlayerController`
*   **파일**: `BRPlayerController.h` / **상속**: `APlayerController`
*   **설명**: 플레이어의 입력, RPC 허브, 주요 UI 메뉴 생성 및 파괴. 상/하체 조작 콘텍스트 스왑 주관.

#### Member Variables
*   `[Prop] LowerBodyContext`, `UpperBodyContext` (UInputMappingContext*): 신체용 각각의 키배열.
*   `[Prop] IA_SpectatorMove`, `IA_SpectatorLook` (UInputAction*): 프리캠 관전용 에셋(WASD/마우스).
*   `[Prop] EntranceMenuWidgetClass`, `JoinMenuWidgetClass`, `LobbyMenuWidgetClass`, `MainScreenWidgetClass` (TSubclassOf): 기초 UMG 블루프린트 에셋 슬롯.
*   `[Prop] OnPawnChanged` (Delegate): 소유 폰이 변경될 때 알림.
*   `[Prop] OnEnterSpectatorModeDelegate` (Delegate): 블루프린트 관전 상태 UI 연동기.
*   `(Internal) CurrentMenuWidget`, `MainScreenWidget` (UUserWidget*): 화면 객체의 캐시 포인터.
*   `(Internal) BeginPlayUITimerHandle`: PIE에서 빠른 로딩으로 인한 UI 씹힘 방지 비동기 대기 타이머.
*   `(Internal) UpperBodyViewInputDelayHandle`: 빙의 직후 시점 고정 강제 재시도용 딜레이 핸들러.
*   `(Internal) TimerHandle_RetrySubmitCustomization`: 서버에 자기 외형 패킷 쏘다 실패 시 재시도.
*   `(Internal) ShutdownListenServerTimerHandle`: 호스트가 방을 나와 메뉴로 갈 때 덤프 정리 타이머.
*   `(Internal) SkipLoadingScreenCheckHandle`: WBP_Loading 스킵 검증 스레드.
*   `(Internal) bSpawnReadyDelegateBound` (bool): 델리게이트를 2번 타지 않게 막는 락커.
*   `(Internal) LastSensitiveRPCTime` (float), `MinSensitiveRPCIntervalSec` (static constexpr): 분당 클릭수/핵(스팸 RPC) 방지.

#### Functions
*   `[Func] SetMainScreenWidget(...)`: 위젯 객체 캐싱.
*   `[Func] ShowEntranceMenu()`, `ShowJoinMenu()`, `ShowLobbyMenu()`, `ShowMainScreen()`, `HideMainScreen()`, `HideCurrentMenu()`: UMG 전환 메서드 랩퍼(Switcher 사용).
*   `[Func] SetMainScreenToEntranceMenu()`, `SetMainScreenToLobbyMenu()`: 서브 위젯 탭 이동.
*   `[Func] GetCurrentMenuWidget()`: 현재 화면에 있는 창 찾기.
*   `[Func] SubmitCustomizationToServer()`: 자신이 고른 모자를 서버에 강제 통보(RPC 촉발).
*   `[Func] StartSpectatingMode()`: 자신을 관전자로 전환해달라고 자체 판정(State 변경 및 물리 제거).
*   `[Func] SetupSpectatorInput()`: 죽은 관전자 전용 키배열로 전환.
*   `[Event] OnEnterSpectatorMode()`: 관전 연출용 블루프린트 오버라이드.
*   `[RPC Server] ServerCreateRoom(...)`, `ServerFindRooms(...)`, `ServerJoinRoom(...)`, `ServerToggleReady()`, `ServerRequestRandomTeams()`, `ServerRequestChangePlayerTeam(...)`, `ServerRequestStartGame()`, `ServerSetPlayerName(...)`, `ServerSetTeamNumber(...)`, `ServerSetPlayerRole(...)`, `ServerRequestAssignToLobbyTeam(...)`, `ServerRequestMoveToLobbyEntry(...)`, `ServerReportSpawnReady()`: 모든 로비 제어/준비 패킷을 캡슐화한 허브.
*   `[RPC Client] ClientHandleSpectatorUI()`: 서버가 죽었다고 인정해준 사람에게 UI 띄우라는 응답.
*   `(Internal) ClearUIForShutdown()`: 종료 시 생성했던 UMG 찌꺼기 말소.
*   `(Internal) SetupRoleInput(bool)`: IsLower 인자에 맞춰 상/하체 조작기 오버라이드.
*   `(Virtual) BeginPlay()`, `EndPlay(...)`, `SetupInputComponent()`, `OnPossess(APawn*)`, `AcknowledgePossession(APawn*)`, `OnRep_Pawn()`: C++ 엔트리 제어.
*   `(Internal) ApplyUpperBodyViewAndInput()`: 상체용 머리 고정 카메라 뷰포인트 강제 집행.
*   `(Internal) HandleNetworkFailure(...)`: 끊김/에러 시 이유를 로그로 남기고 화면 초기화.
*   `(Internal) HandleCreateRoomComplete(...)`: 방 파기 성공 델리게이트 캐치.
*   `(Internal) CheckSensitiveRPCRateLimit()`, `IsInLobbyMap()`: 디도스 스팸 클릭 차단.
*   `(Internal) RequestRandomTeams()`, `RequestChangePlayerTeam(...)`, `RequestStartGame()`, `ShowMenuWidget(...)`: 기타 도우미.
*   `(Internal) TryShutdownListenServerForRoomSearch()`: 메뉴로 갈 때 랜서버 종료 지시.
*   `(Internal) Input_SpectatorMove(...)`, `Input_SpectatorLook(...)`: 스페이스/방향키, 마우스 시점처리기.
*   `(Internal) OnAllClientsSpawnReadyCallback()`: Delegate Broadcast 수신 시 캐릭터 입력 블록(`bMoveInputUnblocked`) 풀어줌.
*   `(Internal) TryUnblockInputIfSkipLoadingScreen()`: 디버깅 용도로 로딩 안띄울 거면 블록 해제함.

---

### 2.5. `ABRPlayerState`
*   **파일**: `BRPlayerState.h` / **상속**: `APlayerState`
*   **설명**: 플레이어 소속 팀, 상/하체 여부 현황판. 접속이 끊겨도 유지되어야 할 개인 기록부.

#### Member Variables
*   `[Prop] TeamNumber` (int32, Rep): 소속 팀 (0=미배정, 1~4=팀).
*   `[Prop] bIsHost` (bool, Rep): 본인이 호스트인지 기록.
*   `[Prop] bIsReady` (bool, Rep): 로비 준비 버튼 상태.
*   `[Prop] bIsSpectatorSlot` (bool, Rep): 대기열에 들어간 스펙터 슬롯인지, 혹은 죽어서 간건지 판별.
*   `[Prop] bIsLowerBody` (bool, Rep): 하체(True) 혹은 상체(False).
*   `[Prop] ConnectedPlayerIndex` (int32, Rep): 서로 한 몸인 파트너의 PlayerArray 번호값.
*   `[Prop] PartnerPlayerState` (ABRPlayerState*, Rep): 파트너 객체의 찐 포인터.
*   `[Prop] UserUID` (FString, Rep): 스팀 고유 해시/ID.
*   `[Prop] CurrentStatus` (EPlayerStatus, Rep): enum 타입 (Alive / Dead / Spectating).
*   `[Prop] OnPlayerStatusChanged`, `OnPlayerRoleChanged`, `OnPlayerSwapAnim`, `OnCustomizationDataChanged`: 블루프린트 송출용 델리게이트들.
*   `[Prop] CustomizationData` (FBRCustomizationData, Rep): 캐릭터 외형 스킨.

#### Functions
*   `[Func] GetUserInfo()`: 개인의 정보 구조체 묶음망 리턴.
*   `[Func] SetUserUID(...)`, `SetPlayerNameString(...)`, `SetTeamNumber(...)`, `SetIsHost(...)`, `ToggleReady(...)`: 값 업데이트 Setter.
*   `[Func] OnRep_...`: `UserUID`, `TeamNumber`, `IsHost`, `IsReady`, `PlayerRole`, `PartnerPlayerState`, `PlayerStatus`, `CustomizationData` 각각의 RepNotify 콜백.
*   `[Func] SetPlayerRole(...)`, `SetSpectator(...)`: 상하체 및 관전 결정권장.
*   `[RPC Client] ClientShowSwapAnim()`: 스위치 오브 먹었을때 연출을 전 클라이언트에 명령.
*   `[Func] SwapControlWithPartner()`: 역할 체인징 스파게티 로직의 최종 엮음 지점.
*   `[RPC Server] ServerSetCustomizationData(...)`: 서버쪽으로만 코스튬 옷입기 허가 통보.
*   `(Internal) SetPlayerStatus(...)`: enum 강제 변경.
*   `(Internal) NotifyUserInfoChanged()`: 정보 갱신 후 게임스테이트 리스트 업데이트 재귀 킥.
*   `(Virtual) GetLifetimeReplicatedProps()`, `BeginPlay()`, `CopyProperties(...)`: 플레이어 이동 간 메모리 카피 엔진 훅.

---

## 3. Characters & Pawns (캐릭터 조작체)

### 3.1. `ABaseCharacter`
*   **파일**: `BaseCharacter.h` / **상속**: `ACharacter`
*   **설명**: 펀치부터 사망체 랙돌(Ragdoll)을 거느리는 기반 액터.

#### Member Variables
*   `[Prop] HeadMesh`, `ChestMesh`, `HandMesh`, `LegMesh`, `FootMesh` (USkeletalMeshComponent*): 부위별 커스터마이징 파츠.
*   `[Prop] AttackComponent` (UBRAttackComponent*): 데미지 발생기 및 허브 컴포넌트.
*   `[Prop] DefaultWalkSpeed`, `MaxHP`, `CurrentHP` (float, Rep): 기초 스탯.
*   `[Prop] OnHPChanged` (Delegate): 프로그레스바 깎는 용도.
*   `[Prop] OneHandedAttackMontage`, `TwoHandedAttackMontage`: 한손/양손 공격 모션 몽타주.
*   `[Prop] PunchMontage_L`, `PunchMontage_R`: 맨주먹 컴뱃.
*   `[Prop] LastDeathInfo` (FDeathDamageInfo, Rep): 마지막에 받은 충격량 값(위치, 각도) 저장기.
*   `[Prop] CurrentWeapon` (ABaseWeapon*, Rep): 들고 있는 무기 포인터.
*   `[Prop] bIsStunned` (bool, Rep): 기절 플래그.
*   `[Prop] StunDuration` (float): 그로기 유지 시간.
*   `[Prop] PunchSwingSound`, `PunchHitSound` (USoundBase*): 사운드 이펙트.
*   `[Prop] PunchVolume` (float): 볼륨.
*   `(Internal) bNextAttackIsLeft`, `bIsCharacterAttacking` (bool): 공속 제어 블록.
*   `(Internal) StunTimerHandle`, `PhysicsReactionTimerHandle`: 각종 경직 처리 비동기 타이머.

#### Functions
*   `[Func] EnhancePhysics(bool)`: 충격 받았을 때 물리 애니메이션 강제 주입(경직용).
*   `[Func] EquipWeapon(...)` / `[RPC Server] ServerEquipWeapon(...)`: 무기 탈착/투척 지시. 
*   `[Func] EquipArmor(...)`, `SetArmorColor(...)`: 커스텀 적용 함수.
*   `[Func] IsDead()`: 체력이 바닥인지 리턴.
*   `[Event] OnEnterStunState()`, `OnRecoverFromStun()`: 기절 연출 별자리 표시용 UI 구동부.
*   `[RPC NetMulticast] MulticastPlayWeaponAttack(...)`, `MulticastPlayPunch(...)`: 공격 몽타주 모두에게 동기화.
*   `[RPC NetMulticast] MulticastPerformDeathVisuals(...)`: 치명타를 받고 죽었을 시 클라이언트들도 똑같은 방향으로 랙돌 날아가게 강제 동기화.
*   `[RPC NetMulticast] MulticastPlayPhysicalHitReaction(...)`: 한쪽 다리나 팔에만 데미지가 들어와 물리 흔들림이 필요할 때 사용.
*   `[RPC NetMulticast] MulticastEnterStunState()`, `MulticastRecoverFromStun()`: 기절 멀티캐스트.
*   `[RPC NetMulticast] MulticastHandleWeaponBroken()`: 내구도 파괴에 따른 무기 해제 및 시각효과(GeometryCollection) 통보 동기화.
*   `(Virtual) TakeDamage(...)`: 범용 데미지 연산.
*   `(Internal) OnRep_CurrentHP()`, `UpdateHPUI()`.
*   `(Internal) RequestAttack()`, `HandleWeaponBroken()`: 로직 연계 허브.
*   `(Internal) Die(...)`: 게임모드에 사망 선언 후 모든 로직 제거 및 랙돌.
*   `(Internal) SetLastHitInfo(...)`: 마지막 공격 정보 갱신처.
*   `(Virtual) BeginPlay()`, `GetLifetimeReplicatedProps()`.

---

### 3.2. `APlayerCharacter`
*   **파일**: `PlayerCharacter.h` / **상속**: `ABaseCharacter`
*   **설명**: 1P가 빙의하여 하체를 컨트롤(이동, 점프, 스태미나 소비)하는 덩어리.

#### Member Variables
*   `[Prop] StaminaComp` (UStaminaComponent*): 스태미나 소비 통제 부품.
*   `[Prop] RearCameraBoom`, `RearCamera`: 3인칭 기본 후방 카메라.
*   `[Prop] HeadMountPoint` (USceneComponent*): 머리 꼭대기 소켓 (여기다 상체 캐릭터를 붙임).
*   `[Prop] DefaultHeadMesh`, `ChestMesh`, `HandMesh` ... : 아무것도 안 입었을 경우의 기본 마네킹 옷.
*   `[Prop] DefaultMappingContext` (UIMC*), `MoveAction`, `LookAction`, `JumpAction`, `SprintAction` (UIA*): 컨트롤 키보드 매핑 연결.
*   `[Prop] WalkSpeed`, `SprintSpeed` (float): 이동 스펙.
*   `[Prop] UpperBodyAimRotation` (FRotator, Rep): 머리핀처럼 상체가 바라보는 에임 방향.
*   `[Prop] FootstepSound` (USoundBase*), `FootstepVolume`, `FootstepDistanceThreshold` (float): 거리 기반 발소리 구동 프로퍼티.
*   `[Prop] ForwardSprintFootstepMultiplier`, `BackwardSprintFootstepMultiplier`, `BackwardWalkFootstepMultiplier` (float): 전후좌우 이동 및 상태별 발소리 간격 디테일 배율.
*   `[Prop] OnStaminaChanged` (Delegate): 게이지 UI 조절용 액션.
*   `(Internal) CurrentUpperBodyPawn` (AUpperBodyPawn*): 타고 있는 2P 포인터.
*   `(Internal) AccumulatedDistance` (float): 여태 이동한 거리 적산치.
*   `(Internal) TimerHandle_RetryBindPartner`: 파트너 정보 다운로드 지연될 경우의 무한 대기루프.
*   `(Internal) bAppearanceLocked`, `bBoundToPartner`, `bMoveInputUnblocked` (bool): 이동 락커.
*   `(Static) Global_RotationRateYaw`, `Global_BrakingFriction`, `Global_BrakingDecelerationWalking`: 무브먼트 컴포넌트 광역 설정용 밸런스.

#### Functions
*   `[Func] UpdatePreviewMesh(...)`: 상점/로비창에서 아바타 구워보기(업데이트) 기능.
*   `(Internal) ProcessFootstep(DeltaTime)`: 거리 공식으로 Tick에서 발자국소리 연산.
*   `(Internal) Move()`, `Look()`, `SprintStart()`, `SprintEnd()`: UInputAction 연동부.
*   `(Virtual) Jump()`, `CanJumpInternal_Implementation()`, `OnJumped_Implementation()`: 공중 점프 스태미나 관리 구동부.
*   `(Internal) HandleSprintStateChanged(...)`, `HandleStaminaChanged(...)`: 델리게이트 수신부.
*   `(Internal) TryApplyCustomization()`: 씌워진 스킨 로드기.
*   `(Internal) BindToPartnerPlayerState(...)`: 파트너 상/하체 연결망 구독.
*   `(Internal) ApplyMeshFromID(...)`: 아이템 ID 파싱 및 스켈레탈 조립.
*   `(Internal) GetUpperBodyPlayerState()`, `GetLowerBodyPlayerState()`: 유틸리티 함수.
*   `(Internal) SetUpperBodyRotation(...)`: 상체 에임 허리 꺾임 적용기 (Pitch 반영).
*   `(Internal) SetUpperBodyPawn(...)`: 포인터 강제 세터.
*   `(Virtual) GetBaseAimRotation()`: 시선 처리용 엔진 파이프라인.
*   `(Virtual) BeginPlay()`, `SetupPlayerInputComponent(...)`, `PossessedBy(...)`, `Restart()`, `GetLifetimeReplicatedProps(...)`, `EndPlay(...)`, `Tick(...)`, `OnRep_PlayerState()`.

---

### 3.3. `AUpperBodyPawn`
*   **파일**: `UpperBodyPawn.h` / **상속**: `APawn`
*   **설명**: 하체 머리에 부착되어 총/칼을 휘두르는 반쪽짜리 상체 플레이어 포대.

#### Member Variables
*   `[Prop] FrontCameraBoom`, `FrontCamera`: 상체 전용 1인칭틱한 줌 시야.
*   `[Prop] UpperBodyMappingContext`, `LookAction`, `AttackAction`, `InteractAction`: 2P 전용 키 매핑.
*   `[Prop] InteractionDistance` (float): 파밍 탐지 거리 (기본 100.0f).
*   `[Prop] AttackMontage` (UAnimMontage*): 테스트용.
*   `[Prop] ParentBodyCharacter` (APlayerCharacter*): 숙주 하체 참조.
*   `(Internal) LastBodyYaw` (float): 시선 억제용 회전 한계선.

#### Functions
*   `[RPC Server] ServerRequestSetAttackDetection(...)`: 공격 버튼을 누를 시 히트박스 켬 패킷.
*   `[RPC Server] ServerRequestInteract(...)`: 땅바닥 확인해서 E 줍게 하는 패킷.
*   `[RPC Server] ServerUpdateAimRotation(...)`: 상체 에임 UDP 전파.
*   `(Internal) Look()`, `Attack()`, `Interact()`: 조작기.
*   `(Internal) OnAttackMontageEnded(...)`: 모션 딜레이 초기화.
*   `(Virtual) BeginPlay()`, `EndPlay()`, `Tick()`, `SetupPlayerInputComponent()`, `OnRep_PlayerState()`.

---

## 4. Components & Objects

### 4.1. `UStaminaComponent`
*   **파일**: `StaminaComponent.h` / **상속**: `UActorComponent`
*   **설명**: 달리기 게이지.

#### Members
*   `[Prop] MaxStamina`, `CurrentStamina`, `JumpCost` (float).
*   `[Prop] StaminaDrainRate`, `StaminaRegenRate` (float).
*   `[Prop] bIsSprinting` (bool, Rep).
*   `[Prop] OnStaminaChanged`, `OnSprintStateChanged` (Delegate).
*   `(Static) Global_SprintDrainRate`, `Global_JumpCost`, `Global_RegenRate`.

#### Functions
*   `[RPC Server] ServerSetSprinting(bool)`.
*   `[Func] ConsumeJumpStamina()`, `CanJump()`, `GetStaminaRatio()`.
*   `(Internal) OnRep_...`

### 4.2. `UBRAttackComponent`
*   **파일**: `BRAttackComponent.h` / **상속**: `UActorComponent`
*   **설명**: 구체적인 펀치/무기 데미지 박스 연산, 역경직.

#### Members
*   `[Prop] StandardMass` (float).
*   `(Internal) HitActors` (TArray).
*   `(Internal) HitStopTimerHandle`.
*   `(Internal) bIsDetectionActive`.
*   `(Static) Global_BasePunchDamage`.

#### Functions
*   `[Func] SetAttackDetection(bool)`.
*   `[Func] GetCalculatedAttackSpeed()`.
*   `[RPC Server] ServerSetAttackDetection(bool)`.
*   `[RPC NetMulticast] MulticastApplyHitStop(float)`.
*   `[RPC NetMulticast] MulticastPlayHitSound(USoundBase*, FVector, float)`: 타격음 전역 동기화 (Volume 포함).
*   `(Internal) ProcessHitDamage(...)`, `InternalHandleOwnerHit(...)`, `ApplyHitStop(...)`, `ResetHitStop()`.

### 4.3. `ABaseWeapon` 및 `ASwitchOrb`
*   **SwitchOrb**: `OnOrbOverlap(...)`을 통해 충돌 즉시 파트너 간의 `SwapControlWithPartner()` (역할 교대)를 발생. `CollisionSphere`와 `OrbNiagaraComp` 파티클 소유.
*   **BaseWeapon**: `CurrentWeaponData`, `WeaponMesh`, `DurabilityReduction` 소유.
    *   `Interact(...)` (획득).
    *   `[Func] DecreaseDurability(...)` (내구 깎기).
    *   `[Func] OnRep_CurrentWeaponData()`: 클라이언트에 데이터 동기화 시 스탯 초기화(InitializeWeaponStats)를 자동 트리거.
    *   `[RPC] Multicast_BreakWeaponVisual(...)` (깨짐 연출, GeometryCollection 투입).
    *   `BreakWeapon()` 및 `IsEquipped()`.

### 4.4. `UBRWidgetFunctionLibrary` (Static)
*   **파일**: `BRWidgetFunctionLibrary.h`
*   **설명**: UMG 블프와 C++ 통신 가교 함수 일체.
*   **주요 API**: `GetBRGameSession()`, `GetBRGameInstance()`, `GetBRGameState()`, `NotifyWidgetIfSpawnReady()`, `GetRoomTitleForDisplay()`, `GetDisplayNameForLobby()`, `RequestAssignToLobbyTeam()`, `ShowMainScreen()` 외 UI 교체 및 세션 명령 래퍼 노드들 모음.

---

## 5. UI Widgets & Blueprint Assets (주요 추가 사항)

### 5.1. UI (UMG) Widgets
*   `WBP_ResultMenu`: 게임 결산 화면을 담당. 승리자 정보 및 로비 복귀 타이머를 표시.
*   `WBP_AIHPBar`: PVE 몬스터(`BP_AI_Enemy` 등) 머리 위에 출력되는 체력 프로그레스바 위젯.
*   `WBP_Loading`, `WBP_Load`, `WBP_InGameScreen`: 레벨 전환 시 및 게임 내 상태 HUD.
*   `WBP_MainMenu`: 타이틀 진입 시 가장 먼저 출력되는 메인 메뉴(게임 로고 및 배경).

### 5.2. Assets & BGM
*   **Logos**: `GameLogo.uasset`, `MainLogo.uasset` (UI에 부착되는 전용 타이틀 이미지).
*   **Stage BGM**: `Stage01_Temple` 부터 `Stage06_Hunting` 까지 각 맵에 대응되는 테마 BGM(e.g., `1MapSound.uasset` ~ `6MapSound.uasset`)이 맵별로 개별 적용됨.
