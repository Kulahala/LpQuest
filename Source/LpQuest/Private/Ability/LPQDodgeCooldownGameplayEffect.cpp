// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/LPQDodgeCooldownGameplayEffect.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayTagContainer.h"

ULPQDodgeCooldownGameplayEffect::ULPQDodgeCooldownGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.45f));

	FInheritedTagContainer GrantedTags;
	bool bHasGrantedTags = false;
	if (const FGameplayTag CooldownDodgeTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Dodge"), false); CooldownDodgeTag.IsValid())
	{
		GrantedTags.AddTag(CooldownDodgeTag);
		bHasGrantedTags = true;
	}
	if (const FGameplayTag ActionLockedTag = FGameplayTag::RequestGameplayTag(TEXT("State.ActionLocked"), false); ActionLockedTag.IsValid())
	{
		GrantedTags.AddTag(ActionLockedTag);
		bHasGrantedTags = true;
	}

	if (bHasGrantedTags)
	{
		UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
		TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTags);
	}
}
