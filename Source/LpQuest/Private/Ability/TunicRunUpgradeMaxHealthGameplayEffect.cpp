// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/TunicRunUpgradeMaxHealthGameplayEffect.h"

#include "Ability/TunicAttributeSet.h"

UTunicRunUpgradeMaxHealthGameplayEffect::UTunicRunUpgradeMaxHealthGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& MaxHealthModifier = Modifiers.AddDefaulted_GetRef();
	MaxHealthModifier.Attribute = UTunicAttributeSet::GetMaxHealthAttribute();
	MaxHealthModifier.ModifierOp = EGameplayModOp::Additive;
	MaxHealthModifier.ModifierMagnitude = FScalableFloat(20.0f);

	FGameplayModifierInfo& HealthModifier = Modifiers.AddDefaulted_GetRef();
	HealthModifier.Attribute = UTunicAttributeSet::GetHealthAttribute();
	HealthModifier.ModifierOp = EGameplayModOp::Additive;
	HealthModifier.ModifierMagnitude = FScalableFloat(20.0f);
}
