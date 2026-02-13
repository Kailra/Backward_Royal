// BRCheatManager.cpp
#include "BRCheatManager.h"
#include "BRPlayerController.h"
#include "BRGameMode.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "BRPlayerState.h"
#include "BRGameState.h"
#include "BRGameInstance.h"

UBRCheatManager::UBRCheatManager()
{
}

void UBRCheatManager::CreateRoom(const FString& RoomName)
{
	UE_LOG(LogTemp, Log, TEXT("[CheatManager] CreateRoom 명령 실행: %s"), *RoomName);
	
	if (APlayerController* PC = GetPlayerController())
	{
		if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
		{
			BRPC->CreateRoom(RoomName);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CheatManager] BRPlayerController를 찾을 수 없습니다."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] PlayerController를 찾을 수 없습니다."));
	}
}

void UBRCheatManager::FindRooms()
{
	UE_LOG(LogTemp, Log, TEXT("[CheatManager] FindRooms 명령 실행"));
	
	if (APlayerController* PC = GetPlayerController())
	{
		if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
		{
			BRPC->FindRooms();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CheatManager] BRPlayerController를 찾을 수 없습니다."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] PlayerController를 찾을 수 없습니다."));
	}
}

void UBRCheatManager::JoinRoom(int32 SessionIndex)
{
	UE_LOG(LogTemp, Log, TEXT("[CheatManager] JoinRoom 명령 실행: SessionIndex=%d"), SessionIndex);
	
	if (APlayerController* PC = GetPlayerController())
	{
		if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
		{
			BRPC->JoinRoom(SessionIndex);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CheatManager] BRPlayerController를 찾을 수 없습니다."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] PlayerController를 찾을 수 없습니다."));
	}
}

void UBRCheatManager::ToggleReady()
{
	UE_LOG(LogTemp, Log, TEXT("[CheatManager] ToggleReady 명령 실행"));
	
	if (APlayerController* PC = GetPlayerController())
	{
		if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
		{
			BRPC->ToggleReady();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CheatManager] BRPlayerController를 찾을 수 없습니다."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] PlayerController를 찾을 수 없습니다."));
	}
}

void UBRCheatManager::RandomTeams()
{
	UE_LOG(LogTemp, Log, TEXT("[CheatManager] RandomTeams 명령 실행"));
	
	if (APlayerController* PC = GetPlayerController())
	{
		if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
		{
			BRPC->RandomTeams();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CheatManager] BRPlayerController를 찾을 수 없습니다."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] PlayerController를 찾을 수 없습니다."));
	}
}

void UBRCheatManager::ChangeTeam(int32 PlayerIndex, int32 TeamNumber)
{
	UE_LOG(LogTemp, Log, TEXT("[CheatManager] ChangeTeam 명령 실행: PlayerIndex=%d, TeamNumber=%d"), PlayerIndex, TeamNumber);
	
	if (APlayerController* PC = GetPlayerController())
	{
		if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
		{
			BRPC->ChangeTeam(PlayerIndex, TeamNumber);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CheatManager] BRPlayerController를 찾을 수 없습니다."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] PlayerController를 찾을 수 없습니다."));
	}
}

void UBRCheatManager::StartGame()
{
	UE_LOG(LogTemp, Log, TEXT("[CheatManager] StartGame 명령 실행"));
	
	if (APlayerController* PC = GetPlayerController())
	{
		if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
		{
			BRPC->StartGame();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CheatManager] BRPlayerController를 찾을 수 없습니다."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] PlayerController를 찾을 수 없습니다."));
	}
}

void UBRCheatManager::ShowRoomInfo()
{
	UE_LOG(LogTemp, Log, TEXT("[CheatManager] ShowRoomInfo 명령 실행"));
	
	if (APlayerController* PC = GetPlayerController())
	{
		if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
		{
			BRPC->ShowRoomInfo();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CheatManager] BRPlayerController를 찾을 수 없습니다."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] PlayerController를 찾을 수 없습니다."));
	}
}

void UBRCheatManager::OpenListenServer()
{
	APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] OpenListenServer: PlayerController 없음"));
		return;
	}
	UWorld* World = PC->GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[CheatManager] OpenListenServer: World 없음"));
		return;
	}
	ENetMode NetMode = World->GetNetMode();
	if (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CheatManager] OpenListenServer: 이미 서버 모드(%s)입니다."),
			NetMode == NM_ListenServer ? TEXT("ListenServer") : TEXT("DedicatedServer"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("이미 Listen Server 모드입니다."));
		return;
	}
	FString MapPath;
	if (AGameModeBase* GM = World->GetAuthGameMode())
	{
		if (ABRGameMode* BRGM = Cast<ABRGameMode>(GM))
		{
			MapPath = BRGM->LobbyMapPath;
		}
	}
	if (MapPath.IsEmpty())
	{
		MapPath = UGameplayStatics::GetCurrentLevelName(World, true);
		if (MapPath.IsEmpty())
		{
			MapPath = World->GetMapName();
			MapPath.RemoveFromStart(World->StreamingLevelsPrefix);
		}
	}
	if (MapPath.IsEmpty())
	{
		MapPath = TEXT("/Game/Main/Level/Main_Scene.Main_Scene");
	}
	// /Game/.../MapName 형식이어야 open 가능. GetCurrentLevelName은 짧은 이름만 줄 수 있음
	if (!MapPath.Contains(TEXT("/")))
	{
		MapPath = FString::Printf(TEXT("/Game/Main/Level/%s.%s"), *MapPath, *MapPath);
	}
	FString Cmd = FString::Printf(TEXT("open %s?listen"), *MapPath);
	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] OpenListenServer: %s"), *Cmd);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan,
			TEXT("Listen Server로 재시작합니다. 맵 로드 후 '방 만들기'를 진행하세요."));
	}
	PC->ConsoleCommand(Cmd, /*bExecInEditor=*/false);
}

void UBRCheatManager::CheckMyPlayerInfo()
{
	UE_LOG(LogTemp, Log, TEXT("[CheatManager] CheckMyPlayerInfo 실행"));

	APlayerController* PC = GetPlayerController();
	if (!PC) return;

	ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC);
	ABRPlayerState* PS = PC->GetPlayerState<ABRPlayerState>();
	UBRGameInstance* GI = Cast<UBRGameInstance>(PC->GetGameInstance());
	ABRGameState* GS = PC->GetWorld() ? PC->GetWorld()->GetGameState<ABRGameState>() : nullptr;

	UE_LOG(LogTemp, Warning, TEXT("========== [My Player Info] =========="));

	// 1. PlayerState Info
	if (PS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerState] Name: %s (ID: %d)"), *PS->GetPlayerName(), PS->GetPlayerId());
		UE_LOG(LogTemp, Warning, TEXT("  - TeamNumber: %d"), PS->TeamNumber);
		UE_LOG(LogTemp, Warning, TEXT("  - Role: %s (IsLowerBody: %s)"),
			PS->bIsSpectatorSlot ? TEXT("Spectator") : (PS->bIsLowerBody ? TEXT("LowerBody") : TEXT("UpperBody")),
			PS->bIsLowerBody ? TEXT("Yes") : TEXT("No"));
		UE_LOG(LogTemp, Warning, TEXT("  - ConnectedPlayerIndex: %d"), PS->ConnectedPlayerIndex);
		UE_LOG(LogTemp, Warning, TEXT("  - IsHost: %s, IsReady: %s"),
			PS->bIsHost ? TEXT("Yes") : TEXT("No"),
			PS->bIsReady ? TEXT("Yes") : TEXT("No"));
		UE_LOG(LogTemp, Warning, TEXT("  - UserUID: %s"), *PS->UserUID);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayerState] Not found!"));
	}

	// 2. GameInstance Info
	if (GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameInstance] PlayerName: %s"), *GI->GetPlayerName());
		UE_LOG(LogTemp, Warning, TEXT("  - UserUID: %s"), *GI->GetUserUID());
		UE_LOG(LogTemp, Warning, TEXT("  - PendingRoomName: %s"), *GI->GetPendingRoomName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GameInstance] Not found!"));
	}

	// 3. GameState Info (My Index)
	if (GS && PS)
	{
		int32 Index = GS->PlayerArray.Find(PS);
		UE_LOG(LogTemp, Warning, TEXT("[GameState] My Index in PlayerArray: %d"), Index);
		UE_LOG(LogTemp, Warning, TEXT("  - Total Players: %d"), GS->PlayerArray.Num());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GameState] Not found or PS is null!"));
	}

	UE_LOG(LogTemp, Warning, TEXT("======================================"));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("Check output log for player info details."));
	}
}

void UBRCheatManager::SetMyTeam(int32 TeamNumber)
{
	UE_LOG(LogTemp, Log, TEXT("[CheatManager] SetMyTeam(%d) 실행"), TeamNumber);
	if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(GetPlayerController()))
	{
		BRPC->SetMyTeamNumber(TeamNumber);
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("Set Team Number to %d"), TeamNumber));
	}
}

void UBRCheatManager::SetMyConnectedIndex(int32 Index)
{
	UE_LOG(LogTemp, Log, TEXT("[CheatManager] SetMyConnectedIndex(%d) 실행"), Index);
	if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(GetPlayerController()))
	{
		BRPC->ServerSetConnectedPlayerIndex(Index);
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("Set Connected Index to %d"), Index));
	}
}

