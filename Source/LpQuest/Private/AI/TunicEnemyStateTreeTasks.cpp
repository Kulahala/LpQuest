// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/TunicEnemyStateTreeTasks.h"

#include "AI/TunicEnemyAIController.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestEnemyStateTree, Log, All);

bool FTunicStateTreeGetCurrentEnemyTargetTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FTunicStateTreeGetCurrentEnemyTargetTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.TargetActor = nullptr;

	ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
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

bool FTunicStateTreeGetCurrentPatrolLocationTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FTunicStateTreeGetCurrentPatrolLocationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.PatrolLocation = FVector::ZeroVector;

	const ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
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

bool FTunicStateTreeGetEnemyHomeLocationTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FTunicStateTreeGetEnemyHomeLocationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.HomeLocation = FVector::ZeroVector;

	const ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree home location failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.HomeLocation = EnemyAIController->GetHomeLocation();
	return EStateTreeRunStatus::Succeeded;
}

bool FTunicStateTreeGetLastKnownTargetLocationTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FTunicStateTreeGetLastKnownTargetLocationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.LastKnownTargetLocation = FVector::ZeroVector;
	InstanceData.AcceptanceRadius = 0.0f;

	const ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
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

bool FTunicStateTreeGetInvestigationDurationTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FTunicStateTreeGetInvestigationDurationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.InvestigationDuration = 0.0f;

	const ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree investigation duration failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.InvestigationDuration = EnemyAIController->GetInvestigationDuration();
	return EStateTreeRunStatus::Succeeded;
}

bool FTunicStateTreeGetCurrentPatrolStopTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FTunicStateTreeGetCurrentPatrolStopTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.HoldDuration = 0.0f;

	const ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
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

bool FTunicStateTreeClearLastKnownTargetLocationTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FTunicStateTreeClearLastKnownTargetLocationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree clear last known target failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	EnemyAIController->ClearLastKnownTargetLocation();
	return EStateTreeRunStatus::Succeeded;
}

bool FTunicStateTreeAdvancePatrolTargetTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FTunicStateTreeAdvancePatrolTargetTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree advance patrol failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	return EnemyAIController->AdvancePatrolTarget() ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
}

bool FTunicStateTreeTryEnemyMeleeAttackTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FTunicStateTreeTryEnemyMeleeAttackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree attack failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	const bool bActivated = EnemyAIController->TryActivateCurrentTargetMeleeAttack();
	return bActivated ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
}

bool FTunicStateTreeEnemyHasCurrentTargetCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FTunicStateTreeEnemyHasCurrentTargetCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const bool bHasCurrentTarget = EnemyAIController && EnemyAIController->HasCurrentCombatTarget();
	return InstanceData.bInvert ? !bHasCurrentTarget : bHasCurrentTarget;
}

bool FTunicStateTreeEnemyTargetInAttackRangeCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FTunicStateTreeEnemyTargetInAttackRangeCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const bool bIsTargetInAttackRange = EnemyAIController && EnemyAIController->IsCurrentTargetInAttackRange();
	return InstanceData.bInvert ? !bIsTargetInAttackRange : bIsTargetInAttackRange;
}

bool FTunicStateTreeEnemyHasPatrolRouteCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FTunicStateTreeEnemyHasPatrolRouteCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const bool bHasPatrolRoute = EnemyAIController && EnemyAIController->HasPatrolRoute();
	return InstanceData.bInvert ? !bHasPatrolRoute : bHasPatrolRoute;
}

bool FTunicStateTreeEnemyHasLastKnownTargetLocationCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FTunicStateTreeEnemyHasLastKnownTargetLocationCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const bool bHasLastKnownTargetLocation = EnemyAIController && EnemyAIController->HasLastKnownTargetLocation();
	return InstanceData.bInvert ? !bHasLastKnownTargetLocation : bHasLastKnownTargetLocation;
}

bool FTunicStateTreeEnemyCurrentPatrolTargetIsStopCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FTunicStateTreeEnemyCurrentPatrolTargetIsStopCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const bool bIsStop = EnemyAIController && EnemyAIController->IsCurrentPatrolTargetStop();
	return InstanceData.bInvert ? !bIsStop : bIsStop;
}
