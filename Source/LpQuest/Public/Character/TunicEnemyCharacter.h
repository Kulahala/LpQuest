// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/TunicCharacterBase.h"
#include "Combat/TunicCombatHitWindowSourceInterface.h"
#include "Combat/TunicCombatTargetInterface.h"
#include "TimerManager.h"
#include "TunicEnemyCharacter.generated.h"

class FLifetimeProperty;
class AActor;
class UAnimMontage;
class UGameplayAbility;
class UGameplayEffect;
struct FOnAttributeChangeData;
struct FOverlapResult;

UCLASS(Blueprintable)
class LPQUEST_API ATunicEnemyCharacter : public ATunicCharacterBase, public ITunicCombatTargetInterface, public ITunicCombatHitWindowSourceInterface
{
	GENERATED_BODY()

public:
	ATunicEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Combat", meta = (ToolTip = "敌人当前是否死亡。死亡后停止 AI、移动和攻击。"))
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Rewards", meta = (ToolTip = "敌人死亡时提供的基础 shared XP 奖励值。是否实际发放由 GameMode 的 reward-source 路由决定。"))
	int32 GetExperienceReward() const;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat", meta = (ToolTip = "请求激活敌人 melee Ability。AI/StateTree 使用；函数本身不直接造成伤害。"))
	bool TryActivateEnemyMeleeAttack();

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat", meta = (ToolTip = "手动开始敌人 melee hit window。通常由 AnimNotifyState 调用。"))
	void BeginEnemyMeleeHitWindow();

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat", meta = (ToolTip = "处理敌人 melee hit window 的一次命中检测。命中和伤害由服务器确认。"))
	void ProcessEnemyMeleeHitWindow();

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat", meta = (ToolTip = "结束敌人 melee hit window，并清理本次窗口已命中过的目标集合。"))
	void EndEnemyMeleeHitWindow();

	void ExecuteEnemyMeleeAttackAbility();
	void HandleHitReaction(AActor* InstigatorActor);
	virtual bool IsCombatTargetAvailable() const override;
	virtual ETunicCombatTeam GetCombatTargetTeam() const override;
	virtual UTunicAbilitySystemComponent* GetCombatTargetAbilitySystemComponent() const override;
	virtual UTunicAttributeSet* GetCombatTargetAttributeSet() const override;
	virtual void HandleCombatTargetHitReaction(AActor* InstigatorActor) override;
	virtual void BeginCombatHitWindow(FName HitWindowName) override;
	virtual void ProcessCombatHitWindow(FName HitWindowName) override;
	virtual void EndCombatHitWindow(FName HitWindowName) override;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug", meta = (ToolTip = "运行时开关敌人 ASC 初始化日志。只影响日志，不影响 gameplay。"))
	void SetAbilitySystemInitializationLoggingEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug", meta = (ToolTip = "运行时开关敌人属性调试绘制。只影响显示，不影响属性值。"))
	void SetAttributeDebugDrawEnabled(bool bEnabled);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Combat", meta = (ToolTip = "敌人死亡状态变化表现 hook。死亡状态由服务器 Health 路径决定。"))
	void OnDeathStateChanged(bool bNewIsDead);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Combat", meta = (ToolTip = "敌人受击表现 hook。不要在这里直接改 Health。"))
	void OnHitReaction(AActor* InstigatorActor);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Combat|Telegraph", meta = (ToolTip = "敌人 melee 攻击预警开始时触发的表现 hook。参数描述当前攻击形状范围；只做闪光、音效、地面提示等表现，不要直接造成伤害。"))
	void OnEnemyMeleeTelegraphStarted(float TelegraphDuration, FVector ShapeOrigin, FVector ShapeForward, float ShapeRange, float ShapeHalfHeight);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出敌人 ASC / AttributeSet 初始化日志。只用于验证 GAS 所有权。"))
	bool bLogAbilitySystemInitialization = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出敌人死亡状态日志。只用于验证死亡复制和表现路径。"))
	bool bLogDeathState = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出敌人受击表现日志。只用于验证 hit reaction 路径。"))
	bool bLogHitReaction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否在屏幕/世界中绘制敌人属性调试信息。只用于开发验证。"))
	bool bDrawAttributeDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ToolTip = "请求敌人 melee attack 时是否立即跑一次 hit window。用于没有完整动画通知时的 prototype 验证。"))
	bool bRunEnemyMeleeHitWindowOnRequest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Telegraph", meta = (ClampMin = "0.0", Units = "s", ToolTip = "敌人 melee attack 命中前的预警时间，单位秒。预警结束后才播放攻击 Montage 或 fallback hit window。"))
	float EnemyMeleeTelegraphDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Telegraph", meta = (ToolTip = "是否绘制敌人 melee telegraph 范围。只用于开发验证，不参与伤害判定。"))
	bool bDrawEnemyMeleeTelegraphDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Telegraph", meta = (ClampMin = "0.0", Units = "s", ToolTip = "敌人 melee telegraph debug 绘制持续时间，单位秒。0 表示只绘制一帧。"))
	float EnemyMeleeTelegraphDebugDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "敌人 melee 扇形攻击向前覆盖距离，单位 cm。telegraph 和服务器 hit window 共用这个范围。"))
	float EnemyMeleeAttackRange = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ClampMin = "1.0", ClampMax = "180.0", Units = "deg", ToolTip = "敌人 melee 扇形攻击总角度，单位度。90 表示面前左右各 45 度。"))
	float EnemyMeleeAttackAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "敌人 melee 扇形攻击上下高度半径，单位 cm。目标与攻击原点的 Z 差超过该值时不会命中。"))
	float EnemyMeleeAttackHalfHeight = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (Units = "cm", ToolTip = "敌人 melee 攻击形状原点沿角色前方的偏移，单位 cm。0 表示以脚下/身体中心为原点。"))
	float EnemyMeleeAttackOriginForwardOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ToolTip = "是否绘制 hit window 实际使用的敌人 melee 扇形攻击范围。只用于验证，不参与伤害判定。"))
	bool bDrawEnemyMeleeAttackShapeDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Tunic|Combat|Hit Confirmation|Legacy", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Legacy serialized field：旧 capsule sweep 半径。当前 melee 命中逻辑不读取这个值，请使用 EnemyMeleeAttackRange / Angle / HalfHeight。"))
	float EnemyMeleeSweepRadius = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Tunic|Combat|Hit Confirmation|Legacy", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Legacy serialized field：旧 capsule sweep 半高。当前 melee 命中逻辑不读取这个值，请使用 EnemyMeleeAttackRange / Angle / HalfHeight。"))
	float EnemyMeleeSweepHalfHeight = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Tunic|Combat|Hit Confirmation|Legacy", meta = (Units = "cm", ToolTip = "Legacy serialized field：旧 capsule sweep 起点偏移。当前 melee 命中逻辑不读取这个值，请使用配置化扇形形状。"))
	FVector EnemyMeleeSweepStartOffset = FVector(55.0f, 0.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Tunic|Combat|Hit Confirmation|Legacy", meta = (Units = "cm", ToolTip = "Legacy serialized field：旧 capsule sweep 终点偏移。当前 melee 命中逻辑不读取这个值，请使用配置化扇形形状。"))
	FVector EnemyMeleeSweepEndOffset = FVector(190.0f, 0.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Debug", meta = (ToolTip = "命中目标时是否应用敌人 melee damage GameplayEffect。只用于当前 prototype 伤害路径。"))
	bool bApplyEnemyMeleeDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Debug", meta = (ToolTip = "是否输出敌人 melee attack 高层日志，例如攻击请求、shape summary、伤害应用或跳过原因。只用于验证攻击路径。"))
	bool bLogEnemyMeleeAttack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Tunic|Combat|Debug", meta = (ToolTip = "是否输出每个 enemy melee overlap candidate 的详细几何日志，包括距离、角度、高度、ASC 和属性信息。日志很吵，只在排查命中范围问题时开启。"))
	bool bLogEnemyMeleeAttackShapeDetails = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Damage", meta = (ToolTip = "敌人 melee 命中时应用的 damage GameplayEffect class。伤害由服务器 hit window 确认后应用。"))
	TSubclassOf<UGameplayEffect> MeleeAttackDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Animation", meta = (ToolTip = "敌人 melee attack 播放的 Montage。表现播放不直接决定最终伤害。"))
	TObjectPtr<UAnimMontage> MeleeAttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Presentation", meta = (ToolTip = "默认受击表现 Montage。只负责表现，不直接改 Health。"))
	TObjectPtr<UAnimMontage> DefaultHitReactionMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Presentation", meta = (ToolTip = "默认死亡表现 Montage。死亡状态由服务器 Health 路径决定。"))
	TObjectPtr<UAnimMontage> DefaultDeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Abilities", meta = (ToolTip = "敌人默认授予的 melee GameplayAbility class。StateTree 只请求激活，不直接结算伤害。"))
	TSubclassOf<UGameplayAbility> DefaultMeleeAttackAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Rewards", meta = (ClampMin = "0", ToolTip = "该敌人死亡时提供的基础 shared XP 奖励。是否实际发放由 GameMode 的 reward-source 路由决定。0 表示不给 XP。"))
	int32 ExperienceReward = 5;

private:
	UFUNCTION()
	void OnRep_IsDead();

	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void GrantDefaultAbilities();
	void SetDead(bool bNewIsDead);
	void ApplyDeathState();
	void EnsureLiveEnemyMovementMode();
	void LogEnemyAbilitySystemDebug() const;
	void DrawAttributeDebug() const;
	void StartEnemyMeleeTelegraph();
	void FinishEnemyMeleeTelegraphAndAttack();
	void ClearEnemyMeleeTelegraph();
	FVector GetEnemyMeleeAttackShapeOrigin() const;
	FVector GetEnemyMeleeAttackShapeForward() const;
	bool IsActorInsideEnemyMeleeAttackShape(const AActor* TargetActor) const;
	void DrawEnemyMeleeFanDebug(FVector ShapeOrigin, FVector ShapeForward, float ShapeRange, float ShapeHalfHeight, float ShapeAngleDegrees, float LifeTime, FColor DebugColor, const TCHAR* DebugLabel) const;
	void DrawEnemyMeleeAttackShapeDebug(float LifeTime, FColor DebugColor, const TCHAR* DebugLabel) const;
	void DrawEnemyMeleeTelegraphDebug(FVector ShapeOrigin, FVector ShapeForward, float ShapeRange, float ShapeHalfHeight) const;
	bool PlayMeleeAttackMontage();
	void CheckMeleeAttackMontageHitWindowTriggered(int32 MontageSerial);
	void HandleEnemyMeleeTargetHit(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget);
	void ApplyEnemyMeleeDamage(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget);
	void LogEnemyMeleeAttackShapeDebug(const TArray<FOverlapResult>& OverlapResults, int32 ProcessedHitCount) const;
	void PlayPresentationMontage(UAnimMontage* MontageToPlay, bool bStopAllMontages);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitReaction(AActor* InstigatorActor);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayMeleeAttackMontage(UAnimMontage* MontageToPlay);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastStartEnemyMeleeTelegraph(float TelegraphDuration, FVector ShapeOrigin, FVector ShapeForward, float ShapeRange, float ShapeHalfHeight);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastDrawEnemyMeleeAttackShape(float LifeTime, FVector ShapeOrigin, FVector ShapeForward, float ShapeRange, float ShapeHalfHeight, float ShapeAngleDegrees);

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	bool bDeathPresentationApplied = false;
	bool bEnemyMeleeTelegraphActive = false;
	bool bEnemyMeleeHitWindowActive = false;
	int32 MeleeAttackMontageActivationSerial = 0;
	int32 MeleeAttackMontageHitWindowSerial = 0;
	FTimerHandle EnemyMeleeTelegraphTimerHandle;
	TSet<TWeakObjectPtr<AActor>> EnemyMeleeHitTargets;
};
