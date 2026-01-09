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

	// 게임 시작 맵 경로
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Settings")
	FString GameMapPath = TEXT("/Game/Main/Level/Map_Test1");

	// 하체 캐릭터 클래스 (팀의 첫 번째 플레이어)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pawn Classes")
	TSubclassOf<APawn> LowerBodyPawnClass;

	// 상체 Pawn 클래스 (팀의 두 번째 플레이어)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pawn Classes")
	TSubclassOf<APawn> UpperBodyPawnClass;

	// 게임 시작
	UFUNCTION(BlueprintCallable, Category = "Game")
	void StartGame();

	// 역할에 따라 Pawn 생성 (Blueprint에서 호출 가능)
	UFUNCTION(BlueprintCallable, Category = "Pawn")
	APawn* SpawnPawnForPlayer(AController* NewPlayer, AActor* StartSpot);

	// 역할에 따라 Pawn 생성 (가상 함수 오버라이드)
	// 주의: AGameModeBase에는 이 함수가 없을 수 있으므로 override 제거
	virtual APawn* SpawnDefaultPawnFor(AController* NewPlayer, AActor* StartSpot);

	// 플레이어 로그인 처리 (Blueprint에서 호출 가능)
	UFUNCTION(BlueprintCallable, Category = "Player")
	void HandlePlayerPostLogin(APlayerController* NewPlayer);

	// 플레이어 로그인 처리 (가상 함수 오버라이드)
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// 플레이어 로그아웃 처리 (Blueprint에서 호출 가능)
	UFUNCTION(BlueprintCallable, Category = "Player")
	void HandlePlayerLogout(AController* Exiting);

	// 플레이어 로그아웃 처리 (가상 함수 오버라이드)
	virtual void Logout(AController* Exiting) override;

protected:
	virtual void BeginPlay() override;
};

