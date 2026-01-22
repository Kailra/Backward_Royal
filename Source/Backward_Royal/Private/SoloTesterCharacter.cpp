#include "SoloTesterCharacter.h"
#include "UpperBodyPawn.h" 
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "Animation/AnimInstance.h"

ASoloTesterCharacter::ASoloTesterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 자유 시점 설정
	bUseControllerRotationYaw = true;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
		GetCharacterMovement()->MaxWalkSpeed = 400.0f;
	}

	if (RearCameraBoom)
	{
		RearCameraBoom->bUsePawnControlRotation = true;
	}
}

void ASoloTesterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 내 몸(하체+상체)이 보이도록 설정
	if (GetMesh()) GetMesh()->SetOwnerNoSee(false);

	// 상체(카메라) 스폰 및 부착
	if (UpperBodyClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();

		UpperBodyInstance = GetWorld()->SpawnActor<AUpperBodyPawn>(UpperBodyClass, GetActorTransform(), SpawnParams);

		if (UpperBodyInstance)
		{
			if (HeadMountPoint)
				UpperBodyInstance->AttachToComponent(HeadMountPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			else
				UpperBodyInstance->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

			UpperBodyInstance->SetOwner(this);
		}
	}
}

void ASoloTesterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (TestAttackAction)
		{
			EnhancedInputComponent->BindAction(TestAttackAction, ETriggerEvent::Started, this, &ASoloTesterCharacter::RelayAttack);
		}
	}
}

void ASoloTesterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 시선 동기화
	if (UpperBodyInstance && GetController())
	{
		UpperBodyInstance->SetActorRotation(GetControlRotation());
	}
	UpperBodyAimRotation = GetControlRotation();
}

void ASoloTesterCharacter::RelayAttack(const FInputActionValue& Value)
{
	// [핵심 변경] 상체 Pawn이 아니라 '나 자신(Body)'의 메쉬를 가져옵니다.
	// PlayerCharacter가 이미 메쉬를 가지고 있기 때문입니다.
	USkeletalMeshComponent* MyMesh = GetMesh();

	if (!MyMesh)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[오류] 캐릭터에 메쉬가 없습니다!"));
		return;
	}

	UAnimMontage* MontageToPlay = TestAttackMontage;

	// 설정된 몽타주가 없으면 상체 데이터에서 가져오기 시도
	if (!MontageToPlay && UpperBodyInstance)
	{
		MontageToPlay = UpperBodyInstance->AttackMontage;
	}

	if (MontageToPlay)
	{
		UAnimInstance* AnimInstance = MyMesh->GetAnimInstance();

		if (!AnimInstance)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[오류] 애니메이션 블루프린트(ABP)가 설정되지 않았습니다."));
			return;
		}

		if (!AnimInstance->Montage_IsPlaying(MontageToPlay))
		{
			AnimInstance->Montage_Play(MontageToPlay);

			// 성공 로그
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT(">> 공격 발동! (내 몸 사용) <<"));

			// 물리 충돌 켜기 (공격 판정)
			MyMesh->SetNotifyRigidBodyCollision(true);
			if (!MyMesh->OnComponentHit.IsAlreadyBound(this, &ASoloTesterCharacter::OnAttackHit))
			{
				MyMesh->OnComponentHit.AddDynamic(this, &ASoloTesterCharacter::OnAttackHit);
			}
		}
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[오류] 공격 몽타주가 비어있습니다!"));
	}
}

void ASoloTesterCharacter::OnAttackHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == this || OtherActor == UpperBodyInstance) return;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("👊 타격 성공! 대상: %s"), *OtherActor->GetName()));
}