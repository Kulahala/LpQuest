// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TunicGameplayAbility_Dodge.generated.h"

UCLASS()
class LPQUEST_API UTunicGameplayAbility_Dodge : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UTunicGameplayAbility_Dodge();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
