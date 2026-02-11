---
title: "Backward Royal: Technical Architecture Document"
author: "Project Team"
date: "2026-02-11"
version: "1.0"
---

# Backward Royal: 기술 아키텍처 명세서

## 목차 (Table of Contents)

1. [개요 (Overview)](#1-개요-overview)
2. [시스템 구조 (System Architecture)](#2-시스템-구조-system-architecture)
3. [핵심 컴포넌트 (Core Components)](#3-핵심-컴포넌트-core-components)
    - [3.1 Game Instance](#31-game-instance-ubrgameinstance)
    - [3.2 Game Mode](#32-game-mode-abrgamemode)
    - [3.3 Game State](#33-game-state-abrgamestate)
    - [3.4 Player Controller & State](#34-player-controller--state)
4. [캐릭터 시스템 (Character System)](#4-캐릭터-시스템-character-system)
    - [4.1 하체 (Lower Body)](#41-하체-lower-body-aplayercharacter)
    - [4.2 상체 (Upper Body)](#42-상체-upper-body-aupperbodypawn)
5. [데이터 및 UI (Data & UI)](#5-데이터-및-ui-data--ui)
6. [실행 흐름 (Execution Flow)](#6-실행-흐름-execution-flow)

---

## 1. 개요 (Overview)

**Backward Royal**은 언리얼 엔진 기반의 멀티플레이어 액션 게임으로, **"2인 1체(Two Players, One Body)"**라는 독창적인 협동 메커니즘을 핵심으로 합니다. 본 문서는 프로젝트의 기술적 구조와 주요 클래스 간의 상호작용을 설명하여 외부 공유 및 기술 검토를 목적으로 작성되었습니다.

*   **장르**: 협동 액션 / 배틀 로얄
*   **플랫폼**: PC (Steam)
*   **엔진**: Unreal Engine 5 (C++)

---

## 2. 시스템 구조 (System Architecture)

프로젝트는 언리얼 엔진의 표준 **Server-Client** 아키텍처를 준수하며, 특수한 게임플레이(2인 1체)를 구현하기 위해 **Split-Pawn** 구조를 채택했습니다.

*   **디렉터리 구조**
    *   `Source/Backward_Royal/`: 핵심 게임 로직 (C++)
    *   `Content/`: 블루프린트, UI, 메시 등 에셋 리소스
    *   `Config/`: 게임 설정 및 초기화 파일

---

## 3. 핵심 컴포넌트 (Core Components)

### 3.1 Game Instance (`UBRGameInstance`)
게임의 수명 주기 동안 유지되는 **영구 데이터 관리자**입니다.
*   **세션 관리**: 방 생성, 검색, 참가 기능을 총괄합니다.
*   **데이터 지속성 (Persistence)**: `FTravelUserInfoSave` 구조체를 통해 레벨 이동(Seamless Travel) 시에도 플레이어의 상태(팀, 준비 여부, 커스터마이징)를 보존 및 복원합니다.
*   **네트워크 모드**: `bUseLANOnly` 플래그를 통해 LAN 및 인터넷(Steam) 환경을 동적으로 전환합니다.

### 3.2 Game Mode (`ABRGameMode`)
서버 측에서 게임의 규칙과 흐름을 제어하는 **권한(Authority)** 클래스입니다.
*   **팀 배정 로직**: 접속한 플레이어들을 팀 단위로 묶고, 각 팀 내에서 **상체(Upper)**와 **하체(Lower)** 역할을 분배합니다.
*   **단계적 스폰 (Staged Spawning)**:
    1.  하체 캐릭터(`APlayerCharacter`)를 먼저 스폰합니다.
    2.  소유권이 있는 상체 폰(`AUpperBodyPawn`)을 스폰하여 하체에 부착(Attach)합니다.
    3.  이 과정이 팀별로 순차적으로 이루어지도록 스케줄링하여 초기화 충돌을 방지합니다.

### 3.3 Game State (`ABRGameState`)
모든 클라이언트에 동기화되는 **게임의 현재 상태**를 관리합니다.
*   **로비 상태 동기화**: 대기열(`LobbyEntrySlots`)과 팀 슬롯(`LobbyTeamSlots`) 정보를 관리하여, 모든 클라이언트가 동일한 로비 화면을 볼 수 있게 합니다.
*   **UI 데이터**: `PlayerListForDisplay`를 통해 접속자 목록 갱신을 효율적으로 처리합니다.

### 3.4 Player Controller & State
*   **Player Controller (`ABRPlayerController`)**: 사용자의 입력 처리를 담당합니다. 역할(상체/하체)이 변경될 때 `EnhanceInput` 시스템의 **Mapping Context**를 동적으로 교체하여 조작계를 변경합니다.
*   **Player State (`ABRPlayerState`)**: 플레이어의 고유 정보(UID, 팀 번호, 생존 여부)를 저장하며, 파트너(`ConnectedPlayerIndex`) 정보를 보유하여 상호 참조를 가능하게 합니다.

---

## 4. 캐릭터 시스템 (Character System)

본 프로젝트의 가장 독특한 기술적 특징은 하나의 유닛을 두 개의 폰(Pawn)이 제어하는 것입니다.

### 4.1 하체 (Lower Body: `APlayerCharacter`)
*   **역할**: 이동 및 생존 (Driver)
*   **기능**:
    *   `ACharacter`를 상속받아 물리적 충돌과 이동(Navigation)을 담당합니다.
    *   WASD 이동, 점프, 전력 질주(Stamina 소모)를 처리합니다.
    *   상체 폰이 부착될 `HeadMountPoint`를 제공합니다.
    *   카메라는 3인칭 숄더 뷰(Back View)를 사용합니다.

### 4.2 상체 (Upper Body: `AUpperBodyPawn`)
*   **역할**: 공격 및 상호작용 (Gunner)
*   **기능**:
    *   `APawn`을 상속받으며, 하체 캐릭터에 부착(Attach)되어 함께 이동합니다.
    *   마우스 회전에 따라 시점을 제어하고 조준(Aim) 봅니다.
    *   공격(Attack) 및 상호작용(Interact) 입력을 처리합니다.
    *   별도의 카메라(Front/Combat View)를 가집니다.

---

## 5. 데이터 및 UI (Data & UI)

*   **GlobalBalanceData**: 게임플레이 밸런스(공격력, 이동 속도, 스태미나 소모량 등)를 JSON 데이터로 관리하여, 컴파일 없이 기획자가 수치를 조정할 수 있는 구조입니다.
*   **BRWidgetFunctionLibrary**: UI 개발 생산성을 높이기 위해 복잡한 게임 로직(GameState 접근, 슬롯 파싱 등)을 캡슐화한 정적 라이브러리입니다.

---

## 6. 실행 흐름 (Execution Flow)

1.  **초기화 (Entrance)**: 클라이언트 실행 시 `BRPlayerController`가 초기 메뉴를 로드합니다.
2.  **세션 연결 (Lobby)**: 호스트가 방을 생성하면 `Seamless Travel`을 통해 로비 맵으로 이동합니다.
3.  **팀 구성 (Setup)**: 로비에서 플레이어들이 슬롯에 배치되면 `GameState`가 이를 동기화합니다.
4.  **게임 시작 (Game Start)**: 호스트가 시작을 요청하면 전체 플레이어가 게임플레이 맵으로 이동합니다.
5.  **동기화 스폰 (Spawning)**: 서버는 각 팀의 하체를 먼저 생성한 뒤, 상체를 생성하여 부착(Attach)하고 컨트롤러를 빙의(Possess)시킵니다.
6.  **게임플레이 (Loop)**: 두 플레이어의 입력이 하나의 통합된 캐릭터 동작으로 발현되며, 최후의 1팀이 남을 때까지 진행됩니다.

---

## 7. 상세 함수 분석 (Detailed Function Analysis)

본 섹션에서는 프로젝트의 핵심 흐름을 담당하는 주요 클래스와 함수들의 내부 로직 및 호출 관계를 상세히 분석합니다.

### 7.1 Game Instance (`UBRGameInstance`)

`UBRGameInstance`는 애플리케이션 수명 주기 관리 및 글로벌 데이터(세션, 설정)의 지속성을 담당합니다.

*   **`Init()`**
    *   **역할**: 게임 인스턴스 초기화, 네트워크 모드 로깅, 플레이어 이름 로드.
    *   **로직**:
        1.  `LoadPlayerNameFromUserInfo()`: 로컬 저장소에서 사용자 이름 불러오기.
        2.  `ReloadAllConfigs()`: `GlobalBalanceData` 등 JSON 기반 설정 파일 로드.
        3.  `FWorldDelegates::OnWorldCleanup`: PIE 종료 시 리소스 정리 델리게이트 등록.

*   **`OnStart()`**
    *   **역할**: 레벨 로드 직후 실행되며, 네트워크 모드 전환 및 세션 복구를 처리합니다.
    *   **로직**:
        1.  **Standalone → ListenServer 전환**: `PendingRoomName`이 설정되어 있고 현재 모드가 Standalone이면, `open MapName?listen` 명령을 실행하여 리슨 서버로 재실행.
        2.  **세션 자동 생성**: ListenServer로 재실행된 후, `PendingRoomName`을 기반으로 `BRGameSession`을 통해 세션을 생성(`CreateRoomSession`).

### 7.2 Game Mode (`ABRGameMode`)

서버 측 게임플레이 규칙과 흐름(Flow)을 제어합니다.

*   **`ApplyRoleChangesForRandomTeams()`**
    *   **역할**: 팀 배정 후 실제 캐릭터(Part) 스폰 및 역할 적용을 총괄하는 오케스트레이터 함수.
    *   **로직**:
        1.  **복원 (Restore)**: `GI->RestorePendingRolesFromTravel`을 호출하여 Seamless Travel 이후 `PlayerState` 정보를 복구.
        2.  **안전성 검사**: 팀 인원수 및 배정 상태 확인.
        3.  **순차 실행**: `ApplyRoleChangesForRandomTeams_ApplyOneTeam()`을 최초 호출하여 체인(Chain) 형태의 스폰 로직 시작.

*   **`ApplyRoleChangesForRandomTeams_ApplyOneTeam()`**
    *   **역할**: 한 팀(2명)의 상체/하체 스폰 및 부착 과정을 처리하고, 다음 팀 처리를 예약하는 재귀적 함수.
    *   **로직**:
        1.  **컨트롤러 대기**: 하체/상체 플레이어의 `Controller`가 모두 유효해질 때까지 타이머로 대기.
        2.  **스폰 및 부착**:
            *   하체: 이미 맵에 스폰된 `APlayerCharacter`를 사용하거나 새로 스폰.
            *   상체: `AUpperBodyPawn`을 스폰하고 하체의 `HeadMountPoint`에 `AttachToComponent`로 부착.
        3.  **빙의 (Possess)**: 각 플레이어 컨트롤러를 해당 Pawn에 빙의시킴.
        4.  **다음 팀 예약**: 작업 완료 후 `SetTimer`를 통해 다음 팀 인덱스에 대해 자기 자신을 호출 (재귀).

### 7.3 Game State (`ABRGameState`)

클라이언트와 동기화되는 게임 상태 및 로비 정보를 관리합니다.

*   **`UpdatePlayerList()`**
    *   **역할**: 접속한 플레이어 목록을 갱신하고 로비 UI 슬롯을 재구성합니다.
    *   **로직**:
        1.  `PlayerArray`를 순회하며 현재 접속자 확인.
        2.  **로비 슬롯 관리**: `LobbyTeamSlots`(팀 배정 상태)와 `LobbyEntrySlots`(대기 상태) 배열을 최신화.
        3.  `OnPlayerListChanged.Broadcast()`: 델리게이트를 통해 위젯(UI)에 변경 사항 알림.

*   **`AssignRandomTeams()`**
    *   **역할**: 대기 중인 플레이어들을 무작위로 섞어 2인 1조 팀을 생성하고 역할을 배분합니다.
    *   **로직**:
        1.  **셔플 (Shuffle)**: `PlayerArray`를 무작위로 섞음.
        2.  **팀 배정**: 2명씩 묶어 `TeamNumber` 할당.
        3.  **역할 배정**: 팀 내 첫 번째는 하체(`bIsLowerBody = true`), 두 번째는 상체(`false`)로 설정.
        4.  **준비 완료**: 모든 플레이어의 `bIsReady`를 true로 설정하여 게임 시작 조건 충족.

### 7.4 Game Session (`BRGameSession`)

Steam Online Subsystem과의 연동 및 세션 수명 주기를 관리합니다.

*   **`CreateRoomSession(RoomName)`**
    *   **역할**: 새로운 온라인 세션을 생성합니다.
    *   **로직**:
        1.  **기존 세션 정리**: 이미 세션이 있다면 `DestroySession` 호출.
        2.  **세션 설정**: `FOnlineSessionSettings` 구성 (최대 인원, 공개 여부, LAN/Steam 모드 자동 감지).
        3.  `SessionInterface->CreateSession()`: 실제 세션 생성 요청.
        4.  **콜백 처리**: 생성 성공 시 `OnCreateSessionCompleteDelegate`에서 `StartSession` 및 맵 이동(`ServerTravel`) 수행.

*   **`FindSessions()`**
    *   **역할**: 참여 가능한 세션을 검색합니다.
    *   **로직**:
        1.  `FOnlineSessionSearch` 객체 생성 및 설정.
        2.  `SessionInterface->FindSessions()` 호출.
        3.  **재시도 로직**: 검색 결과가 0건이고 Steam OSS 초기화 지연이 의심될 경우, 최대 2회까지 자동 재검색 수행.

### 7.5 Player Controller (`ABRPlayerController`)

입력 처리 및 서버와의 통신(RPC)을 담당합니다.

*   **`JoinRoom(SessionIndex)`**
    *   **역할**: 검색된 세션에 참가를 요청합니다.
    *   **로직**:
        1.  `BRGameSession->JoinSessionByIndex(SessionIndex)` 호출.
        2.  성공 시 `ClientTravel`을 통해 해당 서버의 IP/URL로 이동. (서버 RPC가 아닌 클라이언트 로직으로 처리됨이 중요)

*   **`ToggleReady()`**
    *   **역할**: 로비에서 플레이어의 준비 상태를 변경합니다.
    *   **로직**:
        1.  **서버**: `PlayerState`의 `bIsReady`를 토글하고 `CheckCanStartGame()`으로 시작 가능 여부 확인.
        2.  **클라이언트**: `ServerToggleReady()` RPC를 호출하여 서버에 요청 보냄.

### 7.6 Player Character (`APlayerCharacter`) - Lower Body

이동, 체력, 애니메이션 동기화를 담당하는 하체 캐릭터입니다.

*   **`RequestAttack()`**
    *   **역할**: 공격 입력을 받아 적절한 애니메이션 몽타주를 재생합니다.
    *   **로직**:
        1.  **무기 확인**: `CurrentWeapon` 유무에 따라 무기 공격 vs 맨손 공격 분기.
        2.  **몽타주 선택**: 무기 타입(한손/양손)에 맞는 몽타주 선택.
        3.  `MulticastPlayWeaponAttack()`: 서버에서 모든 클라이언트로 몽타주 재생 명령 전송.

*   **`Die(Impulse, HitLocation)`**
    *   **역할**: 캐릭터 사망 처리.
    *   **로직**:
        1.  **상태 변경**: `PlayerState` 상태를 Dead로 변경.
        2.  **이동 정지**: `CharacterMovementComponent` 비활성화.
        3.  `PerformDeathVisuals()`: 캡슐 콜리전 해제 및 메시의 `Simulate Physics` 활성화 (Ragdoll 전환). 사망 타격 방향(`Impulse`)으로 힘을 가해 넘어지는 연출 수행.

### 7.7 Upper Body Pawn (`AUpperBodyPawn`)

상체 전용 로직(회전, 상호작용)을 처리합니다.

*   **`Tick()` (Rotation Logic)**
    *   **역할**: 플레이어의 마우스 입력(Control Rotation)을 상체의 회전으로 변환합니다.
    *   **로직**:
        1.  **Attach Parent 확인**: 하체 캐릭터(`ParentBodyCharacter`) 확인.
        2.  **각도 제한 (Clamping)**: 하체 전방 기준 좌우 90도, 상하 90도로 카메라 회전 각도 제한.
        3.  **동기화**: 계산된 회전값을 하체 캐릭터의 `SetUpperBodyRotation`으로 전달하여, 하체 메시의 허리 본(Bone)이 실제로 회전하도록 유도.

*   **`Interact()`**
    *   **역할**: 주변 상호작용 가능 물체 탐색 및 실행.
    *   **로직**:
        1.  **탐색**: 카메라 위치 기준 구체 검사(`OverlapMultiByChannel`) 수행.
        2.  **필터링**: `UInteractableInterface`를 구현한 액터 중 가장 가까운 대상 선정.
        3.  **요청**: 서버로 `ServerRequestInteract` RPC 전송 → 서버에서 아이템 획득 등의 실제 로직 수행.

### 7.8 Attack Component (`BRAttackComponent`)

전투 판정 로직을 중앙에서 관리하는 컴포넌트입니다.

*   **`ProcessHitDamage()`**
    *   **역할**: 충돌(Hit) 발생 시 데미지와 물리력을 계산하고 적용합니다.
    *   **로직**:
        1.  **데미지 계산**: 충격량(`NormalImpulse`) × 무기 계수 × 글로벌 밸런스 데이터.
        2.  **유효타 검증**: 일정 수치 이상의 데미지만 유효타로 인정.
        3.  `ApplyDamage()`: 언리얼 데미지 프레임워크 호출.
        4.  `ApplyHitStop()`: 타격감 강화를 위해 순간적 시간 지연(Time Dilation) 적용.
        5.  **물리 적용**: 타격 방향으로 적 캐릭터/물체에 `AddImpulse` 적용.
