// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LPQEnemyPatrolRoute.generated.h"

class USceneComponent;
class USplineComponent;

UCLASS(Blueprintable)
class LPQUEST_API ALPQEnemyPatrolRoute : public AActor
{
	GENERATED_BODY()

public:
	ALPQEnemyPatrolRoute();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "LPQ|AI|Patrol", meta = (ToolTip = "返回这条巡逻路线的 RouteId。AIController 通过同名 PatrolRouteId 绑定到这条路线。"))
	FName GetRouteId() const;

	UFUNCTION(BlueprintPure, Category = "LPQ|AI|Patrol", meta = (ToolTip = "返回当前自动生成的 movement samples 数量。AI 实际巡逻目标来自这些采样点，不是 spline 控制点。"))
	int32 GetPatrolPointCount() const;

	UFUNCTION(BlueprintPure, Category = "LPQ|AI|Patrol", meta = (ToolTip = "返回指定采样点的世界位置。位置会尽量投影到 NavMesh，避免 spline 点放高或放低导致 AI 跑偏。"))
	FVector GetPatrolPointLocation(int32 PointIndex) const;

	UFUNCTION(BlueprintPure, Category = "LPQ|AI|Patrol", meta = (ToolTip = "返回指定采样点是否是 stop sample。StateTree 只应在 stop sample 上等待。"))
	bool IsPatrolPointStop(int32 PointIndex) const;

	UFUNCTION(BlueprintPure, Category = "LPQ|AI|Patrol", meta = (ToolTip = "返回指定 stop sample 的等待时间，单位秒。非 stop sample 返回 0。"))
	float GetPatrolStopHoldDuration(int32 PointIndex) const;

	UFUNCTION(BlueprintPure, Category = "LPQ|AI|Patrol", meta = (ToolTip = "返回路线是否循环。循环路线到末尾后回到第一个采样点。"))
	bool IsLoopRoute() const;

	UFUNCTION(BlueprintPure, Category = "LPQ|AI|Patrol", meta = (ToolTip = "返回用于画路线形状的 SplineComponent。一般只用于编辑器调试，不直接驱动战斗逻辑。"))
	USplineComponent* GetPatrolSplineComponent() const;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "LPQ|AI|Patrol", meta = (ToolTip = "路线标识。敌人 AIController 的 PatrolRouteId 必须与这里一致，才会使用这条路线。"))
	FName RouteId = NAME_None;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "LPQ|AI|Patrol", meta = (ToolTip = "是否循环巡逻。开启后最后一个 movement sample 会回到第一个 sample。"))
	bool bLoopRoute = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "LPQ|AI|Patrol", meta = (ToolTip = "在编辑器中显示 spline 路线辅助线。只用于 authoring，不影响 AI 逻辑。"))
	bool bDrawDebugRoute = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "LPQ|AI|Patrol", meta = (ToolTip = "路线 debug 颜色。用于区分不同巡逻路线，不影响玩法。"))
	FLinearColor DebugColor = FLinearColor::Green;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "LPQ|AI|Patrol", meta = (ClampMin = "1.0", Units = "cm", ToolTip = "movement sample 间距，单位 cm。数值越小路线越贴合 spline，但 MoveTo 目标也会更多。"))
	float PatrolSampleDistance = 400.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "LPQ|AI|Patrol", meta = (ClampMin = "1.0", Units = "cm", ToolTip = "stop sample 间距，单位 cm。敌人只在这些稀疏停靠点等待，不会在每个 movement sample 停顿。"))
	float PatrolStopSampleDistance = 3200.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "LPQ|AI|Patrol", meta = (ClampMin = "0.0", Units = "s", ToolTip = "到达 stop sample 后等待的时间，单位秒。等待由 StateTree 的 Delay 消费。"))
	float PatrolStopHoldDuration = 1.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "LPQ|AI|Patrol", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "NavMesh 投影的水平搜索范围，单位 cm。用于把采样点吸附到可走区域。"))
	float NavProjectionExtentXY = 120.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "LPQ|AI|Patrol", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "NavMesh 投影的垂直搜索范围，单位 cm。用于容忍 spline 点放到地面上下。"))
	float NavProjectionExtentZ = 500.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "LPQ|AI|Patrol", meta = (ToolTip = "PIE 运行时绘制路线、movement samples 和 stop samples。只用于验证路线，不参与 gameplay。"))
	bool bDrawRuntimeDebugRoute = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "LPQ|AI|Patrol", meta = (ClampMin = "0.0", Units = "s", ToolTip = "运行时 debug draw 持续时间，单位秒。0 表示只画一帧，适合配合每帧重画验证。"))
	float RuntimeDebugDrawDuration = 0.0f;

private:
	struct FLPQPatrolRouteSample
	{
		FVector Location = FVector::ZeroVector;
		bool bIsStop = false;
	};

	void RebuildSampledPatrolLocations();
	void AddSampledPatrolLocation(const FVector& RawLocation, bool bIsStop);
	int32 FindNearestSampleIndexToDistance(float DistanceAlongSpline) const;
	FVector ProjectLocationToNavigation(const FVector& RawLocation) const;
	void DrawRuntimeDebugRoute() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LPQ|AI|Patrol", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LPQ|AI|Patrol", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> PatrolSplineComponent;

	TArray<FLPQPatrolRouteSample> SampledPatrolLocations;
};
