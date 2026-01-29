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

## 주의사항

1. **컴파일 필요**: C++ 클래스를 생성했으므로 언리얼 엔진에서 프로젝트를 다시 컴파일해야 합니다.
2. **블루프린트 상속 변경**: 각 블루프린트 위젯의 Parent Class를 해당 C++ 클래스로 변경해야 합니다.
3. **이벤트 바인딩**: 블루프린트 이벤트는 자동으로 호출되므로, 블루프린트에서 해당 이벤트를 구현하면 됩니다.
4. **권한 확인**: 방장 전용 기능은 `IsHost()` 함수로 확인 후 UI를 표시/숨김 처리하세요.

## 문제 해결

- **PlayerController를 찾을 수 없음**: 위젯이 생성될 때 PlayerController가 아직 준비되지 않았을 수 있습니다. `NativeConstruct`에서 약간의 지연 후 다시 시도하거나, 블루프린트에서 `GetBRPlayerController`를 호출하여 null 체크를 하세요.
- **이벤트가 호출되지 않음**: GameSession이나 GameState의 이벤트가 제대로 바인딩되었는지 확인하세요. `NativeConstruct`에서 자동으로 바인딩됩니다.
