// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/LPQInteractableInterface.h"
#include "LPQPickupActor.generated.h"

class ALPQPlayerCharacter;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class LPQUEST_API ALPQPickupActor : public AActor, public ILPQInteractableInterface
{
	GENERATED_BODY()

public:
	ALPQPickupActor();

	virtual bool CanInteractWithTunicPlayer_Implementation(ALPQPlayerCharacter* InteractingPlayer) override;
	virtual void InteractWithTunicPlayer_Implementation(ALPQPlayerCharacter* InteractingPlayer) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LPQ|Pickup")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LPQ|Pickup")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LPQ|Pickup", meta = (ToolTip = "拾取后写入玩家 PlayerState 的当前装备标记。None 会拒绝拾取并输出 warning，避免假成功。"))
	FName PickupId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LPQ|Pickup", meta = (ToolTip = "拾取物显示名。v1 只给 Blueprint/UI 表现使用，不参与 gameplay 判定。"))
	FText PickupDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LPQ|Debug", meta = (ToolTip = "是否输出 pickup 交互、拒绝和成功日志。只用于验证，不影响 gameplay。"))
	bool bLogPickupInteraction = true;

	UFUNCTION(BlueprintNativeEvent, Category = "LPQ|Pickup", meta = (ToolTip = "服务器确认拾取成功后、Actor 销毁前触发的表现 hook。不要在这里改装备状态。"))
	void OnPickedUp(ALPQPlayerCharacter* InteractingPlayer);

private:
	bool bPickupConsumed = false;
};
