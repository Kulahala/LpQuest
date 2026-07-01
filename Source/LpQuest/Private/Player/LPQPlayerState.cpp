// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/LPQPlayerState.h"

#include "Ability/LPQAbilitySystemComponent.h"
#include "Ability/LPQAttributeSet.h"
#include "Net/UnrealNetwork.h"

ALPQPlayerState::ALPQPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<ULPQAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<ULPQAttributeSet>(TEXT("AttributeSet"));
	SetNetUpdateFrequency(100.0f);
}

void ALPQPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALPQPlayerState, PendingRunUpgradeChoices);
	DOREPLIFETIME(ALPQPlayerState, CurrentEquipmentId);
}

UAbilitySystemComponent* ALPQPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

ULPQAbilitySystemComponent* ALPQPlayerState::GetTunicAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

ULPQAttributeSet* ALPQPlayerState::GetAttributeSet() const
{
	return AttributeSet;
}

int32 ALPQPlayerState::GetPendingRunUpgradeChoices() const
{
	return PendingRunUpgradeChoices;
}

FName ALPQPlayerState::GetCurrentEquipmentId() const
{
	return CurrentEquipmentId;
}

void ALPQPlayerState::AddPendingRunUpgradeChoices(int32 Amount)
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

bool ALPQPlayerState::TryConsumePendingRunUpgradeChoice()
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

bool ALPQPlayerState::SetCurrentEquipmentId(FName NewEquipmentId)
{
	if (!HasAuthority() || NewEquipmentId.IsNone())
	{
		return false;
	}

	if (CurrentEquipmentId == NewEquipmentId)
	{
		return true;
	}

	CurrentEquipmentId = NewEquipmentId;
	OnCurrentEquipmentChangedEvent.Broadcast(CurrentEquipmentId);
	OnCurrentEquipmentChanged(CurrentEquipmentId);
	return true;
}

void ALPQPlayerState::OnPendingRunUpgradeChoicesChanged_Implementation(int32)
{
}

void ALPQPlayerState::OnRunUpgradeChoiceConsumed_Implementation()
{
}

void ALPQPlayerState::OnCurrentEquipmentChanged_Implementation(FName)
{
}

void ALPQPlayerState::OnRep_PendingRunUpgradeChoices()
{
	OnPendingRunUpgradeChoicesChangedEvent.Broadcast(PendingRunUpgradeChoices);
	OnPendingRunUpgradeChoicesChanged(PendingRunUpgradeChoices);
}

void ALPQPlayerState::OnRep_CurrentEquipmentId()
{
	OnCurrentEquipmentChangedEvent.Broadcast(CurrentEquipmentId);
	OnCurrentEquipmentChanged(CurrentEquipmentId);
}


