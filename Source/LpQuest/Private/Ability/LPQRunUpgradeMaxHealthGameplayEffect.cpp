// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/LPQRunUpgradeMaxHealthGameplayEffect.h"

#include "Ability/LPQAttributeSet.h"

ULPQRunUpgradeMaxHealthGameplayEffect::ULPQRunUpgradeMaxHealthGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& MaxHealthModifier = Modifiers.AddDefaulted_GetRef();
	MaxHealthModifier.Attribute = ULPQAttributeSet::GetMaxHealthAttribute();
	MaxHealthModifier.ModifierOp = EGameplayModOp::Additive;
	MaxHealthModifier.ModifierMagnitude = FScalableFloat(20.0f);

	FGameplayModifierInfo& HealthModifier = Modifiers.AddDefaulted_GetRef();
	HealthModifier.Attribute = ULPQAttributeSet::GetHealthAttribute();
	HealthModifier.ModifierOp = EGameplayModOp::Additive;
	HealthModifier.ModifierMagnitude = FScalableFloat(20.0f);
}
