// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Fall.h"
#include "FallGameMode.h"
#include "FallHUD.h"
#include "FallCharacter.h"

AFallGameMode::AFallGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/Blueprints/MyCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AFallHUD::StaticClass();
}
