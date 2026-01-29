# 블루프린트 위젯과 서버 코드 연결 가이드

## 개요
서버 코드와 블루프린트 위젯을 연결하는 방법은 두 가지가 있습니다:

### 방법 1: Blueprint Function Library 사용 (추천) ⭐
- **상속 변경 불필요**: 기존 블루프린트 위젯을 그대로 사용
- **간단한 사용**: 블루프린트에서 정적 함수로 바로 호출
- `BRWidgetFunctionLibrary` 클래스 사용

### 방법 2: C++ 위젯 베이스 클래스 상속
- 블루프린트 위젯의 Parent Class를 변경해야 함
- `UBR_EntranceMenuWidget`, `UBR_JoinMenuWidget`, `UBR_LobbyMenuWidget` 사용

---

## 방법 1: Blueprint Function Library 사용 (상속 변경 불필요) ⭐

### 장점
- ✅ 블루프린트 위젯의 상속을 변경할 필요 없음
- ✅ 기존 위젯을 그대로 사용 가능
- ✅ 모든 위젯에서 동일한 함수 사용 가능

### 사용 방법

#### 1. 블루프린트에서 함수 호출
블루프린트 그래프에서 `BR Widget Function Library` 카테고리에서 함수를 찾아 호출하면 됩니다.

**예시:**
```
버튼 클릭 이벤트
  → Create Room (BR Widget Function Library)
    - World Context Object: Self (위젯 자신)
    - Room Name: "MyRoom"
```

#### 2. 사용 가능한 함수들

**세션 관련:**
- `Create Room` - 방 생성
- `Find Rooms` - 방 찾기
- `Join Room` - 방 참가 (SessionIndex 필요)

**로비 관련:**
- `Toggle Ready` - 준비 상태 토글
- `Random Teams` - 랜덤 팀 배정 (방장만)
- `Change Team` - 플레이어 팀 변경 (방장만)
- `Start Game` - 게임 시작 (방장만)

**정보 가져오기:**
- `Get BR Player Controller` - PlayerController 가져오기
- `Get BR Game Session` - GameSession 가져오기
- `Get BR Game State` - GameState 가져오기
- `Get Room Title For Display` - 로비 방 제목 "○○'s Game" (캐시 우선, 입장 직후 즉시 표시)
- `Get Display Name For Lobby` - 로비 플레이어 이름 표시용. `FBRUserInfo` 입력 → 표시용 문자열 반환. **User UID는 반환하지 않음.** (5번 섹션 참고)
- `Get BR Player State` - PlayerState 가져오기
- `Is Host` - 방장 여부 확인
- `Is Ready` - 준비 상태 확인

**로비 방 제목 표시 (○○'s Game, 입장 직후 즉시 표시):**
- 로비에서 방 제목을 표시할 때는 **Get Room Title For Display** (BR Widget Function Library)를 사용하세요.
- **Join Menu**에서 방 선택 후 **Join Room** 호출 시, **참가 전에** 해당 방의 `Get Session Name`(방 찾기 결과)을 캐시에 저장합니다. 따라서 **서버 연결 전·연결 대기 없이** 로비에 들어가자마자 "○○'s Game"이 표시됩니다.
- 보조로, 클라이언트 입장 시 서버 RPC로도 방 제목을 보내 캐시를 갱신합니다 (연결 후 일치 확인용).
- 예: `Get Room Title For Display (Self)` → `Conv_StringToText` → 방 제목 TextBlock `Set Text`
- **RPC 수신 시 갱신:** GameInstance의 **On Room Title Received** 이벤트에 바인딩해, RPC 도착 시 제목을 다시 설정하면 PreConstruct 이후 도착한 경우에도 즉시 반영됩니다.

#### 3. 이벤트 바인딩

GameSession의 이벤트를 블루프린트에서 직접 바인딩할 수 있습니다:

```
Event Construct
  → Get BR Game Session (Self)
  → Bind Event to OnCreateSessionComplete
    - Delegate: OnCreateSessionComplete
    - Event: (커스텀 이벤트 생성)
```

### 사용 예시

**WBP_EntranceMenu1에서 방 생성:**
```
[버튼 클릭 이벤트]
  → Create Room (BR Widget Function Library)
    - World Context Object: Self
    - Room Name: "TestRoom"
```

**WBP_JoinMenu1에서 방 참가:**
```
[방 선택 이벤트]
  → Join Room (BR Widget Function Library)
    - World Context Object: Self
    - Session Index: (선택한 방의 인덱스)
```

**WBP_LobbyMenu1에서 준비 토글:**
```
[준비 버튼 클릭]
  → Toggle Ready (BR Widget Function Library)
    - World Context Object: Self
```

---

## 방법 2: C++ 위젯 베이스 클래스 상속 (선택사항)

기존에 생성된 C++ 위젯 클래스를 사용하려면:

## 블루프린트 설정 방법

### 1. WBP_EntranceMenu1 설정

1. **블루프린트 클래스 상속 변경**
   - Content Browser에서 `WBP_EntranceMenu1` 열기
   - Class Settings → Parent Class를 `BR_EntranceMenuWidget`으로 변경

2. **사용 가능한 함수들**
   - `CreateRoom(FString RoomName)` - 방 생성
   - `FindRooms()` - 방 찾기
   - `GetBRPlayerController()` - PlayerController 가져오기

3. **이벤트 바인딩**
   - `OnCreateRoomComplete(bool bWasSuccessful)` - 방 생성 완료 시 호출되는 블루프린트 이벤트
   - `OnFindRoomsComplete()` - 방 찾기 완료 시 호출되는 블루프린트 이벤트

4. **사용 예시**
   ```
   - 버튼 클릭 이벤트 → CreateRoom 함수 호출 (방 이름 입력)
   - OnCreateRoomComplete 이벤트에서 성공/실패 처리
   ```

### 2. WBP_JoinMenu1 설정

1. **블루프린트 클래스 상속 변경**
   - Content Browser에서 `WBP_JoinMenu1` 열기
   - Class Settings → Parent Class를 `BR_JoinMenuWidget`으로 변경

2. **사용 가능한 함수들**
   - `JoinRoom(int32 SessionIndex)` - 방 참가 (세션 인덱스 전달)
   - `RefreshRoomList()` - 방 목록 새로고침
   - `GetBRPlayerController()` - PlayerController 가져오기
   - `GetBRGameSession()` - GameSession 가져오기

3. **이벤트 바인딩**
   - `OnJoinRoomComplete(bool bWasSuccessful)` - 방 참가 완료 시 호출
   - `OnFindRoomsComplete()` - 방 찾기 완료 시 호출

4. **사용 예시**
   ```
   - 방 목록 표시 후, 특정 방 선택 → JoinRoom 함수 호출
   - 새로고침 버튼 → RefreshRoomList 함수 호출
   - OnJoinRoomComplete 이벤트에서 성공 시 로비로 이동
   ```

### 3. WBP_LobbyMenu1 설정

1. **블루프린트 클래스 상속 변경**
   - Content Browser에서 `WBP_LobbyMenu1` 열기
   - Class Settings → Parent Class를 `BR_LobbyMenuWidget`으로 변경

2. **사용 가능한 함수들**
   - `ToggleReady()` - 준비 상태 토글
   - `RequestRandomTeams()` - 랜덤 팀 배정 (방장만)
   - `ChangePlayerTeam(int32 PlayerIndex, int32 TeamNumber)` - 플레이어 팀 변경 (방장만)
   - `RequestStartGame()` - 게임 시작 (방장만)
   - `GetBRPlayerController()` - PlayerController 가져오기
   - `GetBRGameState()` - GameState 가져오기
   - `GetBRPlayerState()` - PlayerState 가져오기
   - `IsHost()` - 방장 여부 확인
   - `IsReady()` - 준비 상태 확인

3. **이벤트 바인딩**
   - `OnPlayerListChanged()` - 플레이어 목록 변경 시 호출
   - `OnTeamChanged()` - 팀 변경 시 호출
   - `OnCanStartGameChanged(bool bCanStart)` - 게임 시작 가능 여부 변경 시 호출

4. **사용 예시**
   ```
   - 준비 버튼 → ToggleReady 함수 호출
   - 랜덤 팀 배정 버튼 (방장만 표시) → RequestRandomTeams 함수 호출
   - 게임 시작 버튼 (방장만, 게임 시작 가능할 때만 활성화) → RequestStartGame 함수 호출
   - OnPlayerListChanged 이벤트에서 플레이어 목록 UI 업데이트
   - OnTeamChanged 이벤트에서 팀 정보 UI 업데이트
   ```

## GameState에서 플레이어 정보 가져오기

로비 메뉴에서 플레이어 목록을 표시하려면:

```cpp
// C++에서 사용 가능한 함수 (블루프린트에서도 호출 가능)
ABRGameState* GameState = GetBRGameState();
TArray<FBRUserInfo> PlayerInfoList = GameState->GetAllPlayerUserInfo();

// 각 플레이어 정보
for (const FBRUserInfo& Info : PlayerInfoList)
{
    // Info.PlayerName - 플레이어 이름
    // Info.TeamID - 팀 번호 (0 = 팀 없음)
    // Info.bIsHost - 방장 여부
    // Info.bIsReady - 준비 상태
    // Info.PlayerIndex - 플레이어 인덱스
}
```

### WBP_Entry (로비 엔트리) – 입장 순서대로 이름 표시

로비에서 **WBP_Entry** (부모: `BR_LobbyEntryWidget`)를 사용할 때:

- `GetAllPlayerUserInfo()`로 받은 목록을 **UpdatePlayerNames**에 넘기면, **들어온 순서(0번, 1번, …)**대로 `UserNameSlot`에 플레이어 이름이 채워집니다.
- 해당 인덱스에 플레이어가 없으면 **빈 슬롯은 공란**으로 둡니다.
- 블루프린트에서 `UserNameSlot` 배열에 TextBlock을 **0번 슬롯, 1번 슬롯, …** 순서로 넣어두면 됩니다.

### WBP_LobbyMenu에서 WBP_Entry 갱신 (플레이어 목록 표시)

**WBP_LobbyMenu**에서 `WBP_Entry`에 입장 순서대로 이름을 넣으려면:

1. **Event Construct** (또는 **Pre Construct**)에서:
   - **Get BR Game State**
   - **Is Valid** (반환된 Game State) → **False**면 아무 것도 하지 않고 종료
   - **True**면:
     - **Add Dynamic** (Target = Game State, **On Player List Changed** → **Custom Event `OnPlayerListUpdated`**)
     - 그다음 **Get BR Game State** → **Get All Player User Info** → **Update Player Names** (Target = **WBP_Entry**, Player Info List = 반환 배열)

2. **Custom Event `OnPlayerListUpdated`** (플레이어 목록 변경 시마다 호출):
   - **Get BR Game State** → **Is Valid** → **False**면 종료
   - **True**면 **Get All Player User Info** → **Update Player Names** (Target = **WBP_Entry**, Player Info List)

3. **Event Destruct**에서:
   - **Get BR Game State** → **Is Valid** → **True**면  
     **Remove Dynamic** (Target = Game State, **On Player List Changed** → **OnPlayerListUpdated**)

4. **Get BR Game State · World Context Object**:  
   이 함수는 `WorldContext` 메타로 **World Context Object** 핀이 **숨겨져 있어서** 블루프린트에서 **self를 직접 연결할 수 없습니다**.  
   **위젯 블루프린트**(WBP_LobbyMenu 등)의 그래프에서 호출하면, 엔진이 **자동으로 해당 위젯(self)** 을 context로 씁니다.  
   따라서 **별도로 self 연결은 하지 않아도 되며**, 연결 불가한 것이 정상입니다.

5. **WBP_Entry** 변수는 로비 메뉴 위젯 계층에서 실제 **WBP_Entry** 자식 위젯을 참조해야 합니다.

**ErrorType=1**이 **Add Delegate** / **Get All Player User Info** 쪽에 나오면,  
Game State가 `null`인 경우(디자인 타임, 맵 로드 직후 등)가 많습니다. 위처럼 **Is Valid (Game State)** 분기로 방어하면 됩니다.

---

## LobbyMenu · Entry 블루프린트 흐름표

아래는 **WBP_LobbyMenu**와 **WBP_Entry** 블루프린트에서 **플레이어 이름 표시**가 이뤄지는 전체 흐름입니다.  
`→` 는 실행/데이터 연결, `├─` / `└─` 는 분기, `[ ]` 는 노드/이벤트 이름입니다.

---

### 1. WBP_Entry 블루프린트 흐름

**역할:** 로비 엔트리 위젯. `UserNameSlot`(TextBlock 배열)을 0~7번 슬롯으로 세팅하고,  
나중에 **Update Player Names** 호출 시 **들어온 순서대로 이름**을 채움. 빈 슬롯은 공란.

#### 1-1. Pre Construct (위젯 생성/리빌드 시 1회)

```
[Event Pre Construct]
    │
    │  IsDesignTime ──────────────────────┐
    │                                     │
    ▼                                     ▼
[Call Parent Function · Pre Construct]    (Parent에 전달)
    │
    │  then (exec)
    ▼
[Kismet Array Library · Array Clear]
    │  · Target Array ← UserNameSlot (Variable Get)
    │
    │  then (exec)
    ▼
[Variable Set · UserNameSlot]
    │  · UserNameSlot ← [Make Array] 결과
    │
    ▼
[Make Array] (8 elements)
    ├─ [0] ← TextBlock_Entry00  (Variable Get)
    ├─ [1] ← TextBlock_Entry01
    ├─ [2] ← TextBlock_Entry02
    ├─ [3] ← TextBlock_Entry03
    ├─ [4] ← TextBlock_Entry04
    ├─ [5] ← TextBlock_Entry05
    ├─ [6] ← TextBlock_Entry06
    └─ [7] ← TextBlock_Entry07
```

**정리:**  
Pre Construct → 부모 Pre Construct 호출 → **UserNameSlot** 초기화(Array Clear) →  
**Make Array**로 TextBlock_Entry00~07을 **0~7 순서**로 넣어 **UserNameSlot**에 Set.

#### 1-2. Update Player Names 호출 시 (C++ / LobbyMenu에서 호출)

```
[Update Player Names] (BR_LobbyEntryWidget)
    │  · self = WBP_Entry 인스턴스
    │  · Player Info List = GameState → GetAllPlayerUserInfo()
    │
    ▼
(C++ BR_LobbyEntryWidget::UpdatePlayerNames)
    │
    ├─ 1) 모든 UserNameSlot[i] → SetText("")  (공란)
    │
    └─ 2) for SlotIndex = 0 .. UserNameSlot.Num()-1:
             ├─ SlotIndex < PlayerInfoList.Num() ?
             │     ├─ Yes → UserNameSlot[SlotIndex] ← 이름 표시
             │     │         (비어 있으면 "Player N")
             │     └─ No  → 그대로 공란 유지
             └─ 다음 슬롯
```

**정리:**  
0번 슬롯 = 0번째로 들어온 플레이어, 1번 = 1번째, … 없으면 공란.

---

### 2. WBP_LobbyMenu 블루프린트 흐름

**역할:** 로비 메뉴 위젯. GameState **On Player List Changed**에 **OnPlayerListUpdated**를 바인딩하고,  
진입 시·플레이어 증감 시 **Get All Player User Info** → **Update Player Names(WBP_Entry)** 로 엔트리 갱신.

#### 2-1. 초기 설정 (Construct 시 1회) — CustomEvent 트리거 필수

```
[Event Construct]  (또는 Pre Construct)
    │
    │  then (exec)
    ▼
[Custom Event "CustomEvent"]  ← 반드시 여기서 호출되도록 연결
    │
    │  then (exec)
    ▼
[Get BR Game State] (BR Widget Function Library)
    │  · World Context = self 자동 (위젯 블루프린트 내 호출 시, 핀 연결 불필요)
    │  · Return Value → GameState
    │
    │  then (exec)
    ▼
[Is Valid] (GameState)
    │
    ├─ False ──→ (종료, 바인딩/갱신 안 함)
    │
    └─ True
         │
         │  then (exec)
         ▼
    [Add Dynamic] (Target = GameState)
         │  · Event: On Player List Changed
         │  · Delegate → Custom Event "OnPlayerListUpdated"
         │
         │  then (exec)
         ▼
    [Get BR Game State] (동일, World Context 자동)
         │  · Return Value → GameState
         │
         │  then (exec)
         ▼
    [Get All Player User Info] (BRGameState)
         │  · self = GameState
         │  · Return Value → Player Info List (Array of FBRUserInfo)
         │
         │  (exec는 Get BR Game State 쪽 then에서 직통)
         ▼
    [Update Player Names] (BR_LobbyEntryWidget)
         │  · self = WBP_Entry (Variable Get)
         │  · Player Info List ← 위 배열
         │
         └─ (완료)
```

**정리:**  
Construct → CustomEvent → **Get BR Game State** → **Is Valid** → True일 때만  
**Add Dynamic(On Player List Changed → OnPlayerListUpdated)** 후  
**Get Game State → Get All Player User Info → Update Player Names(WBP_Entry)** 로 최초 1회 갱신.

#### 2-2. 플레이어 목록 변경 시 (On Player List Changed 발생할 때마다)

```
[GameState · On Player List Changed]  (브로드캐스트)
    │
    │  (Add Dynamic으로 연결됨)
    ▼
[Custom Event "OnPlayerListUpdated"]
    │
    │  then (exec)
    ▼
[Get BR Game State] (World Context 자동)
    │  · Return Value → GameState
    │
    │  then (exec)
    ▼
[Is Valid] (GameState)
    │
    ├─ False ──→ (종료)
    │
    └─ True
         │
         │  then (exec)
         ▼
    [Get All Player User Info] (GameState)
         │  · Return Value → Player Info List
         │
         ▼
    [Update Player Names] (Target = WBP_Entry, Player Info List)
         │
         └─ (완료)
```

**정리:**  
로비 인원 변경 시 **OnPlayerListUpdated** 실행 → **Get Game State** → **Is Valid** →  
True면 **Get All Player User Info** → **Update Player Names(WBP_Entry)** 로 다시 갱신.

#### 2-3. 로비 메뉴 제거 시 (Destruct)

```
[Event Destruct]
    │
    │  then (exec)
    ▼
[Get BR Game State] (World Context 자동)
    │  · Return Value → GameState
    │
    │  then (exec)
    ▼
[Is Valid] (GameState)
    │
    ├─ False ──→ (종료)
    │
    └─ True
         │
         │  then (exec)
         ▼
    [Remove Dynamic] (Target = GameState)
         │  · Event: On Player List Changed
         │  · Delegate → Custom Event "OnPlayerListUpdated"
         │
         └─ (바인딩 해제 완료)
```

**정리:**  
Destruct 시 **Remove Dynamic**으로 **On Player List Changed → OnPlayerListUpdated** 바인딩 해제.

---

### 3. 전체 연동 흐름 (요약)

```
┌─────────────────────────────────────────────────────────────────────────┐
│  WBP_Entry (Pre Construct)                                               │
│    UserNameSlot = [TextBlock_Entry00 .. 07]  (0~7번 슬롯 고정)            │
└─────────────────────────────────────────────────────────────────────────┘
                                      │
                                      │  WBP_Entry는 WBP_LobbyMenu 자식으로 배치
                                      │  WBP_LobbyMenu 변수 "WBP_Entry"로 참조
                                      ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  WBP_LobbyMenu (Construct)                                               │
│    CustomEvent → Get Game State → Is Valid?                              │
│      → Add Dynamic(On Player List Changed → OnPlayerListUpdated)         │
│      → Get Game State → Get All Player User Info                         │
│      → Update Player Names(WBP_Entry, Player Info List)  [최초 1회]      │
└─────────────────────────────────────────────────────────────────────────┘
                                      │
                                      │  플레이어 입장/퇴장 시 GameState가
                                      │  On Player List Changed 브로드캐스트
                                      ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  OnPlayerListUpdated (Custom Event)                                      │
│    Get Game State → Is Valid?                                            │
│      → Get All Player User Info → Update Player Names(WBP_Entry, List)   │
└─────────────────────────────────────────────────────────────────────────┘
                                      │
                                      │  Update Player Names 내부 (C++)
                                      │  SlotIndex 0~7 ↔ PlayerInfoList[0~7]
                                      ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  WBP_Entry · UserNameSlot                                                │
│    [0] 1번째 입장자 이름  [1] 2번째  …  [7] 8번째  /  없으면 공란          │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│  WBP_LobbyMenu (Destruct)                                                │
│    Get Game State → Is Valid? → Remove Dynamic(On Player List Changed)   │
└─────────────────────────────────────────────────────────────────────────┘
```

---

### 4. 블루프린트에서 확인할 것

| 항목 | 확인 내용 |
|------|-----------|
| **CustomEvent 트리거** | Event Construct(또는 Pre Construct)에서 CustomEvent 호출되는지 |
| **Get BR Game State** | 위젯 블루프린트 내 호출 시 **World Context = self 자동 적용**. self 핀 연결 불가·불필요 |
| **Is Valid 분기** | Get BR Game State 직후 **Is Valid**로 null 체크 후 분기하는지 |
| **WBP_Entry 참조** | Update Player Names의 Target이 실제 **WBP_Entry** 자식 위젯인지 |
| **Remove Dynamic** | Event Destruct에서 **On Player List Changed** 바인딩 해제하는지 |
| **로비 이름 표시** | **Player Name**만 사용. **User UID**는 절대 표시하지 말 것. 아래 5번 참고. |

---

### 5. 로비 이름 표시: Player Name만, User UID 금지

플레이어 이름이 **UID**로 나오는 경우, 다음을 확인하세요.

- **로비 플레이어 이름은 반드시 `Player Name`만 사용**합니다. **`User UID`는 절대 표시하지 마세요.**
- **권장:** `Get All Player User Info` → **Update Player Names**(Target = WBP_Entry)만 사용하면, C++에서 `Player Name` 기준으로 표시합니다. `User UID`는 사용하지 않습니다.
- **직접 텍스트 설정 시:** `Get All Player User Info`로 배열을 받은 뒤, 각 `FBRUserInfo`에 대해 **Break BRUserInfo** → **User UID**가 아닌 **Player Name**을 TextBlock에 연결하세요.  
  또는 **Get Display Name For Lobby**(BR Widget Function Library)에 `FBRUserInfo`를 넣어 **표시용 이름**을 받아 사용하세요. 이 함수는 `Player Name`이 비어 있거나 `User UID`와 같으면 `"Player N"`을 반환하고, **`User UID`는 절대 반환하지 않습니다.**

---

## 주의사항

1. **컴파일 필요**: C++ 클래스를 생성했으므로 언리얼 엔진에서 프로젝트를 다시 컴파일해야 합니다.
2. **블루프린트 상속 변경**: 각 블루프린트 위젯의 Parent Class를 해당 C++ 클래스로 변경해야 합니다.
3. **이벤트 바인딩**: 블루프린트 이벤트는 자동으로 호출되므로, 블루프린트에서 해당 이벤트를 구현하면 됩니다.
4. **권한 확인**: 방장 전용 기능은 `IsHost()` 함수로 확인 후 UI를 표시/숨김 처리하세요.

## 문제 해결

- **PlayerController를 찾을 수 없음**: 위젯이 생성될 때 PlayerController가 아직 준비되지 않았을 수 있습니다. `NativeConstruct`에서 약간의 지연 후 다시 시도하거나, 블루프린트에서 `GetBRPlayerController`를 호출하여 null 체크를 하세요.
- **이벤트가 호출되지 않음**: GameSession이나 GameState의 이벤트가 제대로 바인딩되었는지 확인하세요. `NativeConstruct`에서 자동으로 바인딩됩니다.
