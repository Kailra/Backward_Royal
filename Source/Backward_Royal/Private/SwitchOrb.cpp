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

    // 플레이어 직접 오버랩 + 무기/부착물·자식 액터 오버랩 시 소유자/부모로 플레이어 찾기 (인식 불안정 완화)
    APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(OtherActor);
    if (!PlayerChar)
    {
        PlayerChar = Cast<APlayerCharacter>(OtherActor->GetOwner());
        if (!PlayerChar)
            PlayerChar = Cast<APlayerCharacter>(OtherActor->GetInstigator());
        if (!PlayerChar && OtherActor->GetRootComponent())
        {
            for (AActor* Parent = OtherActor->GetAttachParentActor(); Parent; Parent = Parent->GetAttachParentActor())
            {
                PlayerChar = Cast<APlayerCharacter>(Parent);
                if (PlayerChar) break;
            }
        }
    }
    if (!PlayerChar)
    {
        UE_LOG(LogSwitchOrb, Warning, TEXT("실패: 대상이 APlayerCharacter가 아님 (%s)"), *GetNameSafe(OtherActor));
        return;
    }

    ABRPlayerState* MyPS = PlayerChar->GetPlayerState<ABRPlayerState>();

    // [중요 체크포인트] PlayerState 확인
    if (!MyPS)
    {
        UE_LOG(LogSwitchOrb, Warning, TEXT("실패: PlayerState가 없음"));
        return;
    }

    // 관전이거나 팀 없음(TeamID 0)이면 파트너 없음
    if (MyPS->bIsSpectatorSlot || MyPS->TeamNumber <= 0)
    {
        UE_LOG(LogSwitchOrb, Error, TEXT("실패: 관전이거나 팀이 없음 (TeamID=%d). 팀 배정이 되었나요?"), MyPS->TeamNumber);
        return;
    }

    ABRGameState* GS = GetWorld()->GetGameState<ABRGameState>();
    if (!GS)
    {
        UE_LOG(LogSwitchOrb, Error, TEXT("실패: GameState가 없음"));
        return;
    }

    // 파트너 찾기: 같은 TeamID(TeamNumber) + 반대 역할(하체↔상체). PlayerIndex(슬롯) 기준으로 같은 팀의 다른 1P/2P
    ABRPlayerState* PartnerPS = nullptr;
    int32 MyIndex = INDEX_NONE;
    int32 PartnerIndex = INDEX_NONE;
    for (int32 i = 0; i < GS->PlayerArray.Num(); i++)
    {
        ABRPlayerState* Other = Cast<ABRPlayerState>(GS->PlayerArray[i]);
        if (!Other || Other == MyPS) continue;
        if (Other->TeamNumber != MyPS->TeamNumber) continue;
        if (Other->bIsSpectatorSlot) continue;
        // 같은 팀에서 하체/상체가 반대인 유일한 한 명 = 파트너
        if (Other->bIsLowerBody != MyPS->bIsLowerBody)
        {
            PartnerPS = Other;
            MyIndex = GS->PlayerArray.Find(MyPS);
            PartnerIndex = i;
            break;
        }
    }

    if (!PartnerPS || MyIndex == INDEX_NONE || PartnerIndex == INDEX_NONE)
    {
        UE_LOG(LogSwitchOrb, Error, TEXT("실패: 같은 팀의 파트너(반대 역할)를 찾을 수 없음 (TeamID=%d)"), MyPS->TeamNumber);
        return;
    }

    // [Step 1] 논리적 데이터 변경 (상체 <-> 하체). SetPlayerRole 시 상호 인덱스 전달해 ConnectedPlayerIndex 동기화
    bool MyNewRole = !MyPS->bIsLowerBody;
    bool PartnerNewRole = !PartnerPS->bIsLowerBody;

    MyPS->SetPlayerRole(MyNewRole, PartnerIndex);
    PartnerPS->SetPlayerRole(PartnerNewRole, MyIndex);

    // [Step 2] 실질적인 컨트롤러 스왑 및 부착 재설정 실행
    // 이 함수는 PlayerState에 새로 구현합니다.
    MyPS->SwapControlWithPartner();

    ORB_LOG(Log, TEXT("Orb 조작: %s와 %s의 역할 및 제어권 스왑 실행"), *MyPS->GetPlayerName(), *PartnerPS->GetPlayerName());

    Destroy();
}