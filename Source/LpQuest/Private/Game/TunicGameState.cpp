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
	DOREPLIFETIME(ATunicGameState, SharedRunLevel);
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

int32 ATunicGameState::GetSharedRunLevel() const
{
	return SharedRunLevel;
}

bool ATunicGameState::IsCombatActive() const
{
	return RunState == ETunicRunState::CombatActive || RunState == ETunicRunState::PortalEventActive;
}

bool ATunicGameState::IsPartyWiped() const
{
	return RunState == ETunicRunState::PartyWiped;
}

bool ATunicGameState::IsEncounterCleared() const
{
	return RunState == ETunicRunState::EncounterCleared;
}

bool ATunicGameState::IsPortalEventActive() const
{
	return RunState == ETunicRunState::PortalEventActive;
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
	OnRunStateChangedEvent.Broadcast(RunState);
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
	OnFloorIndexChangedEvent.Broadcast(CurrentFloorIndex);
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
	OnSharedRunExperienceChangedEvent.Broadcast(SharedRunExperience, SharedRunExperience - OldSharedRunExperience, SourceActor);
	OnSharedRunExperienceChanged(SharedRunExperience, SharedRunExperience - OldSharedRunExperience, SourceActor);
	RecalculateSharedRunLevel();
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

void ATunicGameState::OnSharedRunLevelChanged_Implementation(int32)
{
}

void ATunicGameState::RecalculateSharedRunLevel()
{
	if (!HasAuthority())
	{
		return;
	}

	const int32 ExperiencePerLevel = FMath::Max(1, SharedExperiencePerLevel);
	const int32 NewSharedRunLevel = FMath::Max(1, 1 + SharedRunExperience / ExperiencePerLevel);
	if (SharedRunLevel == NewSharedRunLevel)
	{
		return;
	}

	SharedRunLevel = NewSharedRunLevel;
	OnSharedRunLevelChangedEvent.Broadcast(SharedRunLevel);
	OnSharedRunLevelChanged(SharedRunLevel);
}

void ATunicGameState::OnRep_RunState()
{
	OnRunStateChangedEvent.Broadcast(RunState);
	OnRunStateChanged(RunState);
}

void ATunicGameState::OnRep_CurrentFloorIndex()
{
	OnFloorIndexChangedEvent.Broadcast(CurrentFloorIndex);
	OnFloorIndexChanged(CurrentFloorIndex);
}

void ATunicGameState::OnRep_SharedRunExperience(int32 OldSharedRunExperience)
{
	OnSharedRunExperienceChangedEvent.Broadcast(SharedRunExperience, SharedRunExperience - OldSharedRunExperience, nullptr);
	OnSharedRunExperienceChanged(SharedRunExperience, SharedRunExperience - OldSharedRunExperience, nullptr);
}

void ATunicGameState::OnRep_SharedRunLevel()
{
	OnSharedRunLevelChangedEvent.Broadcast(SharedRunLevel);
	OnSharedRunLevelChanged(SharedRunLevel);
}

