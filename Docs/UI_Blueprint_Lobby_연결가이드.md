# 로비 UI 블루프린트 연결 가이드 (WBP_LobbyMenu / WBP_SelectTeam / 대기열)

코드 구현이 완료된 상태에서, 블루프린트에서 버튼 클릭 → 팀 배치 요청과 **OnPlayerListChanged** 시 화면 새로고침을 연결하는 방법입니다.

---

## 1. WBP_LobbyMenu (로비 메뉴)

**Parent Class:** `BR_LobbyMenuWidget`

### ⚠️ 중요: C++에서 이미 바인딩함

**GameState의 OnPlayerListChanged는 C++에서 이미 연결되어 있습니다.**

- `BR_LobbyMenuWidget::NativeConstruct()` 에서  
  `CachedGameState->OnPlayerListChanged.AddDynamic(this, &UBR_LobbyMenuWidget::HandlePlayerListChanged);`  
  로 바인딩하고, 델리게이트가 발생하면 **HandlePlayerListChanged** → **OnPlayerListChanged()** (블루프린트 이벤트) 가 호출됩니다.
- 따라서 **블루프린트에서 GameState에 Add Delegate 하지 마세요.**  
  (Event Construct → Get Game State → Add Delegate → Custom Event OnPlayerListUpdated 같은 연결은 제거하는 것이 좋습니다.)
- Destruct에서 **Remove Delegate**도 C++의 NativeDestruct에서 처리하므로 블루프린트에서 제거할 필요 없습니다.

---

### 1-1. OnPlayerListChanged 시 화면 새로고침 (블루프린트에서 할 일)

대기열·팀 슬롯이 바뀌면 C++가 **Event On Player List Changed** 를 호출합니다.  
**이 이벤트 한 군데**에서만 새로고침 로직을 넣으면 됩니다.

1. **Event On Player List Changed** (BR_LobbyMenuWidget 부모 클래스의 BlueprintImplementableEvent) 를 사용합니다.  
   - 우클릭 → **Add Event** → **Event On Player List Changed** (부모에서 상속된 이벤트)  
   - 기존에 만든 **Custom Event "OnPlayerListUpdated"** + **Add Delegate** 조합은 제거해도 됩니다.
2. **Event On Player List Changed** 의 **then** 에 아래를 연결합니다.

**대기열(WBP_Entry) 갱신**

- **Get BR Game State** (BR Widget Function Library)  
  - **World Context Object** = **Self** (이 위젯)
- **Get Lobby Entry Display List** (반환된 GameState에서 호출)  
  - Return Value → **Update Player Names** 의 **Player Info List**
- **Update Player Names** (Target = **WBP_Entry** 변수, **Player Info List** = 위 배열)

**팀 슬롯(WBP_SelectTeam_0~3) 갱신**

- **Update Slot Display** 를 **WBP_SelectTeam_0**, **WBP_SelectTeam_1**, **WBP_SelectTeam_2**, **WBP_SelectTeam_3** 각각에 대해 호출.  
  (각 위젯을 변수로 갖고 있으면, 그 변수를 Target으로 사용.)

**"WBP Select Team Object Reference is not compatible with BR Select Team Widget" 에러가 날 때**

- WBP_LobbyMenu에서 자식 위젯 **WBP_SelectTeam_0** 등의 타입이 **User Widget** 또는 **WBP_SelectTeam_C** 로만 잡혀 있으면, **Update Slot Display** 의 Target(타입: **BR Select Team Widget**)과 맞지 않아 에러가 납니다.
- **해결 1 (Cast 사용):** **WBP Select Team 0** 출력 → **Cast to BR Select Team Widget** → **As BR Select Team Widget** → **Update Slot Display** Target. (Cast 시 경고가 나오면 아래 해결 2 사용.)

**"BR Select Team Widget does not inherit from WBP Select Team (Cast would always fail)" 경고가 날 때**

- 블루프린트가 상속 방향을 잘못 해석해 나는 경고입니다. **WBP_SelectTeam** 의 부모가 **BR_SelectTeamWidget** 이면 실제로는 캐스트가 성공합니다.
- **해결 1:** **WBP_SelectTeam** 블루프린트를 연 뒤 **Class Settings** → **Parent Class** 가 **BR Select Team Widget** 인지 확인. **User Widget** 이면 **BR Select Team Widget** 으로 변경 후 컴파일.
- **해결 2 (권장, 경고 제거):** Cast 대신 **BR Widget Function Library** 의 **Update Select Team Slot Display** 를 사용합니다.
  1. **Update Player Names** (then) 다음에 **Update Select Team Slot Display** 노드를 넣습니다. (Category: BR Widget | Lobby)
  2. **Select Team Widget** 핀에 **WBP_SelectTeam_0** 을 직접 연결.
  3. 그 다음 **then** → 또 **Update Select Team Slot Display** → **WBP_SelectTeam_1** … 같은 식으로 1, 2, 3번까지 4번 호출.
  - 이 함수는 C++에서 내부적으로 Cast 후 **Update Slot Display** 를 호출하므로, 블루프린트에서는 **Cast to BR Select Team Widget** 노드를 쓰지 않아도 되고 경고가 사라집니다.

---

### 1-2. 현재 그래프 정리 (에러 방지)

지금 구조에서 다음을 권장합니다.

| 할 일 | 설명 |
|--------|------|
| **Add Delegate 제거** | Event Construct → Get Game State → **Add Delegate** (Target = GameState, Event = OnPlayerListUpdated) 연결을 제거. C++가 이미 바인딩함. |
| **Custom Event "OnPlayerListUpdated" 대체** | 새로고침 로직을 **Event On Player List Changed** 에만 연결. (부모 클래스에서 제공하는 이벤트) |
| **Destruct의 Remove Delegate 제거** | C++ NativeDestruct에서 이미 해제하므로 블루프린트에서 Remove Delegate 호출 제거. |
| **Get BR Game State에 World Context 넣기** | Get BR Game State 노드에서 **World Context Object** 핀에 **Self** (이 위젯) 연결. |
| **WBP_SelectTeam 갱신 추가** | Event On Player List Changed 쪽에서 WBP_Entry 갱신 뒤, **Update Slot Display** 를 WBP_SelectTeam_0, 1, 2, 3 네 개 위젯에 각각 호출. |

**Event On Player List Changed** 는 부모 클래스 `BR_LobbyMenuWidget` 에서 제공하는 **BlueprintImplementableEvent** 이므로, 오버라이드/구현만 하면 C++가 목록 변경 시 자동으로 호출합니다. 별도 Add Delegate 없이 이 이벤트 하나에만 새로고침 로직을 넣으면 됩니다.

---

## 2. WBP_SelectTeam (팀별 1P/2P 버튼)

**Parent Class:** `BR_SelectTeamWidget` **또는** `User Widget` (아래 “부모 없이 인터페이스만” 사용 시)

각 팀 위젯(WBP_SelectTeam_0 ~ 3)은 **TeamIndex** 하나와 **1P 버튼**, **2P 버튼**을 가집니다.

### 2-0. (선택) 부모를 BR_SelectTeamWidget으로 바꾸지 않고 쓰는 방법

WBP_SelectTeam의 **Parent Class를 User Widget으로 두고** 쓰려면, **인터페이스**만 구현하면 됩니다.

1. **WBP_SelectTeam** 블루프린트 열기 → **Class Settings** → **Interfaces** → **Add** → **BR Lobby Team Slot Display Interface** 추가.
2. **Event Graph**에서 **Event Update Slot Display** (인터페이스 이벤트) 를 추가하고, 아래 로직을 연결합니다.
   - **Get BR Game State** (World Context = **Self**)
   - **Get Lobby Team Slot Info** (GameState, **Team Index** = 이 위젯의 TeamIndex 변수, Slot Index = **0**) → **Get Display Name For Lobby** → 1P용 TextBlock에 **Set Text**
   - **Get Lobby Team Slot Info** (GameState, **Team Index**, Slot Index = **1**) → **Get Display Name For Lobby** → 2P용 TextBlock에 **Set Text**
3. **Team Index** 변수(0~3)와 1P/2P 이름 표시용 **TextBlock** 두 개는 블루프린트에서 직접 추가합니다.
4. WBP_LobbyMenu에서는 **Update Select Team Slot Display** (BR Widget Function Library)에 **WBP_SelectTeam_0~3** 를 그대로 넘기면 됩니다. (인터페이스를 구현한 위젯이면 C++에서 자동으로 **Update Slot Display** 가 호출됩니다.)

이렇게 하면 **BR_SelectTeamWidget을 부모로 두지 않아도** 동작합니다.

### 2-1. TeamIndex 설정

- **Details** → **Lobby** → **Team Index**
  - **WBP_SelectTeam_0** → `0` (1팀)
  - **WBP_SelectTeam_1** → `1` (2팀)
  - **WBP_SelectTeam_2** → `2` (3팀)
  - **WBP_SelectTeam_3** → `3` (4팀)

### 2-2. 1P / 2P 버튼 클릭 → 팀 배치 요청

**1P 버튼 (WBP_MainButtonText_1P 등)**

1. 버튼의 **On Clicked** 이벤트에 연결.
2. **Request Assign To Lobby Team** (BR Widget Function Library, Category: BR Widget | Lobby)
   - **World Context Object** = **Self** (이 위젯)
   - **Team Index** = **Team Index** (이 위젯의 변수, 0~3)
   - **Slot Index** = **0** (1P = 하체)

**2P 버튼 (WBP_MainButtonText_2P 등)**

1. 버튼의 **On Clicked** 이벤트에 연결.
2. **Request Assign To Lobby Team**
   - **World Context Object** = **Self**
   - **Team Index** = **Team Index**
   - **Slot Index** = **1** (2P = 상체)

이렇게 하면 클라이언트가 1P/2P를 눌렀을 때 서버에 팀·슬롯이 반영되고, UserInfo·GameState 갱신 후 복제로 **OnPlayerListChanged**가 떠서 위 1번처럼 화면이 새로고침됩니다.

### 2-3. 팀 슬롯에 이름 표시 (1P/2P 자리 텍스트)

C++의 **Update Slot Display**가 **PlayerNameSlot0**(1P), **PlayerNameSlot1**(2P) 텍스트를 갱신합니다.

- WBP_SelectTeam 안에서:
  - 1P 자리 **이름을 보여줄 TextBlock** → **Details**에서 **Player Name Slot 0** (BindWidgetOptional)에 지정.
  - 2P 자리 **이름을 보여줄 TextBlock** → **Player Name Slot 1**에 지정.

버튼과 텍스트가 같은 컴포넌트라면, 버튼 자체를 TextBlock으로 쓰기보다는 **버튼 안의 자식 TextBlock**을 PlayerNameSlot0/1로 바인딩하는 방식을 권장합니다. (BR_SelectTeamWidget은 `UTextBlock* PlayerNameSlot0/1`을 사용합니다.)

---

## 3. WBP_Entry (대기열)

**Parent Class:** `BR_LobbyEntryWidget`

### 3-1. 대기열 슬롯(이름) 표시

- 대기열 8칸에 해당하는 **TextBlock**들을 **UserName Slot** 배열에 넣습니다.
  - Details → **Lobby Entry** → **User Name Slot**  
  - 요소 0~7에 각 슬롯 TextBlock을 할당.

갱신은 **WBP_LobbyMenu**의 **OnPlayerListChanged**에서 **Get Lobby Entry Display List** → **Update Player Names** 로 하므로, Entry 블루프린트에서 추가 이벤트 연결은 필요 없습니다.

---

## 4. (선택) 팀 슬롯에서 대기열로 돌아가기

이미 들어간 팀의 1P/2P를 다시 눌러서 “대기열로 복귀”만 하고 싶다면, 같은 버튼에서 “내가 이미 이 슬롯에 있으면 MoveToLobbyEntry, 아니면 AssignToLobbyTeam”처럼 분기할 수 있습니다.  
코드에는 **Request Move To Lobby Entry** (World Context, Team Index, Slot Index) 가 있으므로, 블루프린트에서 “대기열로 돌아가기” 버튼/동작이 있으면 그쪽에서 이 함수를 호출하면 됩니다.

---

## 5. 요약 체크리스트

| 위치 | 할 일 |
|------|--------|
| **WBP_LobbyMenu** | Event **On Player List Changed** → Get BR Game State → Get Lobby Entry Display List → WBP_Entry **Update Player Names** 호출 + WBP_SelectTeam_0~3 각각 **Update Slot Display** 호출 |
| **WBP_SelectTeam_0~3** | **Team Index** 0~3 설정. 1P 버튼 **On Clicked** → **Request Assign To Lobby Team** (Self, Team Index, **0**). 2P 버튼 → (Self, Team Index, **1**). |
| **WBP_SelectTeam** | 1P/2P 이름 표시용 TextBlock을 **Player Name Slot 0** / **Player Name Slot 1** 에 바인딩 |
| **WBP_Entry** | 8개 슬롯 TextBlock을 **User Name Slot** 배열에 할당 |

이렇게 연결하면 “1~4팀 중 1P/2P 버튼 클릭 → UserInfo·GameState 갱신 → OnPlayerListChanged로 화면 새로고침”까지 코드와 UI 블루프린트가 맞물려 동작합니다.
