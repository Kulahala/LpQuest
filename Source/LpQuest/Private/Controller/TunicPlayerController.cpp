// Copyright Epic Games, Inc. All Rights Reserved.

#include "Controller/TunicPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "UI/TunicRunStatusWidget.h"

ATunicPlayerController::ATunicPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	RunStatusWidgetClass = UTunicRunStatusWidget::StaticClass();
}

void ATunicPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && RunStatusWidgetClass && !RunStatusWidget)
	{
		RunStatusWidget = CreateWidget<UTunicRunStatusWidget>(this, RunStatusWidgetClass);
		if (RunStatusWidget)
		{
			RunStatusWidget->AddToViewport();
		}
	}

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

