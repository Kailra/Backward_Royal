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
			// 비어 있거나 UID와 같으면 표시용 이름으로 "Player N" 사용 (UID가 이름으로 나오지 않도록)
			if (DisplayName.IsEmpty() || DisplayName == Info.UserUID)
			{
				DisplayName = FString::Printf(TEXT("Player %d"), Info.PlayerIndex + 1);
			}
			UserNameSlot[SlotIndex]->SetText(FText::FromString(DisplayName));
		}
		// else: 슬롯은 이미 공란으로 초기화됨
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
	if (DisplayName.IsEmpty() || DisplayName == Info.UserUID)
	{
		DisplayName = Info.PlayerIndex >= 0
			? FString::Printf(TEXT("Player %d"), Info.PlayerIndex + 1)
			: FString();
	}
	NameText->SetText(FText::FromString(DisplayName));
}
