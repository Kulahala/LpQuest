// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TunicGameplayAbility_Dodge.generated.h"

class UGameplayEffect;

UCLASS()
class LPQUEST_API UTunicGameplayAbility_Dodge : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UTunicGameplayAbility_Dodge();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Dodge", meta = (ToolTip = "Dodge 成功激活后由服务器授予的短暂无敌 GameplayEffect。默认只授予 State.Invulnerable，不包含 cooldown 或 action-lock。"))
	TSubclassOf<UGameplayEffect> DodgeInvulnerabilityGameplayEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Dodge", meta = (ClampMin = "0.0", Units = "s", ToolTip = "Dodge cooldown 和 action-lock 的持续时间，单位秒。服务器 Ability 使用这个值覆盖 cooldown GameplayEffect 的实际 duration。"))
	float DodgeCooldownActionLockDuration = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Dodge", meta = (ClampMin = "0.0", Units = "s", ToolTip = "Dodge 成功激活后 State.Invulnerable 的持续时间，单位秒。建议短于 Dodge 位移时间，不要覆盖完整动作锁。"))
	float DodgeInvulnerabilityDuration = 0.28f;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;
};
