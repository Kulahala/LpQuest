// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/TunicGameState.h"
#include "Widgets/SWidget.h"
#include "TunicRunStatusWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class ATunicPlayerState;

UCLASS(Blueprintable)
class LPQUEST_API UTunicRunStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Run")
	void RefreshRunStatus();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tunic|Run")
	TObjectPtr<UTextBlock> FloorText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tunic|Run")
	TObjectPtr<UTextBlock> RunStateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tunic|Run")
	TObjectPtr<UTextBlock> SharedExperienceText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tunic|Run")
	TObjectPtr<UTextBlock> SharedLevelText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tunic|Run")
	TObjectPtr<UTextBlock> PendingUpgradeText;

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Run")
	void OnRunStatusRefreshed(int32 FloorIndex, ETunicRunState RunState, int32 SharedRunExperience, int32 SharedRunLevel, int32 PendingUpgradeChoices);

private:
	UFUNCTION()
	void HandleRunStateChanged(ETunicRunState NewRunState);

	UFUNCTION()
	void HandleFloorIndexChanged(int32 NewFloorIndex);

	UFUNCTION()
	void HandleSharedRunExperienceChanged(int32 NewValue, int32 Delta, AActor* SourceActor);

	UFUNCTION()
	void HandleSharedRunLevelChanged(int32 NewLevel);

	UFUNCTION()
	void HandlePendingRunUpgradeChoicesChanged(int32 PendingChoices);

	void BindGameState();
	void UnbindGameState();
	void BindPlayerState();
	void UnbindPlayerState();
	static FText GetRunStateText(ETunicRunState RunState);

	UPROPERTY(Transient)
	TObjectPtr<ATunicGameState> BoundGameState;

	UPROPERTY(Transient)
	TObjectPtr<ATunicPlayerState> BoundPlayerState;
};
