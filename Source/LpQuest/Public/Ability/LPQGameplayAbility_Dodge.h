// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "LPQGameplayAbility_Dodge.generated.h"

class UGameplayEffect;
class UAbilityTask_ApplyRootMotionConstantForce;
class ALPQPlayerCharacter;

UCLASS()
class LPQUEST_API ULPQGameplayAbility_Dodge : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULPQGameplayAbility_Dodge();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LPQ|Dodge", meta = (ToolTip = "Dodge 成功激活后由服务器授予的短暂无敌 GameplayEffect。默认只授予 State.Invulnerable，不包含 cooldown 或 action-lock。"))
	TSubclassOf<UGameplayEffect> DodgeInvulnerabilityGameplayEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LPQ|Dodge", meta = (ClampMin = "0.0", Units = "s", ToolTip = "Dodge cooldown 和 action-lock 的持续时间，单位秒。Ability 使用这个值覆盖 cooldown GameplayEffect 的实际 duration，服务器仍确认最终状态。"))
	float DodgeCooldownActionLockDuration = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LPQ|Dodge", meta = (ClampMin = "0.0", Units = "s", ToolTip = "Dodge 成功激活后 State.Invulnerable 的持续时间，单位秒。建议短于 Dodge 位移时间，不要覆盖完整动作锁。"))
	float DodgeInvulnerabilityDuration = 0.28f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LPQ|Dodge|Prediction", meta = (ToolTip = "Dodge predicted RootMotion AbilityTask 的实例名。只用于调试 RootMotionSource，不是玩法数值。"))
	FName DodgeRootMotionTaskName = TEXT("LPQDodgePredictedMovement");

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	UFUNCTION()
	void HandleDodgeRootMotionFinished();

	FVector ResolveDodgeDirection(const FGameplayEventData* TriggerEventData, const ALPQPlayerCharacter* PlayerCharacter) const;
	bool ApplyPredictedDodgeMovement(ALPQPlayerCharacter* PlayerCharacter, const FVector& DodgeDirection);
};
