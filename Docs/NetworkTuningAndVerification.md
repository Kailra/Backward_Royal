# 네트워크 튜닝 및 검증 가이드

네트워크 지연·패킷 유실 환경에서 부드러운 플레이와 서버 안정성을 위한 설정·검증 방법입니다.

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

## 5. 요약 체크리스트

| 항목 | 조치 |
|------|------|
| 예측/보정 튜닝 | `PlayerCharacter` CMC 값 + `DefaultGame.ini` GameNetworkManager |
| 100ms+ 핑 오차 확인 | `net pktlag 100` + `stat net` + 플레이 관찰 |
| 서버 메모리/안정성 | `BRGameMode::EndPlay` 타이머 정리, `stat memory`·장시간 구동 검증 |
| 패킷 유실/보간 | ServerMove 버퍼 재전송(엔진 기본), Exponential 스무딩 + CMC 거리 파라미터, `net pktloss` 테스트 |
