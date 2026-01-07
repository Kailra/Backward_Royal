#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h" // 부모 클래스가 ABaseCharacter라면 유지, 아니라면 ACharacter로 변경
#include "InputActionValue.h"
#include "PlayerCharacter.generated.h" // 이 줄은 무조건 #include 중 가장 마지막에 있어야 합니다.

// 로그 카테고리 선언
DECLARE_LOG_CATEGORY_EXTERN(LogPlayerChar, Log, All);

UCLASS()
class BACKWARD_ROYAL_API APlayerCharacter : public ABaseCharacter // 또는 public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	// 상체(Player A)가 이 캐릭터의 상반신 회전값을 업데이트할 때 호출
	void SetUpperBodyRotation(FRotator NewRotation);

	// [기존] 블루프린트에서 몽타주 재생 (C++에서 호출함)
	UFUNCTION(BlueprintImplementableEvent, Category = "Coop")
	void TriggerUpperBodyAttack();

	// [네트워크 추가] 서버가 모든 클라이언트에게 실행하라고 명령하는 함수
	UFUNCTION(NetMulticast, Reliable)
	void Multi_PlayAttack();

	// [네트워크 추가] 애니메이션 변수 동기화
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Coop|Animation")
	FRotator UpperBodyAimRotation;

protected:
	virtual void BeginPlay() override;

	// [네트워크 추가] 리플리케이션 규칙 정의 (변수 동기화 필수 함수)
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 이동 (Player B 전용)
	void Move(const FInputActionValue& Value);
	// 시선 (Player B 전용 - 후방 카메라 회전)
	void Look(const FInputActionValue& Value);

public:
	// --- Camera (Rear View) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USpringArmComponent* RearCameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* RearCamera;

	// --- Mount Point ---
	// Player A(상반신) Pawn이 부착될 위치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coop")
	class USceneComponent* HeadMountPoint;

	// --- Inputs (Player B) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* JumpAction;
};