// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/TunicGameState.h"
#include "Widgets/SWidget.h"
#include "TunicRunStatusWidget.generated.h"

class UTextBlock;
class UButton;
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

	UFUNCTION(BlueprintCallable, Category = "Tunic|Run", meta = (ToolTip = "立即从 replicated GameState / owning PlayerState 读取 Floor、RunState、XP、Level 和 pending upgrade，并刷新 HUD。"))
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

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tunic|Run")
	TObjectPtr<UButton> SelectUpgradeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tunic|Run")
	TObjectPtr<UTextBlock> SelectUpgradeButtonText;

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Run", meta = (ToolTip = "Run Status HUD 刷新后的表现 hook。只用于 UI/动画/音效，不要在这里修改 XP、Level、RunState 或 pending。"))
	void OnRunStatusRefreshed(int32 FloorIndex, FName NewFloorDestinationId, ETunicRunState RunState, int32 SharedRunExperience, int32 SharedRunLevel, int32 PendingUpgradeChoices);

private:
	UFUNCTION()
	void HandleRunStateChanged(ETunicRunState NewRunState);

	UFUNCTION()
	void HandleFloorIndexChanged(int32 NewFloorIndex);

	UFUNCTION()
	void HandleFloorDestinationChanged(FName NewFloorDestinationId);

	UFUNCTION()
	void HandleSharedRunExperienceChanged(int32 NewValue, int32 Delta, AActor* SourceActor);

	UFUNCTION()
	void HandleSharedRunLevelChanged(int32 NewLevel);

	UFUNCTION()
	void HandlePendingRunUpgradeChoicesChanged(int32 PendingChoices);

	UFUNCTION()
	void HandleSelectUpgradeClicked();

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
