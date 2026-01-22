#pragma once

#include "CoreMinimal.h"
#include "PlayerCharacter.h" // 부모 클래스
#include "Misc/Optional.h"   // [필수] UE 5.5 호환성 (TOptional 에러 해결)

// [절대 규칙] generated.h는 무조건 #include 중 가장 마지막 줄에 있어야 합니다.
#include "TestSoloCharacter.generated.h"

// 전방 선언
class AUpperBodyPawn;
class UInputMappingContext;
class UInputAction;
class UAnimMontage;

UCLASS()
class BACKWARD_ROYAL_API ATestSoloCharacter : public APlayerCharacter
{
	GENERATED_BODY()

public:
	ATestSoloCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;

public:
	// -------------------------------------------------------------------------
	// [설정 변수] 에디터에서 할당
	// -------------------------------------------------------------------------

	// 상체 블루프린트 클래스 (BP_UpperBodyPawn)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Setup")
	TSubclassOf<AUpperBodyPawn> UpperBodyClass;

	// 공격 몽타주 (테스트용 강제 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Setup")
	UAnimMontage* TestAttackMontage;

	// 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* TestAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* TestInteractAction;

	// -------------------------------------------------------------------------
	// [내부 변수]
	// -------------------------------------------------------------------------

	// 실제로 스폰된 상체
	UPROPERTY(VisibleInstanceOnly, Category = "Test Setup")
	AUpperBodyPawn* UpperBodyInstance;

protected:
	// -------------------------------------------------------------------------
	// [기능 함수]
	// -------------------------------------------------------------------------

	// 중계 함수
	UFUNCTION(BlueprintCallable)
	void RelayAttack(const FInputActionValue& Value);
	void RelayInteract(const FInputActionValue& Value);

	// 물리 충돌 감지 (공격 성공 판정)
	UFUNCTION()
	void OnAttackHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};