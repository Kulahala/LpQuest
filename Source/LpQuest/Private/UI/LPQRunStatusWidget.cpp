// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/LPQRunStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Controller/LPQPlayerController.h"
#include "Engine/World.h"
#include "Player/LPQPlayerState.h"

void ULPQRunStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindGameState();
	BindPlayerState();
	if (SelectUpgradeButton)
	{
		SelectUpgradeButton->OnClicked.AddUniqueDynamic(this, &ULPQRunStatusWidget::HandleSelectUpgradeClicked);
	}
	RefreshRunStatus();
}

void ULPQRunStatusWidget::NativeDestruct()
{
	if (SelectUpgradeButton)
	{
		SelectUpgradeButton->OnClicked.RemoveDynamic(this, &ULPQRunStatusWidget::HandleSelectUpgradeClicked);
	}
	UnbindPlayerState();
	UnbindGameState();

	Super::NativeDestruct();
}

TSharedRef<SWidget> ULPQRunStatusWidget::RebuildWidget()
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

void ULPQRunStatusWidget::RefreshRunStatus()
{
	BindGameState();
	BindPlayerState();

	const int32 FloorIndex = BoundGameState ? BoundGameState->GetCurrentFloorIndex() : 1;
	const FName FloorDestinationId = BoundGameState ? BoundGameState->GetCurrentFloorDestinationId() : FName(TEXT("Start"));
	const ELPQRunState RunState = BoundGameState ? BoundGameState->GetRunState() : ELPQRunState::CombatActive;
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

void ULPQRunStatusWidget::OnRunStatusRefreshed_Implementation(int32, FName, ELPQRunState, int32, int32, int32)
{
}

void ULPQRunStatusWidget::HandleRunStateChanged(ELPQRunState)
{
	RefreshRunStatus();
}

void ULPQRunStatusWidget::HandleFloorIndexChanged(int32)
{
	RefreshRunStatus();
}

void ULPQRunStatusWidget::HandleFloorDestinationChanged(FName)
{
	RefreshRunStatus();
}

void ULPQRunStatusWidget::HandleSharedRunExperienceChanged(int32, int32, AActor*)
{
	RefreshRunStatus();
}

void ULPQRunStatusWidget::HandleSharedRunLevelChanged(int32)
{
	RefreshRunStatus();
}

void ULPQRunStatusWidget::HandlePendingRunUpgradeChoicesChanged(int32)
{
	RefreshRunStatus();
}

void ULPQRunStatusWidget::HandleSelectUpgradeClicked()
{
	ALPQPlayerController* LpQuestPlayerController = Cast<ALPQPlayerController>(GetOwningPlayer());
	if (!LpQuestPlayerController)
	{
		return;
	}

	LpQuestPlayerController->RequestSelectRunUpgrade();
}

void ULPQRunStatusWidget::BindGameState()
{
	ALPQGameState* LpQuestGameState = GetWorld() ? GetWorld()->GetGameState<ALPQGameState>() : nullptr;
	if (!LpQuestGameState || BoundGameState == LpQuestGameState)
	{
		return;
	}

	UnbindGameState();

	BoundGameState = LpQuestGameState;
	BoundGameState->OnRunStateChangedEvent.AddUniqueDynamic(this, &ULPQRunStatusWidget::HandleRunStateChanged);
	BoundGameState->OnFloorIndexChangedEvent.AddUniqueDynamic(this, &ULPQRunStatusWidget::HandleFloorIndexChanged);
	BoundGameState->OnFloorDestinationChangedEvent.AddUniqueDynamic(this, &ULPQRunStatusWidget::HandleFloorDestinationChanged);
	BoundGameState->OnSharedRunExperienceChangedEvent.AddUniqueDynamic(this, &ULPQRunStatusWidget::HandleSharedRunExperienceChanged);
	BoundGameState->OnSharedRunLevelChangedEvent.AddUniqueDynamic(this, &ULPQRunStatusWidget::HandleSharedRunLevelChanged);
}

void ULPQRunStatusWidget::UnbindGameState()
{
	if (!BoundGameState)
	{
		return;
	}

	BoundGameState->OnRunStateChangedEvent.RemoveDynamic(this, &ULPQRunStatusWidget::HandleRunStateChanged);
	BoundGameState->OnFloorIndexChangedEvent.RemoveDynamic(this, &ULPQRunStatusWidget::HandleFloorIndexChanged);
	BoundGameState->OnFloorDestinationChangedEvent.RemoveDynamic(this, &ULPQRunStatusWidget::HandleFloorDestinationChanged);
	BoundGameState->OnSharedRunExperienceChangedEvent.RemoveDynamic(this, &ULPQRunStatusWidget::HandleSharedRunExperienceChanged);
	BoundGameState->OnSharedRunLevelChangedEvent.RemoveDynamic(this, &ULPQRunStatusWidget::HandleSharedRunLevelChanged);
	BoundGameState = nullptr;
}

void ULPQRunStatusWidget::BindPlayerState()
{
	APlayerController* OwningPlayerController = GetOwningPlayer();
	ALPQPlayerState* LpQuestPlayerState = OwningPlayerController ? OwningPlayerController->GetPlayerState<ALPQPlayerState>() : nullptr;
	if (!LpQuestPlayerState || BoundPlayerState == LpQuestPlayerState)
	{
		return;
	}

	UnbindPlayerState();

	BoundPlayerState = LpQuestPlayerState;
	BoundPlayerState->OnPendingRunUpgradeChoicesChangedEvent.AddUniqueDynamic(this, &ULPQRunStatusWidget::HandlePendingRunUpgradeChoicesChanged);
}

void ULPQRunStatusWidget::UnbindPlayerState()
{
	if (!BoundPlayerState)
	{
		return;
	}

	BoundPlayerState->OnPendingRunUpgradeChoicesChangedEvent.RemoveDynamic(this, &ULPQRunStatusWidget::HandlePendingRunUpgradeChoicesChanged);
	BoundPlayerState = nullptr;
}

FText ULPQRunStatusWidget::GetRunStateText(ELPQRunState RunState)
{
	switch (RunState)
	{
	case ELPQRunState::CombatActive:
		return NSLOCTEXT("LPQRunStatus", "CombatActive", "CombatActive");
	case ELPQRunState::PartyWiped:
		return NSLOCTEXT("LPQRunStatus", "PartyWiped", "PartyWiped");
	case ELPQRunState::PortalEventActive:
		return NSLOCTEXT("LPQRunStatus", "PortalEventActive", "PortalEventActive");
	case ELPQRunState::FloorTransitionReady:
		return NSLOCTEXT("LPQRunStatus", "FloorTransitionReady", "FloorTransitionReady");
	default:
		return NSLOCTEXT("LPQRunStatus", "Unknown", "Unknown");
	}
}
