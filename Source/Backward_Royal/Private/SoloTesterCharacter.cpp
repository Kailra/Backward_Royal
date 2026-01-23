#include "SoloTesterCharacter.h"
#include "UpperBodyPawn.h" 
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"

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

	// 내 몸(Mesh) 보이게 설정
	if (GetMesh()) GetMesh()->SetOwnerNoSee(false);

	// 상체(카메라) 스폰
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
		if (TestInteractAction)
		{
			EnhancedInputComponent->BindAction(TestInteractAction, ETriggerEvent::Started, this, &ASoloTesterCharacter::RelayInteract);
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
	// [핵심] 메인 캐릭터(부모)가 가진 공격 로직을 그대로 사용합니다.
	// 왼손/오른손 펀치, 무기 공격 여부를 알아서 판단합니다.
	RequestAttack();

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("메인 캐릭터 공격 요청 (RequestAttack)"));
}

void ASoloTesterCharacter::ForceAttack()
{
	RequestAttack();
}

void ASoloTesterCharacter::RelayInteract(const FInputActionValue& Value)
{
	// 상호작용 로직 (필요 시 구현)
}