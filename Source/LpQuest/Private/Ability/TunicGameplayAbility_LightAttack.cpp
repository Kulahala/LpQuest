// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/TunicGameplayAbility_LightAttack.h"

#include "Ability/TunicLightAttackCooldownGameplayEffect.h"
#include "Ability/TunicLightAttackCostGameplayEffect.h"
#include "Character/TunicPlayerCharacter.h"
#include "GameplayTagContainer.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestAbility, Log, All);

UTunicGameplayAbility_LightAttack::UTunicGameplayAbility_LightAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	CostGameplayEffectClass = UTunicLightAttackCostGameplayEffect::StaticClass();
	CooldownGameplayEffectClass = UTunicLightAttackCooldownGameplayEffect::StaticClass();

	FGameplayTagContainer DefaultAbilityTags;
	if (const FGameplayTag LightAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack.Sword.Light"), false); LightAttackTag.IsValid())
	{
		DefaultAbilityTags.AddTag(LightAttackTag);
	}
	SetAssetTags(DefaultAbilityTags);

	if (const FGameplayTag CooldownAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Attack"), false); CooldownAttackTag.IsValid())
	{
		ActivationBlockedTags.AddTag(CooldownAttackTag);
	}

	if (const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"), false); DeadTag.IsValid())
	{
		ActivationBlockedTags.AddTag(DeadTag);
	}

	if (const FGameplayTag ActionLockedTag = FGameplayTag::RequestGameplayTag(TEXT("State.ActionLocked"), false); ActionLockedTag.IsValid())
	{
		ActivationBlockedTags.AddTag(ActionLockedTag);
	}
}

void UTunicGameplayAbility_LightAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ATunicPlayerCharacter* PlayerCharacter = ActorInfo ? Cast<ATunicPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (PlayerCharacter)
	{
		UE_LOG(LogLpQuestAbility, Display, TEXT("Light attack ability activated on server | Character=%s | OwnerActor=%s | AvatarActor=%s"),
			*GetNameSafe(PlayerCharacter),
			*GetNameSafe(ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr),
			*GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr));

		PlayerCharacter->ExecuteLightAttackAbility();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
