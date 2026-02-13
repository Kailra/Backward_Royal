#include "PlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"	
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "BRPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "BRGameInstance.h"
#include "BRPlayerState.h"
#include "BRGameState.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY(LogPlayerChar);

#define LOG_PLAYER(Verbosity, Format, ...) \
    UE_LOG(LogPlayerChar, Verbosity, TEXT("%s - %s"), *FString(__FUNCTION__), *FString::Printf(Format, ##__VA_ARGS__))

APlayerCharacter::APlayerCharacter()
{
	// [기본 설정 유지]
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	//GetMesh()->SetOwnerNoSee(true); // <- 몸 투명화
	GetMesh()->bCastHiddenShadow = true;
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	TArray<USkeletalMeshComponent*> ArmorParts = { HeadMesh, ChestMesh, HandMesh, LegMesh, FootMesh };

	for (USkeletalMeshComponent* Part : ArmorParts)
	{
		if (Part)
		{
			//Part->SetOwnerNoSee(true);   //  <- 몸 투명화
			Part->bCastHiddenShadow = true;

			// A. [틱 순서 고정] 
			// "몸통(GetMesh)의 애니메이션/위치 계산이 완전히 끝난 뒤에 -> 갑옷(Part)을 처리해라"
			// 이 설정이 없으면 몸통은 움직였는데 갑옷은 제자리에 있는 프레임이 섞여서 덜덜 떨립니다.
			Part->AddTickPrerequisiteComponent(GetMesh());

			// B. [부착 관계 확인]
			// 갑옷은 반드시 'Root'나 'Capsule'이 아니라 'GetMesh()'에 붙어있어야 합니다.
			// 왜냐하면 'GetMesh()'에는 네트워크 위치 보정(Smoothing)이 적용되는데, 
			// 캡슐에 붙어있으면 이 보정을 못 받아서 갑옷만 따로 놉니다.
			if (Part->GetAttachParent() != GetMesh())
			{
				Part->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale);
				UE_LOG(LogTemp, Warning, TEXT("[%s] 파츠가 GetMesh에 붙어있지 않아 강제로 재부착했습니다."), *Part->GetName());
			}

			// C. [Leader Pose 재확인]
			// 혹시라도 풀렸을 경우를 대비해 다시 연결
			Part->SetLeaderPoseComponent(GetMesh());
		}
	}

	// 1. 카메라 설정
	RearCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("RearCameraBoom"));
	RearCameraBoom->SetupAttachment(RootComponent);
	RearCameraBoom->TargetArmLength = 0.0f;
	RearCameraBoom->bUsePawnControlRotation = false;
	RearCameraBoom->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	RearCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("RearCamera"));
	RearCamera->SetupAttachment(RearCameraBoom, USpringArmComponent::SocketName);
	RearCamera->bUsePawnControlRotation = false;

	// 2. 상체 부착 지점
	HeadMountPoint = CreateDefaultSubobject<USceneComponent>(TEXT("HeadMountPoint"));
	HeadMountPoint->SetupAttachment(GetCapsuleComponent());
	HeadMountPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));

	// 3. [신규] 스태미나 컴포넌트 생성
	StaminaComp = CreateDefaultSubobject<UStaminaComponent>(TEXT("StaminaComp"));

	// [네트워크] 클라이언트 예측/서버 보정 및 스무딩 튜닝 (고지연·패킷유실 환경 대응)
	bReplicates = true;
	SetNetUpdateFrequency(144.0f);
	SetMinNetUpdateFrequency(100.0f);

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	// 고지연(100ms+) 환경: 보정 허용 거리 확대 → 불필요한 보정·덜컹임 감소
	MoveComp->NetworkMaxSmoothUpdateDistance = 256.0f;   // 이 거리 이하만 스무딩, 그 이상은 보정 허용
	MoveComp->NetworkNoSmoothUpdateDistance = 0.0f;    // 0 = 작은 오차도 스무딩으로 흡수
	// 서버-클라이언트 위치 오차가 이 값(단위: cm) 이하면 보정 생략 → 핑 높을 때 덜 튐
	MoveComp->NetworkLargeClientCorrectionDistance = 120.0f;
}

void APlayerCharacter::BeginPlay()
{
	//GetMesh()->SetVisibility(false, false); //  <- 몸 투명화

	if (StaminaComp)
	{
		StaminaComp->OnStaminaChanged.AddDynamic(this, &APlayerCharacter::HandleStaminaChanged);
		StaminaComp->OnSprintStateChanged.AddDynamic(this, &APlayerCharacter::HandleSprintStateChanged);
	}

	Super::BeginPlay();

	if (HasAuthority())
	{
		UpperBodyAimRotation = GetActorRotation();
	}

	// [기존 코드] 입력 시스템 등록
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

	ABRPlayerState* BRPS = GetPlayerState<ABRPlayerState>();

	if (BRPS)
	{
		// 2. 이미 PlayerState가 있다면 초기화 로직 실행
		// (OnRep_PlayerState 내부에서 TryApplyCustomization 바인딩 및 호출을 수행하도록 구성했다면 이 함수 호출만으로 충분합니다)
		OnRep_PlayerState();
	}
	else
	{
		// 3. PlayerState가 아직 없다면(클라이언트 로딩 지연 등), 0.5초 뒤 재시도
		FTimerHandle RetryHandle;
		GetWorld()->GetTimerManager().SetTimer(RetryHandle, [this]()
			{
				// 람다 내부에서 다시 확인
				if (ABRPlayerState* RetryPS = GetPlayerState<ABRPlayerState>())
				{
					// 재시도 성공 시 초기화 실행
					// OnRep_PlayerState를 수동으로 호출하여 바인딩/적용 로직을 수행
					OnRep_PlayerState();

					UE_LOG(LogTemp, Log, TEXT("[Character] BeginPlay: PlayerState 뒤늦게 로드됨 -> 초기화 수행"));
				}
			}, 0.5f, false);
	}
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 부모 클래스 호출 (안전을 위해)
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Move, Look, Jump 등 기존 바인딩 유지
		if (MoveAction)
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);

		if (LookAction)
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);

		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}

		// [수정] 달리기
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::SprintStart);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::SprintEnd);
		}
	}
}

// [수정] 컴포넌트에 달리기 요청
void APlayerCharacter::SprintStart(const FInputActionValue& Value)
{
	if (StaminaComp)
	{
		StaminaComp->ServerSetSprinting(true);

	}
}

// [수정] 컴포넌트에 멈춤 요청
void APlayerCharacter::SprintEnd(const FInputActionValue& Value)
{
	if (StaminaComp)
	{
		StaminaComp->ServerSetSprinting(false);
	}
}

// [신규] 컴포넌트가 "달리기 상태 변경"을 알릴 때 호출됨
void APlayerCharacter::HandleSprintStateChanged(bool bCanSprint)
{
	// 스태미나가 바닥나서 bCanSprint가 false로 오거나, 
	// 다시 회복되어 true로 올 수 있음.
	// 하지만 여기서는 "현재 달리고 있는가"에 대한 결과에 따라 속도를 맞춥니다.

	// StaminaComponent 로직에 따라 bIsSprinting 상태를 확인하여 속도 적용
	// (단, 여기서는 파라미터를 단순화해서 bCanSprint가 false면 걷게 함)

	if (StaminaComp && StaminaComp->bIsSprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

// [신규] 컴포넌트 스태미나 변화 -> 캐릭터 델리게이트로 중계 -> 위젯이 수신
void APlayerCharacter::HandleStaminaChanged(float CurrentVal, float MaxVal)
{
	if (OnStaminaChanged.IsBound())
	{
		OnStaminaChanged.Broadcast(CurrentVal, MaxVal);
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// 카메라 기준 앞/오른쪽 방향
		const FVector CameraForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector CameraRight = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// [신규 로직 시작] ----------------------------------------------------------

		// 1. 플레이어가 입력한 키(W,A,S,D)가 실제 월드에서 어느 방향인지 계산
		FVector IntentDir = (CameraForward * MovementVector.Y + CameraRight * MovementVector.X).GetSafeNormal();

		// 2. 내적(Dot Product) 계산
		// 캐릭터의 정면(GetActorForwardVector)과 이동하려는 방향(IntentDir)을 비교
		// 결과가 양수(+)면 캐릭터가 보는 방향으로 걷는 것이고, 음수(-)면 뒷걸음질 치는 것
		float Dot = FVector::DotProduct(IntentDir, GetActorForwardVector());

		float SpeedScale = 1.0f;

		// 3. 캐릭터가 바라보는 방향으로 움직일 때 (내적이 0보다 클 때)
		// 현재 캐릭터는 카메라 등 뒤를 보고 있으므로, S키를 눌러 카메라 쪽으로 올 때가 여기에 해당됨
		if (Dot > 0.1f)
		{
			SpeedScale = 0.5f; // 0.5배속 적용
		}
		// [신규 로직 끝] ------------------------------------------------------------

		// 계산된 배율(SpeedScale)을 곱해서 이동 적용
		AddMovementInput(CameraForward, MovementVector.Y * SpeedScale);
		AddMovementInput(CameraRight, MovementVector.X * SpeedScale);
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

void APlayerCharacter::Jump()
{
	// 스태미나가 충분할 때만 점프 시도 (불필요한 입력 방지)
	if (StaminaComp && StaminaComp->CanJump())
	{
		Super::Jump();
	}
}

// 2. 엔진 내부에서 점프 가능 여부를 판단할 때 (서버/클라이언트 모두 호출됨)
bool APlayerCharacter::CanJumpInternal_Implementation() const
{
	// 기존 조건(바닥에 있는지 등) 체크
	bool bCanJump = Super::CanJumpInternal_Implementation();

	// 스태미나 조건 추가
	if (StaminaComp)
	{
		bCanJump = bCanJump && StaminaComp->CanJump();
	}

	return bCanJump;
}

// 3. 실제로 점프가 이루어졌을 때 (MovementComponent가 호출)
void APlayerCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	// 서버인 경우에만 스태미나 소모
	// (클라이언트의 점프 움직임이 서버에 도달하여 처리될 때 호출됨)
	if (HasAuthority() && StaminaComp)
	{
		StaminaComp->ConsumeJumpStamina();
	}
}

void APlayerCharacter::SetUpperBodyRotation(FRotator NewRotation)
{
	UpperBodyAimRotation = NewRotation;
}

FRotator APlayerCharacter::GetBaseAimRotation() const
{
	return UpperBodyAimRotation;
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
		MoveComp->Activate();
	}
}

void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APlayerCharacter, UpperBodyAimRotation);
}

void APlayerCharacter::Restart()
{
	Super::Restart();
}

void APlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (IsLocallyControlled())
	{
		if (ABRPlayerController* PC = Cast<ABRPlayerController>(GetController()))
		{
			PC->SetupRoleInput(true);
		}
	}

	ABRPlayerState* MyPS = Cast<ABRPlayerState>(GetPlayerState());
	if (MyPS)
	{
		// 1-1. 내 커마 정보가 오면 알려줘
		MyPS->OnCustomizationDataChanged.RemoveDynamic(this, &APlayerCharacter::TryApplyCustomization);
		MyPS->OnCustomizationDataChanged.AddDynamic(this, &APlayerCharacter::TryApplyCustomization);

		// 1-2. 내 역할(상/하체)이나 파트너가 정해지면 알려줘
		// (기존 코드에 OnPlayerRoleChanged 델리게이트가 이미 있다고 가정)
		MyPS->OnPlayerRoleChanged.AddDynamic(this, &APlayerCharacter::BindToPartnerPlayerState);

		// 혹시 이미 데이터가 와 있을 수도 있으니 한번 체크
		TryApplyCustomization();

		// 혹시 이미 파트너가 정해져 있을 수도 있으니 체크
		if (MyPS->ConnectedPlayerIndex != -1)
		{
			BindToPartnerPlayerState(MyPS->bIsLowerBody);
		}
	}

	UpdateHPUI();
}

// 상체 플레이어 찾기 (포인터 우선)
ABRPlayerState* APlayerCharacter::GetUpperBodyPlayerState() const
{
	ABRPlayerState* MyPS = Cast<ABRPlayerState>(GetPlayerState());
	if (!MyPS) return nullptr;

	// 1. 내가 상체면 -> 나 자신 리턴
	if (!MyPS->bIsLowerBody) return MyPS;

	// 2. [핵심] 내가 하체면 -> PartnerPlayerState 포인터 확인 (가장 확실함)
	if (MyPS->PartnerPlayerState)
	{
		return MyPS->PartnerPlayerState;
	}
	return nullptr;
}

// 하체 플레이어 찾기 (포인터 우선)
ABRPlayerState* APlayerCharacter::GetLowerBodyPlayerState() const
{
	ABRPlayerState* MyPS = Cast<ABRPlayerState>(GetPlayerState());
	if (!MyPS) return nullptr;

	// 1. 내가 하체면 -> 나 자신 리턴
	if (MyPS->bIsLowerBody) return MyPS;

	// 2. [핵심] 내가 상체면 -> PartnerPlayerState 포인터 확인
	if (MyPS->PartnerPlayerState)
	{
		return MyPS->PartnerPlayerState;
	}

	return nullptr;
}

void APlayerCharacter::TryApplyCustomization()
{
	// [유지] SwitchOrb 등으로 PlayerState가 교체될 때 재적용을 막기 위한 보호막
	if (bUpperBodyApplied && bLowerBodyApplied) return;

	ABRPlayerState* UpperPS = GetUpperBodyPlayerState();
	ABRPlayerState* LowerPS = GetLowerBodyPlayerState();

	// --- 1. 상체 적용 ---
	if (!bUpperBodyApplied && UpperPS)
	{
		// [핵심 수정] 데이터가 유효한지(bIsDataValid) 확인
		if (UpperPS->CustomizationData.bIsDataValid)
		{
			// 진짜 데이터가 도착했으므로 적용하고 잠금(Lock)
			ApplyMeshFromID(EArmorSlot::Head, UpperPS->CustomizationData.HeadID);
			ApplyMeshFromID(EArmorSlot::Chest, UpperPS->CustomizationData.ChestID);
			ApplyMeshFromID(EArmorSlot::Hands, UpperPS->CustomizationData.HandID);

			bUpperBodyApplied = true; // 유효한 데이터이므로 이제 잠금
			LOG_PLAYER(Display, TEXT("Upper Body Applied & Locked (Valid Data)"));
		}
		else
		{
			// 아직 데이터가 도착 안 함 (ID가 0인 상태). 
			// 투명 버그 방지를 위해 기본값은 입혀주되, 플래그는 잠그지 않음!
			ApplyMeshFromID(EArmorSlot::Head, 0);
			ApplyMeshFromID(EArmorSlot::Chest, 0);
			ApplyMeshFromID(EArmorSlot::Hands, 0);

			// bUpperBodyApplied = true; <--- 절대 true로 설정하지 않음. 나중에 진짜 데이터 오면 다시 진입해야 함.
			LOG_PLAYER(Warning, TEXT("Upper Body Waiting... (Invalid Data)"));
		}
	}

	// --- 2. 하체 적용 ---
	if (!bLowerBodyApplied && LowerPS)
	{
		if (LowerPS->CustomizationData.bIsDataValid)
		{
			ApplyMeshFromID(EArmorSlot::Legs, LowerPS->CustomizationData.LegID);
			ApplyMeshFromID(EArmorSlot::Feet, LowerPS->CustomizationData.FootID);

			bLowerBodyApplied = true; // 유효한 데이터이므로 이제 잠금
			LOG_PLAYER(Display, TEXT("Lower Body Applied & Locked (Valid Data)"));
		}
		else
		{
			// 데이터 대기 중: 기본값만 적용하고 잠금 해제 상태 유지
			ApplyMeshFromID(EArmorSlot::Legs, 0);
			ApplyMeshFromID(EArmorSlot::Feet, 0);

			LOG_PLAYER(Warning, TEXT("Lower Body Waiting... (Invalid Data)"));
		}
	}
}

void APlayerCharacter::BindToPartnerPlayerState(bool bIsLowerBody)
{
	ABRPlayerState* MyPS = Cast<ABRPlayerState>(GetPlayerState());
	if (!MyPS)
	{
		// PlayerState가 없으면 잠시 후 재시도
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_RetryBindPartner, [this, bIsLowerBody]() {
			BindToPartnerPlayerState(bIsLowerBody);
			}, 0.5f, false);
		return;
	}

	// --- 파트너 연결 로직 ---
	ABRPlayerState* PartnerPS = MyPS->PartnerPlayerState;

	// 1. 파트너가 있어야 하는데(Index != -1) 아직 포인터가 없는 경우 -> 로딩 대기
	if (!PartnerPS && MyPS->ConnectedPlayerIndex != -1)
	{
		// 아직 파트너 정보가 안 넘어왔으므로 재시도 예약
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_RetryBindPartner, [this, bIsLowerBody]() {
			BindToPartnerPlayerState(bIsLowerBody);
			}, 0.5f, false);
	}
	// 2. 파트너가 있는 경우 -> 이벤트 구독
	else if (PartnerPS)
	{
		// 중복 구독 방지 체크
		if (!bBoundToPartner)
		{
			PartnerPS->OnCustomizationDataChanged.RemoveDynamic(this, &APlayerCharacter::TryApplyCustomization);
			PartnerPS->OnCustomizationDataChanged.AddDynamic(this, &APlayerCharacter::TryApplyCustomization);

			bBoundToPartner = true;
			LOG_PLAYER(Display, TEXT("Bound to Partner Success: %s"), *PartnerPS->GetPlayerName());

			// 성공했으므로 재시도 타이머 해제
			GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RetryBindPartner);
		}
	}
	// 3. 파트너가 아예 없는 경우 (ConnectedIndex == -1)
	else
	{
		// 구독할 대상이 없으므로 타이머 해제
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_RetryBindPartner);
	}

	// [핵심] 파트너 연결 여부와 상관없이, 역할 정보가 갱신되었거나 함수가 호출되었으면
	// 커스터마이징 적용을 시도해야 함. (내가 상체라면 내 데이터를 가져와서 입힘)
	TryApplyCustomization();
}

void APlayerCharacter::ApplyMeshFromID(EArmorSlot Slot, int32 MeshID)
{
	// 1. GameInstance 가져오기
	UBRGameInstance* GI = Cast<UBRGameInstance>(GetGameInstance());
	if (!GI)
	{
		// 에디터 등에서 PIE 시작 전이거나 엣지 케이스
		return;
	}

	// 2. GameInstance의 맵에서 'ArmorData' 테이블 찾기
	UDataTable* ArmorDT = nullptr;
	if (GI->ConfigDataMap.Contains(TEXT("ArmorData")))
	{
		ArmorDT = GI->ConfigDataMap[TEXT("ArmorData")];
	}

	// 테이블이 없으면 중단
	if (!ArmorDT)
	{
		LOG_PLAYER(Error, TEXT("ArmorDataTable Not Found in GameInstance ConfigMap!"));
		return;
	}

	// 3. 타겟 컴포넌트 선정
	USkeletalMeshComponent* TargetMeshComp = nullptr;
	USkeletalMesh* MeshToApply = nullptr; // 적용할 메시를 담을 변수

	switch (Slot)
	{
	case EArmorSlot::Head:
		TargetMeshComp = HeadMesh;
		MeshToApply = DefaultHeadMesh; // 기본값 설정
		break;
	case EArmorSlot::Chest:
		TargetMeshComp = ChestMesh;
		MeshToApply = DefaultChestMesh;
		break;
	case EArmorSlot::Hands:
		TargetMeshComp = HandMesh;
		MeshToApply = DefaultHandMesh;
		break;
	case EArmorSlot::Legs:
		TargetMeshComp = LegMesh;
		MeshToApply = DefaultLegMesh;
		break;
	case EArmorSlot::Feet:
		TargetMeshComp = FootMesh;
		MeshToApply = DefaultFootMesh;
		break;
	}

	if (!TargetMeshComp) return;

	// 4. ID가 0인 경우 (장비 해제) -> 기본(맨몸) 메시 적용
	if (MeshID == 0)
	{
		TargetMeshComp->SetSkeletalMesh(MeshToApply);
		LOG_PLAYER(Display, TEXT("Applied Default Mesh for Slot %d"), (int32)Slot);
		return;
	}

	// 5. 데이터 검색
	FArmorData* FoundData = nullptr;

	static const FString ContextString(TEXT("ApplyMeshFromID_Search"));
	TArray<FArmorData*> AllRows;

	// 테이블의 모든 행을 가져옵니다.
	ArmorDT->GetAllRows<FArmorData>(ContextString, AllRows);

	// 반복문으로 하나씩 ID를 대조합니다.
	for (FArmorData* Row : AllRows)
	{
		if (Row && Row->ID == MeshID)
		{
			FoundData = Row;
			break;
		}
	}

	// 6. 적용
	if (FoundData && FoundData->ArmorMesh)
	{
		TargetMeshComp->SetSkeletalMesh(FoundData->ArmorMesh);
		LOG_PLAYER(Display, TEXT("Applied Mesh ID %d via GameInstance"), MeshID);
	}
	else
	{
		// ID는 있는데 데이터를 못 찾았다면 안전하게 기본 메시 적용
		TargetMeshComp->SetSkeletalMesh(MeshToApply);
	}
}

void APlayerCharacter::UpdatePreviewMesh(const FBRCustomizationData& NewData)
{
	// 각 부위별로 ApplyMeshFromID를 직접 호출 (기존 함수 재활용)
	ApplyMeshFromID(EArmorSlot::Head, NewData.HeadID);
	ApplyMeshFromID(EArmorSlot::Chest, NewData.ChestID);
	ApplyMeshFromID(EArmorSlot::Hands, NewData.HandID);
	ApplyMeshFromID(EArmorSlot::Legs, NewData.LegID);
	ApplyMeshFromID(EArmorSlot::Feet, NewData.FootID);

	// 로그 확인용
	// LOG_PLAYER(Display, TEXT("Preview Updated: HeadID %d"), NewData.HeadID);
}