#include "StaminaComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

UStaminaComponent::UStaminaComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicated(true);
}

void UStaminaComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        CurrentStamina = MaxStamina;
    }
}

void UStaminaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UStaminaComponent, CurrentStamina);
    DOREPLIFETIME(UStaminaComponent, bIsSprinting);
}

void UStaminaComponent::ServerSetSprinting_Implementation(bool bNewSprinting)
{
    // 상태가 변했을 때만 처리
    if (bIsSprinting != bNewSprinting)
    {
        bIsSprinting = bNewSprinting;

        // 서버는 OnRep이 자동 호출되지 않으므로 수동 호출하여 로직 실행
        OnRep_IsSprinting();
    }
}

void UStaminaComponent::OnRep_IsSprinting()
{
    // 캐릭터에게 "상태가 변했으니 속도를 조절해라"라고 알림
    OnSprintStateChanged.Broadcast(bIsSprinting);
}

void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 서버에서만 연산
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        bool bActuallyMoving = GetOwner()->GetVelocity().SizeSquared() > 10.0f;

        if (bIsSprinting && bActuallyMoving)
        {
            CurrentStamina -= StaminaDrainRate * DeltaTime;

            if (CurrentStamina <= 0.0f)
            {
                CurrentStamina = 0.0f;
                bIsSprinting = false;

                // 캐릭터에게 "너 이제 못 달려"라고 알림
                OnSprintStateChanged.Broadcast(false);
            }
        }
        else
        {
            if (CurrentStamina < MaxStamina)
            {
                CurrentStamina += StaminaRegenRate * DeltaTime;

                // 완전히 회복되었을 때 알림 등 추가 가능
                if (CurrentStamina > MaxStamina) CurrentStamina = MaxStamina;
            }
        }

        // 값 변경 시 UI 업데이트 (서버도 UI 갱신 필요 시)
        // OnRep_CurrentStamina()는 클라에서만 자동 호출되므로 서버는 수동 호출하거나
        // 값이 크게 변했을 때만 Broadcast
    }
}

void UStaminaComponent::OnRep_CurrentStamina()
{
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

float UStaminaComponent::GetStaminaRatio() const
{
    return (MaxStamina > 0.f) ? (CurrentStamina / MaxStamina) : 0.f;
}