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

	// 하체 플레이어용 Pawn 클래스 (BP_LowerBodyCharacter)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pawn Classes")
	TSubclassOf<APawn> LowerBodyPawnClass;

	// 상체 플레이어용 Pawn 클래스 (BP_UpperBodyPawn)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Pawn Classes")
	TSubclassOf<APawn> UpperBodyPawnClass;

	// 게임 시작
	UFUNCTION(BlueprintCallable, Category = "Game")
	void StartGame();

	// 플레이어 로그인 처리
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// 플레이어 로그아웃 처리
	virtual void Logout(AController* Exiting) override;

	// 역할에 따라 다른 Pawn 스폰
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

	// 상체 플레이어를 하체 플레이어에 연결
	void AttachUpperBodyToLowerBody(APlayerController* UpperBodyController, int32 LowerBodyPlayerIndex);

	// 상체 플레이어 Pawn 스폰 및 하체에 연결
	void SpawnAndAttachUpperBody(APlayerController* UpperBodyController, int32 LowerBodyPlayerIndex);

	// 상체 플레이어 Pawn 즉시 스폰 (연결 없이, 킥 방지용)
	void SpawnUpperBodyPawnImmediately(APlayerController* UpperBodyController);

protected:
	virtual void BeginPlay() override;
};

