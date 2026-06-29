// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TunicInteractableInterface.generated.h"

class ATunicPlayerCharacter;

UINTERFACE(BlueprintType)
class LPQUEST_API UTunicInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class LPQUEST_API ITunicInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tunic|Interaction", meta = (ToolTip = "服务器检查玩家当前是否可以和这个对象交互。客户端 UI 可只读查询，但不能用它直接改变 gameplay 状态。"))
	bool CanInteractWithTunicPlayer(ATunicPlayerCharacter* InteractingPlayer);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tunic|Interaction", meta = (ToolTip = "服务器确认交互后执行。后续拾取、Portal、机关都应复用这个统一交互入口。"))
	void InteractWithTunicPlayer(ATunicPlayerCharacter* InteractingPlayer);
};
