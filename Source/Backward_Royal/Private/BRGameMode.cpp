// BRGameMode.cpp
#include "BRGameMode.h"
#include "BRGameState.h"
#include "BRPlayerState.h"
#include "BRPlayerController.h"
#include "BRGameSession.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter.h"
#include "UpperBodyPawn.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"

ABRGameMode::ABRGameMode()
{
	// GameState 클래스 설정
	GameStateClass = ABRGameState::StaticClass();
	PlayerStateClass = ABRPlayerState::StaticClass();
	PlayerControllerClass = ABRPlayerController::StaticClass();
	GameSessionClass = ABRGameSession::StaticClass();

	// 리슨 서버 설정
	bUseSeamlessTravel = true;

	// 기본 Pawn 클래스는 nullptr로 설정 (역할에 따라 동적으로 결정)
	DefaultPawnClass = nullptr;
}

void ABRGameMode::BeginPlay()
{
	Super::BeginPlay();

	// GameState에 최소/최대 플레이어 수 설정
	if (ABRGameState* BRGameState = GetGameState<ABRGameState>())
	{
		BRGameState->MinPlayers = MinPlayers;
		BRGameState->MaxPlayers = MaxPlayers;
	}
}

void ABRGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (ABRPlayerState* BRPS = NewPlayer->GetPlayerState<ABRPlayerState>())
	{
		if (ABRGameState* BRGameState = GetGameState<ABRGameState>())
		{
			FString PlayerName = BRPS->GetPlayerName();
			if (PlayerName.IsEmpty())
			{
				PlayerName = FString::Printf(TEXT("Player %d"), BRGameState->PlayerArray.Num());
			}
			UE_LOG(LogTemp, Log, TEXT("[플레이어 입장] %s가 게임에 입장했습니다. (현재 인원: %d/%d)"), 
				*PlayerName, BRGameState->PlayerArray.Num(), BRGameState->MaxPlayers);
			UE_LOG(LogTemp, Log, TEXT("[플레이어 입장] 참고: 실제 방(세션)을 만들려면 'CreateRoom [방이름]' 명령어를 사용하세요."));
			
			// 첫 번째 플레이어를 방장으로 설정
			if (BRGameState->PlayerArray.Num() == 1)
			{
				UE_LOG(LogTemp, Log, TEXT("[플레이어 입장] 첫 번째 플레이어이므로 방장으로 설정됩니다."));
				BRPS->SetIsHost(true);
			}

			// 플레이어 역할 할당 (하체/상체)
			// 홀수번째(1, 3, 5...) = 하체, 짝수번째(2, 4, 6...) = 직전 플레이어의 상체
			int32 CurrentPlayerIndex = BRGameState->PlayerArray.Num() - 1; // 0부터 시작
			int32 PlayerNumber = CurrentPlayerIndex + 1; // 실제 입장 순서 (1부터 시작)
			int32 LowerBodyPlayerIndex = -1; // 하체 플레이어 인덱스 (상체인 경우에만 사용)
			
			if (PlayerNumber % 2 == 1) // 홀수번째 (1, 3, 5...)
			{
				// 홀수번째 플레이어 = 하체
				BRPS->SetPlayerRole(true, -1); // 하체, 연결된 상체 없음
				UE_LOG(LogTemp, Log, TEXT("[플레이어 역할] %s: 하체 역할 할당 (입장 순서: %d번째)"), *PlayerName, PlayerNumber);
			}
			else // 짝수번째 (2, 4, 6...)
			{
				// 짝수번째 플레이어 = 직전 플레이어의 상체
				LowerBodyPlayerIndex = CurrentPlayerIndex - 1;
				if (LowerBodyPlayerIndex >= 0 && LowerBodyPlayerIndex < BRGameState->PlayerArray.Num())
				{
					if (ABRPlayerState* LowerBodyPS = Cast<ABRPlayerState>(BRGameState->PlayerArray[LowerBodyPlayerIndex]))
					{
						// 상체 역할 할당
						BRPS->SetPlayerRole(false, LowerBodyPlayerIndex);
						// 하체 플레이어의 연결된 상체 인덱스 업데이트
						LowerBodyPS->SetPlayerRole(true, CurrentPlayerIndex);
						UE_LOG(LogTemp, Log, TEXT("[플레이어 역할] %s: 상체 역할 할당 (하체 플레이어 인덱스: %d, 입장 순서: %d번째)"), 
							*PlayerName, LowerBodyPlayerIndex, PlayerNumber);
					}
				}
			}

			// 역할 할당 후 Pawn 스폰
			if (PlayerNumber % 2 == 1) // 홀수번째 (하체)
			{
				// 하체 플레이어는 일반적으로 Pawn 스폰
				if (NewPlayer->GetPawn() == nullptr)
				{
					RestartPlayer(NewPlayer);
				}
			}
			else // 짝수번째 (상체)
			{
				// 상체 플레이어는 기본 Pawn을 삭제하고 UpperBodyPawn만 스폰
				APawn* ExistingPawn = NewPlayer->GetPawn();
				if (ExistingPawn)
				{
					// 기본 Pawn이 스폰되었다면 삭제
					UE_LOG(LogTemp, Log, TEXT("[상체 플레이어] 기본 Pawn 삭제: %s"), *ExistingPawn->GetClass()->GetName());
					NewPlayer->UnPossess();
					ExistingPawn->Destroy();
				}
				
				// UpperBodyPawn 스폰
				if (LowerBodyPlayerIndex >= 0)
				{
					// 짧은 딜레이 후 UpperBodyPawn 스폰 및 연결 (Blueprint의 Delay와 동일)
					FTimerHandle TimerHandle;
					GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, NewPlayer, LowerBodyPlayerIndex]()
					{
						SpawnAndAttachUpperBody(NewPlayer, LowerBodyPlayerIndex);
					}, 0.2f, false);
				}
			}
		}
	}

	// 플레이어 목록 업데이트
	if (ABRGameState* BRGameState = GetGameState<ABRGameState>())
	{
		BRGameState->UpdatePlayerList();
	}
}

APawn* ABRGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	if (!NewPlayer)
	{
		return nullptr;
	}

	// PlayerState에서 역할 확인
	ABRPlayerState* BRPS = NewPlayer->GetPlayerState<ABRPlayerState>();
	if (!BRPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pawn 스폰] PlayerState를 찾을 수 없습니다."));
		return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	}

	// 역할에 따라 Pawn 클래스 결정
	TSubclassOf<APawn> PawnClassToSpawn = nullptr;
	
	if (BRPS->bIsLowerBody)
	{
		// 하체 플레이어
		PawnClassToSpawn = LowerBodyPawnClass;
		UE_LOG(LogTemp, Log, TEXT("[Pawn 스폰] 하체 플레이어용 Pawn 스폰"));
	}
	else
	{
		// 상체 플레이어
		PawnClassToSpawn = UpperBodyPawnClass;
		UE_LOG(LogTemp, Log, TEXT("[Pawn 스폰] 상체 플레이어용 Pawn 스폰"));
	}

	// Pawn 클래스가 설정되지 않은 경우 기본 클래스 사용
	if (!PawnClassToSpawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pawn 스폰] Pawn 클래스가 설정되지 않았습니다. 기본 Pawn을 사용합니다."));
		return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	}

	// StartSpot이 없으면 찾기
	if (!StartSpot)
	{
		StartSpot = FindPlayerStart(NewPlayer);
	}

	if (!StartSpot)
	{
		UE_LOG(LogTemp, Error, TEXT("[Pawn 스폰] StartSpot을 찾을 수 없습니다."));
		return nullptr;
	}

	// Pawn 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = NewPlayer;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FVector SpawnLocation = StartSpot->GetActorLocation();
	FRotator SpawnRotation = StartSpot->GetActorRotation();

	APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (SpawnedPawn)
	{
		UE_LOG(LogTemp, Log, TEXT("[Pawn 스폰] 성공: %s"), *SpawnedPawn->GetClass()->GetName());
		// Controller에 Pawn 설정
		NewPlayer->Possess(SpawnedPawn);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Pawn 스폰] 실패: Pawn을 생성할 수 없습니다."));
	}

	return SpawnedPawn;
}

void ABRGameMode::SpawnAndAttachUpperBody(APlayerController* UpperBodyController, int32 LowerBodyPlayerIndex)
{
	if (!UpperBodyController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[상체 스폰] UpperBodyController가 유효하지 않습니다."));
		return;
	}

	// UpperBodyPawn 클래스 확인
	if (!UpperBodyPawnClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[상체 스폰] UpperBodyPawnClass가 설정되지 않았습니다."));
		return;
	}

	// GameState에서 하체 플레이어 찾기
	if (ABRGameState* BRGameState = GetGameState<ABRGameState>())
	{
		if (LowerBodyPlayerIndex < 0 || LowerBodyPlayerIndex >= BRGameState->PlayerArray.Num())
		{
			UE_LOG(LogTemp, Warning, TEXT("[상체 스폰] 하체 플레이어 인덱스가 유효하지 않습니다: %d"), LowerBodyPlayerIndex);
			return;
		}

		// 하체 플레이어의 Controller 찾기
		APlayerController* LowerBodyController = nullptr;
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (ABRPlayerState* PS = PC->GetPlayerState<ABRPlayerState>())
				{
					if (PS == BRGameState->PlayerArray[LowerBodyPlayerIndex])
					{
						LowerBodyController = PC;
						break;
					}
				}
			}
		}

		if (!LowerBodyController)
		{
			UE_LOG(LogTemp, Warning, TEXT("[상체 스폰] 하체 플레이어의 Controller를 찾을 수 없습니다."));
			return;
		}

		// 하체 플레이어의 Pawn 가져오기
		APawn* LowerBodyPawn = LowerBodyController->GetPawn();
		if (!LowerBodyPawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("[상체 스폰] 하체 플레이어의 Pawn을 찾을 수 없습니다."));
			return;
		}

		// PlayerCharacter로 캐스팅
		APlayerCharacter* LowerBodyCharacter = Cast<APlayerCharacter>(LowerBodyPawn);
		if (!LowerBodyCharacter)
		{
			UE_LOG(LogTemp, Warning, TEXT("[상체 스폰] 하체 Pawn을 PlayerCharacter로 캐스팅할 수 없습니다."));
			return;
		}

		// HeadMountPoint 가져오기
		USceneComponent* HeadMountPoint = LowerBodyCharacter->HeadMountPoint;
		if (!HeadMountPoint)
		{
			UE_LOG(LogTemp, Warning, TEXT("[상체 스폰] HeadMountPoint를 찾을 수 없습니다."));
			return;
		}

		// UpperBodyPawn 스폰 (HeadMountPoint 위치에)
		FTransform SpawnTransform = HeadMountPoint->GetComponentTransform();
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = UpperBodyController;
		SpawnParams.Instigator = GetInstigator();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APawn* SpawnedUpperBodyPawn = GetWorld()->SpawnActor<APawn>(UpperBodyPawnClass, SpawnTransform.GetLocation(), SpawnTransform.GetRotation().Rotator(), SpawnParams);
		
		if (SpawnedUpperBodyPawn)
		{
			UE_LOG(LogTemp, Log, TEXT("[상체 스폰] UpperBodyPawn 스폰 성공: %s"), *SpawnedUpperBodyPawn->GetClass()->GetName());
			
			// Controller에 Pawn 설정
			UpperBodyController->Possess(SpawnedUpperBodyPawn);
			
			// HeadMountPoint에 Attach
			FAttachmentTransformRules AttachRules(
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				true
			);

			SpawnedUpperBodyPawn->AttachToComponent(HeadMountPoint, AttachRules);
			UE_LOG(LogTemp, Log, TEXT("[상체 스폰] 상체 플레이어를 하체 플레이어의 HeadMountPoint에 연결했습니다."));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[상체 스폰] UpperBodyPawn 스폰 실패"));
		}
	}
}

void ABRGameMode::AttachUpperBodyToLowerBody(APlayerController* UpperBodyController, int32 LowerBodyPlayerIndex)
{
	if (!UpperBodyController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[상체 연결] UpperBodyController가 유효하지 않습니다."));
		return;
	}

	// 상체 플레이어의 Pawn 가져오기
	APawn* UpperBodyPawn = UpperBodyController->GetPawn();
	if (!UpperBodyPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[상체 연결] 상체 플레이어의 Pawn을 찾을 수 없습니다."));
		return;
	}

	// GameState에서 하체 플레이어 찾기
	if (ABRGameState* BRGameState = GetGameState<ABRGameState>())
	{
		if (LowerBodyPlayerIndex < 0 || LowerBodyPlayerIndex >= BRGameState->PlayerArray.Num())
		{
			UE_LOG(LogTemp, Warning, TEXT("[상체 연결] 하체 플레이어 인덱스가 유효하지 않습니다: %d"), LowerBodyPlayerIndex);
			return;
		}

		// 하체 플레이어의 Controller 찾기
		APlayerController* LowerBodyController = nullptr;
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (ABRPlayerState* PS = PC->GetPlayerState<ABRPlayerState>())
				{
					if (PS == BRGameState->PlayerArray[LowerBodyPlayerIndex])
					{
						LowerBodyController = PC;
						break;
					}
				}
			}
		}

		if (!LowerBodyController)
		{
			UE_LOG(LogTemp, Warning, TEXT("[상체 연결] 하체 플레이어의 Controller를 찾을 수 없습니다."));
			return;
		}

		// 하체 플레이어의 Pawn 가져오기
		APawn* LowerBodyPawn = LowerBodyController->GetPawn();
		if (!LowerBodyPawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("[상체 연결] 하체 플레이어의 Pawn을 찾을 수 없습니다."));
			return;
		}

		// PlayerCharacter로 캐스팅
		APlayerCharacter* LowerBodyCharacter = Cast<APlayerCharacter>(LowerBodyPawn);
		if (!LowerBodyCharacter)
		{
			UE_LOG(LogTemp, Warning, TEXT("[상체 연결] 하체 Pawn을 PlayerCharacter로 캐스팅할 수 없습니다."));
			return;
		}

		// HeadMountPoint 가져오기
		USceneComponent* HeadMountPoint = LowerBodyCharacter->HeadMountPoint;
		if (!HeadMountPoint)
		{
			UE_LOG(LogTemp, Warning, TEXT("[상체 연결] HeadMountPoint를 찾을 수 없습니다."));
			return;
		}

		// 상체를 HeadMountPoint에 Attach
		FAttachmentTransformRules AttachRules(
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,
			true
		);

		UpperBodyPawn->AttachToComponent(HeadMountPoint, AttachRules);
		UE_LOG(LogTemp, Log, TEXT("[상체 연결] 성공: 상체 플레이어를 하체 플레이어의 HeadMountPoint에 연결했습니다."));
	}
}

void ABRGameMode::Logout(AController* Exiting)
{
	// 방장이 나갔을 경우 새로운 방장 지정 및 역할 재할당
	if (ABRPlayerState* ExitingPS = Exiting->GetPlayerState<ABRPlayerState>())
	{
		FString ExitingPlayerName = ExitingPS->GetPlayerName();
		if (ExitingPlayerName.IsEmpty())
		{
			ExitingPlayerName = TEXT("Unknown Player");
		}
		
		if (ABRGameState* BRGameState = GetGameState<ABRGameState>())
		{
			UE_LOG(LogTemp, Log, TEXT("[플레이어 퇴장] %s가 방을 나갔습니다. (남은 인원: %d/%d)"), 
				*ExitingPlayerName, BRGameState->PlayerArray.Num() - 1, BRGameState->MaxPlayers);
			
			// 나가는 플레이어가 상체인 경우, 연결된 하체 플레이어의 연결 해제
			if (!ExitingPS->bIsLowerBody && ExitingPS->ConnectedPlayerIndex >= 0)
			{
				if (ExitingPS->ConnectedPlayerIndex < BRGameState->PlayerArray.Num())
				{
					if (ABRPlayerState* LowerBodyPS = Cast<ABRPlayerState>(BRGameState->PlayerArray[ExitingPS->ConnectedPlayerIndex]))
					{
						LowerBodyPS->SetPlayerRole(true, -1); // 연결된 상체 없음
						UE_LOG(LogTemp, Log, TEXT("[플레이어 퇴장] 하체 플레이어의 상체 연결 해제"));
					}
				}
			}
			// 나가는 플레이어가 하체인 경우, 연결된 상체 플레이어도 처리
			else if (ExitingPS->bIsLowerBody && ExitingPS->ConnectedPlayerIndex >= 0)
			{
				if (ExitingPS->ConnectedPlayerIndex < BRGameState->PlayerArray.Num())
				{
					if (ABRPlayerState* UpperBodyPS = Cast<ABRPlayerState>(BRGameState->PlayerArray[ExitingPS->ConnectedPlayerIndex]))
					{
						// 상체 플레이어도 나가야 하거나, 다른 하체에 연결시킬 수 있음
						// 여기서는 단순히 연결 해제만 함
						UpperBodyPS->SetPlayerRole(false, -1);
						UE_LOG(LogTemp, Log, TEXT("[플레이어 퇴장] 상체 플레이어의 하체 연결 해제"));
					}
				}
			}
			
			if (ExitingPS->bIsHost && BRGameState->PlayerArray.Num() > 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("[플레이어 퇴장] 방장이 나갔으므로 새로운 방장을 지정합니다."));
				// 다음 플레이어를 방장으로 설정
				for (APlayerState* PS : BRGameState->PlayerArray)
				{
					if (PS != ExitingPS)
					{
						if (ABRPlayerState* BRPS = Cast<ABRPlayerState>(PS))
						{
							BRPS->SetIsHost(true);
							break;
						}
					}
				}
			}
		}
	}

	Super::Logout(Exiting);

	// 플레이어 목록 업데이트 및 역할 재할당
	if (ABRGameState* BRGameState = GetGameState<ABRGameState>())
	{
		BRGameState->UpdatePlayerList();
		
		// 남은 플레이어들의 역할 재할당 (순서대로)
		// 홀수번째(1, 3, 5...) = 하체, 짝수번째(2, 4, 6...) = 직전 플레이어의 상체
		for (int32 i = 0; i < BRGameState->PlayerArray.Num(); i++)
		{
			if (ABRPlayerState* BRPS = Cast<ABRPlayerState>(BRGameState->PlayerArray[i]))
			{
				int32 PlayerNumber = i + 1; // 실제 입장 순서 (1부터 시작)
				
				if (PlayerNumber % 2 == 1) // 홀수번째 (1, 3, 5...)
				{
					// 홀수번째 플레이어 = 하체
					BRPS->SetPlayerRole(true, -1);
				}
				else // 짝수번째 (2, 4, 6...)
				{
					// 짝수번째 플레이어 = 직전 플레이어의 상체
					int32 LowerBodyPlayerIndex = i - 1;
					if (LowerBodyPlayerIndex >= 0 && LowerBodyPlayerIndex < BRGameState->PlayerArray.Num())
					{
						if (ABRPlayerState* LowerBodyPS = Cast<ABRPlayerState>(BRGameState->PlayerArray[LowerBodyPlayerIndex]))
						{
							BRPS->SetPlayerRole(false, LowerBodyPlayerIndex);
							LowerBodyPS->SetPlayerRole(true, i);
						}
					}
				}
			}
		}
	}
}

void ABRGameMode::StartGame()
{
	if (!HasAuthority())
		return;

	UE_LOG(LogTemp, Log, TEXT("[게임 시작] 게임 시작 요청 처리 중..."));
	
	if (ABRGameState* BRGameState = GetGameState<ABRGameState>())
	{
		// 호스트인지 확인
		bool bIsHost = false;
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (ABRPlayerState* BRPS = PC->GetPlayerState<ABRPlayerState>())
			{
				bIsHost = BRPS->bIsHost;
			}
		}
		
		// 호스트인 경우: 호스트를 제외한 모든 플레이어가 준비되었는지 확인
		// 호스트가 아닌 경우: 모든 플레이어가 준비되었는지 확인
		bool bCanStart = false;
		if (bIsHost)
		{
			// 호스트는 자신이 준비하지 않아도 다른 모든 플레이어가 준비되었으면 시작 가능
			bCanStart = (BRGameState->PlayerCount >= BRGameState->MinPlayers && 
			            BRGameState->PlayerCount <= BRGameState->MaxPlayers && 
			            BRGameState->AreAllNonHostPlayersReady());
			
			if (bCanStart)
			{
				UE_LOG(LogTemp, Log, TEXT("[게임 시작] 호스트: 다른 모든 플레이어가 준비 완료 - 게임 시작 가능"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[게임 시작] 호스트: 조건 불만족 - 플레이어 수=%d/%d-%d, 호스트 제외 모든 준비=%s"), 
					BRGameState->PlayerCount, BRGameState->MinPlayers, BRGameState->MaxPlayers,
					BRGameState->AreAllNonHostPlayersReady() ? TEXT("예") : TEXT("아니오"));
			}
		}
		else
		{
			// 호스트가 아닌 경우: 기존 로직 사용 (모든 플레이어가 준비되어야 함)
			BRGameState->CheckCanStartGame();
			bCanStart = BRGameState->bCanStartGame;
			
			if (!bCanStart)
			{
				UE_LOG(LogTemp, Error, TEXT("[게임 시작] 실패: 게임을 시작할 수 없습니다."));
				UE_LOG(LogTemp, Warning, TEXT("[게임 시작] 조건 확인: 플레이어 수=%d/%d-%d, 모든 준비=%s"), 
					BRGameState->PlayerCount, BRGameState->MinPlayers, BRGameState->MaxPlayers,
					BRGameState->AreAllPlayersReady() ? TEXT("예") : TEXT("아니오"));
			}
		}
		
		if (!bCanStart)
		{
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[게임 시작] 성공: 맵으로 이동 중... (%s)"), *GameMapPath);
	
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[게임 시작] 실패: World를 찾을 수 없습니다."));
		return;
	}
	
	// 클라이언트에게 게임 시작 알림 (맵 이동 전에 알림)
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
			{
				// 클라이언트에게 게임 시작 알림 (RPC)
				BRPC->ClientNotifyGameStarting();
			}
		}
	}
	
	// 맵 이동 - Seamless Travel 사용 (더 부드러운 전환)
	FString TravelURL = GameMapPath + TEXT("?listen");
	UE_LOG(LogTemp, Log, TEXT("[게임 시작] SeamlessTravel 호출: %s"), *TravelURL);
	
	// SeamlessTravel 사용 - 클라이언트가 부드럽게 따라옵니다
	// bUseSeamlessTravel = true이므로 SeamlessTravel을 사용할 수 있습니다
	if (bUseSeamlessTravel)
	{
		// SeamlessTravel은 GameMode의 함수가 아니라 World의 함수입니다
		// 하지만 ServerTravel을 사용하되, bUseSeamlessTravel이 true이면 자동으로 Seamless Travel을 사용합니다
		World->ServerTravel(TravelURL, true); // 두 번째 파라미터는 bAbsolute (true = 절대 경로)
	}
	else
	{
		World->ServerTravel(TravelURL);
	}
}

