// BR_LobbyEntryWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BRUserInfo.h"
#include "BR_LobbyEntryWidget.generated.h"

class UTextBlock;

/**
 * 로비 플레이어 엔트리 위젯 베이스 클래스 (WBP_Entry용)
 * TeamID가 0(대기열)일 때만 플레이어 이름을 표시합니다.
 */
UCLASS()
class BACKWARD_ROYAL_API UBR_LobbyEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 플레이어 목록에서 TeamID == 0인 플레이어만 필터링하여 UserNameSlot에 이름 표시.
	 * @param PlayerInfoList 전체 플레이어 정보 배열
	 */
	UFUNCTION(BlueprintCallable, Category = "Lobby Entry")
	void UpdatePlayerNames(const TArray<FBRUserInfo>& PlayerInfoList);

	/**
	 * 단일 엔트리 정보 설정 (하위 호환용).
	 * TeamID == 0일 때만 NameText에 플레이어 이름을 표시하고, 그 외에는 비웁니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Lobby Entry")
	void SetEntryInfo(const FBRUserInfo& Info);

	/** 이름 표시용 TextBlock 배열 (블루프린트에서 설정) */
	UPROPERTY(BlueprintReadWrite, Category = "Lobby Entry")
	TArray<TObjectPtr<UTextBlock>> UserNameSlot;

	/** 단일 NameText 블록 (BindWidgetOptional, 하위 호환용) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

protected:
	virtual void NativeConstruct() override;
};
