// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/TunicStaminaRegenGameplayEffect.h"

#include "Ability/TunicAttributeSet.h"

UTunicStaminaRegenGameplayEffect::UTunicStaminaRegenGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period.Value = 0.25f;
	bExecutePeriodicEffectOnApplication = false;

	FGameplayModifierInfo& StaminaRegenModifier = Modifiers.AddDefaulted_GetRef();
	StaminaRegenModifier.Attribute = UTunicAttributeSet::GetStaminaAttribute();
	StaminaRegenModifier.ModifierOp = EGameplayModOp::Additive;
	StaminaRegenModifier.ModifierMagnitude = FScalableFloat(2.5f);
}
