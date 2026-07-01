// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/LPQGameState.h"
#include "Widgets/SWidget.h"
#include "LPQRunStatusWidget.generated.h"

class UTextBlock;
class UButton;
class UVerticalBox;
class ALPQPlayerState;

UCLASS(Blueprintable)
class LPQUEST_API ULPQRunStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintCallable, Category = "LPQ|Run", meta = (ToolTip = "立即从 replicated GameState / owning PlayerState 读取 Floor、RunState、XP、Level 和 pending upgrade，并刷新 HUD。"))
	void RefreshRunStatus();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "LPQ|Run")
	TObjectPtr<UTextBlock> FloorText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "LPQ|Run")
	TObjectPtr<UTextBlock> RunStateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "LPQ|Run")
	TObjectPtr<UTextBlock> SharedExperienceText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "LPQ|Run")
	TObjectPtr<UTextBlock> SharedLevelText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "LPQ|Run")
	TObjectPtr<UTextBlock> PendingUpgradeText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "LPQ|Run")
	TObjectPtr<UButton> SelectUpgradeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "LPQ|Run")
	TObjectPtr<UTextBlock> SelectUpgradeButtonText;

	UFUNCTION(BlueprintNativeEvent, Category = "LPQ|Run", meta = (ToolTip = "Run Status HUD 刷新后的表现 hook。只用于 UI/动画/音效，不要在这里修改 XP、Level、RunState 或 pending。"))
	void OnRunStatusRefreshed(int32 FloorIndex, FName NewFloorDestinationId, ELPQRunState RunState, int32 SharedRunExperience, int32 SharedRunLevel, int32 PendingUpgradeChoices);

private:
	UFUNCTION()
	void HandleRunStateChanged(ELPQRunState NewRunState);

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
	static FText GetRunStateText(ELPQRunState RunState);

	UPROPERTY(Transient)
	TObjectPtr<ALPQGameState> BoundGameState;

	UPROPERTY(Transient)
	TObjectPtr<ALPQPlayerState> BoundPlayerState;
};
