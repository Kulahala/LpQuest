// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LPQInteractableInterface.generated.h"

class ALPQPlayerCharacter;

UINTERFACE(BlueprintType)
class LPQUEST_API ULPQInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class LPQUEST_API ILPQInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LPQ|Interaction", meta = (ToolTip = "服务器检查玩家当前是否可以和这个对象交互。客户端 UI 可只读查询，但不能用它直接改变 gameplay 状态。"))
	bool CanInteractWithTunicPlayer(ALPQPlayerCharacter* InteractingPlayer);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "LPQ|Interaction", meta = (ToolTip = "服务器确认交互后执行。后续拾取、Portal、机关都应复用这个统一交互入口。"))
	void InteractWithTunicPlayer(ALPQPlayerCharacter* InteractingPlayer);
};
