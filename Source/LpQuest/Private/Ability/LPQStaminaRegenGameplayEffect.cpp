// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/LPQStaminaRegenGameplayEffect.h"

#include "Ability/LPQAttributeSet.h"

ULPQStaminaRegenGameplayEffect::ULPQStaminaRegenGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period.Value = 0.25f;
	bExecutePeriodicEffectOnApplication = false;

	FGameplayModifierInfo& StaminaRegenModifier = Modifiers.AddDefaulted_GetRef();
	StaminaRegenModifier.Attribute = ULPQAttributeSet::GetStaminaAttribute();
	StaminaRegenModifier.ModifierOp = EGameplayModOp::Additive;
	StaminaRegenModifier.ModifierMagnitude = FScalableFloat(2.5f);
}
