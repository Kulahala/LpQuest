// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/TunicCharacterBase.h"

#include "Ability/TunicAbilitySystemComponent.h"
#include "Ability/TunicAttributeSet.h"

ATunicCharacterBase::ATunicCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAbilitySystemComponent* ATunicCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UTunicAbilitySystemComponent* ATunicCharacterBase::GetTunicAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UTunicAttributeSet* ATunicCharacterBase::GetAttributeSet() const
{
	return AttributeSet;
}

void ATunicCharacterBase::SetAbilitySystemReferences(UTunicAbilitySystemComponent* InAbilitySystemComponent, UTunicAttributeSet* InAttributeSet)
{
	AbilitySystemComponent = InAbilitySystemComponent;
	AttributeSet = InAttributeSet;
}

