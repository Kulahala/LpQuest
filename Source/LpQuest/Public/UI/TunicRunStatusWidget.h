// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/TunicGameState.h"
#include "Widgets/SWidget.h"
#include "TunicRunStatusWidget.generated.h"

class UTextBlock;
class UVerticalBox;

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

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Run")
	void OnRunStatusRefreshed(int32 FloorIndex, ETunicRunState RunState, int32 SharedRunExperience);

private:
	UFUNCTION()
	void HandleRunStateChanged(ETunicRunState NewRunState);

	UFUNCTION()
	void HandleFloorIndexChanged(int32 NewFloorIndex);

	UFUNCTION()
	void HandleSharedRunExperienceChanged(int32 NewValue, int32 Delta, AActor* SourceActor);

	void BindGameState();
	void UnbindGameState();
	static FText GetRunStateText(ETunicRunState RunState);

	UPROPERTY(Transient)
	TObjectPtr<ATunicGameState> BoundGameState;
};
