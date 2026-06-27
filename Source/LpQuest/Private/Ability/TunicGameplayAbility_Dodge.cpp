// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/TunicGameplayAbility_Dodge.h"

#include "Ability/TunicDodgeCooldownGameplayEffect.h"
#include "Ability/TunicDodgeInvulnerabilityGameplayEffect.h"
#include "Character/TunicPlayerCharacter.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestDodgeAbility, Log, All);

UTunicGameplayAbility_Dodge::UTunicGameplayAbility_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	CooldownGameplayEffectClass = UTunicDodgeCooldownGameplayEffect::StaticClass();
	DodgeInvulnerabilityGameplayEffectClass = UTunicDodgeInvulnerabilityGameplayEffect::StaticClass();

	FGameplayTagContainer DefaultAbilityTags;
	if (const FGameplayTag DodgeTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Movement.Dodge"), false); DodgeTag.IsValid())
	{
		DefaultAbilityTags.AddTag(DodgeTag);
	}
	SetAssetTags(DefaultAbilityTags);

	if (const FGameplayTag CooldownDodgeTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Dodge"), false); CooldownDodgeTag.IsValid())
	{
		ActivationBlockedTags.AddTag(CooldownDodgeTag);
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

void UTunicGameplayAbility_Dodge::ActivateAbility(
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
		if (DodgeInvulnerabilityGameplayEffectClass)
		{
			FGameplayEffectSpecHandle InvulnerabilitySpecHandle = MakeOutgoingGameplayEffectSpec(
				Handle,
				ActorInfo,
				ActivationInfo,
				DodgeInvulnerabilityGameplayEffectClass,
				GetAbilityLevel(Handle, ActorInfo));
			if (InvulnerabilitySpecHandle.IsValid())
			{
				InvulnerabilitySpecHandle.Data->SetDuration(FMath::Max(0.0f, DodgeInvulnerabilityDuration), true);
				ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, InvulnerabilitySpecHandle);
			}
		}

		UE_LOG(LogLpQuestDodgeAbility, Display, TEXT("Dodge ability activated on server | Character=%s | OwnerActor=%s | AvatarActor=%s | CooldownActionLockDuration=%.3f | InvulnerabilityDuration=%.3f"),
			*GetNameSafe(PlayerCharacter),
			*GetNameSafe(ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr),
			*GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr),
			DodgeCooldownActionLockDuration,
			DodgeInvulnerabilityDuration);

		PlayerCharacter->ExecuteDodgeAbility();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UTunicGameplayAbility_Dodge::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!CooldownGameplayEffectClass)
	{
		return;
	}

	FGameplayEffectSpecHandle CooldownSpecHandle = MakeOutgoingGameplayEffectSpec(
		Handle,
		ActorInfo,
		ActivationInfo,
		CooldownGameplayEffectClass,
		GetAbilityLevel(Handle, ActorInfo));
	if (!CooldownSpecHandle.IsValid())
	{
		return;
	}

	CooldownSpecHandle.Data->SetDuration(FMath::Max(0.0f, DodgeCooldownActionLockDuration), true);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpecHandle);
}
