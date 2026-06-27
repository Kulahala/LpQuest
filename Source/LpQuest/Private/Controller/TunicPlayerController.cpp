// Copyright Epic Games, Inc. All Rights Reserved.

#include "Controller/TunicPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Game/TunicGameMode.h"
#include "Player/TunicPlayerState.h"
#include "UI/TunicRunStatusWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestRunUpgrade, Log, All);

ATunicPlayerController::ATunicPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	RunStatusWidgetClass = UTunicRunStatusWidget::StaticClass();
}

void ATunicPlayerController::RequestSelectRunUpgrade()
{
	if (!IsLocalController())
	{
		return;
	}

	ServerRequestSelectRunUpgrade();
}

void ATunicPlayerController::RequestSelectRunUpgradeStub()
{
	RequestSelectRunUpgrade();
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

void ATunicPlayerController::ServerRequestSelectRunUpgrade_Implementation()
{
	ATunicPlayerState* TunicPlayerState = GetPlayerState<ATunicPlayerState>();
	if (!TunicPlayerState)
	{
		UE_LOG(LogLpQuestRunUpgrade, Warning, TEXT("Run upgrade selection rejected: missing player state | Controller=%s"),
			*GetNameSafe(this));
		return;
	}

	ATunicGameMode* TunicGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ATunicGameMode>() : nullptr;
	if (!TunicGameMode)
	{
		UE_LOG(LogLpQuestRunUpgrade, Warning, TEXT("Run upgrade selection rejected: missing game mode | Controller=%s | PlayerState=%s"),
			*GetNameSafe(this),
			*GetNameSafe(TunicPlayerState));
		return;
	}

	const bool bSelectedUpgrade = TunicGameMode->TrySelectRunUpgradeForPlayer(TunicPlayerState);
	UE_LOG(LogLpQuestRunUpgrade, Display, TEXT("Run upgrade selection requested | Controller=%s | PlayerState=%s | Success=%s | RemainingPending=%d"),
		*GetNameSafe(this),
		*GetNameSafe(TunicPlayerState),
		bSelectedUpgrade ? TEXT("true") : TEXT("false"),
		TunicPlayerState->GetPendingRunUpgradeChoices());
}
