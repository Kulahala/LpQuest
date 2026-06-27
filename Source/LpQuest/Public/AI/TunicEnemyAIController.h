// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "TimerManager.h"
#include "TunicEnemyAIController.generated.h"

class ATunicEnemyCharacter;
class ATunicEnemyPatrolRoute;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UStateTree;
class UStateTreeAIComponent;

UENUM(BlueprintType)
enum class ETunicEnemyAwarenessPolicy : uint8
{
	SightOnlyPatrol UMETA(DisplayName = "Sight Only Patrol", ToolTip = "只依赖 Sight 感知。适合守卫和巡逻敌人，背对玩家时不会全向发现目标。"),

	SightAndProximity UMETA(DisplayName = "Sight And Proximity", ToolTip = "Sight 加近距离全向察觉。适合普通近战小怪，玩家绕背但距离很近时也能进入 aggro。"),

	CombatSpawnAggro UMETA(DisplayName = "Combat Spawn Aggro", ToolTip = "生成后短延迟在限定半径内找目标。适合 Spawner 战斗波次，不是全图锁人。")
};

UCLASS(Blueprintable)
class LPQUEST_API ATunicEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ATunicEnemyAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "返回当前敌人使用的 StateTree AI Component。一般只用于调试，不要在客户端驱动 AI。"))
	UStateTreeAIComponent* GetEnemyStateTreeComponent() const;

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI", meta = (ToolTip = "按当前 AwarenessPolicy 刷新 CurrentCombatTarget。服务器 AI 使用，StateTree 通常只消费结果。"))
	void RefreshCurrentCombatTargetFromAwareness();

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI", meta = (ToolTip = "设置当前战斗目标。主要用于 AI/调试；目标仍必须通过可用性检查后才会被攻击。"))
	void SetCurrentCombatTarget(AActor* NewTarget);

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "返回当前战斗目标。StateTree 的 Chase/Attack 应读取这个目标，而不是自行扫描全场玩家。"))
	AActor* GetCurrentCombatTarget() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "当前是否保存了战斗目标。不代表目标一定仍存活或可攻击。"))
	bool HasCurrentCombatTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI", meta = (ToolTip = "清理当前战斗目标。丢失目标、目标死亡或 RunState 离开 CombatActive 时使用。"))
	void ClearCurrentCombatTarget();

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "检查当前目标是否仍是可用 combat target。会过滤死亡或不可用玩家。"))
	bool IsCurrentTargetValid() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "当前目标是否在 AttackActivationRange 内。StateTree 的 Attack transition 应优先用这个判断，避免重复手写距离。"))
	bool IsCurrentTargetInAttackRange() const;

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI", meta = (ToolTip = "请求当前目标近战攻击。这里只触发敌人 melee Ability，不直接改 Health、RunState 或 XP。"))
	bool TryActivateCurrentTargetMeleeAttack();

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI", meta = (ToolTip = "停止敌人 AI 逻辑、移动和目标。通常在死亡或 RunState 不允许 AI 时调用。"))
	void StopEnemyAILogic();

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "返回 AI 进入攻击状态的距离，单位 cm。由 AIController 统一持有，StateTree 不应再手写一份攻击距离。"))
	float GetAttackActivationRange() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "当前 AI 是否成功绑定巡逻路线。无路线时 StateTree 应进入 IdleAtHome fallback。"))
	bool HasPatrolRoute() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "返回当前巡逻 movement sample 位置。StateTree 的 PatrolMove 应把 MoveTo Destination 绑定到这个值。"))
	FVector GetCurrentPatrolLocation() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "当前巡逻采样点是否是 stop sample。StateTree 用它决定是否进入 PatrolStop。"))
	bool IsCurrentPatrolTargetStop() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "返回当前 stop sample 的等待时间，单位秒。StateTree 的 Delay.Duration 可绑定这个值。"))
	float GetCurrentPatrolStopHoldDuration() const;

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI", meta = (ToolTip = "推进到下一个巡逻采样点。StateTree 的 PatrolAdvance 使用；AIController 持有 route index。"))
	bool AdvancePatrolTarget();

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "返回敌人被 Possess 时记录的 HomeLocation。无巡逻路线时 IdleAtHome 使用。"))
	FVector GetHomeLocation() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "当前是否有最后已知目标位置。StateTree 的 Investigate 分支使用；只有服务器 AI 写入。"))
	bool HasLastKnownTargetLocation() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "返回最后已知目标位置。Investigate MoveTo 的 Destination 应绑定到这个值。"))
	FVector GetLastKnownTargetLocation() const;

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI", meta = (ToolTip = "服务器清理最后已知目标位置。StateTree 的 InvestigateClear 使用；客户端调用不会改 AI 状态，也不修改 Health、RunState 或 XP。"))
	void ClearLastKnownTargetLocation();

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "返回调查搜索等待时长，单位秒。StateTree 的 InvestigateSearch Delay.Duration 可绑定这个值。"))
	float GetInvestigationDuration() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI", meta = (ToolTip = "返回调查 MoveTo 接受半径，单位 cm。InvestigateMove 的 acceptable radius 可使用该值。"))
	float GetInvestigationAcceptanceRadius() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|AI", meta = (ToolTip = "敌人默认 StateTree 资产。服务器 Possess 敌人后启动；未配置时敌人安全 idle。"))
	TObjectPtr<UStateTree> DefaultEnemyStateTree;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "AI 请求近战攻击的距离，单位 cm。StateTree 应通过 native condition 读取它。"))
	float AttackActivationRange = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Perception", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Sight 感知半径，单位 cm。只影响 AI 看到玩家的距离，不是攻击距离。"))
	float SightRadius = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Perception", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Sight 遗忘半径，单位 cm。通常应大于 SightRadius，避免刚追击就丢目标。"))
	float LoseSightRadius = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Perception", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg", ToolTip = "视觉半角，单位度。数值越大越容易看到侧面或背后的玩家。"))
	float PeripheralVisionAngleDegrees = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Aggro", meta = (ClampMin = "0.0", Units = "s", ToolTip = "目标丢失后的延迟遗忘时间，单位秒。用于避免 Sight 瞬断导致追击/巡逻抖动。"))
	float AggroForgetDelay = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Aggro", meta = (ToolTip = "敌人获取目标的策略。SightOnlyPatrol 只看视野，SightAndProximity 增加近距离察觉，CombatSpawnAggro 适合生成波次。"))
	ETunicEnemyAwarenessPolicy AwarenessPolicy = ETunicEnemyAwarenessPolicy::SightOnlyPatrol;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Aggro", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "近距离察觉半径，单位 cm。只在 SightAndProximity 或 CombatSpawnAggro 相关逻辑中使用。"))
	float ProximityAggroRadius = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Aggro", meta = (ClampMin = "0.0", Units = "s", ToolTip = "CombatSpawnAggro 生成后等待多久才主动找附近玩家，单位秒。防止出生瞬间立即全图开战。"))
	float CombatSpawnAggroDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Investigation", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "调查最后已知位置时 MoveTo 的接受半径，单位 cm。用于避免敌人必须精确踩到目标丢失点。"))
	float InvestigationAcceptanceRadius = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Investigation", meta = (ClampMin = "0.0", Units = "s", ToolTip = "敌人到达最后已知位置后搜索/等待的时长，单位秒。StateTree Delay 可绑定这个值。"))
	float InvestigationDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Patrol", meta = (ToolTip = "要绑定的巡逻路线 Id。必须匹配关卡中 ATunicEnemyPatrolRoute 的 RouteId。"))
	FName PatrolRouteId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Idle", meta = (ClampMin = "0.0", Units = "deg/s", ToolTip = "无目标且无巡逻路线时的待机扫视转速，单位 deg/s。只帮助 Sight-only 敌人自然发现玩家。"))
	float IdleScanYawRate = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Idle", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Idle Scan 允许离 HomeLocation 的容忍距离，单位 cm。离家太远时不做原地扫视。"))
	float IdleScanHomeTolerance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Idle", meta = (ToolTip = "无目标且无巡逻路线时是否启用轻量扫视。关闭后敌人会更静态地待机。"))
	bool bEnableIdleScan = true;

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	bool CanRunEnemyAI() const;
	bool IsRunCombatActive() const;
	ATunicEnemyCharacter* GetControlledEnemy() const;
	bool IsValidCombatTarget(AActor* TargetActor) const;
	bool IsCombatTargetCurrentlyPerceived(AActor* TargetActor) const;
	bool IsCombatTargetWithinProximity(AActor* TargetActor, float SearchRadius) const;
	AActor* FindBestPerceivedCombatTarget() const;
	AActor* FindBestProximityCombatTarget(float SearchRadius) const;
	void RecordLastKnownTargetLocation(AActor* TargetActor);
	void HandleCombatSpawnAggroDelayElapsed();
	void UpdateIdleScan(float DeltaSeconds);
	void EnsureControlledEnemyCanMove() const;
	void ConfigureSightPerception();
	void RebuildPatrolRoute();
	void StartEnemyStateTree();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> EnemyPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentCombatTarget;

	UPROPERTY(Transient)
	TWeakObjectPtr<ATunicEnemyPatrolRoute> PatrolRouteActor;

	int32 CurrentPatrolPointIndex = 0;
	FVector HomeLocation = FVector::ZeroVector;
	FVector LastKnownTargetLocation = FVector::ZeroVector;
	double CurrentTargetLostTimeSeconds = 0.0;
	bool bCurrentTargetPendingForget = false;
	bool bHasLastKnownTargetLocation = false;
	bool bCombatSpawnAggroReady = false;
	FTimerHandle CombatSpawnAggroTimerHandle;
};
