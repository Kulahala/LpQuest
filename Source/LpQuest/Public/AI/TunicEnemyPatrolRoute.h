// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunicEnemyPatrolRoute.generated.h"

class USceneComponent;
class USplineComponent;

UCLASS(Blueprintable)
class LPQUEST_API ATunicEnemyPatrolRoute : public AActor
{
	GENERATED_BODY()

public:
	ATunicEnemyPatrolRoute();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI|Patrol")
	FName GetRouteId() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI|Patrol")
	int32 GetPatrolPointCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI|Patrol")
	FVector GetPatrolPointLocation(int32 PointIndex) const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI|Patrol")
	bool IsPatrolPointStop(int32 PointIndex) const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI|Patrol")
	float GetPatrolStopHoldDuration(int32 PointIndex) const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI|Patrol")
	bool IsLoopRoute() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI|Patrol")
	USplineComponent* GetPatrolSplineComponent() const;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tunic|AI|Patrol")
	FName RouteId = NAME_None;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tunic|AI|Patrol")
	bool bLoopRoute = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tunic|AI|Patrol")
	bool bDrawDebugRoute = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tunic|AI|Patrol")
	FLinearColor DebugColor = FLinearColor::Green;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tunic|AI|Patrol", meta = (ClampMin = "1.0", Units = "cm"))
	float PatrolSampleDistance = 400.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tunic|AI|Patrol", meta = (ClampMin = "1.0", Units = "cm"))
	float PatrolStopSampleDistance = 3200.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tunic|AI|Patrol", meta = (ClampMin = "0.0", Units = "s"))
	float PatrolStopHoldDuration = 1.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tunic|AI|Patrol", meta = (ClampMin = "0.0", Units = "cm"))
	float NavProjectionExtentXY = 120.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tunic|AI|Patrol", meta = (ClampMin = "0.0", Units = "cm"))
	float NavProjectionExtentZ = 500.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tunic|AI|Patrol")
	bool bDrawRuntimeDebugRoute = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tunic|AI|Patrol", meta = (ClampMin = "0.0", Units = "s"))
	float RuntimeDebugDrawDuration = 0.0f;

private:
	struct FTunicPatrolRouteSample
	{
		FVector Location = FVector::ZeroVector;
		bool bIsStop = false;
	};

	void RebuildSampledPatrolLocations();
	void AddSampledPatrolLocation(const FVector& RawLocation, bool bIsStop);
	int32 FindNearestSampleIndexToDistance(float DistanceAlongSpline) const;
	FVector ProjectLocationToNavigation(const FVector& RawLocation) const;
	void DrawRuntimeDebugRoute() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|AI|Patrol", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|AI|Patrol", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> PatrolSplineComponent;

	TArray<FTunicPatrolRouteSample> SampledPatrolLocations;
};
