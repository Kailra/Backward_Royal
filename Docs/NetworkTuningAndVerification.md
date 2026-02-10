# 네트워크 튜닝 및 검증 가이드

네트워크 지연·패킷 유실 환경에서 부드러운 플레이와 서버 안정성을 위한 설정·검증 방법입니다.

---

## 0. 서버 누수·지연 확인 방법 (요약)

로그 파일만으로는 **Ping/패킷 유실/메모리 수치**가 찍히지 않습니다. 아래처럼 **실행 중 콘솔 명령**과 **장시간 구동 관찰**로 확인해야 합니다.

### 지연(Latency) 확인

| 단계 | 방법 |
|------|------|
| 1 | **서버(방장)** 또는 **클라이언트**에서 게임 실행 중 **`~`(틸드) 키**로 콘솔 열기 |
| 2 | **`stat net`** 입력 → 화면에 네트워크 통계 표시 |
| 3 | 확인할 값: **Ping**(ms), **InLost** / **OutLost**(유실 패킷), **InPackets** / **OutPackets** |
| 4 | (선택) **`net pktlag 100`** 입력 후 플레이 → 100ms 지연 환경 체감·튐 현상 확인 |
| 5 | **`stat net`** 한 번 더 입력하면 통계 숨김(토글) |

- **Ping**이 계속 100ms 이상이면 고지연 환경. **InLost/OutLost**가 계속 늘어나면 패킷 유실 있음.

### 서버 메모리 누수 확인

| 단계 | 방법 |
|------|------|
| 1 | **서버(리슨 또는 전용)** 실행 후 콘솔에서 **`stat memory`** 입력 → 현재 메모리 사용량 확인 |
| 2 | **30분~1시간** 플레이(접속/퇴장, 맵 이동, 게임 진행 반복) |
| 3 | 다시 **`stat memory`** 입력 후 **처음 값과 비교** |
| 4 | **Used Physical** / **Peak Used Physical** 등이 **계속 크게 증가**하면 누수 의심 |

- PIE(에디터 플레이) 종료 시 나오는 **NavigationSystem GC 경고**는 엔진 알려진 이슈이며, **BRGameMode::EndPlay**에서 타이머는 이미 정리해 두었음.

### 서버 CPU / 틱 부하 확인

- 콘솔: **`stat game`**, **`stat unit`** → 프레임 시간·게임 스레드 부하 확인.
- 서버가 느려지면 **Game thread** 수치가 높게 나옴.

### 정리

- **지연**: 플레이 중 **`stat net`**으로 Ping·InLost·OutLost 확인.
- **누수**: **`stat memory`**를 세션 시작 시와 30분~1시간 후에 비교.
- **로그**: 로그 파일에는 위 수치가 기록되지 않으므로, 문제 재현 시 **콘솔 수치를 수동으로 메모**하거나 스크린샷으로 남기면 원인 분석에 유리함.

---

## 1. 클라이언트 예측(Prediction) / 서버 보정(Reconciliation) 수치

### 코드 측 (PlayerCharacter)
- **NetworkSmoothingMode**: `Exponential` — 서버 위치로 지수 보간.
- **NetworkMaxSmoothUpdateDistance**: `256` cm — 이 거리 이하만 스무딩, 그 이상은 즉시 보정 허용.
- **NetworkNoSmoothUpdateDistance**: `0` — 작은 오차도 스무딩으로 흡수.
- **NetworkLargeClientCorrectionDistance**: `500` cm — 이 이하 오차는 큰 보정으로 처리해 한 번에 맞춤.

### 설정 파일 (Config/DefaultGame.ini)
- **ClientErrorUpdateRateLimit**: `0.2` 초 — 서버가 클라이언트 위치 보정을 보내는 최소 간격. 높이면 고지연 시 덜 튐.
- **MAXPOSITIONERRORSQUARED**: `250000` (약 500cm²) — 이 오차 이하는 보정 생략.

---

## 2. 핑 100ms 이상 환경에서 위치 동기화 오차 확인

### 테스트 방법
1. **인위적 지연**: 콘솔에서 `net pktlag 100` (100ms 지연).
2. **플레이**: 이동·점프·전투 후 캐릭터가 자주 “튀는지” 관찰.
3. **통계**: `stat net` 입력 후 다음 확인:
   - **OutPackets / InPackets**: 패킷 수.
   - **OutLost / InLost**: 유실 패킷 (높으면 보정 증가 가능).
   - **Ping**: RTT.

### 오차 범위 해석
- **MAXPOSITIONERRORSQUARED**를 키우면 → 보정을 덜 보내서 “덜 튐”, 대신 위치 오차는 더 허용.
- **MAXPOSITIONERRORSQUARED**를 줄이면 → 보정을 더 보내서 정확해지지만, 고지연 시 “덜컹임” 증가 가능.
- 현재 `250000` ≈ **약 500cm** 오차까지 보정 생략. 필요 시 `DefaultGame.ini`에서 조정.

---

## 3. 서버 메모리 누수 점검 및 CPU 최적화

### 타이머 정리 (구현됨)
- `BRGameMode::EndPlay`에서 다음 타이머를 모두 해제합니다.
  - `InitialRoleApplyTimerHandle`
  - `StagedApplyTimerHandle`
  - `StagedAllLowerReadyHandle`
  - `DirectStartRoleApplyTimerHandle` (테스트 맵 직접 실행 폴백)
- 장시간 구동 시 타이머/콜백 참조로 인한 누수 가능성을 줄입니다.

### 검증 방법
1. **메모리**: `stat memory` — 서버 장시간 실행 전후 비교.
2. **프레임/CPU**: `stat game`, `stat unit` — 서버 틱 부하 확인.
3. **실행**: 리슨/전용 서버 1시간 이상 구동 후 메모리·CPU 추이 확인.

---

## 4. 패킷 유실 시 재전송 및 보간(Interpolation)

### 재전송
- 이동은 **ServerMove** (Unreliable RPC)로 전송됩니다.
- 클라이언트는 **SavedMoves** 버퍼에 이동을 쌓아 두고, 서버가 ACK하지 않은 구간은 다음 ServerMove에 포함해 재전송합니다.
- 따라서 “이동” 패킷 유실은 엔진 쪽에서 이미 재전송·재연으로 상당 부분 상쇄됩니다.

### 보간(Interpolation)
- **Simulated Proxy**(다른 플레이어 캐릭터): 서버에서 받은 `ReplicatedMovement`를 **NetworkSmoothingMode = Exponential**으로 보간해 부드럽게 표시.
- **NetworkMaxSmoothUpdateDistance** / **NetworkNoSmoothUpdateDistance** / **NetworkLargeClientCorrectionDistance**로 “언제 스무딩할지, 언제 한 번에 맞출지”를 조절했습니다.

### 패킷 유실 테스트
- 콘솔: `net pktloss 5` (5% 유실) 등으로 유실을 준 뒤 플레이.
- `stat net`으로 **OutLost / InLost** 확인.
- 유실이 클 때 덜컹임이 보이면 `DefaultGame.ini`의 **ClientErrorUpdateRateLimit**를 약간 올리거나, **MAXPOSITIONERRORSQUARED**를 키워 보정 빈도를 낮춰 볼 수 있습니다.

---

## 5. PIE 테스트 맵 직접 실행 시 전원 하체로 나오는 현상

### 원인
- **로비를 거치지 않고** 게임 맵(테스트 맵)을 에디터에서 **바로** Play하고, **Number of Players = 2**로 실행하면, 상체/하체 역할 적용이 되지 않아 **두 명 모두 하체로만** 나올 수 있습니다.
- 이유:
  1. 상체/하체 역할 저장(`SavePendingRolesForTravel`)과 적용 플래그(`SetPendingApplyRandomTeamRoles(true)`)는 **로비에서 "게임 시작" 버튼으로 맵 이동(Travel)할 때만** 설정됩니다.
  2. 테스트 맵을 **시작 맵으로 두고 바로 실행**하면 "게임 시작" 경로가 아예 실행되지 않아, 저장·플래그가 설정되지 않습니다.
  3. `BRPlayerState` 기본값이 **하체(`bIsLowerBody = true`)** 이므로, 역할 적용이 한 번도 일어나지 않으면 **전원 하체**로만 표시됩니다.
- 따라서 **“서버 유저가 방을 만들어서 팀 지정을 안 해서”**라기보다는, **로비 없이 게임 맵만 직접 실행해서** 팀/역할 저장·적용 경로가 타지 않은 경우에 해당합니다.

### 대응 (코드)
- **BRGameMode**에서, 게임 맵이 로비 없이 시작된 경우를 위한 **폴백**을 두었습니다.
  - BeginPlay 후 일정 시간(2초) 뒤에, “저장된 역할이 없고, 플레이어 2명 이상, 상체로 설정된 사람이 0명”이면 **자동으로 랜덤 팀 배정** 후 역할 저장·상체/하체 Pawn 적용을 한 번 수행합니다.
- **테스트 방법**: PIE에서 Number of Players 2로 **게임 맵을 시작 맵으로 두고** 실행 → 2초 정도 지나면 자동으로 1팀(하체+상체), 2팀(하체+상체)로 나뉘어 상체가 스폰되어야 합니다.

### 요약
| 상황 | 결과 |
|------|------|
| 로비 → 게임 시작 (팀/1P·2P 선택함) | 저장·적용 정상, 상체/하체 나뉨 |
| 로비 → 게임 시작 (아무도 1P·2P 미선택) | 기존 로직으로 자동 랜덤 팀 배정 후 저장·적용 |
| **테스트 맵 바로 실행 (로비 없음)** | **폴백**: 2초 후 자동 랜덤 팀 배정 후 상체/하체 적용 |

---

## 6. 요약 체크리스트

| 항목 | 조치 |
|------|------|
| 예측/보정 튜닝 | `PlayerCharacter` CMC 값 + `DefaultGame.ini` GameNetworkManager |
| 100ms+ 핑 오차 확인 | `net pktlag 100` + `stat net` + 플레이 관찰 |
| 서버 메모리/안정성 | `BRGameMode::EndPlay` 타이머 정리, `stat memory`·장시간 구동 검증 |
| 패킷 유실/보간 | ServerMove 버퍼 재전송(엔진 기본), Exponential 스무딩 + CMC 거리 파라미터, `net pktloss` 테스트 |
