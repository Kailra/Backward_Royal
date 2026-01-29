// BR_LobbyEntryWidget.cpp
#include "BR_LobbyEntryWidget.h"
#include "Components/TextBlock.h"

void UBR_LobbyEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UBR_LobbyEntryWidget::UpdatePlayerNames(const TArray<FBRUserInfo>& PlayerInfoList)
{
	// 1. 모든 슬롯 초기화 (빈 텍스트 = 공란)
	for (UTextBlock* TextSlot : UserNameSlot)
	{
		if (TextSlot)
		{
			TextSlot->SetText(FText::GetEmpty());
		}
	}

	// 2. 들어온 순서(PlayerIndex 0, 1, 2, …)대로 슬롯에 이름 표시. 해당 인덱스에 플레이어 없으면 공란 유지
	for (int32 SlotIndex = 0; SlotIndex < UserNameSlot.Num(); ++SlotIndex)
	{
		if (!UserNameSlot[SlotIndex])
		{
			continue;
		}
		if (SlotIndex < PlayerInfoList.Num())
		{
			const FBRUserInfo& Info = PlayerInfoList[SlotIndex];
			FString DisplayName = Info.PlayerName;
			bool bUseFallback = ShouldUseFallbackDisplayName(DisplayName, Info.UserUID);
			if (bUseFallback)
			{
				DisplayName = FString::Printf(TEXT("Player %d"), Info.PlayerIndex + 1);
			}
			UE_LOG(LogTemp, Warning, TEXT("[로비이름] UpdatePlayerNames 슬롯[%d] | PlayerName='%s' UserUID='%s' fallback=%d → 표시='%s'"),
				SlotIndex, *Info.PlayerName, *Info.UserUID, bUseFallback ? 1 : 0, *DisplayName);
			UserNameSlot[SlotIndex]->SetText(FText::FromString(DisplayName));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[LobbyEntry] 입장 순서 기준 %d슬롯 표시 (플레이어 %d명)"), UserNameSlot.Num(), PlayerInfoList.Num());
}

void UBR_LobbyEntryWidget::SetEntryInfo(const FBRUserInfo& Info)
{
	if (!NameText)
	{
		return;
	}

	FString DisplayName = Info.PlayerName;
	if (ShouldUseFallbackDisplayName(DisplayName, Info.UserUID))
	{
		DisplayName = Info.PlayerIndex >= 0
			? FString::Printf(TEXT("Player %d"), Info.PlayerIndex + 1)
			: FString();
	}
	NameText->SetText(FText::FromString(DisplayName));
}
