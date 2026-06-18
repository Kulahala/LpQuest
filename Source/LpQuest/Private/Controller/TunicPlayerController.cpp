// Copyright Epic Games, Inc. All Rights Reserved.

#include "Controller/TunicPlayerController.h"

#include "EnhancedInputSubsystems.h"

ATunicPlayerController::ATunicPlayerController()
{
}

void ATunicPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!DefaultMappingContext)
	{
		return;
	}

	ULocalPlayer* OwningLocalPlayer = GetLocalPlayer();
	if (!OwningLocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = OwningLocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	InputSubsystem->AddMappingContext(DefaultMappingContext, DefaultMappingPriority);
}

