#include "PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"    
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Net/UnrealNetwork.h" // 리플리케이션 필수 헤더

DEFINE_LOG_CATEGORY(LogPlayerChar);

APlayerCharacter::APlayerCharacter()
{
    // 1. [초기화 상태] 탱크/말 방식 조작 설정 (하체 플레이어 기준)
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;  // 마우스 회전대로 몸통도 같이 돕니다 (초기 상태)
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

    // =================================================================
    // 2. [핵심] 하체 플레이어 시야 가림 방지 (다리 투명화)
    // =================================================================

    // 이 캐릭터의 주인(Owner, 하체 플레이어) 화면에서만 메쉬를 렌더링하지 않습니다.
    // 상체 플레이어는 Owner가 아니므로 다리가 정상적으로 보입니다.
    GetMesh()->SetOwnerNoSee(true);

    // 몸은 안 보여도 그림자는 바닥에 생기게 합니다 (현실감 유지)
    GetMesh()->bCastHiddenShadow = true;

    // =================================================================
    // 3. 카메라 및 부착 지점 설정
    // =================================================================

    // 후방 카메라 (하체 플레이어용)
    RearCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("RearCameraBoom"));
    RearCameraBoom->SetupAttachment(RootComponent);
    RearCameraBoom->TargetArmLength = 0.0f; // 1인칭 시점 (몸통 내부)

    // 카메라 붐은 컨트롤러 회전을 따라가게 설정
    RearCameraBoom->bUsePawnControlRotation = true;
    RearCameraBoom->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

    RearCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("RearCamera"));
    RearCamera->SetupAttachment(RearCameraBoom, USpringArmComponent::SocketName);
    RearCamera->bUsePawnControlRotation = false;

    // 상체 플레이어(AUpperBodyPawn)가 붙을 위치
    HeadMountPoint = CreateDefaultSubobject<USceneComponent>(TEXT("HeadMountPoint"));
    HeadMountPoint->SetupAttachment(GetCapsuleComponent());
    HeadMountPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f)); // 눈높이

    // =================================================================
    // 4. 네트워크 설정
    // =================================================================
    bReplicates = true;
    NetUpdateFrequency = 144.0f;
    MinNetUpdateFrequency = 100.0f;
    GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }
}

// [네트워크] 변수 동기화 규칙
void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 상체 조준 각도(UpperBodyAimRotation)를 서버<->클라이언트 동기화
    DOREPLIFETIME(APlayerCharacter, UpperBodyAimRotation);
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveAction)
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);

        if (LookAction)
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);

        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        }
    }
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // W/S: 전후 이동
        if (MovementVector.Y != 0.0f)
        {
            const FRotator Rotation = Controller->GetControlRotation();
            const FRotator YawRotation(0, Rotation.Yaw, 0);
            const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

            AddMovementInput(ForwardDirection, MovementVector.Y);
        }

        // A/D: 좌우 이동 (현재 설정상 회전하며 이동)
        if (MovementVector.X != 0.0f)
        {
            AddControllerYawInput(MovementVector.X);
        }
    }
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();
    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

// 상체 플레이어가 본인의 시선 각도를 이 함수를 통해 하체에 전달합니다.
void APlayerCharacter::SetUpperBodyRotation(FRotator NewRotation)
{
    UpperBodyAimRotation = NewRotation;
}

// [네트워크] 공격 애니메이션 멀티캐스트
void APlayerCharacter::Multi_PlayAttack_Implementation()
{
    TriggerUpperBodyAttack();
}