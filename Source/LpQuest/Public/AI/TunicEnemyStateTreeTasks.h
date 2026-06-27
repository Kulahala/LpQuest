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
struct LPQUEST_API FTunicStateTreeGetCurrentEnemyTargetTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	TObjectPtr<AActor> TargetActor = nullptr;
};

USTRUCT(meta = (DisplayName = "Get Current Tunic Enemy Target", Category = "Tunic|AI"))
struct LPQUEST_API FTunicStateTreeGetCurrentEnemyTargetTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeGetCurrentEnemyTargetTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

USTRUCT(BlueprintType)
struct LPQUEST_API FTunicStateTreeGetCurrentPatrolLocationTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	FVector PatrolLocation = FVector::ZeroVector;
};

USTRUCT(meta = (DisplayName = "Get Current Tunic Patrol Location", Category = "Tunic|AI"))
struct LPQUEST_API FTunicStateTreeGetCurrentPatrolLocationTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeGetCurrentPatrolLocationTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

USTRUCT(BlueprintType)
struct LPQUEST_API FTunicStateTreeGetEnemyHomeLocationTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	FVector HomeLocation = FVector::ZeroVector;
};

USTRUCT(meta = (DisplayName = "Get Tunic Enemy Home Location", Category = "Tunic|AI"))
struct LPQUEST_API FTunicStateTreeGetEnemyHomeLocationTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeGetEnemyHomeLocationTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

USTRUCT(BlueprintType)
struct LPQUEST_API FTunicStateTreeGetCurrentPatrolStopTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	float HoldDuration = 0.0f;
};

USTRUCT(meta = (DisplayName = "Get Current Tunic Patrol Stop", Category = "Tunic|AI"))
struct LPQUEST_API FTunicStateTreeGetCurrentPatrolStopTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeGetCurrentPatrolStopTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

USTRUCT()
struct LPQUEST_API FTunicStateTreeAdvancePatrolTargetTaskInstanceData
{
	GENERATED_BODY()
};

USTRUCT(meta = (DisplayName = "Advance Tunic Patrol Target", Category = "Tunic|AI"))
struct LPQUEST_API FTunicStateTreeAdvancePatrolTargetTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeAdvancePatrolTargetTaskInstanceData;

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

USTRUCT()
struct LPQUEST_API FTunicStateTreeEnemyConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bInvert = false;
};

USTRUCT(meta = (DisplayName = "Tunic Enemy Has Current Target", Category = "Tunic|AI"))
struct LPQUEST_API FTunicStateTreeEnemyHasCurrentTargetCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeEnemyConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

USTRUCT(meta = (DisplayName = "Tunic Enemy Target In Attack Range", Category = "Tunic|AI"))
struct LPQUEST_API FTunicStateTreeEnemyTargetInAttackRangeCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeEnemyConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

USTRUCT(meta = (DisplayName = "Tunic Enemy Has Patrol Route", Category = "Tunic|AI"))
struct LPQUEST_API FTunicStateTreeEnemyHasPatrolRouteCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeEnemyConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

USTRUCT(meta = (DisplayName = "Tunic Enemy Current Patrol Target Is Stop", Category = "Tunic|AI"))
struct LPQUEST_API FTunicStateTreeEnemyCurrentPatrolTargetIsStopCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeEnemyConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};
