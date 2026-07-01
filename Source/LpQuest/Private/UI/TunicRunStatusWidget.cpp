// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/TunicRunStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Controller/TunicPlayerController.h"
#include "Engine/World.h"
#include "Player/TunicPlayerState.h"

void UTunicRunStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindGameState();
	BindPlayerState();
	if (SelectUpgradeButton)
	{
		SelectUpgradeButton->OnClicked.AddUniqueDynamic(this, &UTunicRunStatusWidget::HandleSelectUpgradeClicked);
	}
	RefreshRunStatus();
}

void UTunicRunStatusWidget::NativeDestruct()
{
	if (SelectUpgradeButton)
	{
		SelectUpgradeButton->OnClicked.RemoveDynamic(this, &UTunicRunStatusWidget::HandleSelectUpgradeClicked);
	}
	UnbindPlayerState();
	UnbindGameState();

	Super::NativeDestruct();
}

TSharedRef<SWidget> UTunicRunStatusWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RunStatusRoot"));
		WidgetTree->RootWidget = RootBox;

		FloorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FloorText"));
		RunStateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RunStateText"));
		SharedExperienceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SharedExperienceText"));
		SharedLevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SharedLevelText"));
		PendingUpgradeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PendingUpgradeText"));
		SelectUpgradeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SelectUpgradeButton"));
		SelectUpgradeButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectUpgradeButtonText"));

		if (FloorText)
		{
			FloorText->SetText(FText::FromString(TEXT("Floor: 1 [Start]")));
			RootBox->AddChildToVerticalBox(FloorText);
		}

		if (RunStateText)
		{
			RunStateText->SetText(FText::FromString(TEXT("Run: CombatActive")));
			RootBox->AddChildToVerticalBox(RunStateText);
		}

		if (SharedExperienceText)
		{
			SharedExperienceText->SetText(FText::FromString(TEXT("XP: 0")));
			RootBox->AddChildToVerticalBox(SharedExperienceText);
		}

		if (SharedLevelText)
		{
			SharedLevelText->SetText(FText::FromString(TEXT("Level: 1")));
			RootBox->AddChildToVerticalBox(SharedLevelText);
		}

		if (PendingUpgradeText)
		{
			PendingUpgradeText->SetText(FText::FromString(TEXT("Upgrade Available: 0")));
			RootBox->AddChildToVerticalBox(PendingUpgradeText);
		}

		if (SelectUpgradeButton && SelectUpgradeButtonText)
		{
			SelectUpgradeButtonText->SetText(FText::FromString(TEXT("Select Upgrade")));
			SelectUpgradeButton->SetContent(SelectUpgradeButtonText);
			SelectUpgradeButton->SetIsEnabled(false);
			RootBox->AddChildToVerticalBox(SelectUpgradeButton);
		}
	}

	return Super::RebuildWidget();
}

void UTunicRunStatusWidget::RefreshRunStatus()
{
	BindGameState();
	BindPlayerState();

	const int32 FloorIndex = BoundGameState ? BoundGameState->GetCurrentFloorIndex() : 1;
	const FName FloorDestinationId = BoundGameState ? BoundGameState->GetCurrentFloorDestinationId() : FName(TEXT("Start"));
	const ETunicRunState RunState = BoundGameState ? BoundGameState->GetRunState() : ETunicRunState::CombatActive;
	const int32 SharedRunExperience = BoundGameState ? BoundGameState->GetSharedRunExperience() : 0;
	const int32 SharedRunLevel = BoundGameState ? BoundGameState->GetSharedRunLevel() : 1;
	const int32 PendingUpgradeChoices = BoundPlayerState ? BoundPlayerState->GetPendingRunUpgradeChoices() : 0;

	if (FloorText)
	{
		FloorText->SetText(FText::Format(
			NSLOCTEXT("LPQRunStatus", "FloorDestinationFormat", "Floor: {0} [{1}]"),
			FloorIndex,
			FText::FromName(FloorDestinationId)));
	}

	if (RunStateText)
	{
		RunStateText->SetText(FText::Format(NSLOCTEXT("LPQRunStatus", "RunStateFormat", "Run: {0}"), GetRunStateText(RunState)));
	}

	if (SharedExperienceText)
	{
		SharedExperienceText->SetText(FText::Format(NSLOCTEXT("LPQRunStatus", "SharedExperienceFormat", "XP: {0}"), SharedRunExperience));
	}

	if (SharedLevelText)
	{
		SharedLevelText->SetText(FText::Format(NSLOCTEXT("LPQRunStatus", "SharedLevelFormat", "Level: {0}"), SharedRunLevel));
	}

	if (PendingUpgradeText)
	{
		PendingUpgradeText->SetText(FText::Format(NSLOCTEXT("LPQRunStatus", "PendingUpgradeFormat", "Upgrade Available: {0}"), PendingUpgradeChoices));
	}

	if (SelectUpgradeButton)
	{
		SelectUpgradeButton->SetIsEnabled(PendingUpgradeChoices > 0);
	}

	OnRunStatusRefreshed(FloorIndex, FloorDestinationId, RunState, SharedRunExperience, SharedRunLevel, PendingUpgradeChoices);
}

void UTunicRunStatusWidget::OnRunStatusRefreshed_Implementation(int32, FName, ETunicRunState, int32, int32, int32)
{
}

void UTunicRunStatusWidget::HandleRunStateChanged(ETunicRunState)
{
	RefreshRunStatus();
}

void UTunicRunStatusWidget::HandleFloorIndexChanged(int32)
{
	RefreshRunStatus();
}

void UTunicRunStatusWidget::HandleFloorDestinationChanged(FName)
{
	RefreshRunStatus();
}

void UTunicRunStatusWidget::HandleSharedRunExperienceChanged(int32, int32, AActor*)
{
	RefreshRunStatus();
}

void UTunicRunStatusWidget::HandleSharedRunLevelChanged(int32)
{
	RefreshRunStatus();
}

void UTunicRunStatusWidget::HandlePendingRunUpgradeChoicesChanged(int32)
{
	RefreshRunStatus();
}

void UTunicRunStatusWidget::HandleSelectUpgradeClicked()
{
	ATunicPlayerController* LpQuestPlayerController = Cast<ATunicPlayerController>(GetOwningPlayer());
	if (!LpQuestPlayerController)
	{
		return;
	}

	LpQuestPlayerController->RequestSelectRunUpgrade();
}

void UTunicRunStatusWidget::BindGameState()
{
	ATunicGameState* LpQuestGameState = GetWorld() ? GetWorld()->GetGameState<ATunicGameState>() : nullptr;
	if (!LpQuestGameState || BoundGameState == LpQuestGameState)
	{
		return;
	}

	UnbindGameState();

	BoundGameState = LpQuestGameState;
	BoundGameState->OnRunStateChangedEvent.AddUniqueDynamic(this, &UTunicRunStatusWidget::HandleRunStateChanged);
	BoundGameState->OnFloorIndexChangedEvent.AddUniqueDynamic(this, &UTunicRunStatusWidget::HandleFloorIndexChanged);
	BoundGameState->OnFloorDestinationChangedEvent.AddUniqueDynamic(this, &UTunicRunStatusWidget::HandleFloorDestinationChanged);
	BoundGameState->OnSharedRunExperienceChangedEvent.AddUniqueDynamic(this, &UTunicRunStatusWidget::HandleSharedRunExperienceChanged);
	BoundGameState->OnSharedRunLevelChangedEvent.AddUniqueDynamic(this, &UTunicRunStatusWidget::HandleSharedRunLevelChanged);
}

void UTunicRunStatusWidget::UnbindGameState()
{
	if (!BoundGameState)
	{
		return;
	}

	BoundGameState->OnRunStateChangedEvent.RemoveDynamic(this, &UTunicRunStatusWidget::HandleRunStateChanged);
	BoundGameState->OnFloorIndexChangedEvent.RemoveDynamic(this, &UTunicRunStatusWidget::HandleFloorIndexChanged);
	BoundGameState->OnFloorDestinationChangedEvent.RemoveDynamic(this, &UTunicRunStatusWidget::HandleFloorDestinationChanged);
	BoundGameState->OnSharedRunExperienceChangedEvent.RemoveDynamic(this, &UTunicRunStatusWidget::HandleSharedRunExperienceChanged);
	BoundGameState->OnSharedRunLevelChangedEvent.RemoveDynamic(this, &UTunicRunStatusWidget::HandleSharedRunLevelChanged);
	BoundGameState = nullptr;
}

void UTunicRunStatusWidget::BindPlayerState()
{
	APlayerController* OwningPlayerController = GetOwningPlayer();
	ATunicPlayerState* LpQuestPlayerState = OwningPlayerController ? OwningPlayerController->GetPlayerState<ATunicPlayerState>() : nullptr;
	if (!LpQuestPlayerState || BoundPlayerState == LpQuestPlayerState)
	{
		return;
	}

	UnbindPlayerState();

	BoundPlayerState = LpQuestPlayerState;
	BoundPlayerState->OnPendingRunUpgradeChoicesChangedEvent.AddUniqueDynamic(this, &UTunicRunStatusWidget::HandlePendingRunUpgradeChoicesChanged);
}

void UTunicRunStatusWidget::UnbindPlayerState()
{
	if (!BoundPlayerState)
	{
		return;
	}

	BoundPlayerState->OnPendingRunUpgradeChoicesChangedEvent.RemoveDynamic(this, &UTunicRunStatusWidget::HandlePendingRunUpgradeChoicesChanged);
	BoundPlayerState = nullptr;
}

FText UTunicRunStatusWidget::GetRunStateText(ETunicRunState RunState)
{
	switch (RunState)
	{
	case ETunicRunState::CombatActive:
		return NSLOCTEXT("LPQRunStatus", "CombatActive", "CombatActive");
	case ETunicRunState::PartyWiped:
		return NSLOCTEXT("LPQRunStatus", "PartyWiped", "PartyWiped");
	case ETunicRunState::PortalEventActive:
		return NSLOCTEXT("LPQRunStatus", "PortalEventActive", "PortalEventActive");
	case ETunicRunState::FloorTransitionReady:
		return NSLOCTEXT("LPQRunStatus", "FloorTransitionReady", "FloorTransitionReady");
	default:
		return NSLOCTEXT("LPQRunStatus", "Unknown", "Unknown");
	}
}
