// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "LPQGameplayAbility_LightAttack.generated.h"

UCLASS()
class LPQUEST_API ULPQGameplayAbility_LightAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULPQGameplayAbility_LightAttack();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
