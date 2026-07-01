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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output", meta = (ToolTip = "输出 AIController 当前保存的战斗目标。Chase/Attack 应消费这个值，不要自行扫描玩家。"))
	TObjectPtr<AActor> TargetActor = nullptr;
};

USTRUCT(meta = (DisplayName = "Get Current LPQ Enemy Target", Category = "LPQ|AI", ToolTip = "读取 AIController 当前目标并输出 TargetActor。只读节点，不会选择新目标或修改 combat 状态。"))
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output", meta = (ToolTip = "输出当前巡逻 movement sample 的世界位置。MoveTo 的 Destination 应绑定到这个值。"))
	FVector PatrolLocation = FVector::ZeroVector;
};

USTRUCT(meta = (DisplayName = "Get Current LPQ Patrol Location", Category = "LPQ|AI", ToolTip = "读取 AIController 当前巡逻采样点位置。只读节点，不会推进路线索引。"))
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output", meta = (ToolTip = "输出敌人 HomeLocation。无巡逻路线时 IdleAtHome 可以 MoveTo 这个位置。"))
	FVector HomeLocation = FVector::ZeroVector;
};

USTRUCT(meta = (DisplayName = "Get LPQ Enemy Home Location", Category = "LPQ|AI", ToolTip = "读取敌人被 AIController Possess 时记录的 HomeLocation。只读节点，不修改巡逻或目标状态。"))
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
struct LPQUEST_API FTunicStateTreeGetLastKnownTargetLocationTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output", meta = (ToolTip = "输出 AIController 记录的最后已知目标位置。InvestigateMove 的 MoveTo Destination 应绑定到这个值。"))
	FVector LastKnownTargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output", meta = (ToolTip = "输出调查 MoveTo 接受半径，单位 cm。InvestigateMove 的 Acceptable Radius 可绑定到这个值。"))
	float AcceptanceRadius = 0.0f;
};

USTRUCT(meta = (DisplayName = "Get LPQ Enemy Last Known Target Location", Category = "LPQ|AI", ToolTip = "读取最后已知目标位置和调查 MoveTo 接受半径。只读节点，不会选择目标、移动敌人或修改 combat 状态。"))
struct LPQUEST_API FTunicStateTreeGetLastKnownTargetLocationTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeGetLastKnownTargetLocationTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

USTRUCT(BlueprintType)
struct LPQUEST_API FTunicStateTreeGetInvestigationDurationTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output", meta = (ToolTip = "输出 investigation 搜索等待时长，单位秒。InvestigateSearch 的 Delay.Duration 可绑定到这个值。"))
	float InvestigationDuration = 0.0f;
};

USTRUCT(meta = (DisplayName = "Get LPQ Enemy Investigation Duration", Category = "LPQ|AI", ToolTip = "读取 AIController 的调查搜索等待时长。只读节点，不会推进 StateTree 或清理状态。"))
struct LPQUEST_API FTunicStateTreeGetInvestigationDurationTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeGetInvestigationDurationTaskInstanceData;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output", meta = (ToolTip = "输出当前 stop sample 的等待时间，单位秒。可绑定到 StateTree 内置 Delay.Duration。"))
	float HoldDuration = 0.0f;
};

USTRUCT(meta = (DisplayName = "Get Current LPQ Patrol Stop", Category = "LPQ|AI", ToolTip = "读取当前巡逻点的 stop 数据。当前点不是 stop 时失败；不会推进路线索引。"))
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
struct LPQUEST_API FTunicStateTreeClearLastKnownTargetLocationTaskInstanceData
{
	GENERATED_BODY()
};

USTRUCT(meta = (DisplayName = "Clear LPQ Enemy Last Known Target Location", Category = "LPQ|AI", ToolTip = "清理 AIController 的最后已知目标位置。只清理 investigation 意图状态，不修改 Health、RunState、XP 或 GameplayTags。"))
struct LPQUEST_API FTunicStateTreeClearLastKnownTargetLocationTask : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeClearLastKnownTargetLocationTaskInstanceData;

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

USTRUCT(meta = (DisplayName = "Advance LPQ Patrol Target", Category = "LPQ|AI", ToolTip = "推进 AIController 当前巡逻 route index 到下一个采样点。只改变 AI 巡逻意图，不修改 combat、Health、RunState 或 XP。"))
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

USTRUCT(meta = (DisplayName = "Try LPQ Enemy Melee Attack", Category = "LPQ|AI", ToolTip = "请求敌人对当前目标激活 melee Ability。节点本身不直接造成伤害，命中仍走 hit window 和 GameplayEffect。"))
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

	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ToolTip = "反转条件结果。UE StateTree UI 不一定在条件名上显示 NOT，判断时以这里是否勾选为准。"))
	bool bInvert = false;
};

USTRUCT(meta = (DisplayName = "LPQ Enemy Has Current Target", Category = "LPQ|AI", ToolTip = "检查 AIController 是否有有效 current target。可用 bInvert 表示没有目标。"))
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

USTRUCT(meta = (DisplayName = "LPQ Enemy Target In Attack Range", Category = "LPQ|AI", ToolTip = "检查 current target 是否在 AIController 的 AttackActivationRange 内。用于 Chase/Attack 切换，避免手写重复距离。"))
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

USTRUCT(meta = (DisplayName = "LPQ Enemy Has Patrol Route", Category = "LPQ|AI", ToolTip = "检查 AIController 是否绑定到有效巡逻路线。可用 bInvert 表示无路线并进入 IdleAtHome。"))
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

USTRUCT(meta = (DisplayName = "LPQ Enemy Has Last Known Target Location", Category = "LPQ|AI", ToolTip = "检查 AIController 是否有最后已知目标位置。可用 bInvert 表示没有 investigation 目标。"))
struct LPQUEST_API FTunicStateTreeEnemyHasLastKnownTargetLocationCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTunicStateTreeEnemyConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
};

USTRUCT(meta = (DisplayName = "LPQ Enemy Current Patrol Target Is Stop", Category = "LPQ|AI", ToolTip = "检查当前巡逻采样点是否是 stop sample。PatrolMove 成功后用它决定进入 PatrolStop 还是 PatrolAdvance。"))
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
