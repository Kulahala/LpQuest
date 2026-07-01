// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/LPQGameState.h"

#include "Net/UnrealNetwork.h"

ALPQGameState::ALPQGameState()
{
}

void ALPQGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALPQGameState, RunState);
	DOREPLIFETIME(ALPQGameState, CurrentFloorIndex);
	DOREPLIFETIME(ALPQGameState, CurrentFloorDestinationId);
	DOREPLIFETIME(ALPQGameState, SharedRunExperience);
	DOREPLIFETIME(ALPQGameState, SharedRunLevel);
}

ELPQRunState ALPQGameState::GetRunState() const
{
	return RunState;
}

int32 ALPQGameState::GetCurrentFloorIndex() const
{
	return CurrentFloorIndex;
}

FName ALPQGameState::GetCurrentFloorDestinationId() const
{
	return CurrentFloorDestinationId;
}

int32 ALPQGameState::GetSharedRunExperience() const
{
	return SharedRunExperience;
}

int32 ALPQGameState::GetSharedRunLevel() const
{
	return SharedRunLevel;
}

bool ALPQGameState::IsCombatActive() const
{
	return RunState == ELPQRunState::CombatActive || RunState == ELPQRunState::PortalEventActive;
}

bool ALPQGameState::IsPartyWiped() const
{
	return RunState == ELPQRunState::PartyWiped;
}

bool ALPQGameState::IsPortalEventActive() const
{
	return RunState == ELPQRunState::PortalEventActive;
}

bool ALPQGameState::IsFloorTransitionReady() const
{
	return RunState == ELPQRunState::FloorTransitionReady;
}

void ALPQGameState::SetRunState(ELPQRunState NewRunState)
{
	if (!HasAuthority() || RunState == NewRunState)
	{
		return;
	}

	RunState = NewRunState;
	OnRunStateChangedEvent.Broadcast(RunState);
	OnRunStateChanged(RunState);
}

void ALPQGameState::SetCurrentFloorIndex(int32 NewFloorIndex)
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

void ALPQGameState::SetCurrentFloorDestinationId(FName NewFloorDestinationId)
{
	if (!HasAuthority())
	{
		return;
	}

	const FName SanitizedFloorDestinationId = NewFloorDestinationId.IsNone() ? FName(TEXT("Next")) : NewFloorDestinationId;
	if (CurrentFloorDestinationId == SanitizedFloorDestinationId)
	{
		return;
	}

	CurrentFloorDestinationId = SanitizedFloorDestinationId;
	OnFloorDestinationChangedEvent.Broadcast(CurrentFloorDestinationId);
	OnFloorDestinationChanged(CurrentFloorDestinationId);
}

void ALPQGameState::AddSharedRunExperience(int32 Amount, AActor* SourceActor)
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

void ALPQGameState::OnRunStateChanged_Implementation(ELPQRunState)
{
}

void ALPQGameState::OnFloorIndexChanged_Implementation(int32)
{
}

void ALPQGameState::OnFloorDestinationChanged_Implementation(FName)
{
}

void ALPQGameState::OnSharedRunExperienceChanged_Implementation(int32, int32, AActor*)
{
}

void ALPQGameState::OnSharedRunLevelChanged_Implementation(int32)
{
}

void ALPQGameState::RecalculateSharedRunLevel()
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

void ALPQGameState::OnRep_RunState()
{
	OnRunStateChangedEvent.Broadcast(RunState);
	OnRunStateChanged(RunState);
}

void ALPQGameState::OnRep_CurrentFloorIndex()
{
	OnFloorIndexChangedEvent.Broadcast(CurrentFloorIndex);
	OnFloorIndexChanged(CurrentFloorIndex);
}

void ALPQGameState::OnRep_CurrentFloorDestinationId()
{
	OnFloorDestinationChangedEvent.Broadcast(CurrentFloorDestinationId);
	OnFloorDestinationChanged(CurrentFloorDestinationId);
}

void ALPQGameState::OnRep_SharedRunExperience(int32 OldSharedRunExperience)
{
	OnSharedRunExperienceChangedEvent.Broadcast(SharedRunExperience, SharedRunExperience - OldSharedRunExperience, nullptr);
	OnSharedRunExperienceChanged(SharedRunExperience, SharedRunExperience - OldSharedRunExperience, nullptr);
}

void ALPQGameState::OnRep_SharedRunLevel()
{
	OnSharedRunLevelChangedEvent.Broadcast(SharedRunLevel);
	OnSharedRunLevelChanged(SharedRunLevel);
}

