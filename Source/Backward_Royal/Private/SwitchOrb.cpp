// SwitchOrb.cpp
#include "SwitchOrb.h"
#include "Components/SphereComponent.h"
#include "PlayerCharacter.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "BRPlayerState.h"
#include "BRGameState.h"

DEFINE_LOG_CATEGORY(LogSwitchOrb);

ASwitchOrb::ASwitchOrb()
{
    // 1. 충돌체 설정
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    RootComponent = CollisionSphere;
    CollisionSphere->SetSphereRadius(100.0f);
    CollisionSphere->SetCollisionProfileName(TEXT("Trigger"));

    // 2. 나이아가라 컴포넌트 설정
    OrbNiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OrbNiagaraComp"));
    OrbNiagaraComp->SetupAttachment(RootComponent);

    bReplicates = true;
}

void ASwitchOrb::BeginPlay()
{
    Super::BeginPlay();

    // 서버에서만 오버랩 이벤트를 바인딩
    if (HasAuthority())
    {
        CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASwitchOrb::OnOrbOverlap);
    }
}

void ASwitchOrb::OnOrbOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    FString DebugMsg = FString::Printf(TEXT("Orb Touched by: %s (Authority: %s)"),
        *GetNameSafe(OtherActor),
        HasAuthority() ? TEXT("Server") : TEXT("Client"));

    // Key: -1(새 줄), Time: 5초, Color: Red
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, DebugMsg);

    // 로그도 남김
    UE_LOG(LogSwitchOrb, Log, TEXT("%s"), *DebugMsg);

    // 1. 권한 및 대상 확인
    if (!HasAuthority())
    {
        // 클라이언트라서 리턴되는 경우는 정상입니다 (서버에서만 처리)
        return;
    }

    if (!OtherActor) return;

    APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(OtherActor);
    if (!PlayerChar)
    {
        UE_LOG(LogSwitchOrb, Warning, TEXT("실패: 대상이 APlayerCharacter가 아님 (%s)"), *GetNameSafe(OtherActor));
        return;
    }

    ABRPlayerState* MyPS = PlayerChar->GetPlayerState<ABRPlayerState>();

    // [중요 체크포인트] PlayerState 및 연결된 플레이어 확인
    if (!MyPS)
    {
        UE_LOG(LogSwitchOrb, Warning, TEXT("실패: PlayerState가 없음"));
        return;
    }

    if (MyPS->ConnectedPlayerIndex == -1)
    {
        UE_LOG(LogSwitchOrb, Error, TEXT("실패: ConnectedPlayerIndex가 -1임 (파트너 없음). 팀 배정이 되었나요?"));
        return;
    }

    // 파트너 찾기
    ABRGameState* GS = GetWorld()->GetGameState<ABRGameState>();
    if (!GS || !GS->PlayerArray.IsValidIndex(MyPS->ConnectedPlayerIndex))
    {
        UE_LOG(LogSwitchOrb, Error, TEXT("실패: GameState가 없거나 인덱스가 범위를 벗어남 (Index: %d, ArrayNum: %d)"),
            MyPS->ConnectedPlayerIndex, (GS ? GS->PlayerArray.Num() : -1));
        return;
    }

    ABRPlayerState* PartnerPS = Cast<ABRPlayerState>(GS->PlayerArray[MyPS->ConnectedPlayerIndex]);
    if (!PartnerPS)
    {
        UE_LOG(LogSwitchOrb, Error, TEXT("실패: 파트너 PlayerState 캐스팅 실패"));
        return;
    }

    // [Step 1] 논리적 데이터 변경 (상체 <-> 하체)
    bool MyNewRole = !MyPS->bIsLowerBody;
    bool PartnerNewRole = !PartnerPS->bIsLowerBody;

    MyPS->SetPlayerRole(MyNewRole, MyPS->ConnectedPlayerIndex);
    PartnerPS->SetPlayerRole(PartnerNewRole, PartnerPS->ConnectedPlayerIndex);

    // [Step 2] 실질적인 컨트롤러 스왑 및 부착 재설정 실행
    // 이 함수는 PlayerState에 새로 구현합니다.
    MyPS->SwapControlWithPartner();

    ORB_LOG(Log, TEXT("Orb 조작: %s와 %s의 역할 및 제어권 스왑 실행"), *MyPS->GetPlayerName(), *PartnerPS->GetPlayerName());

    Destroy();
}