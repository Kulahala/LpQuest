// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TunicGameplayAbility_EnemyMeleeAttack.generated.h"

UCLASS()
class LPQUEST_API UTunicGameplayAbility_EnemyMeleeAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UTunicGameplayAbility_EnemyMeleeAttack();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
