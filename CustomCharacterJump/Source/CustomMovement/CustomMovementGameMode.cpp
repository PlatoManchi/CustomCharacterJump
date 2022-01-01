// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomMovementGameMode.h"
#include "CustomMovementCharacter.h"
#include "UObject/ConstructorHelpers.h"

ACustomMovementGameMode::ACustomMovementGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
