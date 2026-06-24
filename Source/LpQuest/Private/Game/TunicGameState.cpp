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
	DOREPLIFETIME(ATunicGameState, CurrentFloorIndex);
}

ETunicRunState ATunicGameState::GetRunState() const
{
	return RunState;
}

int32 ATunicGameState::GetCurrentFloorIndex() const
{
	return CurrentFloorIndex;
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

bool ATunicGameState::IsFloorTransitionReady() const
{
	return RunState == ETunicRunState::FloorTransitionReady;
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

void ATunicGameState::SetCurrentFloorIndex(int32 NewFloorIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	const int32 ClampedFloorIndex = FMath::Max(1, NewFloorIndex);
	if (CurrentFloorIndex == ClampedFloorIndex)
	{
		return;
	}

	CurrentFloorIndex = ClampedFloorIndex;
	OnFloorIndexChanged(CurrentFloorIndex);
}

void ATunicGameState::OnRunStateChanged_Implementation(ETunicRunState)
{
}

void ATunicGameState::OnFloorIndexChanged_Implementation(int32)
{
}

void ATunicGameState::OnRep_RunState()
{
	OnRunStateChanged(RunState);
}

void ATunicGameState::OnRep_CurrentFloorIndex()
{
	OnFloorIndexChanged(CurrentFloorIndex);
}

