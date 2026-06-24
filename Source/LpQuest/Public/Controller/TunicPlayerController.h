// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TunicPlayerController.generated.h"

class UInputMappingContext;
class UTunicRunStatusWidget;

UCLASS(Blueprintable)
class LPQUEST_API ATunicPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATunicPlayerController();

	UFUNCTION(BlueprintCallable, Category = "Tunic|Run")
	void RequestSelectRunUpgradeStub();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input")
	int32 DefaultMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|UI")
	TSubclassOf<UTunicRunStatusWidget> RunStatusWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UTunicRunStatusWidget> RunStatusWidget;

private:
	UFUNCTION(Server, Reliable)
	void ServerRequestSelectRunUpgradeStub();
};

