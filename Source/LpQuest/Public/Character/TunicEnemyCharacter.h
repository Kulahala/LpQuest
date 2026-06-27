// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/TunicCharacterBase.h"
#include "Combat/TunicCombatHitWindowSourceInterface.h"
#include "Combat/TunicCombatTargetInterface.h"
#include "TunicEnemyCharacter.generated.h"

class FLifetimeProperty;
class AActor;
class UAnimMontage;
class UGameplayAbility;
class UGameplayEffect;
struct FOnAttributeChangeData;
struct FHitResult;

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

	UFUNCTION(BlueprintPure, Category = "Tunic|Rewards", meta = (ToolTip = "敌人死亡时提供的 XP 奖励值。只有 Spawner encounter 成员死亡才会通过 GameMode 加到共享 XP。"))
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出敌人 ASC / AttributeSet 初始化日志。只用于验证 GAS 所有权。"))
	bool bLogAbilitySystemInitialization = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出敌人死亡状态日志。只用于验证死亡复制和表现路径。"))
	bool bLogDeathState = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出敌人受击表现日志。只用于验证 hit reaction 路径。"))
	bool bLogHitReaction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否在屏幕/世界中绘制敌人属性调试信息。只用于开发验证。"))
	bool bDrawAttributeDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Prototype AI", meta = (ToolTip = "旧 prototype 自动攻击开关。正式 AI 应使用 AIController + StateTree，这个只保留作调试 fallback。"))
	bool bEnablePrototypeAutoAttack = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Prototype AI", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "prototype 自动攻击距离，单位 cm。正式 StateTree AI 不应依赖这个值。"))
	float PrototypeAutoAttackRange = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Prototype AI", meta = (ClampMin = "0.05", Units = "s", ToolTip = "prototype 自动攻击间隔，单位秒。只用于旧调试路径。"))
	float PrototypeAutoAttackInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ToolTip = "请求敌人 melee attack 时是否立即跑一次 hit window。用于没有完整动画通知时的 prototype 验证。"))
	bool bRunEnemyMeleeHitWindowOnRequest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "敌人 melee capsule sweep 半径，单位 cm。实际命中由服务器检测。"))
	float EnemyMeleeSweepRadius = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "敌人 melee capsule sweep 半高，单位 cm。影响上下命中范围。"))
	float EnemyMeleeSweepHalfHeight = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (Units = "cm", ToolTip = "敌人 melee sweep 起点的角色本地偏移，单位 cm。X 通常表示敌人前方。"))
	FVector EnemyMeleeSweepStartOffset = FVector(55.0f, 0.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (Units = "cm", ToolTip = "敌人 melee sweep 终点的角色本地偏移，单位 cm。决定攻击向前覆盖多远。"))
	FVector EnemyMeleeSweepEndOffset = FVector(190.0f, 0.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Debug", meta = (ToolTip = "命中目标时是否应用敌人 melee damage GameplayEffect。只用于当前 prototype 伤害路径。"))
	bool bApplyEnemyMeleeDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Debug", meta = (ToolTip = "是否输出敌人 melee attack 和 hit sweep 日志。只用于验证攻击路径。"))
	bool bLogEnemyMeleeAttack = true;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Rewards", meta = (ClampMin = "0", ToolTip = "该敌人死亡时提供的共享 XP 奖励。只有 Spawner encounter 成员会通过 GameMode 发放。"))
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
	void UpdatePrototypeAutoAttack(float DeltaSeconds);
	AActor* FindPrototypeAutoAttackTarget() const;
	bool PlayMeleeAttackMontage();
	void CheckMeleeAttackMontageHitWindowTriggered(int32 MontageSerial);
	FVector GetEnemyMeleeSweepPoint(const FVector& LocalOffset) const;
	void HandleEnemyMeleeTargetHit(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget);
	void ApplyEnemyMeleeDamage(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget);
	void LogEnemyMeleeHitSweepDebug(const TArray<FHitResult>& HitResults, int32 ProcessedHitCount) const;
	void PlayPresentationMontage(UAnimMontage* MontageToPlay, bool bStopAllMontages);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitReaction(AActor* InstigatorActor);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayMeleeAttackMontage(UAnimMontage* MontageToPlay);

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	bool bDeathPresentationApplied = false;
	bool bEnemyMeleeHitWindowActive = false;
	int32 MeleeAttackMontageActivationSerial = 0;
	int32 MeleeAttackMontageHitWindowSerial = 0;
	float PrototypeAutoAttackElapsedTime = 0.0f;
	TSet<TWeakObjectPtr<AActor>> EnemyMeleeHitTargets;
};
