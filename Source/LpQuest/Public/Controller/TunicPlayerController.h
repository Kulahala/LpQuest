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

	UFUNCTION(BlueprintCallable, Category = "Tunic|Run", meta = (ToolTip = "本地 HUD 调用的临时升级选择入口。请求会走 owning PlayerController 的 Server RPC，由 PlayerState 消耗 pending。"))
	void RequestSelectRunUpgradeStub();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input", meta = (ToolTip = "本地玩家 BeginPlay 时加入的 Enhanced Input Mapping Context。"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input", meta = (ToolTip = "DefaultMappingContext 加入 Enhanced Input 时使用的优先级。数值越高越优先。"))
	int32 DefaultMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|UI", meta = (ToolTip = "本地 viewport 创建的 Run Status HUD class。可替换为 WBP 子类。"))
	TSubclassOf<UTunicRunStatusWidget> RunStatusWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UTunicRunStatusWidget> RunStatusWidget;

private:
	UFUNCTION(Server, Reliable)
	void ServerRequestSelectRunUpgradeStub();
};

