// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/TunicInteractableInterface.h"
#include "TunicPortalActor.generated.h"

class ATunicEnemyCharacter;
class ATunicPlayerCharacter;
class FLifetimeProperty;
class USceneComponent;
class USphereComponent;

UCLASS(Blueprintable)
class LPQUEST_API ATunicPortalActor : public AActor, public ITunicInteractableInterface
{
	GENERATED_BODY()

public:
	ATunicPortalActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Portal", meta = (ToolTip = "Portal 当前是否已激活。玩家通过统一交互键启动 Portal Event 后，服务器会激活 Portal。"))
	bool IsPortalActive() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Portal", meta = (ToolTip = "Portal 当前是否正在充能。需要足够存活玩家在范围内。"))
	bool IsPortalCharging() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Portal", meta = (ToolTip = "Portal 是否已 ready。ready 后由 GameMode 推进 FloorTransitionReady / floor stub。"))
	bool IsPortalReady() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Portal", meta = (ToolTip = "Portal 当前充能进度，范围 0 到 1。由服务器复制给客户端显示。"))
	float GetActivationProgress() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Portal", meta = (ToolTip = "当前需要进入 Portal 范围的存活玩家数量。死亡玩家不计入要求。"))
	int32 GetRequiredLivingPlayerCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Portal", meta = (ToolTip = "当前已经在 Portal 范围内的存活玩家数量。用于验证充能条件。"))
	int32 GetPresentLivingPlayerCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Portal", meta = (ToolTip = "Portal 交互半径，单位 cm。玩家在范围内按统一交互键 E，服务器验证后可启动 Portal Event。"))
	float GetInteractionRadius() const;

	int32 ConsumePortalPressureExperienceReward(ATunicEnemyCharacter* DeadEnemy);
	void ResetPortalForNextFloorStub();
	virtual bool CanInteractWithTunicPlayer_Implementation(ATunicPlayerCharacter* InteractingPlayer) override;
	virtual void InteractWithTunicPlayer_Implementation(ATunicPlayerCharacter* InteractingPlayer) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Portal")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Portal")
	TObjectPtr<USphereComponent> PortalRadiusPreview;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Portal", meta = (ClampMin = "1.0", Units = "cm", ToolTip = "Portal 检测玩家的半径，单位 cm。OnConstruction 会同步预览 Sphere 半径。"))
	float ActivationRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Portal", meta = (ClampMin = "1.0", Units = "cm", ToolTip = "玩家按统一交互键 E 启动 Portal Event 的最大距离，单位 cm。与充能半径分开，便于后续交互提示和充能范围分别调参。"))
	float InteractionRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Portal", meta = (ClampMin = "0.0", Units = "s", ToolTip = "满足人数后充满 Portal 所需时间，单位秒。0 表示满足条件后立即 ready。"))
	float ChargeDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Portal|Boss", meta = (ToolTip = "Portal Event 启动后由服务器生成的测试 Boss 敌人类。v1 仍使用 ATunicEnemyCharacter 子 Blueprint，不需要 Boss C++ 类；未配置时保持直接充能。"))
	TSubclassOf<ATunicEnemyCharacter> PortalBossEnemyClass;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tunic|Portal|Boss", meta = (ToolTip = "测试 Boss 出生点。未配置时使用 Portal 自身 Transform。"))
	TObjectPtr<AActor> PortalBossSpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Portal|Pressure", meta = (ToolTip = "Portal 充能压力刷怪使用的普通敌人类。Boss 死亡且 Portal Event 仍激活时，服务器按间隔生成；未配置则不刷压力怪。"))
	TSubclassOf<ATunicEnemyCharacter> PortalPressureEnemyClass;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tunic|Portal|Pressure", meta = (ToolTip = "Portal 压力怪出生点列表。为空或引用无效时 fallback 到 Portal 自身 Transform。"))
	TArray<TObjectPtr<AActor>> PortalPressureSpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Portal|Pressure", meta = (ClampMin = "0.1", Units = "s", ToolTip = "Portal 充能阶段压力怪生成间隔，单位秒。玩家离开充能圈只暂停充能，不暂停压力刷怪。"))
	float PortalPressureSpawnInterval = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Portal|Pressure", meta = (ClampMin = "0", ToolTip = "同一 Portal Event 中最多同时存活的压力怪数量。0 表示不生成压力怪。"))
	int32 MaxAlivePortalPressureEnemies = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Portal|Pressure", meta = (ClampMin = "0", ToolTip = "同一 Portal Event 中压力怪最多可授予的共享 XP 总预算。0 表示压力怪不给 XP。"))
	int32 PortalPressureExperienceBudget = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出 Portal 激活、人数、充能、ready 和 reset 日志。只用于验证，不影响玩法。"))
	bool bLogPortalState = true;

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Portal", meta = (ToolTip = "Portal Event 由玩家交互启动后激活 Portal 时触发的表现 hook。不要在这里推进 RunState。"))
	void OnPortalActivated();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Portal", meta = (ToolTip = "Portal 充能开始或暂停时触发的表现 hook。可用于开关特效或音效。"))
	void OnPortalChargingStateChanged(bool bNewIsCharging);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Portal", meta = (ToolTip = "Portal 充能进度变化时触发的表现 hook。NewProgress 范围 0 到 1。"))
	void OnPortalChargeChanged(float NewProgress);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Portal", meta = (ToolTip = "Portal 充满并 ready 时触发的表现 hook。最终切层仍由 GameMode 管。"))
	void OnPortalReady();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Portal", meta = (ToolTip = "floor stub 重置 Portal 时触发的表现 hook。可关闭特效或恢复默认外观。"))
	void OnPortalReset();

private:
	void TryActivatePortalFromRunState();
	void EvaluatePortalCharge(float DeltaSeconds);
	void CompletePortal();
	void SpawnPortalBossIfNeeded();
	bool IsPortalBossDefeated() const;
	void CleanupPortalBoss();
	void ResetPortalPressureState();
	void TickPortalPressureSpawns(float DeltaSeconds);
	bool ShouldSpawnPortalPressure() const;
	void SpawnPortalPressureEnemy();
	FTransform GetNextPortalPressureSpawnTransform();
	int32 GetAlivePortalPressureEnemyCount() const;
	bool OwnsPortalPressureEnemy(const ATunicEnemyCharacter* EnemyCharacter) const;
	bool HasPortalPressureEnemyBeenRewarded(const ATunicEnemyCharacter* EnemyCharacter) const;
	void CleanupPortalPressureEnemies();
	bool CountLivingPlayersInRange(int32& OutRequiredLivingPlayerCount, int32& OutPresentLivingPlayerCount) const;
	void UpdatePortalRadiusPreview();
	void SetPortalActive(bool bNewIsPortalActive);
	void SetPortalCharging(bool bNewIsCharging);
	void SetPortalReady(bool bNewIsReady);
	void SetActivationProgress(float NewActivationProgress);
	void SetPortalPlayerCounts(int32 NewRequiredLivingPlayerCount, int32 NewPresentLivingPlayerCount);

	UFUNCTION()
	void OnRep_IsPortalActive();

	UFUNCTION()
	void OnRep_IsPortalCharging();

	UFUNCTION()
	void OnRep_IsPortalReady();

	UFUNCTION()
	void OnRep_ActivationProgress();

	UFUNCTION()
	void OnRep_PortalResetSerial();

	UPROPERTY(ReplicatedUsing = OnRep_IsPortalActive)
	bool bIsPortalActive = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsPortalCharging)
	bool bIsPortalCharging = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsPortalReady)
	bool bIsPortalReady = false;

	UPROPERTY(ReplicatedUsing = OnRep_ActivationProgress)
	float ActivationProgress = 0.0f;

	UPROPERTY(Replicated)
	int32 RequiredLivingPlayerCount = 0;

	UPROPERTY(Replicated)
	int32 PresentLivingPlayerCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_PortalResetSerial)
	int32 PortalResetSerial = 0;

	TWeakObjectPtr<ATunicEnemyCharacter> SpawnedPortalBossEnemy;
	bool bPortalBossSpawnFailed = false;
	TArray<TWeakObjectPtr<ATunicEnemyCharacter>> SpawnedPortalPressureEnemies;
	TArray<TWeakObjectPtr<ATunicEnemyCharacter>> RewardedPortalPressureEnemies;
	float PortalPressureSpawnTimer = 0.0f;
	int32 PortalPressureSpawnPointIndex = 0;
	int32 RemainingPortalPressureExperienceBudget = 0;
};
