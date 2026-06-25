// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/TunicEnemyStateTreeTasks.h"

#include "AI/TunicEnemyAIController.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestEnemyStateTree, Log, All);

bool FTunicStateTreeFindNearestEnemyTargetTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

EStateTreeRunStatus FTunicStateTreeFindNearestEnemyTargetTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.TargetActor = nullptr;

	ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	if (!EnemyAIController)
	{
		UE_LOG(LogLpQuestEnemyStateTree, Warning, TEXT("Enemy StateTree find target failed: missing AI controller"));
		return EStateTreeRunStatus::Failed;
	}

	AActor* TargetActor = EnemyAIController->FindNearestAvailableCombatTarget();
	if (!TargetActor)
	{
		EnemyAIController->ClearCurrentCombatTarget();
		return EStateTreeRunStatus::Failed;
	}

	EnemyAIController->SetCurrentCombatTarget(TargetActor);
	InstanceData.TargetActor = TargetActor;
	return EStateTreeRunStatus::Succeeded;
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

bool FTunicStateTreeEnemyTargetInAttackRangeCondition::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	return true;
}

bool FTunicStateTreeEnemyTargetInAttackRangeCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(&Context.GetExternalData(AIControllerHandle));
	return EnemyAIController && EnemyAIController->IsCurrentTargetInAttackRange();
}
