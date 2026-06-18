// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/TunicDamageGameplayEffect.h"

#include "Ability/TunicAttributeSet.h"

UTunicDamageGameplayEffect::UTunicDamageGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& DamageModifier = Modifiers.AddDefaulted_GetRef();
	DamageModifier.Attribute = UTunicAttributeSet::GetHealthAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	DamageModifier.ModifierMagnitude = FScalableFloat(-10.0f);
}
