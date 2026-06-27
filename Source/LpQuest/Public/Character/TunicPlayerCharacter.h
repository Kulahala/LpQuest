// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Character/TunicCharacterBase.h"
#include "Combat/TunicCombatHitWindowSourceInterface.h"
#include "Combat/TunicCombatTargetInterface.h"
#include "TunicPlayerCharacter.generated.h"

class UCameraComponent;
class UGameplayAbility;
class UGameplayEffect;
class UInputAction;
class UAnimMontage;
class UTunicAbilitySystemComponent;
class AActor;
class FLifetimeProperty;
struct FInputActionValue;
struct FOnAttributeChangeData;
struct FHitResult;
class USpringArmComponent;

UCLASS(Blueprintable)
class LPQUEST_API ATunicPlayerCharacter : public ATunicCharacterBase, public ITunicCombatTargetInterface, public ITunicCombatHitWindowSourceInterface
{
	GENERATED_BODY()

public:
	ATunicPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsCombatTargetAvailable() const override;
	virtual ETunicCombatTeam GetCombatTargetTeam() const override;
	virtual UTunicAbilitySystemComponent* GetCombatTargetAbilitySystemComponent() const override;
	virtual UTunicAttributeSet* GetCombatTargetAttributeSet() const override;
	virtual void HandleCombatTargetHitReaction(AActor* InstigatorActor) override;
	virtual void BeginCombatHitWindow(FName HitWindowName) override;
	virtual void ProcessCombatHitWindow(FName HitWindowName) override;
	virtual void EndCombatHitWindow(FName HitWindowName) override;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat", meta = (ToolTip = "手动开始玩家轻击 hit window。通常由 AnimNotifyState 调用，用于开启服务器命中检测窗口。"))
	void BeginLightAttackHitWindow();

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat", meta = (ToolTip = "处理玩家轻击 hit window 的一次命中检测。命中结果由服务器确认并通过 GameplayEffect 结算。"))
	void ProcessLightAttackHitWindow();

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat", meta = (ToolTip = "结束玩家轻击 hit window，并清理本次窗口已命中过的目标集合。"))
	void EndLightAttackHitWindow();

	void ExecuteLightAttackAbility();
	void ExecuteDodgeAbility();
	void NotifyDodgeInvulnerabilitySuccess(AActor* InstigatorActor);

	UFUNCTION(BlueprintPure, Category = "Tunic|Combat", meta = (ToolTip = "玩家当前是否死亡。死亡后不能攻击或 dodge。"))
	bool IsDead() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input", meta = (ToolTip = "移动输入 Action。由 Enhanced Input 触发，角色会按固定视角换算成世界方向。"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input", meta = (ToolTip = "Dodge 输入 Action。实际 dodge 通过服务器 GameplayAbility 和手动 RootMotionSource 执行。"))
	TObjectPtr<UInputAction> DodgeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input", meta = (ToolTip = "轻击输入 Action。当前会请求服务器激活 LightAttackAbility。"))
	TObjectPtr<UInputAction> LightAttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input", meta = (ToolTip = "重击输入 Action。当前主要用于表现 hook，占位给后续重击/连招。"))
	TObjectPtr<UInputAction> HeavyAttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input", meta = (ToolTip = "瞄准输入 Action。当前主要触发表现 hook，占位给后续远程/魔法。"))
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input", meta = (ToolTip = "交互输入 Action。当前触发表现 hook，占位给后续拾取/交互。"))
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input", meta = (ToolTip = "切换武器输入 Action。当前触发表现 hook，占位给后续装备/武器系统。"))
	TObjectPtr<UInputAction> SwitchWeaponAction;

	void Move(const FInputActionValue& Value);
	void StartDodge(const FInputActionValue& Value);
	void LightAttack(const FInputActionValue& Value);
	void HeavyAttack(const FInputActionValue& Value);
	void StartAim(const FInputActionValue& Value);
	void StopAim(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void SwitchWeapon(const FInputActionValue& Value);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input", meta = (ToolTip = "本地按下 Dodge 时的表现 hook。实际动作锁和位移由服务器 Dodge Ability / RootMotionSource 控制。"))
	void OnDodgeRequested();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input", meta = (ToolTip = "本地请求轻击时的表现 hook。实际攻击激活、hit window 和伤害由服务器路径控制。"))
	void OnLightAttackRequested();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Combat", meta = (ToolTip = "玩家受击表现 hook。不要在这里直接改 Health。"))
	void OnHitReaction(AActor* InstigatorActor);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Combat", meta = (ToolTip = "死亡状态变化表现 hook。死亡状态由服务器 Health 路径决定并复制。"))
	void OnDeathStateChanged(bool bNewIsDead);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input", meta = (ToolTip = "本地请求重击时的表现 hook。当前不应用伤害或能力奖励。"))
	void OnHeavyAttackRequested();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input", meta = (ToolTip = "瞄准开始时的表现 hook。当前不授予技能或改变属性。"))
	void OnAimStarted();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input", meta = (ToolTip = "瞄准结束时的表现 hook。当前只用于表现扩展。"))
	void OnAimStopped();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input", meta = (ToolTip = "交互输入请求时的表现 hook。当前不执行正式拾取或开门逻辑。"))
	void OnInteractRequested();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input", meta = (ToolTip = "切换武器输入请求时的表现 hook。当前不改变装备状态。"))
	void OnSwitchWeaponRequested();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出玩家 ASC / AttributeSet 初始化日志。只用于验证 GAS 所有权。"))
	bool bLogAbilitySystemInitialization = true;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug", meta = (ToolTip = "运行时开关玩家 ASC 初始化日志。只影响日志，不影响 gameplay。"))
	void SetAbilitySystemInitializationLoggingEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出 Dodge 请求和服务器确认日志。只用于验证输入/RPC/Ability 路径。"))
	bool bLogDodgeRequests = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "Dodge 无敌帧成功挡掉一次伤害时，是否在 owning client 屏幕上显示验证提示。只用于开发调试。"))
	bool bShowDodgeInvulnerabilityDebugMessage = true;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug", meta = (ToolTip = "运行时开关 Dodge 请求日志。只影响日志，不影响冷却或位移。"))
	void SetDodgeRequestLoggingEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出轻击请求和服务器确认日志。只用于验证输入/RPC/Ability 路径。"))
	bool bLogLightAttackRequests = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出玩家受击表现日志。只用于验证 hit reaction 路径。"))
	bool bLogHitReaction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出玩家死亡状态日志。只用于验证死亡复制和表现路径。"))
	bool bLogDeathState = true;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug", meta = (ToolTip = "运行时开关轻击请求日志。只影响日志，不影响攻击。"))
	void SetLightAttackRequestLoggingEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出轻击 sweep 命中检测日志。只用于验证命中范围和目标过滤。"))
	bool bLogLightAttackHitSweep = true;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug", meta = (ToolTip = "运行时开关轻击 sweep 日志。只影响日志，不影响命中检测。"))
	void SetLightAttackHitSweepLoggingEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否在屏幕/世界中绘制玩家属性调试信息。只用于开发验证。"))
	bool bDrawAttributeDebug = true;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug", meta = (ToolTip = "运行时开关玩家属性调试绘制。只影响显示，不影响属性值。"))
	void SetAttributeDebugDrawEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug", meta = (DeprecatedFunction, DeprecationMessage = "Use SetLightAttackHitSweepLoggingEnabled.", ToolTip = "已废弃：请使用 SetLightAttackHitSweepLoggingEnabled。保留只为旧蓝图兼容。"))
	void SetLightAttackTargetQueryLoggingEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ToolTip = "请求轻击时是否立即跑一次 hit window。用于没有完整动画通知时的 prototype 验证。"))
	bool bRunLightAttackHitWindowOnRequest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "轻击 capsule sweep 半径，单位 cm。实际命中仍由服务器检测。"))
	float LightAttackSweepRadius = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "轻击 capsule sweep 半高，单位 cm。影响上下命中范围。"))
	float LightAttackSweepHalfHeight = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (Units = "cm", ToolTip = "轻击 sweep 起点的角色本地偏移，单位 cm。X 通常表示角色前方。"))
	FVector LightAttackSweepStartOffset = FVector(60.0f, 0.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (Units = "cm", ToolTip = "轻击 sweep 终点的角色本地偏移，单位 cm。决定攻击向前覆盖多远。"))
	FVector LightAttackSweepEndOffset = FVector(225.0f, 0.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Debug", meta = (ToolTip = "命中目标时是否应用当前 debug damage GameplayEffect。只用于 prototype 伤害验证。"))
	bool bApplyLightAttackDebugDamage = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Debug", meta = (ToolTip = "轻击命中时应用的 debug damage GameplayEffect class。后续正式武器/技能会替代这个路径。"))
	TSubclassOf<UGameplayEffect> LightAttackDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Animation", meta = (ToolTip = "轻击播放的 Montage。表现播放不直接决定最终伤害，伤害仍由服务器 hit window 确认。"))
	TObjectPtr<UAnimMontage> LightAttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Presentation", meta = (ToolTip = "默认受击表现 Montage。只负责表现，不直接改 Health。"))
	TObjectPtr<UAnimMontage> DefaultHitReactionMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Presentation", meta = (ToolTip = "默认死亡表现 Montage。死亡状态由服务器 Health 路径决定。"))
	TObjectPtr<UAnimMontage> DefaultDeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Movement|Animation", meta = (ToolTip = "Dodge 表现 Montage。当前位移来自服务器 RootMotionSource 手动短冲刺，不依赖动画 Root Motion。"))
	TObjectPtr<UAnimMontage> DodgeMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Movement|Dodge", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Dodge 手动短冲刺距离，单位 cm。由服务器 RootMotionSource 执行。"))
	float DodgeDistance = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Movement|Dodge", meta = (ClampMin = "0.01", Units = "s", ToolTip = "Dodge 手动位移持续时间，单位秒。越短速度越快。"))
	float DodgeDuration = 0.32f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Movement|Dodge", meta = (ClampMin = "0", ToolTip = "Dodge RootMotionSource 优先级。用于覆盖普通移动输入，避免短冲刺被低优先级移动抵消。"))
	int32 DodgeRootMotionSourcePriority = 500;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Movement|Dodge", meta = (ClampMin = "0.0", Units = "s", ToolTip = "Dodge 方向输入宽限时间，单位秒。刚松开移动键后仍可按最后移动方向 dodge；超时则按角色朝向。"))
	float DodgeMovementInputDirectionGraceTime = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Abilities", meta = (ToolTip = "玩家默认授予的轻击 GameplayAbility class。实际激活由 ASC 和服务器 authority 控制。"))
	TSubclassOf<UGameplayAbility> LightAttackAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Abilities", meta = (ToolTip = "玩家默认授予的 Dodge GameplayAbility class。负责 cooldown/action-lock，位移由角色执行。"))
	TSubclassOf<UGameplayAbility> DodgeAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Abilities", meta = (ToolTip = "玩家默认应用的耐力恢复 GameplayEffect class。由 PlayerState-owned ASC 持有。"))
	TSubclassOf<UGameplayEffect> StaminaRegenEffectClass;

private:
	bool GetMouseFacingYaw(float& OutYaw) const;
	void GetFixedViewMovementDirections(FVector& OutScreenUpDirection, FVector& OutScreenRightDirection) const;
	void UpdateFacingToMouse(float DeltaSeconds, bool bForceServerUpdate);
	void InitializePlayerAbilitySystem();
	void BindHealthChangeDelegate(UTunicAbilitySystemComponent* PlayerAbilitySystemComponent);
	void GrantDefaultAbilities(UTunicAbilitySystemComponent* PlayerAbilitySystemComponent);
	void ApplyDefaultEffects(UTunicAbilitySystemComponent* PlayerAbilitySystemComponent);
	void LogPlayerAbilitySystemDebug(const class ATunicPlayerState* TunicPlayerState, const class UTunicAbilitySystemComponent* PlayerAbilitySystemComponent, const class UTunicAttributeSet* PlayerAttributeSet) const;
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void SetDead(bool bNewIsDead);
	void ApplyDeathState();
	void PlayPresentationMontage(UAnimMontage* MontageToPlay, bool bStopAllMontages);
	void RequestDodge();
	void HandleDodgeRequest();
	bool TryActivateDodgeAbility();
	bool PlayDodgeMontage();
	bool StartManualDodgeMovement();
	FVector GetDodgeDirection() const;
	void LogDodgeRequestDebug() const;
	void RequestLightAttack();
	void HandleLightAttackRequest();
	bool TryActivateLightAttackAbility();
	bool PlayLightAttackMontage();
	void CheckLightAttackMontageHitWindowTriggered(int32 MontageSerial);
	void LogLightAttackRequestDebug() const;
	void DrawAttributeDebug() const;
	FVector GetLightAttackSweepPoint(const FVector& LocalOffset) const;
	void LogLightAttackHitSweepDebug(const TArray<FHitResult>& HitResults, int32 ProcessedHitCount) const;
	void ApplyLightAttackDebugDamage(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget);
	void HandleLightAttackTargetHit(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget);
	void LogServerInputRequestDebug(const TCHAR* RequestName, bool bShouldLog) const;

	UFUNCTION(Server, Reliable)
	void ServerRequestDodge(FVector RequestedDodgeDirection);

	UFUNCTION(Server, Reliable)
	void ServerRequestLightAttack();

	UFUNCTION(Server, Unreliable)
	void ServerSetFacingYaw(float NewYaw, bool bSnap);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayLightAttackMontage(UAnimMontage* MontageToPlay);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayDodgeMontage(UAnimMontage* MontageToPlay);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitReaction(AActor* InstigatorActor);

	UFUNCTION(Client, Unreliable)
	void ClientShowDodgeInvulnerabilitySuccess(AActor* InstigatorActor);

	UFUNCTION()
	void OnRep_IsDead();

	UPROPERTY(EditAnywhere, Category = "Tunic|Combat|Facing", meta = (ToolTip = "攻击时是否强制面向鼠标方向。只影响朝向同步，不直接决定命中结果。"))
	bool bFaceMouseOnAttack = true;

	UPROPERTY(EditAnywhere, Category = "Tunic|Combat|Facing", meta = (ClampMin = "0.01", Units = "s", ToolTip = "向服务器同步鼠标朝向的最小间隔，单位秒。数值越小同步越频繁。"))
	float MouseFacingServerUpdateInterval = 0.05f;

	FActiveGameplayEffectHandle StaminaRegenEffectHandle;
	float TimeSinceLastMouseFacingServerUpdate = 0.0f;
	bool bLightAttackHitWindowActive = false;
	bool bDeathPresentationApplied = false;
	int32 LightAttackMontageActivationSerial = 0;
	int32 LightAttackMontageHitWindowSerial = 0;
	FVector LastNonZeroMovementInputWorldDirection = FVector::ZeroVector;
	float LastMovementInputDirectionUpdateTime = -1.0f;
	TSet<TWeakObjectPtr<AActor>> LightAttackHitTargets;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;
};

