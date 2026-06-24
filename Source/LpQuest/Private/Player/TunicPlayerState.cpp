// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/TunicPlayerState.h"

#include "Ability/TunicAbilitySystemComponent.h"
#include "Ability/TunicAttributeSet.h"
#include "Net/UnrealNetwork.h"

ATunicPlayerState::ATunicPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UTunicAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UTunicAttributeSet>(TEXT("AttributeSet"));
	SetNetUpdateFrequency(100.0f);
}

void ATunicPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATunicPlayerState, PendingRunUpgradeChoices);
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

int32 ATunicPlayerState::GetPendingRunUpgradeChoices() const
{
	return PendingRunUpgradeChoices;
}

void ATunicPlayerState::AddPendingRunUpgradeChoices(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0)
	{
		return;
	}

	const int64 NewPendingChoices = static_cast<int64>(PendingRunUpgradeChoices) + static_cast<int64>(Amount);
	PendingRunUpgradeChoices = static_cast<int32>(FMath::Clamp<int64>(NewPendingChoices, 0, MAX_int32));
	OnPendingRunUpgradeChoicesChangedEvent.Broadcast(PendingRunUpgradeChoices);
	OnPendingRunUpgradeChoicesChanged(PendingRunUpgradeChoices);
}

bool ATunicPlayerState::TryConsumePendingRunUpgradeChoice()
{
	if (!HasAuthority() || PendingRunUpgradeChoices <= 0)
	{
		return false;
	}

	--PendingRunUpgradeChoices;
	OnPendingRunUpgradeChoicesChangedEvent.Broadcast(PendingRunUpgradeChoices);
	OnPendingRunUpgradeChoicesChanged(PendingRunUpgradeChoices);
	OnRunUpgradeChoiceConsumedEvent.Broadcast();
	OnRunUpgradeChoiceConsumed();
	return true;
}

void ATunicPlayerState::OnPendingRunUpgradeChoicesChanged_Implementation(int32)
{
}

void ATunicPlayerState::OnRunUpgradeChoiceConsumed_Implementation()
{
}

void ATunicPlayerState::OnRep_PendingRunUpgradeChoices()
{
	OnPendingRunUpgradeChoicesChangedEvent.Broadcast(PendingRunUpgradeChoices);
	OnPendingRunUpgradeChoicesChanged(PendingRunUpgradeChoices);
}


