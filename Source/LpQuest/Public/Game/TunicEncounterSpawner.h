// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunicEncounterSpawner.generated.h"

class ATunicEnemyCharacter;

UCLASS(Blueprintable)
class LPQUEST_API ATunicEncounterSpawner : public AActor
{
	GENERATED_BODY()

public:
	ATunicEncounterSpawner();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "当前 registry 是否有有效 registered placed encounter 成员。普通 generated wave 由 ATunicFloorWaveEnemySpawnSource 负责。"))
	bool HasActiveEncounter() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "返回本 registry 当前仍存在的 registered placed encounter 敌人数量。登记后可通过 encounter ownership 给 XP；未登记不会通过 encounter ownership 给 XP。"))
	int32 GetPlacedEncounterEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "返回本 registry 当前仍存在的全部 registered placed encounter 成员数量。普通 generated wave 由 ATunicFloorWaveEnemySpawnSource 统计。"))
	int32 GetTotalEncounterEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner", meta = (ToolTip = "返回本 registry 当前仍存活的 registered placed encounter 成员数量。用于调试和奖励归属。"))
	int32 GetAliveEncounterEnemyCount() const;

	bool IsEncounterEnemy(const ATunicEnemyCharacter* EnemyCharacter) const;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tunic|Spawner", meta = (ToolTip = "手动摆放并显式登记为 encounter 成员的敌人。登记后可通过 encounter ownership 按敌人自身 ExperienceReward 给 XP；未登记的手摆敌人不会通过 encounter ownership 给 XP。普通 generated wave 请使用 ATunicFloorWaveEnemySpawnSource。"))
	TArray<TObjectPtr<ATunicEnemyCharacter>> PlacedEncounterEnemies;
};
