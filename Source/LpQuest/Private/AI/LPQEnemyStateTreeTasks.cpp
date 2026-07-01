// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/LPQEnemyStateTreeTasks.h"

#include "AI/LPQEnemyAIController.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestEnemyStateTree, Log, All);

bool FLPQStateTreeGetCurrentEnemyTargetTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FLPQStateTreeGetCurrentEnemyTargetTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.TargetActor = nullptr;

	ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree current target failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	EnemyAIController->RefreshCurrentCombatTargetFromAwareness();

	AActor* TargetActor = EnemyAIController->GetCurrentCombatTarget();
	if (!TargetActor)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.TargetActor = TargetActor;
	return EStateTreeRunStatus::Succeeded;
}

bool FLPQStateTreeGetCurrentPatrolLocationTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FLPQStateTreeGetCurrentPatrolLocationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.PatrolLocation = FVector::ZeroVector;

	const ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree patrol location failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	if (!EnemyAIController->HasPatrolRoute())
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.PatrolLocation = EnemyAIController->GetCurrentPatrolLocation();
	return EStateTreeRunStatus::Succeeded;
}

bool FLPQStateTreeGetEnemyHomeLocationTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FLPQStateTreeGetEnemyHomeLocationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.HomeLocation = FVector::ZeroVector;

	const ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree home location failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.HomeLocation = EnemyAIController->GetHomeLocation();
	return EStateTreeRunStatus::Succeeded;
}

bool FLPQStateTreeGetLastKnownTargetLocationTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FLPQStateTreeGetLastKnownTargetLocationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.LastKnownTargetLocation = FVector::ZeroVector;
	InstanceData.AcceptanceRadius = 0.0f;

	const ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree last known target location failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	if (!EnemyAIController->HasLastKnownTargetLocation())
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.LastKnownTargetLocation = EnemyAIController->GetLastKnownTargetLocation();
	InstanceData.AcceptanceRadius = EnemyAIController->GetInvestigationAcceptanceRadius();
	return EStateTreeRunStatus::Succeeded;
}

bool FLPQStateTreeGetInvestigationDurationTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FLPQStateTreeGetInvestigationDurationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.InvestigationDuration = 0.0f;

	const ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree investigation duration failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.InvestigationDuration = EnemyAIController->GetInvestigationDuration();
	return EStateTreeRunStatus::Succeeded;
}

bool FLPQStateTreeGetCurrentPatrolStopTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FLPQStateTreeGetCurrentPatrolStopTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.HoldDuration = 0.0f;

	const ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree patrol stop failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	if (!EnemyAIController->HasPatrolRoute() || !EnemyAIController->IsCurrentPatrolTargetStop())
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.HoldDuration = EnemyAIController->GetCurrentPatrolStopHoldDuration();
	return EStateTreeRunStatus::Succeeded;
}

bool FLPQStateTreeClearLastKnownTargetLocationTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FLPQStateTreeClearLastKnownTargetLocationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree clear last known target failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	EnemyAIController->ClearLastKnownTargetLocation();
	return EStateTreeRunStatus::Succeeded;
}

bool FLPQStateTreeAdvancePatrolTargetTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FLPQStateTreeAdvancePatrolTargetTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree advance patrol failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	return EnemyAIController->AdvancePatrolTarget() ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
}

bool FLPQStateTreeTryEnemyMeleeAttackTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FLPQStateTreeTryEnemyMeleeAttackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree attack failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	const bool bActivated = EnemyAIController->TryActivateCurrentTargetMeleeAttack();
	return bActivated ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
}

bool FLPQStateTreeEnemyHasCurrentTargetCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FLPQStateTreeEnemyHasCurrentTargetCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const bool bHasCurrentTarget = EnemyAIController && EnemyAIController->HasCurrentCombatTarget();
	return InstanceData.bInvert ? !bHasCurrentTarget : bHasCurrentTarget;
}

bool FLPQStateTreeEnemyTargetInAttackRangeCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FLPQStateTreeEnemyTargetInAttackRangeCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const bool bIsTargetInAttackRange = EnemyAIController && EnemyAIController->IsCurrentTargetInAttackRange();
	return InstanceData.bInvert ? !bIsTargetInAttackRange : bIsTargetInAttackRange;
}

bool FLPQStateTreeEnemyHasPatrolRouteCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FLPQStateTreeEnemyHasPatrolRouteCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const bool bHasPatrolRoute = EnemyAIController && EnemyAIController->HasPatrolRoute();
	return InstanceData.bInvert ? !bHasPatrolRoute : bHasPatrolRoute;
}

bool FLPQStateTreeEnemyHasLastKnownTargetLocationCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FLPQStateTreeEnemyHasLastKnownTargetLocationCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const bool bHasLastKnownTargetLocation = EnemyAIController && EnemyAIController->HasLastKnownTargetLocation();
	return InstanceData.bInvert ? !bHasLastKnownTargetLocation : bHasLastKnownTargetLocation;
}

bool FLPQStateTreeEnemyCurrentPatrolTargetIsStopCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FLPQStateTreeEnemyCurrentPatrolTargetIsStopCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const bool bIsStop = EnemyAIController && EnemyAIController->IsCurrentPatrolTargetStop();
	return InstanceData.bInvert ? !bIsStop : bIsStop;
}
