// BR_LobbyEntryWidget.cpp
#include "BR_LobbyEntryWidget.h"
#include "Components/TextBlock.h"
#include "Misc/Char.h"

namespace
{
	/** PlayerName이 UID 또는 기본 ID(Player_1234, Player_1_날짜 등) 형식이면 true. 이럴 땐 "Player N"으로 표시 */
	bool ShouldUseFallbackDisplayName(const FString& PlayerName, const FString& UserUID)
	{
		if (PlayerName.IsEmpty()) return true;
		if (PlayerName == UserUID) return true;
		if (!PlayerName.StartsWith(TEXT("Player_"))) return false;
		const FString Suffix = PlayerName.Mid(7);
		if (Suffix.IsEmpty()) return true;
		// "1_2025.01.29..." (UserUID 형식)
		if (Suffix.Contains(TEXT("_"))) return true;
		// "1234" (GI 기본 Player_1234)
		if (Suffix.Len() == 4)
		{
			for (int32 i = 0; i < 4; i++) { if (!FChar::IsDigit(Suffix[i])) return false; }
			return true;
		}
		return false;
	}
}

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
			if (ShouldUseFallbackDisplayName(DisplayName, Info.UserUID))
			{
				DisplayName = FString::Printf(TEXT("Player %d"), Info.PlayerIndex + 1);
			}
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
