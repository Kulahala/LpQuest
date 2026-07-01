// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunicEnemySpawnSource.generated.h"

class ATunicEnemyCharacter;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Abstract, Blueprintable)
class LPQUEST_API ATunicEnemySpawnSource : public AActor
{
	GENERATED_BODY()

public:
	ATunicEnemySpawnSource();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "LPQ|Spawn", meta = (ToolTip = "返回本生成源当前仍存在的 generated 敌人数量，包含已死亡但尚未清理的尸体。"))
	int32 GetSpawnedEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "LPQ|Spawn", meta = (ToolTip = "返回本生成源当前仍存活的 generated 敌人数量。"))
	int32 GetAliveSpawnedEnemyCount() const;

	int32 SpawnEnemies();
	void CleanupSpawnedEnemies();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LPQ|Spawn")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LPQ|Spawn")
	TObjectPtr<USphereComponent> RadiusPreview;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LPQ|Spawn")
	TObjectPtr<UStaticMeshComponent> SpawnSourceEditorMarker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LPQ|Spawn", meta = (ToolTip = "本生成源要生成的敌人类。普通楼层波次直接在这个 Actor 上配置敌人类型。"))
	TSubclassOf<ATunicEnemyCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LPQ|Spawn", meta = (ClampMin = "0", ToolTip = "本生成源一次生成的敌人数量。0 表示不生成。"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LPQ|Spawn", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "随机生成半径。0 表示使用本 Actor 原始 Transform，不随机也不投射 NavMesh。"))
	float SpawnRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LPQ|Spawn", meta = (EditCondition = "SpawnRadius > 0.0", ToolTip = "SpawnRadius 大于 0 时，是否把随机点投射到 NavMesh。"))
	bool bProjectRandomSpawnToNavigation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LPQ|Spawn", meta = (EditCondition = "SpawnRadius > 0.0", ToolTip = "随机点投射到 NavMesh 时使用的搜索范围。"))
	FVector NavProjectionExtent = FVector(300.0f, 300.0f, 500.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LPQ|Debug", meta = (ToolTip = "是否输出生成源刷怪、NavMesh 投射失败和清理日志。只用于验证，不影响玩法。"))
	bool bLogSpawnSourceState = true;

	FTransform ResolveSpawnTransform() const;
	void UpdateRadiusPreview();

private:
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ATunicEnemyCharacter>> SpawnedEnemies;
};

UCLASS(Blueprintable)
class LPQUEST_API ATunicFloorWaveEnemySpawnSource : public ATunicEnemySpawnSource
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "LPQ|Spawn", meta = (ToolTip = "返回本生成源最近一次生成对应的 floor index。0 表示尚未生成。"))
	int32 GetCurrentSpawnFloorIndex() const;

	int32 SpawnForFloor(int32 FloorIndex);
	void ResetForNextFloor();

private:
	int32 CurrentSpawnFloorIndex = 0;
};
