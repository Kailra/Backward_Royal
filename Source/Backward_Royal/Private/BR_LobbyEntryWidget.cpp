// BR_LobbyEntryWidget.cpp
#include "BR_LobbyEntryWidget.h"
#include "Components/TextBlock.h"

void UBR_LobbyEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UBR_LobbyEntryWidget::UpdatePlayerNames(const TArray<FBRUserInfo>& PlayerInfoList)
{
	UE_LOG(LogTemp, Warning, TEXT("[LobbyEntry] UpdatePlayerNames 호출됨. 전체 플레이어 수: %d, UserNameSlot 개수: %d"), 
		PlayerInfoList.Num(), UserNameSlot.Num());

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
	int32 TeamID0Count = 0;
	for (const FBRUserInfo& Info : PlayerInfoList)
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbyEntry] 플레이어[%d]: Name=%s, TeamID=%d, PlayerIndex=%d"), 
			TeamID0Count, *Info.PlayerName, Info.TeamID, Info.PlayerIndex);
		
		if (Info.TeamID == 0)
		{
			TeamID0Count++;
			if (SlotIndex < UserNameSlot.Num() && UserNameSlot[SlotIndex])
			{
				FString DisplayName = Info.PlayerName;
				if (DisplayName.IsEmpty())
				{
					DisplayName = FString::Printf(TEXT("Player %d"), Info.PlayerIndex + 1);
				}
				UserNameSlot[SlotIndex]->SetText(FText::FromString(DisplayName));
				UE_LOG(LogTemp, Warning, TEXT("[LobbyEntry] 슬롯[%d]에 이름 표시: %s"), SlotIndex, *DisplayName);
				SlotIndex++;
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[LobbyEntry] 슬롯[%d] 사용 불가 (UserNameSlot.Num()=%d 또는 nullptr)"), 
					SlotIndex, UserNameSlot.Num());
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[LobbyEntry] TeamID 0 플레이어 %d명 중 %d명 표시 완료"), TeamID0Count, SlotIndex);
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
