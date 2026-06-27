// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "TunicPlayerState.generated.h"

class UAbilitySystemComponent;
class UTunicAbilitySystemComponent;
class UTunicAttributeSet;
class FLifetimeProperty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTunicPendingRunUpgradeChoicesChangedSignature, int32, PendingChoices);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTunicRunUpgradeChoiceConsumedSignature);

UCLASS(Blueprintable)
class LPQUEST_API ATunicPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATunicPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Ability", meta = (ToolTip = "返回玩家 PlayerState-owned ASC。玩家角色作为 AvatarActor，能力和属性归 PlayerState 持有。"))
	UTunicAbilitySystemComponent* GetTunicAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Attributes", meta = (ToolTip = "返回玩家 AttributeSet。属性复制和 GameplayEffect 结算通过 PlayerState-owned ASC 管理。"))
	UTunicAttributeSet* GetAttributeSet() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run", meta = (ToolTip = "返回该玩家待选择的 run upgrade 次数。每个玩家独立拥有 pending，不是共享池。"))
	int32 GetPendingRunUpgradeChoices() const;

	UPROPERTY(BlueprintAssignable, Category = "Tunic|Run")
	FTunicPendingRunUpgradeChoicesChangedSignature OnPendingRunUpgradeChoicesChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Tunic|Run")
	FTunicRunUpgradeChoiceConsumedSignature OnRunUpgradeChoiceConsumedEvent;

	void AddPendingRunUpgradeChoices(int32 Amount);
	bool TryConsumePendingRunUpgradeChoice();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Run", meta = (ToolTip = "Pending upgrade choice 数量变化时的表现 hook。不要在这里直接应用属性或技能奖励。"))
	void OnPendingRunUpgradeChoicesChanged(int32 PendingChoices);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Run", meta = (ToolTip = "服务器成功消耗一次 pending upgrade choice 后触发的表现 hook。当前 stub 不授予真实奖励。"))
	void OnRunUpgradeChoiceConsumed();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Ability")
	TObjectPtr<UTunicAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Attributes")
	TObjectPtr<UTunicAttributeSet> AttributeSet;

private:
	UFUNCTION()
	void OnRep_PendingRunUpgradeChoices();

	UPROPERTY(ReplicatedUsing = OnRep_PendingRunUpgradeChoices)
	int32 PendingRunUpgradeChoices = 0;
};

