// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/TunicPlayerState.h"

#include "Ability/TunicAbilitySystemComponent.h"
#include "Ability/TunicAttributeSet.h"

ATunicPlayerState::ATunicPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UTunicAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UTunicAttributeSet>(TEXT("AttributeSet"));
	SetNetUpdateFrequency(100.0f);
}

UAbilitySystemComponent* ATunicPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UTunicAbilitySystemComponent* ATunicPlayerState::GetTunicAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UTunicAttributeSet* ATunicPlayerState::GetAttributeSet() const
{
	return AttributeSet;
}


