#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "UpperBodyPawn.generated.h" // 항상 마지막 include여야 함

class APlayerCharacter;
class UInputMappingContext;
class UInputAction;

UCLASS()
class BACKWARD_ROYAL_API AUpperBodyPawn : public APawn
{
	GENERATED_BODY()

public:
	AUpperBodyPawn();

protected:
	virtual void BeginPlay() override;

	// 매 프레임마다 몸통 회전을 따라가기 위해 Tick이 필요합니다.
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void Look(const FInputActionValue& Value);
	void Attack(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);

	// [네트워크 추가] 서버에게 공격 요청 (Server RPC)
	UFUNCTION(Server, Reliable)
	void Server_RequestAttack();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USpringArmComponent* FrontCameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* FrontCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* UpperBodyMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* AttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionDistance = 300.0f;

	// 공격 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimMontage* AttackMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsAttacking;

private:
	UPROPERTY()
	class APlayerCharacter* ParentBodyCharacter;

	// 지난 프레임의 몸통 각도를 저장할 변수
	float LastBodyYaw;
};