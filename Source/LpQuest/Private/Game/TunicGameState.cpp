// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/TunicGameState.h"

#include "Net/UnrealNetwork.h"

ATunicGameState::ATunicGameState()
{
}

void ATunicGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATunicGameState, RunState);
}

ETunicRunState ATunicGameState::GetRunState() const
{
	return RunState;
}

bool ATunicGameState::IsCombatActive() const
{
	return RunState == ETunicRunState::CombatActive;
}

bool ATunicGameState::IsPartyWiped() const
{
	return RunState == ETunicRunState::PartyWiped;
}

bool ATunicGameState::IsEncounterCleared() const
{
	return RunState == ETunicRunState::EncounterCleared;
}

void ATunicGameState::SetRunState(ETunicRunState NewRunState)
{
	if (!HasAuthority() || RunState == NewRunState)
	{
		return;
	}

	RunState = NewRunState;
	OnRunStateChanged(RunState);
}

void ATunicGameState::OnRunStateChanged_Implementation(ETunicRunState)
{
}

void ATunicGameState::OnRep_RunState()
{
	OnRunStateChanged(RunState);
}

