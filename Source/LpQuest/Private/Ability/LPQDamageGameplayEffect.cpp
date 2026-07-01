// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/LPQDamageGameplayEffect.h"

#include "Ability/LPQAttributeSet.h"

ULPQDamageGameplayEffect::ULPQDamageGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& DamageModifier = Modifiers.AddDefaulted_GetRef();
	DamageModifier.Attribute = ULPQAttributeSet::GetHealthAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	DamageModifier.ModifierMagnitude = FScalableFloat(-10.0f);
}
