// Copyright Epic Games, Inc. All Rights Reserved.

#include "Backward_RoyalGameMode.h"
#include "Backward_RoyalCharacter.h"
#include "UObject/ConstructorHelpers.h"

ABackward_RoyalGameMode::ABackward_RoyalGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
