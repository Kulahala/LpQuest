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
