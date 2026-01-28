// BR_LobbyEntryWidget.cpp
#include "BR_LobbyEntryWidget.h"
#include "Components/TextBlock.h"

void UBR_LobbyEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UBR_LobbyEntryWidget::UpdatePlayerNames(const TArray<FBRUserInfo>& PlayerInfoList)
{
	// 1. 모든 슬롯 초기화 (빈 텍스트로 설정)
	for (UTextBlock* TextSlot : UserNameSlot)
	{
		if (TextSlot)
		{
			TextSlot->SetText(FText::GetEmpty());
		}
	}

	// 2. TeamID가 0인 플레이어만 필터링하여 순서대로 슬롯에 표시
	int32 SlotIndex = 0;
	for (const FBRUserInfo& Info : PlayerInfoList)
	{
		if (Info.TeamID == 0)
		{
			if (SlotIndex < UserNameSlot.Num() && UserNameSlot[SlotIndex])
			{
				FString DisplayName = Info.PlayerName;
				if (DisplayName.IsEmpty())
				{
					DisplayName = FString::Printf(TEXT("Player %d"), Info.PlayerIndex + 1);
				}
				UserNameSlot[SlotIndex]->SetText(FText::FromString(DisplayName));
				SlotIndex++;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[LobbyEntry] TeamID 0 플레이어 %d명 표시"), SlotIndex);
}

void UBR_LobbyEntryWidget::SetEntryInfo(const FBRUserInfo& Info)
{
	if (!NameText)
	{
		return;
	}

	if (Info.TeamID == 0)
	{
		FString DisplayName = Info.PlayerName;
		if (DisplayName.IsEmpty())
		{
			DisplayName = FString::Printf(TEXT("Player %d"), Info.PlayerIndex + 1);
		}
		NameText->SetText(FText::FromString(DisplayName));
		NameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		NameText->SetText(FText::FromString(TEXT("")));
		NameText->SetVisibility(ESlateVisibility::Collapsed);
	}
}
