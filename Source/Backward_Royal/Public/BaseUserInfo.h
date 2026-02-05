#pragma once

#include "CoreMinimal.h"
#include "CustomizationInfo.h"
#include "BaseUserInfo.generated.h"

USTRUCT(BlueprintType)
struct FBaseUserInfo {
  GENERATED_BODY()

  // 사용자 고유 ID (예: Steam ID, 계정 ID 등)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
  FString UserUID;

  // 플레이어 이름
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
  FString PlayerName;

  // 플레이어 레벨
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
  int32 PlayerLevel = 1;

  // 현재 경험치
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
  int32 CurrentXP = 0;

  // 다음 레벨까지 필요한 경험치
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
  int32 MaxXP = 100;

  // [Refactored] 커스터마이징 데이터 통합 (기존 SkinID 등 대체)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization")
  FBRCustomizationData UserCustomData;
};

/**
 *
 */
UCLASS()
class BACKWARD_ROYAL_API UBaseUserInfo : public UObject {
  GENERATED_BODY()
};
