// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/TunicEnemyPatrolRoute.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"

ATunicEnemyPatrolRoute::ATunicEnemyPatrolRoute()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRootComponent;

	PatrolSplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("PatrolSpline"));
	if (PatrolSplineComponent)
	{
		PatrolSplineComponent->SetupAttachment(SceneRootComponent);
		PatrolSplineComponent->SetClosedLoop(bLoopRoute, false);
	}
}

void ATunicEnemyPatrolRoute::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (PatrolSplineComponent)
	{
		PatrolSplineComponent->SetClosedLoop(bLoopRoute, false);
		PatrolSplineComponent->SetVisibility(bDrawDebugRoute, true);
#if WITH_EDITORONLY_DATA
		PatrolSplineComponent->SetUnselectedSplineSegmentColor(DebugColor);
		PatrolSplineComponent->SetSelectedSplineSegmentColor(DebugColor);
#endif
	}
}

void ATunicEnemyPatrolRoute::BeginPlay()
{
	Super::BeginPlay();

	if (bDrawRuntimeDebugRoute)
	{
		DrawRuntimeDebugRoute();
	}
}

FName ATunicEnemyPatrolRoute::GetRouteId() const
{
	return RouteId;
}

int32 ATunicEnemyPatrolRoute::GetPatrolPointCount() const
{
	return PatrolSplineComponent ? PatrolSplineComponent->GetNumberOfSplinePoints() : 0;
}

FVector ATunicEnemyPatrolRoute::GetPatrolPointLocation(int32 PointIndex) const
{
	if (!PatrolSplineComponent)
	{
		return GetActorLocation();
	}

	const int32 PointCount = PatrolSplineComponent->GetNumberOfSplinePoints();
	if (PointCount <= 0)
	{
		return GetActorLocation();
	}

	const int32 SafePointIndex = FMath::Clamp(PointIndex, 0, PointCount - 1);
	const FVector RawLocation = PatrolSplineComponent->GetLocationAtSplinePoint(SafePointIndex, ESplineCoordinateSpace::World);
	return ProjectLocationToNavigation(RawLocation);
}

bool ATunicEnemyPatrolRoute::IsLoopRoute() const
{
	return bLoopRoute;
}

USplineComponent* ATunicEnemyPatrolRoute::GetPatrolSplineComponent() const
{
	return PatrolSplineComponent;
}

FVector ATunicEnemyPatrolRoute::ProjectLocationToNavigation(const FVector& RawLocation) const
{
	const UWorld* World = GetWorld();
	const UNavigationSystemV1* NavigationSystem = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!NavigationSystem)
	{
		return RawLocation;
	}

	const FVector ProjectionExtent(
		FMath::Max(0.0f, NavProjectionExtentXY),
		FMath::Max(0.0f, NavProjectionExtentXY),
		FMath::Max(0.0f, NavProjectionExtentZ));

	FNavLocation ProjectedLocation;
	if (NavigationSystem->ProjectPointToNavigation(RawLocation, ProjectedLocation, ProjectionExtent))
	{
		return ProjectedLocation.Location;
	}

	return RawLocation;
}

void ATunicEnemyPatrolRoute::DrawRuntimeDebugRoute() const
{
	UWorld* World = GetWorld();
	if (!World || !PatrolSplineComponent)
	{
		return;
	}

	const int32 PointCount = PatrolSplineComponent->GetNumberOfSplinePoints();
	if (PointCount <= 0)
	{
		return;
	}

	const FColor DrawColor = DebugColor.ToFColor(true);
	const bool bPersistent = RuntimeDebugDrawDuration <= 0.0f;
	const float LifeTime = bPersistent ? -1.0f : RuntimeDebugDrawDuration;
	const float SphereRadius = 24.0f;
	const float LineThickness = 3.0f;

	for (int32 PointIndex = 0; PointIndex < PointCount; ++PointIndex)
	{
		const FVector PointLocation = GetPatrolPointLocation(PointIndex);
		DrawDebugSphere(World, PointLocation, SphereRadius, 12, DrawColor, bPersistent, LifeTime, 0, LineThickness);
		DrawDebugString(World, PointLocation + FVector(0.0f, 0.0f, 48.0f), FString::Printf(TEXT("%s:%d"), *RouteId.ToString(), PointIndex), nullptr, DrawColor, LifeTime, true);

		const bool bHasNextPoint = PointIndex + 1 < PointCount;
		if (bHasNextPoint || bLoopRoute)
		{
			const int32 NextPointIndex = bHasNextPoint ? PointIndex + 1 : 0;
			const FVector NextPointLocation = GetPatrolPointLocation(NextPointIndex);
			DrawDebugLine(World, PointLocation, NextPointLocation, DrawColor, bPersistent, LifeTime, 0, LineThickness);
		}
	}
}
