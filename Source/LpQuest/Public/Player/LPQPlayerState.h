// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "LPQPlayerState.generated.h"

class UAbilitySystemComponent;
class ULPQAbilitySystemComponent;
class ULPQAttributeSet;
class FLifetimeProperty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLPQPendingRunUpgradeChoicesChangedSignature, int32, PendingChoices);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLPQRunUpgradeChoiceConsumedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLPQCurrentEquipmentChangedSignature, FName, CurrentEquipmentId);

UCLASS(Blueprintable)
class LPQUEST_API ALPQPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ALPQPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "LPQ|Ability", meta = (ToolTip = "返回玩家 PlayerState-owned ASC。玩家角色作为 AvatarActor，能力和属性归 PlayerState 持有。"))
	ULPQAbilitySystemComponent* GetTunicAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category = "LPQ|Attributes", meta = (ToolTip = "返回玩家 AttributeSet。属性复制和 GameplayEffect 结算通过 PlayerState-owned ASC 管理。"))
	ULPQAttributeSet* GetAttributeSet() const;

	UFUNCTION(BlueprintPure, Category = "LPQ|Run", meta = (ToolTip = "返回该玩家待选择的 run upgrade 次数。每个玩家独立拥有 pending，不是共享池。"))
	int32 GetPendingRunUpgradeChoices() const;

	UFUNCTION(BlueprintPure, Category = "LPQ|Equipment", meta = (ToolTip = "返回当前拾取/装备标记。v1 只保存一个 FName，不代表完整库存系统。"))
	FName GetCurrentEquipmentId() const;

	UPROPERTY(BlueprintAssignable, Category = "LPQ|Run")
	FLPQPendingRunUpgradeChoicesChangedSignature OnPendingRunUpgradeChoicesChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "LPQ|Run")
	FLPQRunUpgradeChoiceConsumedSignature OnRunUpgradeChoiceConsumedEvent;

	UPROPERTY(BlueprintAssignable, Category = "LPQ|Equipment")
	FLPQCurrentEquipmentChangedSignature OnCurrentEquipmentChangedEvent;

	void AddPendingRunUpgradeChoices(int32 Amount);
	bool TryConsumePendingRunUpgradeChoice();
	bool SetCurrentEquipmentId(FName NewEquipmentId);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "LPQ|Run", meta = (ToolTip = "Pending upgrade choice 数量变化时的表现 hook。不要在这里直接应用属性或技能奖励。"))
	void OnPendingRunUpgradeChoicesChanged(int32 PendingChoices);

	UFUNCTION(BlueprintNativeEvent, Category = "LPQ|Run", meta = (ToolTip = "服务器成功消耗一次 pending upgrade choice 后触发的表现 hook。真实升级效果由 GameMode 授予到 PlayerState-owned ASC。"))
	void OnRunUpgradeChoiceConsumed();

	UFUNCTION(BlueprintNativeEvent, Category = "LPQ|Equipment", meta = (ToolTip = "当前装备标记变化时的表现 hook。v1 只表示拾取到的 CurrentEquipmentId，不要在这里授予 Ability 或改属性。"))
	void OnCurrentEquipmentChanged(FName NewCurrentEquipmentId);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LPQ|Ability")
	TObjectPtr<ULPQAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LPQ|Attributes")
	TObjectPtr<ULPQAttributeSet> AttributeSet;

private:
	UFUNCTION()
	void OnRep_PendingRunUpgradeChoices();

	UFUNCTION()
	void OnRep_CurrentEquipmentId();

	UPROPERTY(ReplicatedUsing = OnRep_PendingRunUpgradeChoices)
	int32 PendingRunUpgradeChoices = 0;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentEquipmentId)
	FName CurrentEquipmentId = NAME_None;
};

