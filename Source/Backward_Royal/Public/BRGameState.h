// BRGameState.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BRUserInfo.h"
#include "BRGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerListChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTeamChanged);

UCLASS()
class BACKWARD_ROYAL_API ABRGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ABRGameState();

	// 최소 플레이어 수
	UPROPERTY(BlueprintReadOnly, Category = "Room Settings")
	int32 MinPlayers = 4;

	// 최대 플레이어 수
	UPROPERTY(BlueprintReadOnly, Category = "Room Settings")
	int32 MaxPlayers = 8;

	// 현재 플레이어 수
	UPROPERTY(ReplicatedUsing = OnRep_PlayerCount, BlueprintReadOnly, Category = "Room")
	int32 PlayerCount;

	// 게임 시작 가능 여부
	UPROPERTY(ReplicatedUsing = OnRep_CanStartGame, BlueprintReadOnly, Category = "Room")
	bool bCanStartGame;

	// 플레이어 목록 변경 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerListChanged OnPlayerListChanged;

	// 팀 변경 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTeamChanged OnTeamChanged;

	// 플레이어 목록 업데이트
	UFUNCTION(BlueprintCallable, Category = "Room")
	void UpdatePlayerList();

	// 모든 플레이어의 UserInfo 배열 가져오기 (UI에서 사용)
	UFUNCTION(BlueprintCallable, Category = "Room")
	TArray<FBRUserInfo> GetAllPlayerUserInfo() const;

	// 특정 플레이어의 UserInfo 가져오기
	UFUNCTION(BlueprintCallable, Category = "Room")
	FBRUserInfo GetPlayerUserInfo(int32 PlayerIndex) const;

	/** 방장(호스트) 플레이어 이름 가져오기. "○○'s Game" 표시용 */
	UFUNCTION(BlueprintCallable, Category = "Room", meta = (DisplayName = "Get Host Player Name"))
	FString GetHostPlayerName() const;

	/** 서버에서 설정·복제되는 방 제목 (예: "○○'s Game"). 입장한 클라이언트도 동일하게 표시됨 */
	UPROPERTY(ReplicatedUsing = OnRep_RoomTitle, BlueprintReadOnly, Category = "Room")
	FString RoomTitle;

	/** 방 제목 표시용. RoomTitle이 있으면 그대로 반환, 없으면 GetHostPlayerName() + "'s Game" (블루프린트/UI에서 사용) */
	UFUNCTION(BlueprintCallable, Category = "Room", meta = (DisplayName = "Get Room Title Display"))
	FString GetRoomTitleDisplay() const;

	/** 서버 전용: 방 제목 설정 (방장 입장 시 호출) */
	void SetRoomTitle(const FString& InRoomTitle);

	// 게임 시작 가능 여부 확인
	UFUNCTION(BlueprintCallable, Category = "Room")
	void CheckCanStartGame();

	// 랜덤 팀 배정
	UFUNCTION(BlueprintCallable, Category = "Team")
	void AssignRandomTeams();

	// 모든 플레이어가 준비되었는지 확인
	UFUNCTION(BlueprintCallable, Category = "Room")
	bool AreAllPlayersReady() const;
	
	// 호스트를 제외한 모든 플레이어가 준비되었는지 확인
	UFUNCTION(BlueprintCallable, Category = "Room")
	bool AreAllNonHostPlayersReady() const;

	// 플레이어 수 변경 시 호출
	UFUNCTION()
	void OnRep_PlayerCount();

	// 게임 시작 가능 여부 변경 시 호출
	UFUNCTION()
	void OnRep_CanStartGame();

	// 방 제목 복제 수신 시 호출
	UFUNCTION()
	void OnRep_RoomTitle();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
};

