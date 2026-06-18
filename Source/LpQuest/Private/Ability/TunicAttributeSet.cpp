// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/TunicAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UTunicAttributeSet::UTunicAttributeSet()
{
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitStamina(100.0f);
	InitMaxStamina(100.0f);
	InitElementalPower(0.0f);
}

void UTunicAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UTunicAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTunicAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTunicAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTunicAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTunicAttributeSet, ElementalPower, COND_None, REPNOTIFY_Always);
}

void UTunicAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UTunicAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UTunicAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	float NewValue = 0.0f;
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		NewValue = GetHealth();
		ClampAttribute(Data.EvaluatedData.Attribute, NewValue);
		SetHealth(NewValue);
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		NewValue = GetStamina();
		ClampAttribute(Data.EvaluatedData.Attribute, NewValue);
		SetStamina(NewValue);
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		NewValue = GetHealth();
		ClampAttribute(GetHealthAttribute(), NewValue);
		SetHealth(NewValue);
	}
	else if (Data.EvaluatedData.Attribute == GetMaxStaminaAttribute())
	{
		NewValue = GetStamina();
		ClampAttribute(GetStaminaAttribute(), NewValue);
		SetStamina(NewValue);
	}
}

void UTunicAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetElementalPowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UTunicAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTunicAttributeSet, Health, OldHealth);
}

void UTunicAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTunicAttributeSet, MaxHealth, OldMaxHealth);
}

void UTunicAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTunicAttributeSet, Stamina, OldStamina);
}

void UTunicAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTunicAttributeSet, MaxStamina, OldMaxStamina);
}

void UTunicAttributeSet::OnRep_ElementalPower(const FGameplayAttributeData& OldElementalPower)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTunicAttributeSet, ElementalPower, OldElementalPower);
}

