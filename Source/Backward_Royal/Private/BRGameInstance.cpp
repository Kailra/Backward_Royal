// BRGameInstance.cpp
#include "BRGameInstance.h"
#include "BRPlayerController.h"
#include "BRGameSession.h"
#include "BRGameMode.h"
#include "HAL/PlatformFileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "BaseWeapon.h"
#include "GameFramework/GameModeBase.h"

DEFINE_LOG_CATEGORY(LogBRGameInstance);

#define GI_LOG(Verbosity, Format, ...) UE_LOG(LogBRGameInstance, Verbosity, TEXT("%s: ") Format, *FString(__FUNCTION__), ##__VA_ARGS__)

UBRGameInstance::UBRGameInstance()
{
}

void UBRGameInstance::Init()
{
	Super::Init();
	UE_LOG(LogTemp, Log, TEXT("[GameInstance] BRGameInstance 초기화 완료 - 콘솔 명령어 사용 가능"));
	ReloadAllConfigs();
}

void UBRGameInstance::CreateRoom(const FString& RoomName)
{
	UE_LOG(LogTemp, Log, TEXT("[GameInstance] CreateRoom 명령 실행: %s"), *RoomName);
	
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("[GameInstance] World가 없습니다. 게임을 먼저 시작해주세요."));
		return;
	}

	// PlayerController를 통한 방법
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
		{
			UE_LOG(LogTemp, Log, TEXT("[GameInstance] PlayerController를 통해 방 생성 요청"));
			BRPC->CreateRoom(RoomName);
			return;
		}
	}

	// GameMode를 통한 직접 접근 방법 (게임이 시작되지 않았을 때)
	if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode())
	{
		if (ABRGameSession* GameSession = Cast<ABRGameSession>(GameMode->GameSession))
		{
			UE_LOG(LogTemp, Log, TEXT("[GameInstance] GameSession을 통해 직접 방 생성 요청"));
			GameSession->CreateRoomSession(RoomName);
			return;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("[GameInstance] 방 생성을 위한 필요한 객체를 찾을 수 없습니다."));
	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] 게임을 시작한 후 다시 시도해주세요."));
}

void UBRGameInstance::FindRooms()
{
	UE_LOG(LogTemp, Log, TEXT("[GameInstance] FindRooms 명령"));
	
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
			{
				BRPC->FindRooms();
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[GameInstance] BRPlayerController를 찾을 수 없습니다."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[GameInstance] PlayerController를 찾을 수 없습니다. 게임이 시작되지 않았을 수 있습니다."));
		}
	}
}

void UBRGameInstance::ToggleReady()
{
	UE_LOG(LogTemp, Log, TEXT("[GameInstance] ToggleReady 명령"));
	
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
			{
				BRPC->ToggleReady();
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[GameInstance] BRPlayerController를 찾을 수 없습니다."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[GameInstance] PlayerController를 찾을 수 없습니다. 게임이 시작되지 않았을 수 있습니다."));
		}
	}
}

void UBRGameInstance::RandomTeams()
{
	UE_LOG(LogTemp, Log, TEXT("[GameInstance] RandomTeams 명령"));
	
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
			{
				BRPC->RandomTeams();
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[GameInstance] BRPlayerController를 찾을 수 없습니다."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[GameInstance] PlayerController를 찾을 수 없습니다. 게임이 시작되지 않았을 수 있습니다."));
		}
	}
}

void UBRGameInstance::ChangeTeam(int32 PlayerIndex, int32 TeamNumber)
{
	UE_LOG(LogTemp, Log, TEXT("[GameInstance] ChangeTeam 명령: PlayerIndex=%d, TeamNumber=%d"), PlayerIndex, TeamNumber);
	
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
			{
				BRPC->ChangeTeam(PlayerIndex, TeamNumber);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[GameInstance] BRPlayerController를 찾을 수 없습니다."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[GameInstance] PlayerController를 찾을 수 없습니다. 게임이 시작되지 않았을 수 있습니다."));
		}
	}
}

void UBRGameInstance::StartGame()
{
	UE_LOG(LogTemp, Log, TEXT("[GameInstance] StartGame 명령"));
	
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
			{
				BRPC->StartGame();
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[GameInstance] BRPlayerController를 찾을 수 없습니다."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[GameInstance] PlayerController를 찾을 수 없습니다. 게임이 시작되지 않았을 수 있습니다."));
		}
	}
}

void UBRGameInstance::ShowRoomInfo()
{
	UE_LOG(LogTemp, Log, TEXT("[GameInstance] ShowRoomInfo 명령"));
	
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ABRPlayerController* BRPC = Cast<ABRPlayerController>(PC))
			{
				BRPC->ShowRoomInfo();
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[GameInstance] BRPlayerController를 찾을 수 없습니다."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[GameInstance] PlayerController를 찾을 수 없습니다. 게임이 시작되지 않았을 수 있습니다."));
		}
	}
}

void UBRGameInstance::ReloadAllConfigs()
{
	GI_LOG(Display, TEXT("Starting Global Config Reload..."));

	for (auto& Elem : ConfigDataMap)
	{
		// Key가 파일 이름이 되고, Value가 대상 테이블이 됨
		LoadConfigFromJson(Elem.Key, Elem.Value);
	}

	// 1. 월드에 이미 존재하는 무기들에게 최신 설계도를 다시 읽으라고 시킵니다.
	for (TActorIterator<ABaseWeapon> It(GetWorld()); It; ++It)
	{
		// 이 함수 내부에서 MyDataTable->FindRow를 다시 호출하여 
		// 갱신된 JSON 수치를 CurrentWeaponData에 덮어씁니다.
		It->LoadWeaponData();
	}
}

void UBRGameInstance::LoadConfigFromJson(const FString& FileName, UDataTable* TargetTable)
{
	if (!TargetTable) return;

	// 경로: 프로젝트/Config/파일명.json
	FString FilePath = GetConfigDirectory() + FileName + TEXT(".json");
	FString JsonString;

	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		GI_LOG(Warning, TEXT("File not found: %s"), *FilePath);
		return;
	}

	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* DataArray;
		// 모든 JSON의 최상위 배열 키를 "Data"로 통일하거나 파일명과 맞춥니다.
		if (RootObject->TryGetArrayField(TEXT("Data"), DataArray))
		{
			// const UScriptStruct* 로 선언하여 타입 에러 해결
			const UScriptStruct* TableStruct = TargetTable->GetRowStruct();

			for (const auto& Value : *DataArray)
			{
				TSharedPtr<FJsonObject> DataObj = Value->AsObject();
				if (!DataObj.IsValid()) continue;

				FName RowID = FName(*DataObj->GetStringField(TEXT("Name")));
				uint8* RowPtr = TargetTable->FindRowUnchecked(RowID);

				if (RowPtr && TableStruct)
				{
					// 수치 데이터 주입
					FJsonObjectConverter::JsonObjectToUStruct(DataObj.ToSharedRef(), TableStruct, RowPtr);
					GI_LOG(Log, TEXT("[%s.json] Row Updated: %s"), *FileName, *RowID.ToString());
				}
			}
		}
	}
}

FString UBRGameInstance::GetConfigDirectory()
{
	FString TargetPath;

#if WITH_EDITOR
	// 1. 에디터 환경: 프로젝트 루트의 Data 폴더
	TargetPath = FPaths::ProjectDir() / TEXT("Data/");
#else
	// 2. 패키징 환경: 빌드된 .exe 옆의 Data 폴더 (예: Build/Windows/MyProject/Data/)
	// FPaths::ProjectDir()는 패키징 후에도 실행 파일 기준 경로를 반환합니다.
	TargetPath = FPaths::ProjectDir() / TEXT("Data/");
#endif

	return TargetPath;
}