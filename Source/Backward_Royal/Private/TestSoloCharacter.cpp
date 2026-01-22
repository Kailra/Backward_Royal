#include "TestSoloCharacter.h"
#include "UpperBodyPawn.h" 

// [필수 헤더]
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "Animation/AnimInstance.h"

// 주의: TextureBuildSettings.h는 절대 포함하지 마세요!

ATestSoloCharacter::ATestSoloCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 자유 시점 설정
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
		GetCharacterMovement()->MaxWalkSpeed = 400.0f;
	}

	if (RearCameraBoom)
	{
		RearCameraBoom->bUsePawnControlRotation = true;
		RearCameraBoom->bInheritPitch = true;
		RearCameraBoom->bInheritYaw = true;
		RearCameraBoom->bInheritRoll = false;
	}
}

void ATestSoloCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (GetMesh()) GetMesh()->SetOwnerNoSee(false);

	// 상체 스폰
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

void ATestSoloCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (TestAttackAction)
		{
			EnhancedInputComponent->BindAction(TestAttackAction, ETriggerEvent::Started, this, &ATestSoloCharacter::RelayAttack);
		}
		if (TestInteractAction)
		{
			EnhancedInputComponent->BindAction(TestInteractAction, ETriggerEvent::Started, this, &ATestSoloCharacter::RelayInteract);
		}
	}
}

void ATestSoloCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (UpperBodyInstance && GetController())
	{
		UpperBodyInstance->SetActorRotation(GetControlRotation());
	}

	UpperBodyAimRotation = GetControlRotation();
}

void ATestSoloCharacter::RelayAttack(const FInputActionValue& Value)
{
	if (!UpperBodyInstance) return;

	USkeletalMeshComponent* UpperMesh = UpperBodyInstance->GetComponentByClass<USkeletalMeshComponent>();
	if (!UpperMesh)
	{
		TArray<USkeletalMeshComponent*> Meshes;
		UpperBodyInstance->GetComponents(Meshes);
		if (Meshes.Num() > 0) UpperMesh = Meshes[0];
	}

	if (!UpperMesh) return;

	// 몽타주 결정
	UAnimMontage* MontageToPlay = TestAttackMontage;
	if (!MontageToPlay) MontageToPlay = UpperBodyInstance->AttackMontage;

	if (MontageToPlay)
	{
		UAnimInstance* AnimInstance = UpperMesh->GetAnimInstance();
		if (AnimInstance && !AnimInstance->Montage_IsPlaying(MontageToPlay))
		{
			AnimInstance->Montage_Play(MontageToPlay);

			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("공격 시작! (물리 충돌 ON)"));

			UpperMesh->SetNotifyRigidBodyCollision(true);

			if (!UpperMesh->OnComponentHit.IsAlreadyBound(this, &ATestSoloCharacter::OnAttackHit))
			{
				UpperMesh->OnComponentHit.AddDynamic(this, &ATestSoloCharacter::OnAttackHit);
			}
		}
	}
}

void ATestSoloCharacter::OnAttackHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == this || OtherActor == UpperBodyInstance) return;

	FString HitMsg = FString::Printf(TEXT("👊 타격 성공! 대상: %s"), *OtherActor->GetName());

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, HitMsg);
}

void ATestSoloCharacter::RelayInteract(const FInputActionValue& Value)
{
	// 상호작용
}