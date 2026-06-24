// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/TunicRunStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/World.h"

void UTunicRunStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindGameState();
	RefreshRunStatus();
}

void UTunicRunStatusWidget::NativeDestruct()
{
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

		if (FloorText)
		{
			FloorText->SetText(FText::FromString(TEXT("Floor: 1")));
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
	}

	return Super::RebuildWidget();
}

void UTunicRunStatusWidget::RefreshRunStatus()
{
	BindGameState();

	const int32 FloorIndex = BoundGameState ? BoundGameState->GetCurrentFloorIndex() : 1;
	const ETunicRunState RunState = BoundGameState ? BoundGameState->GetRunState() : ETunicRunState::CombatActive;
	const int32 SharedRunExperience = BoundGameState ? BoundGameState->GetSharedRunExperience() : 0;

	if (FloorText)
	{
		FloorText->SetText(FText::Format(NSLOCTEXT("TunicRunStatus", "FloorFormat", "Floor: {0}"), FloorIndex));
	}

	if (RunStateText)
	{
		RunStateText->SetText(FText::Format(NSLOCTEXT("TunicRunStatus", "RunStateFormat", "Run: {0}"), GetRunStateText(RunState)));
	}

	if (SharedExperienceText)
	{
		SharedExperienceText->SetText(FText::Format(NSLOCTEXT("TunicRunStatus", "SharedExperienceFormat", "XP: {0}"), SharedRunExperience));
	}

	OnRunStatusRefreshed(FloorIndex, RunState, SharedRunExperience);
}

void UTunicRunStatusWidget::OnRunStatusRefreshed_Implementation(int32, ETunicRunState, int32)
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

void UTunicRunStatusWidget::HandleSharedRunExperienceChanged(int32, int32, AActor*)
{
	RefreshRunStatus();
}

void UTunicRunStatusWidget::BindGameState()
{
	ATunicGameState* TunicGameState = GetWorld() ? GetWorld()->GetGameState<ATunicGameState>() : nullptr;
	if (!TunicGameState || BoundGameState == TunicGameState)
	{
		return;
	}

	UnbindGameState();

	BoundGameState = TunicGameState;
	BoundGameState->OnRunStateChangedEvent.AddUniqueDynamic(this, &UTunicRunStatusWidget::HandleRunStateChanged);
	BoundGameState->OnFloorIndexChangedEvent.AddUniqueDynamic(this, &UTunicRunStatusWidget::HandleFloorIndexChanged);
	BoundGameState->OnSharedRunExperienceChangedEvent.AddUniqueDynamic(this, &UTunicRunStatusWidget::HandleSharedRunExperienceChanged);
}

void UTunicRunStatusWidget::UnbindGameState()
{
	if (!BoundGameState)
	{
		return;
	}

	BoundGameState->OnRunStateChangedEvent.RemoveDynamic(this, &UTunicRunStatusWidget::HandleRunStateChanged);
	BoundGameState->OnFloorIndexChangedEvent.RemoveDynamic(this, &UTunicRunStatusWidget::HandleFloorIndexChanged);
	BoundGameState->OnSharedRunExperienceChangedEvent.RemoveDynamic(this, &UTunicRunStatusWidget::HandleSharedRunExperienceChanged);
	BoundGameState = nullptr;
}

FText UTunicRunStatusWidget::GetRunStateText(ETunicRunState RunState)
{
	switch (RunState)
	{
	case ETunicRunState::CombatActive:
		return NSLOCTEXT("TunicRunStatus", "CombatActive", "CombatActive");
	case ETunicRunState::PartyWiped:
		return NSLOCTEXT("TunicRunStatus", "PartyWiped", "PartyWiped");
	case ETunicRunState::EncounterCleared:
		return NSLOCTEXT("TunicRunStatus", "EncounterCleared", "EncounterCleared");
	case ETunicRunState::FloorTransitionReady:
		return NSLOCTEXT("TunicRunStatus", "FloorTransitionReady", "FloorTransitionReady");
	default:
		return NSLOCTEXT("TunicRunStatus", "Unknown", "Unknown");
	}
}
