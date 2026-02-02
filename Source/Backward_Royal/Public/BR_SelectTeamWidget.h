// BR_SelectTeamWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BRUserInfo.h"
#include "BR_LobbyTeamSlotDisplayInterface.h"
#include "BR_SelectTeamWidget.generated.h"

class UTextBlock;
class ABRGameState;

/**
 * 로비 SelectTeam 파츠 위젯 (WBP_SelectTeam용)
 * 팀 하나당 2슬롯(1Player=SlotIndex0, 2Player=SlotIndex1) 표시.
 * TeamIndex 0~3 = 팀1~4. OnPlayerListChanged 시 UpdateSlotDisplay 호출.
 * IBR_LobbyTeamSlotDisplayInterface 구현 → 부모를 바꾸지 않은 WBP_SelectTeam은 블루프린트에서 인터페이스만 구현하면 됨.
 */
UCLASS()
class BACKWARD_ROYAL_API UBR_SelectTeamWidget : public UUserWidget, public IBR_LobbyTeamSlotDisplayInterface
{
	GENERATED_BODY()

public:
	/** 팀 인덱스 (0=팀1, 1=팀2, 2=팀3, 3=팀4). 블루프린트에서 설정 */
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby")
	int32 TeamIndex = 0;

	/** 1Player 이름 표시 (BindWidgetOptional) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlayerNameSlot0;

	/** 2Player 이름 표시 (BindWidgetOptional) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlayerNameSlot1;

	/** GameState GetLobbyTeamSlotInfo(TeamIndex, 0/1)로 두 슬롯 텍스트 갱신. OnPlayerListChanged 시 호출 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lobby")
	void UpdateSlotDisplay();

	// IBR_LobbyTeamSlotDisplayInterface
	virtual void UpdateSlotDisplay_Implementation() override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** OnPlayerListChanged 바인딩 해제용 */
	UFUNCTION()
	void HandlePlayerListChanged();

	UPROPERTY()
	TObjectPtr<ABRGameState> CachedGameState;
};
