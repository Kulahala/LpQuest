// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/LPQLightAttackCooldownGameplayEffect.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayTagContainer.h"

ULPQLightAttackCooldownGameplayEffect::ULPQLightAttackCooldownGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.35f));

	if (const FGameplayTag CooldownAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Attack"), false); CooldownAttackTag.IsValid())
	{
		FInheritedTagContainer GrantedTags;
		GrantedTags.AddTag(CooldownAttackTag);
		if (const FGameplayTag ActionLockedTag = FGameplayTag::RequestGameplayTag(TEXT("State.ActionLocked"), false); ActionLockedTag.IsValid())
		{
			GrantedTags.AddTag(ActionLockedTag);
		}
		UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
		TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTags);
	}
}
