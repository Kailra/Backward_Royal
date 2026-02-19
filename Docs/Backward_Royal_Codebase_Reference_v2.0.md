# Backward Royal 코드 베이스 분석서 (Codebase Reference v2.0)

## 1. 개요 (Overview)
본 문서는 `Source/Backward_Royal` 디렉토리 내의 핵심 소스 코드 파일(헤더)을 **전수 조사(Inventory)**하여 작성된 상세 분석서입니다. 요약 없이 모든 멤버 변수와 함수를 나열하는 것을 원칙으로 합니다.

*   **Version**: v2.0 (Full Inventory)
*   **Coverage**: Core Framework, Characters, Items, Components, UI Library
*   **Legend**:
    *   `[Prop]`: `UPROPERTY` (변수)
    *   `[Func]`: `UFUNCTION` (함수)
    *   `[Virtual]`: 가상 함수
    *   `[RPC]`: 네트워크 함수 (Server/Client/NetMulticast)

---

## 2. Core Framework (게임 로직)

### 2.1. `ABRGameMode`
*   **파일**: `BRGameMode.h` / **상속**: `AGameModeBase`
*   **설명**: 게임 규칙, 스폰, 역할 배정, 상태 전이 관리.

#### Member Variables
*   `[Prop] MinPlayers` (int32): 최소 시작 인원 (Default: 4).
*   `[Prop] MaxPlayers` (int32): 최대 인원 (Default: 8).
*   `[Prop] LobbyMapPath` (FString): 로비 레벨 경로.
*   `[Prop] GameMapPath` (FString): 게임 시작 레벨 경로 (Legacy).
*   `[Prop] StageFolderPath` (FString): 랜덤 스테이지 폴더 경로 (/Game/Main/Level/Stage).
*   `[Prop] StageMapPathsFallback` (TArray<FString>): 스테이지 폴더 검색 실패 시 사용할 맵 목록.
*   `[Prop] bUseRandomMap` (bool): 랜덤 맵 사용 여부.
*   `[Prop] SpawnDelayBetweenTeams` (float): 팀 스폰 간격 (1.2초).
*   `[Prop] DelayBeforeReturnToLobby` (float): 승리 후 로비 귀환 대기 시간 (2.0초).
*   `[Prop] UpperBodyClass` (TSubclassOf<AUpperBodyPawn>): 상체 Pawn 클래스 참조.

#### Functions
*   `[Func] StartGame()`: 게임 시작 프로세스 진입.
*   `[Virtual] PostLogin(APlayerController* NewPlayer)`: 플레이어 접속 처리.
*   `[Virtual] Logout(AController* Exiting)`: 플레이어 퇴장 처리.
*   `ApplyRoleChangesForRandomTeams()`: 저장된 팀 정보를 기반으로 전체 역할 적용 시작.
*   `ApplyRoleChangesForRandomTeams_ApplyOneTeam()`: 한 팀씩 순차적으로 스폰/빙의 처리.
*   `OnPlayerDied(ABaseCharacter* Victim)`: 플레이어 사망 시 호출.
*   `CheckAndEndGameIfWinner()`: 생존 팀 확인 및 승리 판정.
*   `SwitchTeamToSpectatorByPlayerIndices(...)`: 특정 팀을 관전 모드로 전환.
*   `SwitchEliminatedTeamToSpectator(...)`: 탈락 팀 전체 관전 전환.
*   `TravelToLobby()`: 로비 맵으로 이동.
*   `GetAvailableStageMapPaths()`: 스테이지 맵 목록 수집.
*   `ScheduleInitialRoleApplyIfNeeded()`: 게임 시작 직후 역할 적용 타이머 예약.

---

### 2.2. `ABRGameState`
*   **파일**: `BRGameState.h` / **상속**: `AGameStateBase`
*   **설명**: 방 정보, 플레이어 목록, 팀 슬롯 상태 동기화.

#### Member Variables
*   `[Prop] MinPlayers` (int32): 최소 인원.
*   `[Prop] MaxPlayers` (int32): 최대 인원.
*   `[Prop] PlayerCount` (int32, Rep): 현재 접속자 수.
*   `[Prop] PlayerListForDisplay` (TArray<FBRUserInfo>, Rep): UI용 전체 플레이어 정보.
*   `[Prop] LobbyEntrySlots` (TArray<int32>, Rep): 대기열 슬롯 상태 (PlayerIndex 저장).
*   `[Prop] LobbyTeamSlots` (TArray<int32>, Rep): 팀 배정 슬롯 상태.
*   `[Prop] bCanStartGame` (bool, Rep): 게임 시작 가능 여부.
*   `[Prop] WinningTeamNumber` (int32, Rep): 승리 팀 번호.
*   `[Prop] RoomTitle` (FString, Rep): 방 제목.
*   `[Prop] OnPlayerListChanged` (Delegate): 플레이어 목록 변경 알림.
*   `[Prop] OnTeamChanged` (Delegate): 팀 변경 알림.
*   `[Prop] OnGameEndedWithWinner` (Delegate): 게임 종료 알림.
*   `[Prop] OnMatchEnded` (Delegate): 매치 종료 세부 정보 알림.

#### Functions
*   `[Func] UpdatePlayerList()`: PlayerArray 기반 PlayreListForDisplay 갱신.
*   `[Func] GetAllPlayerUserInfo()`: 전체 유저 정보 반환.
*   `[Func] GetPlayerUserInfo(int32 Index)`: 특정 인덱스 유저 정보 반환.
*   `[Func] GetLobbyEntryDisplayList()`: 대기열 UI용 리스트 반환.
*   `[Func] GetLobbyTeamSlotInfo(...)`: 팀 슬롯 UI용 정보 반환.
*   `[Func] AssignRandomTeams()`: 대기열 인원 랜덤 팀 배정.
*   `[Func] CheckCanStartGame()`: 시작 조건(인원, 준비 완료) 확인.
*   `[Func] CheckCanStartGame()`: 시작 조건 확인.
*   `[Func] AreAllPlayersReady()`, `AreAllNonHostPlayersReady()`: 준비 상태 확인.
*   `AssignPlayerToLobbyTeam(...)`: (Server) 특정 슬롯으로 유저 이동.
*   `MovePlayerToLobbyEntry(...)`: (Server) 유저를 대기열로 복귀.
*   `EndGameWithWinner(int32 TeamNum)`: (Server) 승리 확정 및 종료 처리.
*   `[RPC] MulticastMatchEnded`: 매치 종료 정보 브로드캐스트.

---

### 2.3. `UBRGameInstance`
*   **파일**: `BRGameInstance.h` / **상속**: `UGameInstance`
*   **설명**: 데이터 보존, 세션 관리 래퍼, 설정 파일 로드.

#### Member Variables
*   `[Prop] ConfigDataMap` (TMap): 데이터 테이블 로드 설정.
*   `[Prop] PlayerName` (FString): 로컬 플레이어 이름.
*   `[Prop] UserUID` (FString): 로컬 유저 고유 ID.
*   `[Prop] bUseLANOnly` (bool): LAN 전용 여부.
*   `[Prop] LocalCustomizationData` (FBRCustomizationData): 로컬 커스터마이징 정보.
*   `[Prop] OnRoomTitleReceived` (Delegate): 방 제목 수신 알림.
*   `PendingRoleRestoreByName` (TMap): 레벨 이동 간 역할 백업 데이터.

#### Functions
*   `[Func] CreateRoom(Name)`, `FindRooms()`, `JoinRoom()`, `StartGame()`: 세션 함수 래퍼.
*   `[Func] ToggleReady()`, `RandomTeams()`, `ChangeTeam()`: 로비 액션 래퍼.
*   `[Func] ReloadAllConfigs()`, `UpdateDataTableFromJson()`: 데이터 테이블 갱신.
*   `[Func] SetUseLANOnly(bool)`: LAN 모드 설정.
*   `SavePendingRolesForTravel(GameState)`: 레벨 이동 전 상태 백업.
*   `RestorePendingRolesFromTravel(GameState)`: 레벨 이동 후 상태 복원.
*   `SetPendingRoomName(...)`, `GetPendingRoomName()`: 방 재생성을 위한 이름 임시 저장.

---

### 2.4. `ABRPlayerController`
*   **파일**: `BRPlayerController.h` / **상속**: `APlayerController`
*   **설명**: 입력 처리, UI 관리, RPC 통신.

#### Member Variables
*   `[Prop] LowerBodyContext` (UIMC*): 하체용 입력 매핑.
*   `[Prop] UpperBodyContext` (UIMC*): 상체용 입력 매핑.
*   `[Prop] EntranceMenuWidgetClass`, `LobbyMenuWidgetClass`, `MainScreenWidgetClass`: UI 위젯 클래스.
*   `[Prop] OnPawnChanged` (Delegate): 빙의 변경 알림.

#### Functions
*   `[Func] CreateRoom(...)`, `FindRooms()`, `JoinRoom(...)`: 세션 관리 명령.
*   `[Func] ToggleReady()`, `ChangeTeam(...)`, `StartGame()`: 게임 진행 명령.
*   `[Func] RequestAssignToLobbyTeam(...)`, `RequestMoveToLobbyEntry(...)`: 로비 슬롯 이동 요청.
*   `[Func] SetupRoleInput(bool bIsLower)`: 역할에 따른 IMC 교체.
*   `[Func] ShowEntranceMenu()`, `ShowLobbyMenu()`, `ShowMainScreen()`: UI 전환.
*   `[RPC] ServerCreateRoom`, `ServerFindRooms`, `ServerJoinRoom`: 서버 요청 RPC.
*   `[RPC] ServerRequestAssignToLobbyTeam`: 슬롯 이동 서버 요청.
*   `[RPC] ClientNotifyGameStarting`: 게임 시작 알림 수신.
*   `[RPC] ClientTravelToGameMap`: 강제 레벨 이동.

---

### 2.5. `ABRPlayerState`
*   **파일**: `BRPlayerState.h` / **상속**: `APlayerState`
*   **설명**: 플레이어 상태(팀, 역할, 파트너) 동기화.

#### Member Variables
*   `[Prop] TeamNumber` (int32, Rep): 팀 번호.
*   `[Prop] bIsHost` (bool, Rep): 방장 여부.
*   `[Prop] bIsReady` (bool, Rep): 준비 완료 여부.
*   `[Prop] bIsLowerBody` (bool, Rep): 하체(1P) 여부.
*   `[Prop] bIsSpectatorSlot` (bool, Rep): 관전 슬롯 여부.
*   `[Prop] ConnectedPlayerIndex` (int32, Rep): 파트너의 PlayerArray 인덱스.
*   `[Prop] PartnerPlayerState` (ABRPlayerState*, Rep): 파트너 객체 참조.
*   `[Prop] UserUID` (FString, Rep): 유저 고유 ID.
*   `[Prop] CustomizationData` (Struct, Rep): 외형 장비 정보.
*   `[Prop] CurrentStatus` (Enum, Rep): Alive/Dead/Spectating 상태.
*   `[Prop] OnPlayerRoleChanged`, `OnPlayerStatusChanged`, `OnCustomizationDataChanged`: 상태 변경 델리게이트.

#### Functions
*   `[Func] GetUserInfo()`: FBRUserInfo 구조체 반환.
*   `[Func] SetTeamNumber()`, `SetIsHost()`, `ToggleReady()`: 상태 설정.
*   `[Func] SetPlayerRole(bLower, PartnerIdx)`: 역할 및 파트너 설정.
*   `[Func] SwapControlWithPartner()`: 파트너와 제어권(Possess) 교체.
*   `[RPC] ServerSetCustomizationData`: 커스터마이징 정보 서버 전송.
*   `NotifyUserInfoChanged()`: 정보 변경 시 GameState 갱신 요청.

---

## 3. Characters & Pawns (캐릭터)

### 3.1. `ABaseCharacter`
*   **파일**: `BaseCharacter.h` / **상속**: `ACharacter`
*   **설명**: 캐릭터 공통 기능 (메쉬, 스탯, 전투).

#### Member Variables
*   `[Prop] HeadMesh`, `ChestMesh`, `HandMesh`, `LegMesh`, `FootMesh`: 방어구 메쉬 컴포넌트.
*   `[Prop] AttackComponent` (UBRAttackComponent*): 공격 판정 컴포넌트.
*   `[Prop] MaxHP`, `CurrentHP` (Rep): 체력 정보.
*   `[Prop] DefaultWalkSpeed`: 기본 이동 속도.
*   `[Prop] OneHandedAttackMontage`, `TwoHandedAttackMontage`: 무기 공격 몽타주.
*   `[Prop] PunchMontage_L`, `PunchMontage_R`: 맨손 공격 몽타주.
*   `[Prop] CurrentWeapon` (ABaseWeapon*, Rep): 현재 장착 무기.
*   `[Prop] LastDeathInfo` (Struct, Rep): 사망 정보 (위치, 회전, 충격량).
*   `[Prop] OnHPChanged` (Delegate): 체력 변경 알림.

#### Functions
*   `[Func] IsDead()`: 사망 여부 확인.
*   `[Func] EquipWeapon()`, `DropCurrentWeapon()`: 무기 장착/해제.
*   `[Func] EquipArmor()`, `SetArmorColor()`: 방어구 설정.
*   `[Func] EnhancePhysics(bool)`: 물리 시뮬레이션(랙돌) 제어.
*   `RequestAttack()`: 공격 요청 처리.
*   `Die()`: 사망 처리 (랙돌 전환, GameMode 알림).
*   `PerformDeathVisuals()`: 사망 연출 (랙돌).
*   `[RPC] MulticastPlayWeaponAttack`: 공격 애니메이션 재생.

---

### 3.2. `APlayerCharacter`
*   **파일**: `PlayerCharacter.h` / **상속**: `ABaseCharacter`
*   **설명**: 플레이어블 하체 캐릭터. 상체 Pawn을 부착하고 이동 담당.

#### Member Variables
*   `[Prop] StaminaComp` (UStaminaComponent*): 스태미나 컴포넌트.
*   `[Prop] RearCameraBoom`, `RearCamera`: 3인칭 카메라 (하체 시점).
*   `[Prop] HeadMountPoint` (USceneComponent*): 상체 부착 소켓.
*   `[Prop] UpperBodyAimRotation` (FRotator, Rep): 상체가 조준 중인 회전값.
*   `[Prop] MoveAction`, `LookAction`, `JumpAction`, `SprintAction`: 입력 액션 에셋.
*   `[Prop] WalkSpeed`, `SprintSpeed`: 이동 속도 설정.
*   `[Prop] OnStaminaChanged` (Delegate): 스태미나 변경 알림.

#### Functions
*   `Action Functions`: `Move()`, `Look()`, `Jump()`, `SprintStart()`, `SprintEnd()`.
*   `[Func] UpdatePreviewMesh()`: UI 미리보기용 메쉬 갱신.
*   `HandleSprintStateChanged()`: 달리기 상태 변경 처리.
*   `TryApplyCustomization()`: 커스터마이징 데이터 적용 시도.
*   `BindToPartnerPlayerState()`: 파트너 상태 변경 구독.
*   `SetUpperBodyRotation()`: 상체 회전값 동기화 설정.

---

### 3.3. `AUpperBodyPawn`
*   **파일**: `UpperBodyPawn.h` / **상속**: `APawn`
*   **설명**: 하체 위에 올라타는 상체 캐릭터. 공격 및 상호작용 담당.

#### Member Variables
*   `[Prop] FrontCameraBoom`, `FrontCamera`: 1인칭/3인칭 카메라 (상체 시점).
*   `[Prop] UpperBodyMappingContext`, `LookAction`, `AttackAction`, `InteractAction`: 입력 에셋.
*   `[Prop] InteractionDistance`: 상호작용 거리.
*   `[Prop] AttackMontage`: 공격 몽타주.
*   `[Prop] ParentBodyCharacter` (APlayerCharacter*): 부착된 부모 캐릭터.

#### Functions
*   `Action Functions`: `Look()`, `Attack()`, `Interact()`.
*   `[RPC] ServerRequestSetAttackDetection`: 공격 판정 요청.
*   `[RPC] ServerRequestInteract`: 상호작용 요청 (아이템, 스위치 등).
*   `[RPC] ServerUpdateAimRotation`: 회전값 동기화.

---

## 4. Items & Components (아이템 및 컴포넌트)

### 4.1. `ASwitchOrb`
*   **파일**: `SwitchOrb.h` / **상속**: `ADropItem`
*   **설명**: 획득 시 파트너와 역할을 교체하는 오브젝트.

#### Members
*   `[Prop] CollisionSphere` (USphereComponent*): 충돌 감지.
*   `[Prop] OrbNiagaraComp` (UNiagaraComponent*): 시각 효과.
*   `OnOrbOverlap(...)`: 충돌 핸들러 -> `PlayerState::SwapControlWithPartner` 호출.

### 4.2. `ABaseWeapon`
*   **파일**: `BaseWeapon.h` / **상속**: `AActor` + `IInteractableInterface`
*   **설명**: 무기 베이스 클래스.

#### Members
*   `[Prop] WeaponMesh`: 무기 메쉬.
*   `[Prop] CurrentWeaponData`: 무기 스탯 정보.
*   `[Prop] DurabilityReduction`: 내구도 감소량.

#### Functions
*   `Interact()`: 무기 획득 처리.
*   `[Func] DecreaseDurability()`: 내구도 감소.
*   `BreakWeapon()`: 내구도 0 시 파괴 처리.

### 4.3. `UStaminaComponent`
*   **파일**: `StaminaComponent.h`
*   **설명**: 스태미나 관리.

#### Members & Functions
*   `[Prop] MaxStamina`, `CurrentStamina`, `StaminaDrainRate`, `StaminaRegenRate`.
*   `[Prop] bIsSprinting`: 달리기 상태.
*   `[RPC] ServerSetSprinting(bool)`.
*   `[Func] ConsumeJumpStamina()`, `CanJump()`, `GetStaminaRatio()`.

### 4.4. `UBRAttackComponent`
*   **파일**: `BRAttackComponent.h`
*   **설명**: 공격 판정 및 피격 처리.

#### Members & Functions
*   `[Func] SetAttackDetection(bool)`: 공격 판정 활성화.
*   `[RPC] ServerSetAttackDetection`.
*   `ProcessHitDamage(...)`: 실제 데미지 적용.
*   `ApplyHitStop(...)`: 역경직 효과 적용.

### 4.5. `UBRWidgetFunctionLibrary`
*   **파일**: `BRWidgetFunctionLibrary.h`
*   **설명**: UMG용 정적 헬퍼 함수 모음.

#### Static Functions
*   `GetBRGameInstance`, `GetBRPlayerController`, `GetBRGameState`, `GetBRPlayerState`.
*   `SetPlayerName`: 플레이어 이름 설정 편의 함수.
*   `ToggleReady`, `RandomTeams`, `ChangeTeam`, `StartGame`: 로비 액션 래퍼.
*   `RequestAssignToLobbyTeam`, `RequestMoveToLobbyEntry`.
*   `ShowEntranceMenu`, `ShowLobbyMenu`, `ShowMainScreen`: UI 전환.
*   `IsHost`, `IsReady`, `IsConnectedToServer`: 상태 확인.
*   `GetRoomTitleForDisplay`, `GetDisplayNameForLobby`: 텍스트 포맷팅.
