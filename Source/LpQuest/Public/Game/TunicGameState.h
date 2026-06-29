// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TunicGameState.generated.h"

class FLifetimeProperty;
class AActor;

UENUM(BlueprintType)
enum class ETunicRunState : uint8
{
	CombatActive UMETA(DisplayName = "Combat Active", ToolTip = "战斗进行中。Spawner/AI/击杀 XP 正常工作。"),

	PartyWiped UMETA(DisplayName = "Party Wiped", ToolTip = "队伍全灭。不会自动进入下一层，也不会继续发放击杀 XP。"),

	EncounterCleared UMETA(DisplayName = "Encounter Cleared", ToolTip = "当前测试 encounter 已清场。保留给 Spawner/奖励验证；Portal Event 长期由交互启动。"),

	FloorTransitionReady UMETA(DisplayName = "Floor Transition Ready", ToolTip = "Portal 已 ready，等待 GameMode 的 floor transition stub 完成。"),

	PortalEventActive UMETA(DisplayName = "Portal Event Active", ToolTip = "Portal event 已由玩家交互启动。后续 Boss、充能和压力刷怪基于这个状态推进。")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTunicRunStateChangedSignature, ETunicRunState, NewRunState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTunicFloorIndexChangedSignature, int32, NewFloorIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTunicSharedRunExperienceChangedSignature, int32, NewValue, int32, Delta, AActor*, SourceActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTunicSharedRunLevelChangedSignature, int32, NewLevel);

UCLASS(Blueprintable)
class LPQUEST_API ATunicGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ATunicGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run", meta = (ToolTip = "返回服务器复制的当前 RunState。UI 只读显示，不应从客户端修改。"))
	ETunicRunState GetRunState() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run", meta = (ToolTip = "返回当前 run 内楼层编号。floor stub 完成后递增，不代表永久存档进度。"))
	int32 GetCurrentFloorIndex() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run", meta = (ToolTip = "返回全队共享的 run-local XP。由 GameMode 在 spawned encounter 敌人死亡时增加。"))
	int32 GetSharedRunExperience() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run", meta = (ToolTip = "返回全队共享的 run-local Level。服务器根据 SharedRunExperience 和 SharedExperiencePerLevel 推导。"))
	int32 GetSharedRunLevel() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run", meta = (ToolTip = "当前 RunState 是否允许普通战斗、AI、Spawner 和击杀奖励推进。CombatActive 和 PortalEventActive 都返回 true。"))
	bool IsCombatActive() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run", meta = (ToolTip = "当前 RunState 是否为 PartyWiped。全灭状态不会被 Portal 或 floor stub 自动覆盖。"))
	bool IsPartyWiped() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run", meta = (ToolTip = "当前 RunState 是否为 EncounterCleared。用于测试 encounter 清场显示；Portal Event 不再依赖它自动启动。"))
	bool IsEncounterCleared() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run", meta = (ToolTip = "当前 RunState 是否为 PortalEventActive。表示玩家已通过 Portal 交互启动本层事件。"))
	bool IsPortalEventActive() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run", meta = (ToolTip = "当前 RunState 是否为 FloorTransitionReady。表示 Portal 已充满并等待 GameMode stub。"))
	bool IsFloorTransitionReady() const;

	UPROPERTY(BlueprintAssignable, Category = "Tunic|Run")
	FTunicRunStateChangedSignature OnRunStateChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Tunic|Run")
	FTunicFloorIndexChangedSignature OnFloorIndexChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Tunic|Run")
	FTunicSharedRunExperienceChangedSignature OnSharedRunExperienceChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Tunic|Run")
	FTunicSharedRunLevelChangedSignature OnSharedRunLevelChangedEvent;

	void SetRunState(ETunicRunState NewRunState);
	void SetCurrentFloorIndex(int32 NewFloorIndex);
	void AddSharedRunExperience(int32 Amount, AActor* SourceActor);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Run", meta = (ToolTip = "RunState 变化时的表现 hook。不要在这里反向修改 RunState。"))
	void OnRunStateChanged(ETunicRunState NewRunState);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Run", meta = (ToolTip = "楼层编号变化时的表现 hook。只用于 UI/音效/表现。"))
	void OnFloorIndexChanged(int32 NewFloorIndex);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Run", meta = (ToolTip = "共享 XP 变化时的表现 hook。SourceActor 是奖励来源，可能为空。"))
	void OnSharedRunExperienceChanged(int32 NewValue, int32 Delta, AActor* SourceActor);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Run", meta = (ToolTip = "共享 run level 变化时的表现 hook。本阶段 level 不直接改变属性或技能。"))
	void OnSharedRunLevelChanged(int32 NewLevel);

private:
	void RecalculateSharedRunLevel();

	UFUNCTION()
	void OnRep_RunState();

	UFUNCTION()
	void OnRep_CurrentFloorIndex();

	UFUNCTION()
	void OnRep_SharedRunExperience(int32 OldSharedRunExperience);

	UFUNCTION()
	void OnRep_SharedRunLevel();

	UPROPERTY(ReplicatedUsing = OnRep_RunState)
	ETunicRunState RunState = ETunicRunState::CombatActive;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentFloorIndex)
	int32 CurrentFloorIndex = 1;

	UPROPERTY(ReplicatedUsing = OnRep_SharedRunExperience)
	int32 SharedRunExperience = 0;

	UPROPERTY(ReplicatedUsing = OnRep_SharedRunLevel)
	int32 SharedRunLevel = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Tunic|Run", meta = (ClampMin = "1", ToolTip = "每级需要的共享 XP。Level 公式为 1 + SharedRunExperience / SharedExperiencePerLevel。"))
	int32 SharedExperiencePerLevel = 5;
};

