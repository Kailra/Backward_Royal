#pragma once

#include "CoreMinimal.h"
#include "PlayerCharacter.h" // 부모 클래스 (이미 공격 기능 보유)
#include "Misc/Optional.h"
#include "SoloTesterCharacter.generated.h"

class AUpperBodyPawn;
class UInputMappingContext;
class UInputAction;

UCLASS()
class BACKWARD_ROYAL_API ASoloTesterCharacter : public APlayerCharacter
{
	GENERATED_BODY()

public:
	ASoloTesterCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;

public:
	// -------------------------------------------------------------------------
	// [설정] 에디터에서 할당
	// -------------------------------------------------------------------------

	// 상체 카메라용 Pawn (BP_UpperBodyPawn)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Setup")
	TSubclassOf<AUpperBodyPawn> UpperBodyClass;

	// 공격 키 (IA_Attack)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* TestAttackAction;

	// 상호작용 키 (IA_Interact)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* TestInteractAction;

	// -------------------------------------------------------------------------
	// [내부 변수]
	// -------------------------------------------------------------------------
	UPROPERTY(VisibleInstanceOnly, Category = "Test Setup")
	AUpperBodyPawn* UpperBodyInstance;

protected:
	// 공격 실행 (메인 캐릭터 로직 호출)
	void RelayAttack(const FInputActionValue& Value);

	void RelayInteract(const FInputActionValue& Value);

	// 블루프린트 강제 호출용
	UFUNCTION(BlueprintCallable)
	void ForceAttack();
};