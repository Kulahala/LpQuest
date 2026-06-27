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

	RebuildSampledPatrolLocations();
}

void ATunicEnemyPatrolRoute::BeginPlay()
{
	Super::BeginPlay();

	RebuildSampledPatrolLocations();

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
	return SampledPatrolLocations.Num();
}

FVector ATunicEnemyPatrolRoute::GetPatrolPointLocation(int32 PointIndex) const
{
	const int32 PointCount = SampledPatrolLocations.Num();
	if (PointCount <= 0)
	{
		return GetActorLocation();
	}

	const int32 SafePointIndex = FMath::Clamp(PointIndex, 0, PointCount - 1);
	return SampledPatrolLocations[SafePointIndex].Location;
}

bool ATunicEnemyPatrolRoute::IsPatrolPointStop(int32 PointIndex) const
{
	const int32 PointCount = SampledPatrolLocations.Num();
	if (PointCount <= 0)
	{
		return false;
	}

	const int32 SafePointIndex = FMath::Clamp(PointIndex, 0, PointCount - 1);
	return SampledPatrolLocations[SafePointIndex].bIsStop;
}

float ATunicEnemyPatrolRoute::GetPatrolStopHoldDuration(int32 PointIndex) const
{
	return IsPatrolPointStop(PointIndex) ? FMath::Max(0.0f, PatrolStopHoldDuration) : 0.0f;
}

bool ATunicEnemyPatrolRoute::IsLoopRoute() const
{
	return bLoopRoute;
}

USplineComponent* ATunicEnemyPatrolRoute::GetPatrolSplineComponent() const
{
	return PatrolSplineComponent;
}

void ATunicEnemyPatrolRoute::RebuildSampledPatrolLocations()
{
	SampledPatrolLocations.Reset();

	if (!PatrolSplineComponent)
	{
		AddSampledPatrolLocation(GetActorLocation(), true);
		return;
	}

	const int32 SplinePointCount = PatrolSplineComponent->GetNumberOfSplinePoints();
	if (SplinePointCount <= 0)
	{
		AddSampledPatrolLocation(GetActorLocation(), true);
		return;
	}

	PatrolSplineComponent->SetClosedLoop(bLoopRoute, false);

	const float SplineLength = PatrolSplineComponent->GetSplineLength();
	const float SafeSampleDistance = FMath::Max(1.0f, PatrolSampleDistance);
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		for (int32 PointIndex = 0; PointIndex < SplinePointCount; ++PointIndex)
		{
			AddSampledPatrolLocation(PatrolSplineComponent->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World), PointIndex == 0);
		}
		return;
	}

	for (float Distance = 0.0f; Distance < SplineLength; Distance += SafeSampleDistance)
	{
		AddSampledPatrolLocation(PatrolSplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World), false);
	}

	if (bLoopRoute && SampledPatrolLocations.Num() < 2)
	{
		AddSampledPatrolLocation(PatrolSplineComponent->GetLocationAtDistanceAlongSpline(SplineLength * 0.5f, ESplineCoordinateSpace::World), false);
	}

	if (!bLoopRoute)
	{
		AddSampledPatrolLocation(PatrolSplineComponent->GetLocationAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::World), false);
	}
	else if (SampledPatrolLocations.Num() <= 0)
	{
		AddSampledPatrolLocation(PatrolSplineComponent->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World), true);
	}

	if (SampledPatrolLocations.Num() > 0)
	{
		SampledPatrolLocations[0].bIsStop = true;
	}

	const float SafeStopSampleDistance = FMath::Max(1.0f, PatrolStopSampleDistance);
	for (float StopDistance = SafeStopSampleDistance; StopDistance < SplineLength; StopDistance += SafeStopSampleDistance)
	{
		const int32 NearestSampleIndex = FindNearestSampleIndexToDistance(StopDistance);
		if (SampledPatrolLocations.IsValidIndex(NearestSampleIndex))
		{
			SampledPatrolLocations[NearestSampleIndex].bIsStop = true;
		}
	}

	if (!bLoopRoute && SampledPatrolLocations.Num() > 0)
	{
		SampledPatrolLocations.Last().bIsStop = true;
	}
}

void ATunicEnemyPatrolRoute::AddSampledPatrolLocation(const FVector& RawLocation, bool bIsStop)
{
	const FVector ProjectedLocation = ProjectLocationToNavigation(RawLocation);
	if (SampledPatrolLocations.Num() > 0 && FVector::DistSquared(SampledPatrolLocations.Last().Location, ProjectedLocation) <= FMath::Square(1.0f))
	{
		SampledPatrolLocations.Last().bIsStop |= bIsStop;
		return;
	}

	FTunicPatrolRouteSample NewSample;
	NewSample.Location = ProjectedLocation;
	NewSample.bIsStop = bIsStop;
	SampledPatrolLocations.Add(NewSample);
}

int32 ATunicEnemyPatrolRoute::FindNearestSampleIndexToDistance(float DistanceAlongSpline) const
{
	if (!PatrolSplineComponent || SampledPatrolLocations.Num() <= 0)
	{
		return INDEX_NONE;
	}

	const FVector StopLocation = PatrolSplineComponent->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
	int32 BestSampleIndex = INDEX_NONE;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (int32 SampleIndex = 0; SampleIndex < SampledPatrolLocations.Num(); ++SampleIndex)
	{
		const float DistanceSquared = FVector::DistSquared2D(SampledPatrolLocations[SampleIndex].Location, StopLocation);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestSampleIndex = SampleIndex;
		}
	}

	return BestSampleIndex;
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

	const int32 SampleCount = SampledPatrolLocations.Num();
	if (SampleCount <= 0)
	{
		return;
	}

	const FColor DrawColor = DebugColor.ToFColor(true);
	const FColor MovementSampleColor = FLinearColor::Yellow.ToFColor(true);
	const FColor StopSampleColor = FLinearColor::Red.ToFColor(true);
	const bool bPersistent = RuntimeDebugDrawDuration <= 0.0f;
	const float LifeTime = bPersistent ? -1.0f : RuntimeDebugDrawDuration;
	const float SplineLineThickness = 2.0f;
	const float SampleLineThickness = 4.0f;
	const float MovementSampleSphereRadius = 22.0f;
	const float StopSampleSphereRadius = 36.0f;

	const int32 SplineDrawSteps = FMath::Max(8, FMath::CeilToInt(PatrolSplineComponent->GetSplineLength() / 150.0f));
	FVector PreviousSplineLocation = PatrolSplineComponent->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
	for (int32 StepIndex = 1; StepIndex <= SplineDrawSteps; ++StepIndex)
	{
		const float Alpha = static_cast<float>(StepIndex) / static_cast<float>(SplineDrawSteps);
		const float Distance = PatrolSplineComponent->GetSplineLength() * Alpha;
		const FVector SplineLocation = PatrolSplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		DrawDebugLine(World, PreviousSplineLocation, SplineLocation, DrawColor, bPersistent, LifeTime, 0, SplineLineThickness);
		PreviousSplineLocation = SplineLocation;
	}

	for (int32 PointIndex = 0; PointIndex < SampleCount; ++PointIndex)
	{
		const FVector PointLocation = GetPatrolPointLocation(PointIndex);
		const bool bIsStop = IsPatrolPointStop(PointIndex);
		const FColor SampleColor = bIsStop ? StopSampleColor : MovementSampleColor;
		const float SampleSphereRadius = bIsStop ? StopSampleSphereRadius : MovementSampleSphereRadius;
		DrawDebugSphere(World, PointLocation, SampleSphereRadius, 12, SampleColor, bPersistent, LifeTime, 0, SampleLineThickness);
		DrawDebugString(World, PointLocation + FVector(0.0f, 0.0f, 48.0f), FString::Printf(TEXT("%s:%s%d"), *RouteId.ToString(), bIsStop ? TEXT("Stop") : TEXT("Move"), PointIndex), nullptr, SampleColor, LifeTime, true);

		const bool bHasNextPoint = PointIndex + 1 < SampleCount;
		if (bHasNextPoint || bLoopRoute)
		{
			const int32 NextPointIndex = bHasNextPoint ? PointIndex + 1 : 0;
			const FVector NextPointLocation = GetPatrolPointLocation(NextPointIndex);
			DrawDebugLine(World, PointLocation, NextPointLocation, MovementSampleColor, bPersistent, LifeTime, 0, SampleLineThickness);
		}
	}
}
