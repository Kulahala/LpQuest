// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/LPQCharacterBase.h"

#include "Ability/LPQAbilitySystemComponent.h"
#include "Ability/LPQAttributeSet.h"

ALPQCharacterBase::ALPQCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAbilitySystemComponent* ALPQCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

ULPQAbilitySystemComponent* ALPQCharacterBase::GetLPQAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

ULPQAttributeSet* ALPQCharacterBase::GetAttributeSet() const
{
	return AttributeSet;
}

void ALPQCharacterBase::SetAbilitySystemReferences(ULPQAbilitySystemComponent* InAbilitySystemComponent, ULPQAttributeSet* InAttributeSet)
{
	AbilitySystemComponent = InAbilitySystemComponent;
	AttributeSet = InAttributeSet;
}
