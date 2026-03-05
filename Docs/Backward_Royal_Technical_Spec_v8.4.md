# Backward Royal 기술 분석서 (Exhaustive Causal Chain Spec v8.4)

## 1. 개요 (Overview)
본 문서는 Backward Royal 프로젝트의 **모든 실행 흐름(Execution Flow)** 을 인과 사슬(Causal Chain)로 정리하고, 각 단계에서 **블루프린트로 제어 가능한 지점(BP Hook / API)** 을 완벽하게 망라한 최종 기술 명세서입니다.

*   **Causal Chain**: 사건의 원인(Trigger)과 결과(Result)를 연결하여 로직의 흐름을 파악합니다.
*   **Blueprint API**: C++에서 노출한 `BlueprintCallable` 함수들을 명시하여, 스크립팅 가능 시점을 제공합니다.

---

## 2. Chain 1: 세션 생성 및 접속 (Session & Connection)

### 1-1. 방 만들기 (Create Room)
*   **Trigger**: UI에서 'Create' 버튼 클릭
*   **Flow**:
    1.  **BP Call**: `BRPlayerController::CreateRoomWithPlayerName(RoomName, PlayerName)`
        *   내부적으로 `GameInstance::SetPlayerName` 저장 후 `ServerCreateRoom` 호출.
    2.  `BRGameSession::CreateRoomSession` -> 세션 생성 성공.
    3.  `ServerTravel("/Game/Main/Level/LobbyMap?listen")` 실행.

### 1-2. 방 찾기 및 입장 (Join Room)
*   **Trigger**: 'Find Room' 후 슬롯 클릭
*   **Flow**:
    1.  **BP Call**: `BRPlayerController::FindRooms()` -> `ServerFindRooms`.
    2.  **BP Call**: `BRPlayerController::JoinRoomWithPlayerName(Index, Name)`.
    3.  `ClientTravel` -> 로비 맵 진입.

### 1-3. 접속 후 처리 (Post Login)
*   **Trigger**: 플레이어 로비 진입 완료 (`PostLogin`)
*   **Flow**:
    1.  `BRGameMode::PostLogin`: `PlayerState` 설정.
    2.  `BRGameState::UpdatePlayerList`: 접속자 목록 갱신.
    3.  **BP Event**: `BRPlayerController::OnPlayerListChanged` 방송.
        *   -> 로비 UI(`WBP_Lobby`)에서 이를 감지하여 리스트 갱신.

---

## 3. Chain 2: 로비 관리 (Lobby Management)

### 3-1. 팀 슬롯 배치 (Assign Team)
*   **Trigger**: UI에서 팀 슬롯 버튼 클릭
*   **Flow**:
    1.  **BP Call**: `BRPlayerController::RequestAssignToLobbyTeam(TeamIndex, SlotIndex)`
        *   `TeamIndex`: 0~3 (팀 1~4)
        *   `SlotIndex`: 1(하체), 2(상체)
    2.  `BRGameState::AssignPlayerToLobbyTeam` (Server)
        *   -> `LobbyTeamSlots` 갱신.
        *   -> **[Logic Update]**: 팀 배정 시 자동으로 `bIsReady = true`로 설정됨.
    3.  **BP Event**: `OnPlayerListChanged` 방송 -> UI 슬롯 갱신.

### 3-2. 대기열 복귀 (Return to Entry)
*   **Trigger**: 선택한 슬롯 다시 클릭
*   **Flow**:
    1.  **BP Call**: `BRPlayerController::RequestMoveMyPlayerToLobbyEntry()`
    2.  `BRGameState`: `LobbyTeamSlots`에서 제거 -> `LobbyEntrySlots`로 이동.
    3.  `bIsReady = false`로 자동 변경.

### 3-3. 게임 시작 가능 확인 (Ready Check)
*   **Trigger**: 팀 배정 변경 시마다
*   **Flow**:
    1.  `BRGameState::CheckCanStartGame`
    2.  조건: `MinPlayers` 충족 && `AreAllPlayersReady` (팀 배정 인원 모두)
    3.  **Result**: 호스트의 'Start Game' 버튼 활성화 (`bCanStartGame` Replicated).

---

## 4. Chain 3: 게임 시작 및 스폰 (Game Start)

### 4-1. 레벨 이동 (Level Transition)
*   **Trigger**: 호스트가 'Start Game' 클릭
*   **Flow**:
    1.  **BP Call**: `BRPlayerController::StartGame()`
    2.  `BRGameInstance::SavePendingRolesForTravel`: 현재 로비 상태 백업.
    3.  `ServerTravel("/Game/Main/Level/BattleMap")`.

### 4-2. 스폰 및 빙의 (Spawn & Possess)
*   **Trigger**: 맵 로드 후 `GameMode` 타이머 (약 2~3초)
*   **Flow**:
    1.  `BRGameMode::ApplyRoleChangesForRandomTeams`: 백업된 역할 복원.
    2.  **Spawn**:
        *   하체(`PlayerCharacter`) 스폰.
        *   상체(`UpperBodyPawn`) 스폰 -> 하체 머리에 `Attach`.
    3.  **Possess**: 각 `PlayerController`가 자신의 Pawn에 빙의.
    4.  *(신규) 클라이언트 스폰 완료 동기화 (All Clients Spawn Ready)*:
        *   클라이언트 `PlayerController`는 빙의(Possess) 직후 즉각적으로 `ServerReportSpawnReady()` RPC를 발생시켜 스스로 맵 로드와 스폰이 끝났음을 서버에 알림 (호스트는 직접 카운트).
        *   서버의 `BRGameState`는 접속한 모든 클라이언트가 이 `ReportClientSpawnReady`를 쐈는지 집계.
        *   모두 수집되면 `OnAllClientsSpawnReady`가 NetMulticast로 브로드캐스트.
    5.  **Input Setup**:
        *   `bAllClientsSpawnReady`가 true가 되기 전까지, 최하단 `PlayerCharacter::Move` 함수의 이동 입력(`bMoveInputUnblocked`)은 차단됨.
        *   `SetupRoleInput(true)`: WASD 이동 (하체).
        *   `SetupRoleInput(false)`: 마우스 회전 (상체).
    6.  **BP Hook**: `GameState::OnAllClientsSpawnReady` 델리게이트를 통해 `WBP_Loading`을 숨기거나 카운트다운(`WBP_InGameScreen`)을 띄우는 용도로 사용 가능 (`NotifyWidgetIfSpawnReady` 활용).
    7.  **BP Hook**: `GameState::OnBodyAssignmentComplete`에 바인딩하여 InGame UI(크로스헤어, 체력바 등) 띄우기 수행 가능.

---

## 5. Chain 4: 이동 및 전투 (Movement & Combat)

### 5-1. 이동 (Movement)
*   **Trigger**: WASD 입력
*   **Flow**:
    1.  `Input Action Move` 트리거.
    2.  `PlayerCharacter::Move`: 전방/후방 벡터 내적 계산.
        *   뒤로 걸으면 속도 0.5배 페널티.
    3.  `PlayerCharacter::Tick` -> `ProcessFootstep(DeltaTime)`:
        *   이동할 때마다 `AccumulatedDistance`를 누적하여, `FootstepDistanceThreshold` 초과 시 **발자국 사운드 재생**.
    4.  **BP Hook**: `PlayerCharacter`의 `OnStaminaChanged` 등을 바인딩하여 HUD에 표시 가능.

### 5-2. 달리기 (Sprint)
*   **Trigger**: Shift 입력
*   **Flow**:
    1.  `SprintStart` -> `StaminaComponent::ServerSetSprinting(true)`.
    2.  **BP Hook**: `PlayerCharacter::OnSprintStateChanged` 델리게이트.

### 5-3. 공격 및 타격 판정 (Attack & Hit Detection)
*   **Trigger**: 마우스 클릭
*   **Flow**:
    1.  `UpperBodyPawn::Attack` (Input) -> `ServerRequestAttack`.
    2.  `BaseCharacter::RequestAttack` (하체 본체).
    3.  `MulticastPlayWeaponAttack`: 애니메이션 재생 및 `AttackComponent`의 공격 판정 활성화.
    4.  **타격 처리 (`UBRAttackComponent::ProcessHitDamage`)**:
        *   **데미지 계산**: 무기 타격 시 데미지 계산 및 `ApplyDamage` 호출 (맨손 추가 데미지 보정 등).
        *   **타격음 연출**: 데미지 로직 직후 `MulticastPlayHitSound`를 쏴 타격 부위에서 사운드를 동기화 재생.
        *   **역경직 (Hit Stop)**: 타격 성공 시 `MulticastApplyHitStop(0.1f)` 호출 (잠시 후 몽타주 강제 종료).
        *   **물리 반발력 (Physics Reaction)**: 충돌한 메쉬(Mesh)의 뼈(BoneName)를 찾아 `AddImpulseAtLocation`을 이용해 명시적 충격을 가함 (거친 피지컬 애니메이션과 래그돌 흔들림 반응 유도).
    5.  **무기 내구도 결과**: 무기 충돌 시 `DecreaseDurability`가 호출되며, 내구도가 0 이하라면 `BreakWeapon`이 동작하여 무기를 파괴 처리함.

### 5-4. 피격 및 사망 (Damage & Death)
*   **Trigger**: 무기 충돌 (`ApplyDamage`)
*   **Flow**:
    1.  `BaseCharacter::TakeDamage`.
    2.  `CurrentHP` 차감 -> **BP Event**: `OnHPChanged` 방송.
    3.  `CurrentHP <= 0` -> `Die()`.
    4.  `Die()` Flow:
        *   `PerformDeathVisuals`: 랙돌(Ragdoll) 활성화.
        *   `BRGameMode::OnPlayerDied(Victim)`.

---

## 6. Chain 5: 상호작용 및 교체 (Interaction & Swap)

### 6-1. 상호작용 (Interact)
*   **Trigger**: 상체 플레이어가 **'E' 키** 입력
*   **Flow**:
    1.  `UpperBodyPawn::Interact` (Input Action)
    2.  `SphereOverlap` -> `SwitchOrb` 감지.
    3.  `ServerRequestInteract` RPC 전송.

### 6-2. 역할 교체 (Swap Role)
*   **Trigger**: `SwitchOrb::OnOrbOverlap` (Server)
*   **Flow**:
    1.  `BRPlayerState::SwapControlWithPartner`.
    2.  **Logic**:
        *   `UnPossess` -> Pawn 교환 -> `Possess`.
        *   `ConnectedPlayerIndex` 정보 업데이트.
    3.  **Input Update**: `ClientRestart` -> `SetupRoleInput` 재실행.
        *   상체였던 유저 -> WASD 이동 가능.
        *   하체였던 유저 -> 마우스 회전 가능.

---

## 7. Chain 7: 네트워크 장애 처리 (Network Failure Handling)

### 7-1. 예기치 않은 접속 종료 (Connection Lost / Timeout)
*   **Trigger**: 엔진 `NetDriver`에서 타임아웃, 접속 유실 등 장애 감지
*   **Flow**:
    1.  `BRPlayerController::HandleNetworkFailure` 콜백 호출.
    2.  에러 사유(`FailureType`) 분석:
        *   `ConnectionLost`, `ConnectionTimeout`, `PendingConnectionFailure`, `OutdatedClient`, `OutdatedServer` 등.
    3.  로컬 디버그 로그 및 화면 출력.
    4.  `ClientTravelToGameMap` (사실상 `ClientTravel("/Game/Main/Level/EntranceMenu?listen", TRAVEL_Absolute)`) 실행:
        *   예외가 발생한 클라이언트를 로비(EntranceMenu)로 강제 회귀시킴.

---

## 8. Chain 8: 게임 종료 (Game End)

### 8-1. 사망 및 관전 전환 (Death & Spectating)
*   **Trigger**: 무기 공격으로 체력이 0이 되어 `BaseCharacter::Die` 실행
*   **Flow**:
    1.  `PlayerState::CurrentStatus`를 `Dead`로 변경.
    2.  `MulticastPerformDeathVisuals`: 치명타 충격량과 위치를 받아 랙돌(Ragdoll) 물리 효과 즉시 동기화.
    3.  `BRGameMode::OnPlayerDied(Victim)` 내부 후속 처리:
        *   Victim과 동일 팀 파트너(`ConnectedPlayerIndex` 기준)를 찾아 둘 다 `CurrentStatus = Dead`, `SetSpectator(true)`로 변경.
        *   동시에 `CheckMatchWinner()` 즉각 호출.
        *   2초 후 타이머를 통해 `SwitchTeamToSpectator` 호출하여 죽은 팀원 양쪽 모두 관전 모드로 전환.

### 8-2. 승리 판정 및 결산 처리 (Win Check & Result)
*   **Trigger**: `CheckMatchWinner` 실행
*   **Flow**:
    1.  `BRGameMode::CheckMatchWinner`: `PlayerArray`를 순회하며 살아있는 팀(`AliveTeams`) 집계.
    2.  생존 팀이 오직 1팀만 남은 경우 결산 진입.
    3.  결산 정보 수집: 승리 팀의 위치(`WinnerLocation`) 및 상/하체 플레이어 이름 추출.
    4.  `BRGameState::MulticastMatchEnded` (RPC) 방송:
        *   전 클라이언트에 승리 위치와 팀원들 이름 데이터가 전파되어 결과(Result) UI 띄움 가능.
    5.  `BRGameMode`의 10초 타이머(`ReturnToLobbyTimerHandle`)를 세팅하여 시간이 되면 로비 맵으로 전원 자동 복귀(`ReturnToLobby`).

---

## 9. Chain 9: 사운드 동기화 (Sound Synchronization)

### 9-1. 발소리 다방향 배율 동기화 (Footstep Directional Sync)
*   **Trigger**: 하체 플레이어의 WASD 틱 이동 (`PlayerCharacter::ProcessFootstep`)
*   **Flow**:
    1.  `GetVelocity`의 수평 속도와 `GetActorForwardVector`를 내적(Dot Product)하여 이동 방향(전진/후진) 판별.
    2.  `StaminaComp`를 통해 현재 달리기(`bIsSprinting`) 여부 획득.
    3.  뱡향과 달리기 상태에 따라 3가지 배율 모델 적용:
        *   뒤로 달릴 때: `FootstepDistanceThreshold * BackwardSprintFootstepMultiplier`
        *   앞으로 달릴 때: `FootstepDistanceThreshold * ForwardSprintFootstepMultiplier`
        *   뒤로 걸을 때: `FootstepDistanceThreshold * BackwardWalkFootstepMultiplier`
    4.  이동 누적 거리(`AccumulatedDistance`)가 위 임계치를 넘으면 `PlaySoundAtLocation` 호출 (로컬 기반 재생이며 위치가 복제되므로 타 클라에서도 자연스럽게 재생).

### 9-2. 타격음 전역 동기화 (Combat Hit Sound Sync)
*   **Trigger**: 무기 공격 혹은 맨손 펀치가 적중했을 때 (`BRAttackComponent::ProcessHitDamage`)
*   **Flow**:
    1.  `BaseCharacter`에 장착된 무기(`MyWeapon`)가 있다면 무기의 고유 정보(`CurrentWeaponData.HitSound`) 로드.
    2.  무기가 없다면(맨손) 캐릭터 본체의 `PunchHitSound` 로드.
    3.  `MulticastPlayHitSound(Sound, ImpactPoint, Volume=1.0)` RPC 기폭.
    4.  전 클라이언트의 `MulticastPlayHitSound_Implementation` 진입 -> 해당 충돌 좌표 허공에서 모두가 공통된 타격음을 듣게 됨.

---

## 10. Blueprint API Reference (주요 함수 목록)

### BRPlayerController
*   `CreateRoom(RoomName)`
*   `CreateRoomWithPlayerName(RoomName, PlayerName)`
*   `FindRooms()`
*   `JoinRoom(SessionIndex)`
*   `LeaveRoom()`
*   `ToggleReady()`
*   `RequestAssignToLobbyTeam(TeamIndex, SlotIndex)`
*   `RequestMoveMyPlayerToLobbyEntry()`
*   `StartGame()` (Host Only)
*   `ShowRoomInfo()`
*   `SetMainScreenWidget(Widget)`: 메인 UI 위젯 등록
*   `ShowEntranceMenu()`, `ShowLobbyMenu()`, `ShowMainScreen()`: UI 전환
*   `StartSpectatingMode()` (관전 모드 진입)
*   `[Event] OnEnterSpectatorModeDelegate`: 관전 UI 갱신 바인딩용 블루프린트 노드

### BRGameState
*   `[Event] OnPlayerListChanged`: 대기열/팀 슬롯 변경 시 갱신 통보
*   `[Event] OnTeamChanged`: 팀 번호 복제 갱신
*   `[Event] OnMatchEnded`: 게임 결산 완료 정보 브로드캐스트
*   `[Event] OnAllClientsSpawnReady`: 전체 스폰 싱크 완료 (게임 시작 타이밍)
*   `[Event] OnBodyAssignmentComplete`: 상/하체 모델링 배정 완료 (인게임 UI용)
*   `NotifyWidgetIfSpawnReady(...)`: 스폰 완료 타이밍을 안전하게 호출

### PlayerCharacter / BaseCharacter
*   `OnHPChanged` (Delegate): 체력 변경 알림 (HUD 연결용).
*   `OnStaminaChanged` (Delegate): 스태미나 변경 알림.
*   `ApplyMeshFromID`: 커스터마이징 적용 함수.

이 문서는 Backward Royal의 모든 코드 흐름을 블루프린트 관점에서 제어할 수 있도록 재구성한 것입니다.
새롭게 추가된 **Chain 9 (사운드 동기화)** 기능과 위젯 연동(`NotifyWidgetIfSpawnReady` 등)의 훅(Hook) 설계 사항을 확인하시고 구현에 참고하십시오.
