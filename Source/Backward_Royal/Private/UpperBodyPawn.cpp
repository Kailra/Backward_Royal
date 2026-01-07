#include "UpperBodyPawn.h"
#include "PlayerCharacter.h"
#include "DropItem.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/SkeletalMeshComponent.h" 
#include "DrawDebugHelpers.h"

AUpperBodyPawn::AUpperBodyPawn()
{
	// [중요] 틱 그룹 설정: 모든 물리/이동 계산이 끝난 후 카메라를 갱신해야 떨림이 없습니다.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	FrontCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("FrontCameraBoom"));
	FrontCameraBoom->SetupAttachment(RootComponent);

	// =================================================================
	// [핵심 1] 절대 회전 사용 (Absolute Rotation)
	// 몸통(척추)이 애니메이션으로 흔들려도, 카메라는 마우스 방향을 굳건히 유지합니다.
	// =================================================================
	FrontCameraBoom->SetUsingAbsoluteRotation(true);

	FrontCameraBoom->bUsePawnControlRotation = true;
	FrontCameraBoom->bDoCollisionTest = false;
	FrontCameraBoom->TargetArmLength = 0.0f;

	FrontCameraBoom->bInheritPitch = true;
	FrontCameraBoom->bInheritYaw = true;
	FrontCameraBoom->bInheritRoll = false;

	// =================================================================
	// [핵심 2] 카메라 렉(Lag) 끄기
	// 공격 속도가 빠를 때 카메라가 뒤쳐져서 캐릭터가 화면 밖으로 나가는 현상을 방지합니다.
	// =================================================================
	FrontCameraBoom->bEnableCameraLag = false;
	FrontCameraBoom->bEnableCameraRotationLag = false;

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FrontCamera"));
	FrontCamera->SetupAttachment(FrontCameraBoom);
	FrontCamera->bUsePawnControlRotation = false;

	LastBodyYaw = 0.0f;
	ParentBodyCharacter = nullptr;

	InteractionDistance = 300.0f;

	// [필수] 이 폰은 네트워크 복제가 되어야 함
	bReplicates = true;
	SetReplicateMovement(true); // 움직임(부착된 상태) 동기화
}

void AUpperBodyPawn::BeginPlay()
{
	Super::BeginPlay();

	// 입력 매핑 등록
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (UpperBodyMappingContext)
			{
				Subsystem->AddMappingContext(UpperBodyMappingContext, 0);
			}
		}
	}
}

void AUpperBodyPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. 부모(몸통) 찾기 및 초기화
	if (!ParentBodyCharacter)
	{
		ParentBodyCharacter = Cast<APlayerCharacter>(GetAttachParentActor());

		if (ParentBodyCharacter && Controller)
		{
			float CurrentBodyYaw = ParentBodyCharacter->GetActorRotation().Yaw;
			FRotator NewRotation = Controller->GetControlRotation();
			NewRotation.Yaw = CurrentBodyYaw + 180.0f;
			Controller->SetControlRotation(NewRotation);
			LastBodyYaw = CurrentBodyYaw;
			return;
		}
	}

	if (!ParentBodyCharacter || !Controller) return;

	// 2. 몸통 회전 동기화
	float CurrentBodyYaw = ParentBodyCharacter->GetActorRotation().Yaw;
	float DeltaYaw = CurrentBodyYaw - LastBodyYaw;

	if (!FMath::IsNearlyZero(DeltaYaw))
	{
		FRotator CurrentRot = Controller->GetControlRotation();
		CurrentRot.Yaw += DeltaYaw;
		Controller->SetControlRotation(CurrentRot);
	}

	LastBodyYaw = CurrentBodyYaw;

	// -----------------------------------------------------------------
	// [각도 제한 로직]
	// -----------------------------------------------------------------
	FRotator CurrentControlRot = Controller->GetControlRotation();

	// 3. 좌우(Yaw) 시야각 제한
	float BodyFrontYaw = ParentBodyCharacter->GetActorRotation().Yaw + 180.0f;
	float RelativeYaw = FRotator::NormalizeAxis(CurrentControlRot.Yaw - BodyFrontYaw);
	float ClampedYaw = FMath::Clamp(RelativeYaw, -90.0f, 90.0f);

	// 4. 위아래(Pitch) 시야각 제한 (-90 ~ 90)
	float CurrentPitch = FRotator::NormalizeAxis(CurrentControlRot.Pitch);
	float ClampedPitch = FMath::Clamp(CurrentPitch, -90.0f, 90.0f);

	// 5. 제한된 각도 적용
	bool bYawChanged = !FMath::IsNearlyEqual(RelativeYaw, ClampedYaw);
	bool bPitchChanged = !FMath::IsNearlyEqual(CurrentPitch, ClampedPitch);

	if (bYawChanged || bPitchChanged)
	{
		CurrentControlRot.Yaw = BodyFrontYaw + ClampedYaw;
		CurrentControlRot.Pitch = ClampedPitch;
		Controller->SetControlRotation(CurrentControlRot);
	}
}

void AUpperBodyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (LookAction)
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AUpperBodyPawn::Look);

		if (AttackAction)
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AUpperBodyPawn::Attack);

		if (InteractAction)
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AUpperBodyPawn::Interact);
	}
}

void AUpperBodyPawn::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y * -1.0f); // 마우스 Y축 반전 (위로 올리면 위를 봄)

	if (ParentBodyCharacter)
	{
		// 애니메이션 블루프린트로 회전값을 전달 (허리 비틀기용)
		ParentBodyCharacter->SetUpperBodyRotation(GetControlRotation());
	}
}

void AUpperBodyPawn::Attack(const FInputActionValue& Value)
{
	// [네트워크] 서버에 공격 요청 (로컬에서 바로 실행 X)
	Server_RequestAttack();
}

// [네트워크] 서버 RPC 구현 (실제 로직은 여기서)
void AUpperBodyPawn::Server_RequestAttack_Implementation()
{
	if (!ParentBodyCharacter)
	{
		ParentBodyCharacter = Cast<APlayerCharacter>(GetAttachParentActor());
	}

	if (ParentBodyCharacter)
	{
		// 서버가 PlayerCharacter의 멀티캐스트 함수를 호출 -> 모든 클라이언트에 전파
		ParentBodyCharacter->Multi_PlayAttack();
	}
}

void AUpperBodyPawn::Interact(const FInputActionValue& Value)
{
	// =================================================================
	// [테스트 1] 입력 확인용 메시지 출력
	// =================================================================
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("E Key Pressed!"));
	}

	// 1. 부모(몸통) 캐릭터 확인
	if (!ParentBodyCharacter)
	{
		ParentBodyCharacter = Cast<APlayerCharacter>(GetAttachParentActor());
		if (!ParentBodyCharacter) return;
	}

	// =================================================================
	// [수정됨] 레이캐스트 시작점: 카메라 -> 머리 메쉬의 'head' 소켓 위치
	// =================================================================
	FVector Start;

	// 'head'라는 뼈(소켓)가 존재하면 그 위치를 사용
	if (ParentBodyCharacter->GetMesh() && ParentBodyCharacter->GetMesh()->DoesSocketExist(TEXT("head")))
	{
		Start = ParentBodyCharacter->HeadMountPoint->GetComponentLocation();
	}
	else
	{
		// 만약 소켓을 못 찾으면 안전하게 카메라 위치 사용 (혹은 로그 출력)
		Start = FrontCamera->GetComponentLocation();
		UE_LOG(LogTemp, Warning, TEXT("Cannot find 'head' socket on HeadMesh. Using Camera location instead."));
	}

	// 방향: 카메라는 계속 정면을 보고 있으므로, 카메라의 Forward Vector를 사용합니다.
	FVector End = Start + (FrontCamera->GetForwardVector() * InteractionDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(ParentBodyCharacter);

	// 3. 레이캐스트 발사
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	// =================================================================
	// [테스트 2] 레이캐스트 시각화 (레이저 쏘기)
	// =================================================================
	if (bHit)
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 2.0f, 0, 1.0f);
		DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 10.0f, 12, FColor::Green, false, 2.0f);
	}
	else
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.0f, 0, 1.0f);
	}

	// 4. 아이템 확인 및 상호작용
	if (bHit)
	{
		if (ADropItem* DetectedItem = Cast<ADropItem>(HitResult.GetActor()))
		{
			// 화면에 감지된 아이템 이름 출력
			if (GEngine)
			{
				FString Msg = FString::Printf(TEXT("Item Detected: %s"), *DetectedItem->GetName());
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, Msg);
			}

			// 아이템 획득 시도
			DetectedItem->Interact(ParentBodyCharacter);
		}
	}
}