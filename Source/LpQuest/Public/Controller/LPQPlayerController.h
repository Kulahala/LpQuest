// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LPQPlayerController.generated.h"

class UInputMappingContext;
class ULPQRunStatusWidget;

UCLASS(Blueprintable)
class LPQUEST_API ALPQPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALPQPlayerController();

	UFUNCTION(BlueprintCallable, Category = "LPQ|Run", meta = (ToolTip = "本地 HUD 调用的 run upgrade 选择入口。请求会走 owning PlayerController 的 Server RPC，由 GameMode 校验 pending 并授予服务器 GameplayEffect。"))
	void RequestSelectRunUpgrade();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LPQ|Input", meta = (ToolTip = "本地玩家 BeginPlay 时加入的 Enhanced Input Mapping Context。"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LPQ|Input", meta = (ToolTip = "DefaultMappingContext 加入 Enhanced Input 时使用的优先级。数值越高越优先。"))
	int32 DefaultMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LPQ|UI", meta = (ToolTip = "本地 viewport 创建的 Run Status HUD class。可替换为 WBP 子类。"))
	TSubclassOf<ULPQRunStatusWidget> RunStatusWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<ULPQRunStatusWidget> RunStatusWidget;

private:
	UFUNCTION(Server, Reliable)
	void ServerRequestSelectRunUpgrade();
};

