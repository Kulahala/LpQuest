// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunicEncounterSpawner.generated.h"

class ATunicEnemyCharacter;

USTRUCT(BlueprintType)
struct LPQUEST_API FTunicEncounterSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Spawner", meta = (ToolTip = "本波次要生成的敌人类。Spawner 只追踪它生成出来的 encounter 成员。"))
	TSubclassOf<ATunicEnemyCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Spawner", meta = (ClampMin = "0", ToolTip = "该敌人类在本波次生成的数量。出生点不足时会循环使用 SpawnPoints。"))
	int32 Count = 1;
};

UCLASS(Blueprintable)
class LPQUEST_API ATunicEncounterSpawner : public AActor
{
	GENERATED_BODY()

public:
	ATunicEncounterSpawner();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "当前 spawner 是否已经为当前 encounter 生成过敌人。"))
	bool HasSpawnedEncounter() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "当前 spawner 是否有有效 encounter 成员。成员包括 generated enemies 和显式登记的手摆敌人。"))
	bool HasActiveEncounter() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "返回本 spawner 当前仍存在的 generated encounter 敌人数量。"))
	int32 GetSpawnedEncounterEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "返回本 spawner 当前仍存在的 registered placed encounter 敌人数量。登记后可通过 encounter ownership 给 XP；未登记不会通过 encounter ownership 给 XP。"))
	int32 GetPlacedEncounterEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "返回本 spawner 当前仍存在的全部 encounter 成员数量，包含 generated enemies 和 registered placed enemies。"))
	int32 GetTotalEncounterEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "返回本 spawner 当前仍存活的 encounter 成员数量，包含 generated enemies 和 registered placed enemies。用于调试、奖励归属和后续刷怪策略。"))
	int32 GetAliveEncounterEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "返回当前 encounter 对应的 floor index。0 表示还没有有效 encounter。"))
	int32 GetCurrentEncounterFloorIndex() const;

	void SpawnEncounterForFloor(int32 FloorIndex);
	void ResetEncounterForNextFloor();
	bool IsEncounterEnemy(const ATunicEnemyCharacter* EnemyCharacter) const;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tunic|Spawner", meta = (ToolTip = "关卡里可视化放置的出生点 Actor。Spawner 会读取它们的 Transform 生成敌人。"))
	TArray<TObjectPtr<AActor>> SpawnPoints;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tunic|Spawner", meta = (ToolTip = "固定波次配置。每个 entry 指定敌人类和数量，本阶段不做随机刷怪。"))
	TArray<FTunicEncounterSpawnEntry> SpawnEntries;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tunic|Spawner", meta = (ToolTip = "手动摆放并显式登记为本 encounter 成员的敌人。登记后可通过 encounter ownership 按敌人自身 ExperienceReward 给 XP；未登记的手摆敌人不会通过 encounter ownership 给 XP。"))
	TArray<TObjectPtr<ATunicEnemyCharacter>> PlacedEncounterEnemies;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出 Spawner 生成、清场评估和数量日志。只用于验证，不影响玩法。"))
	bool bLogSpawnerState = true;

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Spawner", meta = (ToolTip = "服务器激活 encounter 后触发的表现 hook。可能包含 generated enemies、registered placed enemies 或两者都有；不要在这里决定 RunState 或奖励。"))
	void OnEncounterSpawned(int32 FloorIndex);

private:
	void CleanupSpawnedEnemies();
	void RefreshActivePlacedEncounterEnemies();

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ATunicEnemyCharacter>> SpawnedEncounterEnemies;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ATunicEnemyCharacter>> ActivePlacedEncounterEnemies;

	int32 CurrentEncounterFloorIndex = 0;
	bool bHasSpawnedEncounter = false;
};
