# 🎮 Backward_Royal

> 5인 개발 프로젝트의 원활한 협업을 위한 통합 가이드라인입니다.

## 📚 Quick Links (주요 문서)

[![Google Drive](https://img.shields.io/badge/Google_Drive-프로젝트_폴더-FBBC05?style=for-the-badge&logo=googledrive&logoColor=white)](https://drive.google.com/drive/folders/1etBfwQcma6jF_t84D6MC_Kz6qs5d9qQz?usp=sharing)
[![Google Docs](https://img.shields.io/badge/Google_Docs-기획서-4285F4?style=for-the-badge&logo=googledocs&logoColor=white)](https://docs.google.com/document/d/1gOZUQdxsdSx1_8nlnFHAVHUjaaqVJ2rRD9nGF-k7c9Y/edit?usp=sharing)
[![Google Docs](https://img.shields.io/badge/Google_Docs-협업_규칙-34A853?style=for-the-badge&logo=googledocs&logoColor=white)](https://docs.google.com/document/d/1NX_WE08bDh2432Rs42QQr6q8Jr_KsDkwkf30p7JBhzk/edit?usp=sharing)
[![Jira](https://img.shields.io/badge/Jira-스케줄_관리-0052CC?style=for-the-badge&logo=jirasoftware&logoColor=white)](https://giman979.atlassian.net/jira/software/projects/SCRUM/summary)
[![Google Docs](https://img.shields.io/badge/Google_Docs-기술_문서-1A7D90?style=for-the-badge&logo=googledocs&logoColor=white)](https://docs.google.com/document/d/1JPuusqn3770yIvaBkk9mt0OYxdQ8c-YQrQeYgs1FwiU/edit?usp=sharing)

---

## 🛠 협업 가이드 (핵심 요약)

### 1. 브랜치 전략: Master-Feature
* **Master**: 언제든 빌드 가능한 안정 버전.
* **Feature**: 기능 단위 작업 브랜치 (`feature/기능명`). 
* **수시 동기화**: `Master`에 새로운 머지가 발생하면 즉시 내 브랜치로 **Pull(Merge)** 하여 최신 상태를 유지합니다.

### 2. 폴더 구조 및 작업 영역
* **공용 영역**: `Content/Main` (검증된 베이스 에셋 보관)
* **개인 영역**: `Content/Develop/[Name]` (작업 중인 개인 폴더)
* **주의**: `Main` 폴더의 에셋을 수정할 때는 반드시 팀원에게 구두로 알립니다.

### 3. Jira 및 커밋 규칙
* **이슈 키 포함**: 모든 커밋 메시지 시작점에 Jira 이슈 키를 작성합니다.
  * 예: `[PROJ-12] 캐릭터 이동 로직 구현`
* **상태 업데이트**: 작업 시작 시 Jira 티켓을 `In Progress`로, 머지 대기 시 `Merge Reservation`으로 이동합니다.

---

## 📅 마일스톤 요약
<!--
| 주차 | 주요 목표 | 상태 |
| :--- | :--- | :---: |
| **1주차** | 협업 기반 구축 및 기획 고도화 | 진행 중 |
| **2주차** | 핵심 시스템 기초 구현 (이동, 서버연결) | 대기 |
| **3주차** | 전투 및 조작 고도화 (전투 시스템) | 대기 |
-->

---

## ⚠️ 주의사항
* **Fix Up Redirectors**: 폴더 이동이나 이름 변경 후 반드시 실행 후 커밋하세요.
* **Merge 전 노티**: Master에 머지하기 전, 변경된 노드/코드를 팀원들에게 브리핑(코드 리뷰) 합니다.



---
최종 업데이트: 2026-01-12
