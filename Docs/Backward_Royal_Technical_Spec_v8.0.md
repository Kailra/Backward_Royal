# Backward Royal 기술 분석서 (Exhaustive Causal Chain Spec v8.0)

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
    4.  **Input Setup**:
        *   `SetupRoleInput(true)`: WASD 이동 (하체).
        *   `SetupRoleInput(false)`: 마우스 회전 (상체).

---

## 5. Chain 4: 이동 및 전투 (Movement & Combat)

### 5-1. 이동 (Movement)
*   **Trigger**: WASD 입력
*   **Flow**:
    1.  `Input Action Move` 트리거.
    2.  `PlayerCharacter::Move`: 전방/후방 벡터 내적 계산.
        *   뒤로 걸으면 속도 0.5배 페널티.
    3.  **BP Hook**: `PlayerCharacter`의 `OnStaminaChanged` 등을 바인딩하여 HUD에 표시 가능.

### 5-2. 달리기 (Sprint)
*   **Trigger**: Shift 입력
*   **Flow**:
    1.  `SprintStart` -> `StaminaComponent::ServerSetSprinting(true)`.
    2.  **BP Hook**: `PlayerCharacter::OnSprintStateChanged` 델리게이트.

### 5-3. 공격 (Attack)
*   **Trigger**: 마우스 클릭
*   **Flow**:
    1.  `UpperBodyPawn::Attack` (Input) -> `ServerRequestAttack`.
    2.  `BaseCharacter::RequestAttack` (하체 본체).
    3.  **Logic**: 무기 타입에 따라 `OneHanded` / `TwoHanded` 몽타주 선택.
    4.  `MulticastPlayWeaponAttack`: 애니메이션 재생.

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

## 7. Chain 6: 게임 종료 (Game End)

### 7-1. 승리 판정 (Win Check)
*   **Trigger**: `OnPlayerDied` 호출 시
*   **Flow**:
    1.  `BRGameMode::CheckAndEndGameIfWinner`.
    2.  생존 팀이 1팀만 남았는지 확인.

### 7-2. 결과 처리 (End Game)
*   **Trigger**: 승리 확정
*   **Flow**:
    1.  `BRGameMode::EndGameWithWinner(WinningTeamIdx)`.
    2.  `BRGameState::WinningTeamNumber` 변수 복제.
    3.  **BP Event**: `BRGameState::OnGameEndedWithWinner` 델리게이트 방송.
        *   **[Must Implement]**: 레벨 블루프린트나 HUD 위젯에서 이 이벤트를 잡아 **"Victory" UI**를 띄워야 함.
    4.  3초 후 로비로 자동 이동.

---

## 8. Blueprint API Reference (주요 함수 목록)

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

### BRGameState
*   `GetPlayerListForDisplay()`: 로비 UI용 접속자 정보 배열 반환.
*   `OnPlayerListChanged` (Delegate): 접속자/팀 변경 시 알림.
*   `OnGameEndedWithWinner` (Delegate): 게임 종료 및 승리팀 알림.

### PlayerCharacter / BaseCharacter
*   `OnHPChanged` (Delegate): 체력 변경 알림 (HUD 연결용).
*   `OnStaminaChanged` (Delegate): 스태미나 변경 알림.
*   `ApplyMeshFromID`: 커스터마이징 적용 함수.

이 문서는 Backward Royal의 모든 코드 흐름을 블루프린트 관점에서 제어할 수 있도록 재구성한 것입니다.
특히 **Chain 7-2 (게임 종료)** 와 **Chain 6-1 (상호작용)** 의 수정 사항을 확인하시고 구현에 참고하십시오.
