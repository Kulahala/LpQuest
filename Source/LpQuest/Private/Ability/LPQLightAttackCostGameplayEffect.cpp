// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/LPQLightAttackCostGameplayEffect.h"

#include "Ability/LPQAttributeSet.h"

ULPQLightAttackCostGameplayEffect::ULPQLightAttackCostGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& StaminaCostModifier = Modifiers.AddDefaulted_GetRef();
	StaminaCostModifier.Attribute = ULPQAttributeSet::GetStaminaAttribute();
	StaminaCostModifier.ModifierOp = EGameplayModOp::Additive;
	StaminaCostModifier.ModifierMagnitude = FScalableFloat(-10.0f);
}
