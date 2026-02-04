                                      // BRGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BRGameMode.generated.h"

UCLASS()
class BACKWARD_ROYAL_API ABRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABRGameMode();

	// 최소 플레이어 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room Settings")
	int32 MinPlayers = 4;

	// 최대 플레이어 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room Settings")
	int32 MaxPlayers = 8;

	// 방 생성 후 이동할 로비 맵 경로. 비어 있으면 현재 맵 유지.
	// 예: /Game/Main/Level/Main_Scene 또는 /Game/Main/Level/Stage/Stage01_Temple
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings", meta = (DisplayName = "로비 맵 경로"))
	FString LobbyMapPath;

	// 게임 시작 맵 경로 (레거시 - 랜덤 맵 선택 시 사용되지 않음)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings")
	FString GameMapPath = TEXT("/Game/Main/Level/Stage/Stage01_Temple");

	/** Stage 맵 폴더 경로. 지정 시 이 폴더(및 하위)의 맵 중 하나를 랜덤 선택. 예: /Game/Main/Level/Stage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings", meta = (DisplayName = "Stage 맵 폴더 경로"))
	FString StageMapFolderPath = TEXT("/Game/Main/Level/Stage");

	/** Stage 폴더 스캔 시 하위 폴더 포함 여부 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings")
	bool bRecursiveStageFolder = true;

	/** Stage 맵 목록 (폴더 스캔 실패 시 fallback) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings")
	TArray<FString> StageMapPaths = {
		TEXT("/Game/Main/Level/Stage/Stage01_Temple"),
		TEXT("/Game/Main/Level/Stage/Stage02_Bushes"),
		TEXT("/Game/Main/Level/Stage/Stage03_Arena")
	};

	// 랜덤 맵 선택 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings")
	bool bUseRandomMap = true;

	// 게임 시작
	UFUNCTION(BlueprintCallable, Category = "Game")
	void StartGame();

	// 플레이어 로그인 처리
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// 플레이어 로그아웃 처리
	virtual void Logout(AController* Exiting) override;

	// 랜덤 팀 배정 후 상체/하체 Pawn 재배치 (상체 스폰 및 빙의)
	void ApplyRoleChangesForRandomTeams();

	// 플레이어 사망 시 호출되는 함수
	void OnPlayerDied(class ABaseCharacter* VictimCharacter);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** StageMapFolderPath에서 맵 목록을 Asset Registry로 조회. 빈 배열이면 StageMapPaths 사용 */
	TArray<FString> GetStageMapPaths() const;

	/** 모든 클라이언트에게 게임 시작 알림 (ServerTravel 직전 호출) */
	void NotifyAllClientsGameStarting();

	// 에디터(BP_BRGameMode)에서 BP_UpperBodyPawn을 할당할 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classes")
	TSubclassOf<class AUpperBodyPawn> UpperBodyClass;
};

