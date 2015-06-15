// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "WatchGame.h"
#include "WatchGameGameMode.h"
#include "WatchGameHUD.h"
#include "WatchGameCharacter.h"

AWatchGameGameMode::AWatchGameGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AWatchGameHUD::StaticClass();
}
