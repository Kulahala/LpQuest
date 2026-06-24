// Copyright Epic Games, Inc. All Rights Reserved.

#include "Controller/TunicPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Player/TunicPlayerState.h"
#include "UI/TunicRunStatusWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestRunUpgrade, Log, All);

ATunicPlayerController::ATunicPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	RunStatusWidgetClass = UTunicRunStatusWidget::StaticClass();
}

void ATunicPlayerController::RequestSelectRunUpgradeStub()
{
	if (!IsLocalController())
	{
		return;
	}

	ServerRequestSelectRunUpgradeStub();
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

void ATunicPlayerController::ServerRequestSelectRunUpgradeStub_Implementation()
{
	ATunicPlayerState* TunicPlayerState = GetPlayerState<ATunicPlayerState>();
	if (!TunicPlayerState)
	{
		UE_LOG(LogLpQuestRunUpgrade, Warning, TEXT("Run upgrade selection rejected: missing player state | Controller=%s"),
			*GetNameSafe(this));
		return;
	}

	if (!TunicPlayerState->TryConsumePendingRunUpgradeChoice())
	{
		UE_LOG(LogLpQuestRunUpgrade, Display, TEXT("Run upgrade selection rejected: no pending choices | Controller=%s | PlayerState=%s"),
			*GetNameSafe(this),
			*GetNameSafe(TunicPlayerState));
		return;
	}

	UE_LOG(LogLpQuestRunUpgrade, Display, TEXT("Run upgrade selection consumed | Controller=%s | PlayerState=%s | RemainingPending=%d"),
		*GetNameSafe(this),
		*GetNameSafe(TunicPlayerState),
		TunicPlayerState->GetPendingRunUpgradeChoices());
}
