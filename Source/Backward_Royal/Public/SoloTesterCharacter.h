#pragma once

#include "CoreMinimal.h"
#include "PlayerCharacter.h" // 부모 클래스 (이미 Mesh를 가지고 있음)
#include "Misc/Optional.h"
#include "SoloTesterCharacter.generated.h"

class AUpperBodyPawn;
class UInputMappingContext;
class UInputAction;
class UAnimMontage;

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
	// 상체 블루프린트 (카메라 역할)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Setup")
	TSubclassOf<AUpperBodyPawn> UpperBodyClass;

	// 공격 몽타주 (테스트용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Setup")
	UAnimMontage* TestAttackMontage;

	// 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* TestAttackAction;

	// 스폰된 상체 (카메라용)
	UPROPERTY(VisibleInstanceOnly, Category = "Test Setup")
	AUpperBodyPawn* UpperBodyInstance;

protected:
	// 공격 실행 함수
	void RelayAttack(const FInputActionValue& Value);

	// 물리 충돌 감지
	UFUNCTION()
	void OnAttackHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};