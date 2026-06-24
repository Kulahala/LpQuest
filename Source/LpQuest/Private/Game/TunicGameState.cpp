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
	DOREPLIFETIME(ATunicGameState, SharedRunExperience);
}

ETunicRunState ATunicGameState::GetRunState() const
{
	return RunState;
}

int32 ATunicGameState::GetCurrentFloorIndex() const
{
	return CurrentFloorIndex;
}

int32 ATunicGameState::GetSharedRunExperience() const
{
	return SharedRunExperience;
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

void ATunicGameState::AddSharedRunExperience(int32 Amount, AActor* SourceActor)
{
	if (!HasAuthority() || Amount <= 0)
	{
		return;
	}

	const int32 OldSharedRunExperience = SharedRunExperience;
	const int64 NewSharedRunExperience = static_cast<int64>(SharedRunExperience) + static_cast<int64>(Amount);
	SharedRunExperience = static_cast<int32>(FMath::Clamp<int64>(NewSharedRunExperience, 0, MAX_int32));
	OnSharedRunExperienceChanged(SharedRunExperience, SharedRunExperience - OldSharedRunExperience, SourceActor);
}

void ATunicGameState::OnRunStateChanged_Implementation(ETunicRunState)
{
}

void ATunicGameState::OnFloorIndexChanged_Implementation(int32)
{
}

void ATunicGameState::OnSharedRunExperienceChanged_Implementation(int32, int32, AActor*)
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

void ATunicGameState::OnRep_SharedRunExperience(int32 OldSharedRunExperience)
{
	OnSharedRunExperienceChanged(SharedRunExperience, SharedRunExperience - OldSharedRunExperience, nullptr);
}

