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

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "返回本 spawner 当前仍存在的 generated encounter 敌人数量。"))
	int32 GetSpawnedEncounterEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "返回本 spawner 当前仍存活的 generated encounter 敌人数量。清场判定使用这个值。"))
	int32 GetAliveEncounterEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "返回当前 encounter 对应的 floor index。0 表示还没有有效 encounter。"))
	int32 GetCurrentEncounterFloorIndex() const;

	void SpawnEncounterForFloor(int32 FloorIndex);
	void ResetEncounterForNextFloor();
	bool EvaluateEncounterClear(int32& OutTotalEnemyCount, int32& OutAliveEnemyCount);
	bool IsEncounterEnemy(const ATunicEnemyCharacter* EnemyCharacter) const;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tunic|Spawner", meta = (ToolTip = "关卡里可视化放置的出生点 Actor。Spawner 会读取它们的 Transform 生成敌人。"))
	TArray<TObjectPtr<AActor>> SpawnPoints;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tunic|Spawner", meta = (ToolTip = "固定波次配置。每个 entry 指定敌人类和数量，本阶段不做随机刷怪。"))
	TArray<FTunicEncounterSpawnEntry> SpawnEntries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug", meta = (ToolTip = "是否输出 Spawner 生成、清场评估和数量日志。只用于验证，不影响玩法。"))
	bool bLogSpawnerState = true;

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Spawner", meta = (ToolTip = "服务器生成 encounter 后触发的表现 hook。不要在这里决定 RunState 或奖励。"))
	void OnEncounterSpawned(int32 FloorIndex);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Spawner", meta = (ToolTip = "本 spawner 追踪的 encounter 全部死亡后触发的表现 hook。RunState 仍由 GameMode 决定。"))
	void OnEncounterCleared(int32 FloorIndex);

private:
	void CleanupSpawnedEnemies();

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ATunicEnemyCharacter>> SpawnedEncounterEnemies;

	int32 CurrentEncounterFloorIndex = 0;
	bool bHasSpawnedEncounter = false;
};
