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

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
