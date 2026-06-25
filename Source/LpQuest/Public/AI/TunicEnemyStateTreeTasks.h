// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "StateTreeExecutionTypes.h"
#include "Tasks/StateTreeAITask.h"
#include "TunicEnemyStateTreeTasks.generated.h"

class AAIController;
class AActor;
class ATunicEnemyAIController;
struct FStateTreeLinker;

USTRUCT(BlueprintType)
struct LPQUEST_API FTunicStateTreeFindNearestEnemyTargetTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	TObjectPtr<AActor> TargetActor = nullptr;
};

USTRUCT(meta = (DisplayName = "Find Nearest Tunic Enemy Target", Category = "Tunic|AI"))
struct LPQUEST_API FTunicStateTreeFindNearestEnemyTargetTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeFindNearestEnemyTargetTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

USTRUCT()
struct LPQUEST_API FTunicStateTreeTryEnemyMeleeAttackTaskInstanceData
{
	GENERATED_BODY()
};

USTRUCT(meta = (DisplayName = "Try Tunic Enemy Melee Attack", Category = "Tunic|AI"))
struct LPQUEST_API FTunicStateTreeTryEnemyMeleeAttackTask : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeTryEnemyMeleeAttackTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

USTRUCT(meta = (DisplayName = "Tunic Enemy Target In Attack Range", Category = "Tunic|AI"))
struct LPQUEST_API FTunicStateTreeEnemyTargetInAttackRangeCondition : public FStateTreeConditionBase
{
	GENERATED_BODY()

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};
