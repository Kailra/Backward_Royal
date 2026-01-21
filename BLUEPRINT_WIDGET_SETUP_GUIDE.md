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

## WBP_MainScreen1에 위젯 연결하기

**문제:** `WBP_MainScreen1`에 `WBP_EntranceMenu1`, `WBP_JoinMenu1`, `WBP_LobbyMenu1`을 연결하려고 하는데 변수가 검색되지 않음

**해결 방법:**

### 1단계: WBP_MainScreen1에 변수 추가

1. **블루프린트 에디터에서 WBP_MainScreen1 열기**
   - Content Browser에서 `WBP_MainScreen1` 더블클릭

2. **변수 추가**
   - 왼쪽 패널 "My Blueprint" → "Variables" 섹션
   - **"+" 버튼 클릭** → 변수 추가
   - 변수 이름: `WBP_EntranceMenu` (또는 원하는 이름)
   - 변수 타입: **Widget** (또는 `WBP_EntranceMenu1`으로 직접 지정)
     - 변수 타입을 클릭 → "Object Reference" → "WBP_EntranceMenu1" 검색 후 선택
   - **"Instance Editable" 체크** (디자이너에서 할당 가능하도록)
   - **"Expose on Spawn" 체크** (선택사항)

3. **다른 위젯들도 동일하게 추가**
   - `WBP_JoinMenu` 변수 추가 (타입: `WBP_JoinMenu1`)
   - `WBP_LobbyMenu` 변수 추가 (타입: `WBP_LobbyMenu1`)

### 2단계: 디자이너에서 위젯 할당

1. **디자이너 탭으로 이동**
   - 상단 탭에서 "Designer" 선택

2. **위젯 연결**
   - Content Browser에서 `WBP_EntranceMenu1`을 찾기
   - Details 패널에서 "WBP_EntranceMenu" 변수 찾기
   - 드롭다운을 클릭하여 `WBP_EntranceMenu1` 선택
   - 또는 Content Browser에서 위젯을 드래그하여 할당

3. **다른 위젯들도 동일하게 할당**
   - `WBP_JoinMenu` 변수에 `WBP_JoinMenu1` 할당
   - `WBP_LobbyMenu` 변수에 `WBP_LobbyMenu1` 할당

### 3단계: 블루프린트에서 위젯 사용

**위젯 표시/숨김:**

```
[버튼 클릭 이벤트]
  → Add to Viewport (WBP_EntranceMenu 변수)
    - 또는 Set Visibility
      - Visibility: Visible / Hidden / Collapsed
```

**위젯 전환 예시:**

```
[EntranceMenu에서 JoinMenu로 전환]
  → Remove from Parent (WBP_EntranceMenu)
  → Add to Viewport (WBP_JoinMenu)
```

**또는 PlayerController 함수 사용:**

```
[EntranceMenu에서 방 생성 성공 시]
  → Get BR Player Controller (BR Widget Function Library)
    - World Context Object: Self
  → Show Join Menu (또는 Show Lobby Menu)
```

### 4단계: 변수 검색 문제 해결

**문제: 변수가 검색되지 않음**

**해결 방법:**

1. **변수 타입을 명확하게 지정**
   - 변수 타입을 "Object Reference" → 특정 위젯 클래스 선택
   - 예: `WBP_EntranceMenu1` (정확한 클래스명)

2. **컴파일 확인**
   - 블루프린트 컴파일 후 저장
   - 다른 블루프린트에서 변수 검색

3. **변수 범위 확인**
   - "Public" 변수인지 확인
   - "Instance Editable" 체크 확인

4. **재생성**
   - 변수를 삭제하고 다시 생성
   - 정확한 타입으로 다시 추가

**참고:**
- 위젯 변수는 `User Widget` 타입이 아니라 **구체적인 위젯 클래스**로 지정하는 것이 좋습니다
- 예: `WBP_EntranceMenu1` 타입으로 지정하면 해당 위젯만 할당 가능

---

## 방법 1: Blueprint Function Library 사용 (상속 변경 불필요) ⭐

### 장점
- ✅ 블루프린트 위젯의 상속을 변경할 필요 없음
- ✅ 기존 위젯을 그대로 사용 가능
- ✅ 모든 위젯에서 동일한 함수 사용 가능

### 사용 방법

#### 1. 블루프린트에서 함수 호출하는 방법

**단계별 가이드:**

1. **블루프린트 그래프 열기**
   - Content Browser에서 위젯 파일 더블클릭

2. **이벤트 노드 생성**
   - 그래프에서 우클릭 → "Event" 검색
   - 버튼이면 "On Clicked" 이벤트 추가

3. **BR Widget Function Library 함수 찾기**
   - 그래프에서 우클릭 → 함수 이름 검색 (예: "Create Room")
   - 또는 "BR Widget" 카테고리에서 찾기
   - 노드를 드래그하여 이벤트에 연결

4. **World Context Object 연결**
   - 함수 노드의 "World Context Object" 핀에
   - "Self" 노드를 드래그하여 연결 (또는 자동 연결됨)
   - ⚠️ **중요**: `meta = (WorldContext = "WorldContextObject")`가 설정되어 있어서 자동으로 연결되어야 하지만, 수동으로 연결하는 것이 더 안전합니다

5. **필수 파라미터 입력**
   - **Room Name**: 방 이름을 입력해야 합니다
     - 텍스트 상자에서 가져오기: `Get Text (Text Input Widget)` → `Get Text` 연결
     - 또는 직접 문자열 입력: "TestRoom" 등

**예시:**
```
[버튼 클릭 이벤트]
  → Create Room (BR Widget Function Library)
    - World Context Object: Self (위젯 자신, 수동 연결 권장)
    - Room Name: "MyRoom" 또는 텍스트 입력 위젯에서 가져오기
```

**⚠️ 주의사항:**

1. **World Context Object는 반드시 연결해야 합니다** ⭐
   - ❌ **잘못된 연결**: `Self`를 `Target` 핀에 연결하는 것
   - ✅ **올바른 연결**: `Self`를 `World Context Object` 핀에 연결하는 것
   - `BRWidgetFunctionLibrary`는 정적 함수이므로 `Target` 핀이 없습니다
   - `World Context Object` 핀에 `Self`를 연결해야 합니다
   - 연결되지 않으면 PlayerController를 찾을 수 없어서 함수가 실행되지 않습니다
   - 로그에 "PlayerController를 찾을 수 없습니다" 에러가 나타납니다

2. **Room Name 파라미터는 필수입니다**
   - 비어있으면 방 이름이 빈 문자열로 전달됩니다
   - 빈 문자열이어도 방은 생성되지만, 명확한 이름을 지정하는 것이 좋습니다

3. **디버깅 방법**
   - 함수가 호출되지 않으면 로그에 에러가 나타나지 않습니다
   - 다음 로그들이 나타나야 합니다:
     ```
     LogTemp: Log: [WidgetFunctionLibrary] 방 생성 요청: [방이름]
     LogTemp: Log: [방 생성] 명령 실행: [방이름]
     ```
   - 로그가 없으면 함수가 호출되지 않은 것입니다 (파라미터 연결 확인)

**🔧 자주 발생하는 오류:**

**오류 1: Self를 Target에 연결**
```
❌ 잘못된 연결:
  Create Room
    - Target: Self ← 이것은 잘못됨!

✅ 올바른 연결:
  Create Room
    - World Context Object: Self ← 이것이 맞음!
    - Room Name: "TestRoom"
```

**오류 2: World Context Object 핀을 찾을 수 없음**

**증상:**
- `Create Room` 노드에 `World Context Object` 핀이 보이지 않음
- 블루프린트 코드에서 `bHidden=True`로 표시됨
- 함수가 실행되지 않음

**해결 방법:**

1. **노드 새로고침**
   - `Create Room` 노드를 선택
   - 노드를 우클릭 → "Refresh Node" 선택
   - 컴파일 후 `World Context Object` 핀이 표시되는지 확인

2. **노드 재생성 (가장 확실한 방법)**
   - 기존 `Create Room` 노드 삭제
   - 그래프에서 우클릭 → "Create Room" 검색
   - `BR Widget Function Library`의 `Create Room` 함수 선택
   - 새로 생성된 노드에 `Self`를 `World Context Object` 핀에 연결

3. **World Context Object 명시적 연결**
   - `Self` 노드를 그래프에 추가 (없으면 우클릭 → "Self" 검색)
   - `Self` 노드를 `Create Room` 노드로 드래그
   - `World Context Object` 핀에 연결 (자동 연결될 수도 있음)
   - 연결이 안 되면 노드를 우클릭 → "Refresh Node"

4. **Target 핀 확인**
   - `Target` 핀에 `Self`가 연결되어 있다면 **반드시 연결을 끊어야 합니다**
   - `Target` 핀은 정적 함수에서는 사용하지 않습니다
   - `Target` 핀은 자동으로 `Default__BRWidgetFunctionLibrary`로 설정됩니다 (숨겨져 있음)

**확인 방법:**
- `Create Room` 노드에서 `World Context Object` 핀이 보이는지 확인
- `Target` 핀에 연결이 없어야 함 (또는 자동으로 숨겨진 값만 있어야 함)
- `Room Name` 핀에 값이 있는지 확인

**블루프린트 코드 확인:**
```
CustomProperties Pin (PinId=67AA8E8F4015D86D8DCD62845243FD88,
   PinName="WorldContextObject",
   bHidden=True,  ← 이것이 True이면 문제!
   ...
)
```

**해결:**
- 노드를 새로고침하거나 재생성하면 `bHidden=False`가 되고 핀이 표시됩니다

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
- `Get BR Player State` - PlayerState 가져오기
- `Is Host` - 방장 여부 확인
- `Is Ready` - 준비 상태 확인

#### 3. 이벤트 바인딩 (방 생성/참가 완료 처리)

GameSession의 이벤트를 블루프린트에서 직접 바인딩할 수 있습니다:

**단계별 가이드:**

1. **Event Construct에서 시작**
   - 그래프에서 우클릭 → "Event Construct" 추가

2. **GameSession 가져오기**
   - 우클릭 → "Get BR Game Session" 검색 (BR Widget Function Library)
   - World Context Object: Self 연결

3. **이벤트 바인딩 노드 추가**
   - GameSession 노드에서 핀을 드래그하여 노드 생성
   - "Bind Event to OnCreateSessionComplete" 또는 다른 이벤트 선택

4. **커스텀 이벤트 생성**
   - 우클릭 → "Custom Event" 생성
   - 이벤트 이름 지정 (예: "HandleCreateComplete")
   - 이벤트에 매개변수 추가 (bool bWasSuccessful)

5. **이벤트 처리 로직 작성**
   - 커스텀 이벤트에서 성공/실패 분기 처리

**예시 (방 생성 완료):**
```
Event Construct
  → Get BR Game Session (BR Widget Function Library)
    - World Context Object: Self
  → Bind Event to OnCreateSessionComplete
    - Delegate: OnCreateSessionComplete
    - Event: HandleCreateComplete (커스텀 이벤트)

HandleCreateComplete (bool bWasSuccessful)
  → Branch (bWasSuccessful)
    - True: 로비 메뉴로 이동
    - False: 에러 메시지 표시
```

**예시 (방 참가 완료):**
```
Event Construct
  → Get BR Game Session (BR Widget Function Library)
    - World Context Object: Self
  → Bind Event to OnJoinSessionComplete
    - Delegate: OnJoinSessionComplete
    - Event: HandleJoinComplete (커스텀 이벤트)

HandleJoinComplete (bool bWasSuccessful)
  → Branch (bWasSuccessful)
    - True: 로비 메뉴로 이동
    - False: 에러 메시지 표시
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

#### 방법 A: BRWidgetFunctionLibrary 사용 (추천, 상속 변경 불필요) ⭐

**블루프린트 연결 단계:**

1. **블루프린트 그래프 열기**
   - Content Browser에서 `WBP_EntranceMenu1` 더블클릭하여 열기

2. **방 생성 함수 연결**
   ```
   [방 생성 버튼 클릭 이벤트]
   → Create Room (BR Widget Function Library 노드 검색)
     - World Context Object: Self (자동 연결)
     - Room Name: (텍스트 입력 위젯에서 가져온 방 이름)
   ```

3. **방 찾기 함수 연결**
   ```
   [방 찾기 버튼 클릭 이벤트]
   → Find Rooms (BR Widget Function Library 노드 검색)
     - World Context Object: Self (자동 연결)
   ```

4. **이벤트 바인딩 (방 생성 완료 처리)**
   ```
   Event Construct
   → Get BR Game Session (BR Widget Function Library)
     - World Context Object: Self
   → Bind Event to OnCreateSessionComplete
     - Delegate: OnCreateSessionComplete
     - Event: (새 커스텀 이벤트 생성: "HandleCreateComplete")
   
   HandleCreateComplete (bool bWasSuccessful)
   → Branch (bWasSuccessful로 분기)
     - True: 로비 메뉴로 이동 등 성공 처리
     - False: 에러 메시지 표시 등 실패 처리
   ```

#### 방법 B: C++ 위젯 베이스 클래스 상속 (Parent Class 변경 필요)

1. **블루프린트 클래스 상속 변경**
   - Content Browser에서 `WBP_EntranceMenu1` 우클릭 → "Edit" 또는 더블클릭
   - 상단 메뉴: "Class Settings" 클릭
   - Details 패널에서 "Parent Class" 찾기
   - "Parent Class" 드롭다운 클릭 → `BR_EntranceMenuWidget` 선택
   - 컴파일 완료 대기

2. **방 생성 함수 연결**
   ```
   [방 생성 버튼 클릭 이벤트]
   → CreateRoom (Self에서 함수 호출)
     - Room Name: (텍스트 입력 위젯에서 가져온 방 이름)
   ```

3. **방 찾기 함수 연결**
   ```
   [방 찾기 버튼 클릭 이벤트]
   → FindRooms (Self에서 함수 호출)
   ```

4. **이벤트 구현 (Override Functions)**
   - 그래프에서 우클릭 → "Add Event" → "Event" 검색
   - "On Create Room Complete" 이벤트 추가
     ```
     On Create Room Complete (bool bWasSuccessful)
     → Branch (bWasSuccessful로 분기)
       - True: 로비 메뉴로 이동 등 성공 처리
       - False: 에러 메시지 표시 등 실패 처리
     ```
   
   - "On Find Rooms Complete" 이벤트 추가
     ```
     On Find Rooms Complete
     → 방 목록 화면으로 전환 또는 방 목록 UI 업데이트
     ```

**사용 가능한 함수들:**
   - `CreateRoom(FString RoomName)` - 방 생성
   - `FindRooms()` - 방 찾기
   - `GetBRPlayerController()` - PlayerController 가져오기

**블루프린트에서 사용 예시:**
   ```
   - 버튼 클릭 이벤트 → CreateRoom 함수 호출 (방 이름 입력)
   - OnCreateRoomComplete 이벤트에서 성공/실패 처리
   ```

### 2. WBP_JoinMenu1 설정

#### 방법 A: BRWidgetFunctionLibrary 사용 (추천, 상속 변경 불필요) ⭐

**블루프린트 연결 단계:**

1. **블루프린트 그래프 열기**
   - Content Browser에서 `WBP_JoinMenu1` 더블클릭하여 열기

2. **방 참가 함수 연결**
   ```
   [방 선택 버튼 클릭 이벤트]
   → Join Room (BR Widget Function Library 노드 검색)
     - World Context Object: Self (자동 연결)
     - Session Index: (선택한 방의 인덱스, 예: 0, 1, 2...)
   ```

3. **방 목록 새로고침**
   ```
   [새로고침 버튼 클릭 이벤트]
   → Find Rooms (BR Widget Function Library 노드 검색)
     - World Context Object: Self (자동 연결)
   ```

4. **이벤트 바인딩 (방 참가 완료 처리)**
   ```
   Event Construct
   → Get BR Game Session (BR Widget Function Library)
     - World Context Object: Self
   → Bind Event to OnJoinSessionComplete
     - Delegate: OnJoinSessionComplete
     - Event: (새 커스텀 이벤트 생성: "HandleJoinComplete")
   
   HandleJoinComplete (bool bWasSuccessful)
   → Branch (bWasSuccessful로 분기)
     - True: 로비 메뉴로 이동 등 성공 처리
     - False: 에러 메시지 표시 등 실패 처리
   ```

#### 방법 B: C++ 위젯 베이스 클래스 상속 (Parent Class 변경 필요)

1. **블루프린트 클래스 상속 변경**
   - Content Browser에서 `WBP_JoinMenu1` 우클릭 → "Edit" 또는 더블클릭
   - 상단 메뉴: "Class Settings" 클릭
   - Details 패널에서 "Parent Class" 찾기
   - "Parent Class" 드롭다운 클릭 → `BR_JoinMenuWidget` 선택
   - 컴파일 완료 대기

2. **방 참가 함수 연결**
   ```
   [방 선택 버튼 클릭 이벤트]
   → JoinRoom (Self에서 함수 호출)
     - Session Index: (선택한 방의 인덱스)
   ```

3. **방 목록 새로고침**
   ```
   [새로고침 버튼 클릭 이벤트]
   → RefreshRoomList (Self에서 함수 호출)
   ```

4. **이벤트 구현 (Override Functions)**
   - 그래프에서 우클릭 → "Add Event" → "Event" 검색
   - "On Join Room Complete" 이벤트 추가
     ```
     On Join Room Complete (bool bWasSuccessful)
     → Branch (bWasSuccessful로 분기)
       - True: 로비 메뉴로 이동 등 성공 처리
       - False: 에러 메시지 표시 등 실패 처리
     ```
   
   - "On Find Rooms Complete" 이벤트 추가
     ```
     On Find Rooms Complete
     → 방 목록 UI 업데이트 (예: 리스트뷰 새로고침)
     ```

**사용 가능한 함수들:**
   - `JoinRoom(int32 SessionIndex)` - 방 참가 (세션 인덱스 전달)
   - `RefreshRoomList()` - 방 목록 새로고침
   - `GetBRPlayerController()` - PlayerController 가져오기
   - `GetBRGameSession()` - GameSession 가져오기

**블루프린트에서 사용 예시:**
   ```
   - 방 목록 표시 후, 특정 방 선택 → JoinRoom 함수 호출
   - 새로고침 버튼 → RefreshRoomList 함수 호출
   - OnJoinRoomComplete 이벤트에서 성공 시 로비로 이동
   ```

#### WBP_SessionList 더블클릭으로 자동 참가 구현

**목표:** `WBP_SessionList` 위젯을 더블클릭하면 자동으로 해당 세션에 참가하도록 구현

**⚠️ 중요: SessionIndex 처리 방법**

**Q: 리스트 UI를 누르면 자동으로 인덱스 값이 지정되는가?**
**A: 아니요. SessionIndex는 수동으로 지정해야 합니다.**

**SessionIndex란?**
- GameSession의 `SearchResults` 배열에서 해당 세션이 몇 번째인지를 나타내는 인덱스입니다 (0, 1, 2, ...)
- 예: 검색 결과가 3개라면, 인덱스는 0, 1, 2입니다
- 이 인덱스는 `JoinSessionByIndex(int32 SessionIndex)` 함수에서 사용됩니다

**SessionIndex 지정 방법:**

1. **WBP_JoinMenu1에서 세션 목록 생성 시 지정**
   - 방 찾기가 완료되면 세션 목록을 생성합니다
   - **For Loop을 사용**하여 각 세션마다 `WBP_SessionList` 아이템을 생성합니다
   - **For Loop의 Index가 바로 SessionIndex입니다** (0부터 시작)
   - 각 아이템 생성 시 해당 인덱스를 전달합니다

2. **블루프린트에서 세션 목록 가져오기**
   - 현재는 C++에서만 세션 목록에 직접 접근할 수 있습니다
   - 블루프린트에서는 **GameSession의 이벤트**를 통해 세션 개수를 확인하고 처리해야 합니다
   - 또는 세션 정보를 별도 구조체로 만들어 블루프린트에서 사용할 수 있도록 해야 합니다

**구현 단계:**

1. **WBP_SessionList 위젯에 세션 인덱스 변수 추가**
   - 블루프린트 에디터에서 `WBP_SessionList` 열기
   - "My Blueprint" 패널 → "Variables" 섹션
   - "+" 버튼 클릭 → 변수 추가
   - 변수 이름: `SessionIndex`
   - 변수 타입: `Integer` (int32)
   - "Instance Editable" 체크 (부모 위젯에서 설정 가능하도록)

2. **더블클릭 이벤트 추가**
   - 그래프에서 우클릭 → "Add Event"
   - 위젯 타입 선택 (예: Button이면 "On Button Double-Clicked", 다른 위젯이면 "On Mouse Button Double Click" 사용)
   - 또는 위젯의 "Event Graph"에서 "Event On Mouse Button Double Click" 추가

3. **더블클릭 시 Join Room 호출**
   ```
   [On Mouse Button Double Click 또는 On Button Double-Clicked]
   → Join Room (BR Widget Function Library)
     - World Context Object: Self
     - Session Index: SessionIndex (변수 가져오기)
   ```

4. **WBP_JoinMenu1에서 세션 리스트 아이템 생성 시 인덱스 설정**
   
   **⚠️ 현재 제한사항:**
   - `OnFindSessionsComplete` 델리게이트는 C++ 전용입니다
   - 블루프린트에서 직접 세션 목록을 가져올 수 없습니다
   
   **해결 방법 A: GameSession 이벤트 사용 (임시 방법)**
   ```
   Event Construct
   → Get BR Game Session
   → (세션 목록은 C++에서 관리되므로 블루프린트에서 직접 가져올 수 없음)
   → 대안: 리스트뷰 아이템 인덱스를 사용 (단, 세션 인덱스와 일치해야 함)
   ```
   
   **해결 방법 B: 간단한 방법 - 수동 인덱스 지정 (추천)**
   
   세션 목록 생성 시 **For Loop Index를 SessionIndex로 사용**:
   ```
   [방 찾기 완료 후 처리]
   
   방법 1: 리스트뷰 사용 시
   → Clear List Items (리스트뷰)
   → Get BR Game Session
   → (세션 개수는 미리 알고 있어야 함 - 예: 변수로 저장)
   → For Loop (0부터 세션 개수-1까지)
     → Create Widget (WBP_SessionList)
     → Set SessionIndex
       - SessionIndex: Loop Index (0, 1, 2, ...)
     → Add Item to List View
   ```
   
   **⚠️ 주의:**
   - 세션 목록을 생성할 때 **세션의 순서가 변경되지 않아야** 합니다
   - 리스트뷰의 아이템 인덱스와 세션 인덱스가 일치해야 합니다
   - 세션 목록이 업데이트되면 기존 아이템들을 모두 제거하고 새로 생성해야 합니다

**전체 연결 예시:**

**WBP_JoinMenu1에서:**
```
Event Construct
  → Get BR Game Session
  → Bind Event to OnFindSessionsComplete
    - Event: HandleFindRoomsComplete

HandleFindRoomsComplete
  → Clear Children (리스트뷰 또는 패널)
  → Get BR Game Session
  → (세션 목록 가져오기 - C++에서 구현 필요)
  → For Each Loop
    → Create Widget (WBP_SessionList)
    → Set SessionIndex
      - SessionIndex: Loop Index
    → Add Child to List View (또는 Panel)
```

**WBP_SessionList에서:**
```
[On Mouse Button Double Click 또는 위젯 더블클릭 이벤트]
  → Join Room (BR Widget Function Library)
    - World Context Object: Self
    - Session Index: SessionIndex
```

**세션 목록 생성 예시 (블루프린트):**

**⚠️ 문제점:**
현재 블루프린트에서 세션 목록을 직접 가져올 수 없으므로, 다음 중 하나의 방법을 사용해야 합니다:

**방법 1: 리스트뷰 아이템 인덱스 사용 (간단)**
```
[새로고침 버튼 클릭]
  → Find Rooms (BR Widget Function Library)
  
[세션 목록 업데이트 - C++에서 이벤트로 알림을 받아야 함]
  → Clear List Items (리스트뷰)
  → For Loop (0부터 예상 세션 개수까지)
    → Create Widget (WBP_SessionList)
    → Set SessionIndex
      - SessionIndex: Loop Index
    → Add Item to List View
```

**방법 2: C++ 수정 필요 (권장)**
블루프린트에서 세션 목록을 사용하려면 다음 중 하나를 구현해야 합니다:
1. `BRGameSession`에 블루프린트 호출 가능한 `GetSessionCount()` 함수 추가
2. `OnFindSessionsComplete`를 블루프린트 호출 가능한 델리게이트로 변경
3. 세션 정보를 구조체로 만들어 블루프린트로 전달

**방법 3: 임시 해결책 (간단한 테스트용)**
- 세션 개수를 변수로 저장하고 관리
- 리스트뷰 아이템 인덱스 = 세션 인덱스로 가정
- 리스트뷰의 `On Item Double Clicked` 이벤트 사용

**리스트뷰 아이템 더블클릭 사용 예시:**
```
[List View - On Item Double Clicked 이벤트]
  → Get Selected Index (리스트뷰)
  → Join Room (BR Widget Function Library)
    - Session Index: Selected Index
```

### 3. WBP_LobbyMenu1 설정

#### 방법 A: BRWidgetFunctionLibrary 사용 (추천, 상속 변경 불필요) ⭐

**블루프린트 연결 단계:**

1. **블루프린트 그래프 열기**
   - Content Browser에서 `WBP_LobbyMenu1` 더블클릭하여 열기

2. **준비 상태 토글**
   ```
   [준비 버튼 클릭 이벤트]
   → Toggle Ready (BR Widget Function Library 노드 검색)
     - World Context Object: Self (자동 연결)
   ```

3. **방장 전용 기능들**
   ```
   [랜덤 팀 배정 버튼 클릭 이벤트]
   → Random Teams (BR Widget Function Library)
     - World Context Object: Self
   
   [팀 변경 버튼 클릭 이벤트]
   → Change Team (BR Widget Function Library)
     - World Context Object: Self
     - Player Index: (변경할 플레이어 인덱스)
     - Team Number: (변경할 팀 번호)
   
   [게임 시작 버튼 클릭 이벤트]
   → Start Game (BR Widget Function Library)
     - World Context Object: Self
   ```

4. **상태 확인 함수들**
   ```
   Event Tick 또는 필요할 때
   → Is Host (BR Widget Function Library)
     - World Context Object: Self
   → Branch (방장 여부에 따라 UI 표시/숨김)
   
   → Is Ready (BR Widget Function Library)
     - World Context Object: Self
   → 준비 상태 UI 업데이트
   ```

5. **GameState 이벤트 바인딩 (플레이어 목록/팀 변경 감지)**
   ```
   Event Construct
   → Get BR Game State (BR Widget Function Library)
     - World Context Object: Self
   → Bind Event to OnPlayerListChanged
     - Delegate: OnPlayerListChanged
     - Event: (커스텀 이벤트: "HandlePlayerListChanged")
   
   HandlePlayerListChanged
   → 플레이어 목록 UI 업데이트
     (Get BR Game State → GetAllPlayerUserInfo 사용)
   
   → Bind Event to OnTeamChanged
     - Delegate: OnTeamChanged
     - Event: (커스텀 이벤트: "HandleTeamChanged")
   
   HandleTeamChanged
   → 팀 정보 UI 업데이트
   ```

#### 방법 B: C++ 위젯 베이스 클래스 상속 (Parent Class 변경 필요)

1. **블루프린트 클래스 상속 변경**
   - Content Browser에서 `WBP_LobbyMenu1` 우클릭 → "Edit" 또는 더블클릭
   - 상단 메뉴: "Class Settings" 클릭
   - Details 패널에서 "Parent Class" 찾기
   - "Parent Class" 드롭다운 클릭 → `BR_LobbyMenuWidget` 선택
   - 컴파일 완료 대기

2. **준비 상태 토글**
   ```
   [준비 버튼 클릭 이벤트]
   → ToggleReady (Self에서 함수 호출)
   ```

3. **방장 전용 기능들**
   ```
   [랜덤 팀 배정 버튼 클릭 이벤트]
   → RequestRandomTeams (Self에서 함수 호출)
   
   [팀 변경 버튼 클릭 이벤트]
   → ChangePlayerTeam (Self에서 함수 호출)
     - Player Index: (변경할 플레이어 인덱스)
     - Team Number: (변경할 팀 번호)
   
   [게임 시작 버튼 클릭 이벤트]
   → RequestStartGame (Self에서 함수 호출)
   ```

4. **방장/준비 상태 확인**
   ```
   Event Tick 또는 UI 업데이트 필요할 때
   → IsHost (Self에서 함수 호출)
   → Branch (방장 여부에 따라 UI 표시/숨김)
   
   → IsReady (Self에서 함수 호출)
   → 준비 상태 UI 업데이트
   ```

5. **이벤트 구현 (Override Functions)**
   - "On Player List Changed" 이벤트 추가
     ```
     On Player List Changed
     → Get BR Game State
     → GetAllPlayerUserInfo
     → 플레이어 목록 UI 업데이트
     ```
   
   - "On Team Changed" 이벤트 추가
     ```
     On Team Changed
     → 팀 정보 UI 업데이트
     ```
   
   - "On Can Start Game Changed" 이벤트 추가
     ```
     On Can Start Game Changed (bool bCanStart)
     → 게임 시작 버튼 활성화/비활성화 처리
     ```

**사용 가능한 함수들:**
   - `ToggleReady()` - 준비 상태 토글
   - `RequestRandomTeams()` - 랜덤 팀 배정 (방장만)
   - `ChangePlayerTeam(int32 PlayerIndex, int32 TeamNumber)` - 플레이어 팀 변경 (방장만)
   - `RequestStartGame()` - 게임 시작 (방장만)
   - `GetBRPlayerController()` - PlayerController 가져오기
   - `GetBRGameState()` - GameState 가져오기
   - `GetBRPlayerState()` - PlayerState 가져오기
   - `IsHost()` - 방장 여부 확인
   - `IsReady()` - 준비 상태 확인

**블루프린트에서 사용 예시:**
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

### ❗ "EntranceMenuWidgetClass가 설정되지 않았습니다" 경고 해결

**문제:**
```
LogTemp: Warning: [PlayerController] EntranceMenuWidgetClass가 설정되지 않았습니다. 블루프린트에서 설정해주세요.
```

**원인:**
- `BRPlayerController`의 위젯 클래스 변수가 블루프린트 기본 설정에 할당되지 않았습니다.

**해결 방법:**

1. **블루프린트 PlayerController 클래스 찾기**
   - Content Browser에서 `BP_PlayerController` 또는 `BR_PlayerController`를 상속받은 블루프린트 클래스를 찾습니다
   - 없으면 새로 생성합니다:
     - Content Browser에서 우클릭 → "Blueprint Class" 선택
     - 부모 클래스: `BR_PlayerController` 검색 후 선택
     - 이름: `BP_BR_PlayerController` (또는 원하는 이름)

2. **위젯 클래스 설정**
   - 블루프린트 PlayerController 더블클릭하여 열기
   - Details 패널에서 "UI" 카테고리 찾기
   - 다음 변수들을 설정:
     - **Entrance Menu Widget Class**: `WBP_EntranceMenu` 선택
     - **Join Menu Widget Class**: `WBP_JoinMenu` 선택
     - **Lobby Menu Widget Class**: `WBP_LobbyMenu` 선택

3. **GameMode에 PlayerController 설정**
   - GameMode 블루프린트를 열기 (또는 `BRGameMode` 블루프린트)
   - Details 패널에서 "Player Controller Class" 찾기
   - 위에서 만든 `BP_BR_PlayerController` 선택

4. **World Settings 확인**
   - 레벨 편집기에서 상단 "Edit" → "World Settings"
   - "Game Mode Override"에서 올바른 GameMode가 선택되어 있는지 확인

**참고:**
- `EditDefaultsOnly`는 블루프린트 클래스의 **기본 설정(Defaults)**에서만 설정 가능합니다
- 게임 중에 동적으로 변경할 수 없습니다
- 각 블루프린트 인스턴스마다 설정할 필요 없이 클래스 단위로 설정하면 됩니다

### ❗ "방 생성 함수가 실행되지 않음" 문제 해결

**증상:**
- 버튼 클릭은 정상 작동 (UI 애니메이션 등)
- 하지만 로그가 나타나지 않음
- 함수가 실행되지 않는 것 같음

**원인 1: World Context Object가 연결되지 않음**

**해결:**
```
Create Room 노드의 "World Context Object" 핀 확인
→ 비어있으면 "Self" 노드를 드래그하여 연결
→ 또는 그래프에서 우클릭 → "Self" 검색 → 드래그하여 연결
```

**원인 2: Room Name 파라미터가 연결되지 않음**

**해결:**
```
방법 1: 직접 문자열 입력
→ Create Room 노드의 "Room Name" 핀에 문자열 노드 연결
→ 그래프에서 우클릭 → "Make Literal String" 또는 "Text" 검색
→ "TestRoom" 등 원하는 이름 입력

방법 2: 텍스트 입력 위젯에서 가져오기
→ Text Input Widget (또는 Editable Text) 선택
→ "Get Text" 함수 호출
→ 결과를 "Room Name" 핀에 연결
```

**원인 3: 이벤트가 제대로 연결되지 않음**

**확인:**
- 버튼의 "On Clicked" 또는 "On Released" 이벤트가 제대로 연결되어 있는지 확인
- 함수 노드의 "execute" 핀이 이벤트의 "then" 핀과 연결되어 있는지 확인

**디버깅 체크리스트:**

1. ✅ 버튼 이벤트가 발생하는가? (UI 애니메이션 확인)
2. ✅ Create Room 함수 노드의 "execute" 핀이 이벤트에 연결되어 있는가?
3. ✅ "World Context Object" 핀에 "Self"가 연결되어 있는가?
4. ✅ "Room Name" 핀에 값이 연결되어 있는가?
5. ✅ 함수 노드 다음에 다른 노드가 연결되어 있으면, 그 노드도 실행되는가?

**예상되는 로그 메시지 (정상 실행 시):**
```
LogTemp: Log: [WidgetFunctionLibrary] 방 생성 요청: TestRoom
LogTemp: Log: [방 생성] 명령 실행: TestRoom
LogTemp: Log: [방 생성] 세션 생성 시작: TestRoom
```

**에러 로그 (문제가 있을 때):**
```
LogTemp: Error: [WidgetFunctionLibrary] PlayerController를 찾을 수 없습니다.
→ World Context Object가 제대로 연결되지 않았거나, PlayerController가 아직 생성되지 않음
```
