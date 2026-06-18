// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/TunicAttributeSet.h"

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

