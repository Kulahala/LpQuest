// Copyright Epic Games, Inc. All Rights Reserved.

#include "Controller/LPQPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Game/LPQGameMode.h"
#include "Player/LPQPlayerState.h"
#include "UI/LPQRunStatusWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestRunUpgrade, Log, All);

ALPQPlayerController::ALPQPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	RunStatusWidgetClass = ULPQRunStatusWidget::StaticClass();
}

void ALPQPlayerController::RequestSelectRunUpgrade()
{
	if (!IsLocalController())
	{
		return;
	}

	ServerRequestSelectRunUpgrade();
}

void ALPQPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && RunStatusWidgetClass && !RunStatusWidget)
	{
		RunStatusWidget = CreateWidget<ULPQRunStatusWidget>(this, RunStatusWidgetClass);
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

void ALPQPlayerController::ServerRequestSelectRunUpgrade_Implementation()
{
	ALPQPlayerState* LpQuestPlayerState = GetPlayerState<ALPQPlayerState>();
	if (!LpQuestPlayerState)
	{
		UE_LOG(LogLpQuestRunUpgrade, Warning, TEXT("Run upgrade selection rejected: missing player state | Controller=%s"),
			*GetNameSafe(this));
		return;
	}

	ALPQGameMode* LpQuestGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALPQGameMode>() : nullptr;
	if (!LpQuestGameMode)
	{
		UE_LOG(LogLpQuestRunUpgrade, Warning, TEXT("Run upgrade selection rejected: missing game mode | Controller=%s | PlayerState=%s"),
			*GetNameSafe(this),
			*GetNameSafe(LpQuestPlayerState));
		return;
	}

	const bool bSelectedUpgrade = LpQuestGameMode->TrySelectRunUpgradeForPlayer(LpQuestPlayerState);
	UE_LOG(LogLpQuestRunUpgrade, Display, TEXT("Run upgrade selection requested | Controller=%s | PlayerState=%s | Success=%s | RemainingPending=%d"),
		*GetNameSafe(this),
		*GetNameSafe(LpQuestPlayerState),
		bSelectedUpgrade ? TEXT("true") : TEXT("false"),
		LpQuestPlayerState->GetPendingRunUpgradeChoices());
}
