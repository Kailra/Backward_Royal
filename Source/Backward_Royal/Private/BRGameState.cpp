// BRGameState.cpp
#include "BRGameState.h"
#include "BRPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"

ABRGameState::ABRGameState()
{
	PlayerCount = 0;
	bCanStartGame = false;
}

void ABRGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABRGameState, PlayerCount);
	DOREPLIFETIME(ABRGameState, bCanStartGame);
}

void ABRGameState::BeginPlay()
{
	Super::BeginPlay();
}

void ABRGameState::UpdatePlayerList()
{
	if (HasAuthority())
	{
		int32 OldCount = PlayerCount;
		PlayerCount = PlayerArray.Num();
		if (OldCount != PlayerCount)
		{
			UE_LOG(LogTemp, Log, TEXT("[플레이어 목록] 업데이트: %d -> %d명"), OldCount, PlayerCount);
		}
		OnRep_PlayerCount();
		CheckCanStartGame();
		OnPlayerListChanged.Broadcast();
	}
}

void ABRGameState::CheckCanStartGame()
{
	if (HasAuthority())
	{
		bool bCanStart = (PlayerCount >= MinPlayers && PlayerCount <= MaxPlayers && AreAllPlayersReady());
		if (bCanStart != bCanStartGame)
		{
			bCanStartGame = bCanStart;
			if (bCanStart)
			{
				UE_LOG(LogTemp, Log, TEXT("[게임 시작] 조건 만족: 게임 시작 가능"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[게임 시작] 조건 불만족: 플레이어 수=%d/%d-%d, 모든 준비=%s"), 
					PlayerCount, MinPlayers, MaxPlayers,
					AreAllPlayersReady() ? TEXT("예") : TEXT("아니오"));
			}
			OnRep_CanStartGame();
		}
	}
}

void ABRGameState::AssignRandomTeams()
{
	if (!HasAuthority())
		return;

	UE_LOG(LogTemp, Log, TEXT("[랜덤 팀 배정] 시작"));
	TArray<ABRPlayerState*> Players;
	for (APlayerState* PS : PlayerArray)
	{
		if (ABRPlayerState* BRPS = Cast<ABRPlayerState>(PS))
		{
			Players.Add(BRPS);
		}
	}

	int32 NumPlayers = Players.Num();
	UE_LOG(LogTemp, Log, TEXT("[랜덤 팀 배정] 총 %d명의 플레이어"), NumPlayers);

	// 플레이어를 섞기
	for (int32 i = NumPlayers - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		Players.Swap(i, j);
	}

	// 2인 1조로 팀 배정
	for (int32 i = 0; i < NumPlayers; i++)
	{
		int32 TeamNumber = (i / 2) + 1; // 0,1 -> 팀1, 2,3 -> 팀2, ...
		FString PlayerName = Players[i]->GetPlayerName();
		if (PlayerName.IsEmpty())
		{
			PlayerName = FString::Printf(TEXT("Player %d"), i + 1);
		}
		Players[i]->SetTeamNumber(TeamNumber);
		UE_LOG(LogTemp, Log, TEXT("[랜덤 팀 배정] %s -> 팀 %d"), *PlayerName, TeamNumber);
	}

	UE_LOG(LogTemp, Log, TEXT("[랜덤 팀 배정] 완료: 총 %d개 팀 생성"), (NumPlayers + 1) / 2);
	OnTeamChanged.Broadcast();
}

bool ABRGameState::AreAllPlayersReady() const
{
	if (PlayerCount < MinPlayers)
		return false;

	for (APlayerState* PS : PlayerArray)
	{
		if (ABRPlayerState* BRPS = Cast<ABRPlayerState>(PS))
		{
			if (!BRPS->bIsReady)
				return false;
		}
	}

	return true;
}

void ABRGameState::OnRep_PlayerCount()
{
	OnPlayerListChanged.Broadcast();
}

void ABRGameState::OnRep_CanStartGame()
{
	// UI 업데이트를 위한 이벤트 발생 가능
}

