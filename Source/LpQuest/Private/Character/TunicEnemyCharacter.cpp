// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/TunicEnemyCharacter.h"

#include "Ability/TunicAbilitySystemComponent.h"
#include "Ability/TunicAttributeSet.h"

ATunicEnemyCharacter::ATunicEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UTunicAbilitySystemComponent* EnemyAbilitySystemComponent = CreateDefaultSubobject<UTunicAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	EnemyAbilitySystemComponent->SetIsReplicated(true);
	EnemyAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	UTunicAttributeSet* EnemyAttributeSet = CreateDefaultSubobject<UTunicAttributeSet>(TEXT("AttributeSet"));
	SetAbilitySystemReferences(EnemyAbilitySystemComponent, EnemyAttributeSet);
}

void ATunicEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

