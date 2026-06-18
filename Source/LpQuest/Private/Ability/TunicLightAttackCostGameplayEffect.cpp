// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/TunicLightAttackCostGameplayEffect.h"

#include "Ability/TunicAttributeSet.h"

UTunicLightAttackCostGameplayEffect::UTunicLightAttackCostGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& StaminaCostModifier = Modifiers.AddDefaulted_GetRef();
	StaminaCostModifier.Attribute = UTunicAttributeSet::GetStaminaAttribute();
	StaminaCostModifier.ModifierOp = EGameplayModOp::Additive;
	StaminaCostModifier.ModifierMagnitude = FScalableFloat(-10.0f);
}
