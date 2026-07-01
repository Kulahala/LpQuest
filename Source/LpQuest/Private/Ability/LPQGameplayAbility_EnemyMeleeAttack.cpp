// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/LPQGameplayAbility_EnemyMeleeAttack.h"

#include "Ability/LPQEnemyMeleeAttackCooldownGameplayEffect.h"
#include "Character/LPQEnemyCharacter.h"
#include "GameplayTagContainer.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestEnemyAbility, Log, All);

ULPQGameplayAbility_EnemyMeleeAttack::ULPQGameplayAbility_EnemyMeleeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	CooldownGameplayEffectClass = ULPQEnemyMeleeAttackCooldownGameplayEffect::StaticClass();

	FGameplayTagContainer DefaultAbilityTags;
	if (const FGameplayTag EnemyMeleeAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack.Enemy.Melee"), false); EnemyMeleeAttackTag.IsValid())
	{
		DefaultAbilityTags.AddTag(EnemyMeleeAttackTag);
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

void ULPQGameplayAbility_EnemyMeleeAttack::ActivateAbility(
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

	ALPQEnemyCharacter* EnemyCharacter = ActorInfo ? Cast<ALPQEnemyCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (EnemyCharacter)
	{
		UE_LOG(LogLpQuestEnemyAbility, Display, TEXT("Enemy melee attack ability activated on server | Character=%s | OwnerActor=%s | AvatarActor=%s"),
			*GetNameSafe(EnemyCharacter),
			*GetNameSafe(ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr),
			*GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr));

		EnemyCharacter->ExecuteEnemyMeleeAttackAbility();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
