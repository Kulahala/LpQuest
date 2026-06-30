// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/TunicEnemySpawnSource.h"

#include "Character/TunicEnemyCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "NavigationSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestSpawnSource, Log, All);

ATunicEnemySpawnSource::ATunicEnemySpawnSource()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	RadiusPreview = CreateDefaultSubobject<USphereComponent>(TEXT("RadiusPreview"));
	RadiusPreview->SetupAttachment(SceneRoot);
	RadiusPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RadiusPreview->SetSphereRadius(SpawnRadius);
}

void ATunicEnemySpawnSource::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdateRadiusPreview();
}

int32 ATunicEnemySpawnSource::GetSpawnedEnemyCount() const
{
	int32 SpawnedEnemyCount = 0;
	for (const TWeakObjectPtr<ATunicEnemyCharacter>& EnemyPtr : SpawnedEnemies)
	{
		if (EnemyPtr.IsValid())
		{
			++SpawnedEnemyCount;
		}
	}

	return SpawnedEnemyCount;
}

int32 ATunicEnemySpawnSource::GetAliveSpawnedEnemyCount() const
{
	int32 AliveEnemyCount = 0;
	for (const TWeakObjectPtr<ATunicEnemyCharacter>& EnemyPtr : SpawnedEnemies)
	{
		const ATunicEnemyCharacter* EnemyCharacter = EnemyPtr.Get();
		if (EnemyCharacter && !EnemyCharacter->IsDead())
		{
			++AliveEnemyCount;
		}
	}

	return AliveEnemyCount;
}

bool ATunicEnemySpawnSource::OwnsSpawnedEnemy(const ATunicEnemyCharacter* EnemyCharacter) const
{
	if (!EnemyCharacter)
	{
		return false;
	}

	for (const TWeakObjectPtr<ATunicEnemyCharacter>& EnemyPtr : SpawnedEnemies)
	{
		if (EnemyPtr.Get() == EnemyCharacter)
		{
			return true;
		}
	}

	return false;
}

int32 ATunicEnemySpawnSource::SpawnEnemies()
{
	if (!HasAuthority())
	{
		return 0;
	}

	if (!EnemyClass)
	{
		if (Count > 0 && bLogSpawnSourceState)
		{
			UE_LOG(LogLpQuestSpawnSource, Warning, TEXT("Enemy spawn skipped: no enemy class configured | Source=%s | Count=%d"),
				*GetNameSafe(this),
				Count);
		}
		return 0;
	}

	if (Count <= 0)
	{
		return 0;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	int32 SpawnedEnemyCount = 0;
	for (int32 EnemyIndex = 0; EnemyIndex < Count; ++EnemyIndex)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ATunicEnemyCharacter* SpawnedEnemy = World->SpawnActor<ATunicEnemyCharacter>(EnemyClass, ResolveSpawnTransform(), SpawnParameters);
		if (!SpawnedEnemy)
		{
			if (bLogSpawnSourceState)
			{
				UE_LOG(LogLpQuestSpawnSource, Warning, TEXT("Enemy spawn failed | Source=%s | EnemyClass=%s"),
					*GetNameSafe(this),
					*GetNameSafe(EnemyClass.Get()));
			}
			continue;
		}

		SpawnedEnemies.Add(SpawnedEnemy);
		++SpawnedEnemyCount;
	}

	if (bLogSpawnSourceState)
	{
		UE_LOG(LogLpQuestSpawnSource, Display, TEXT("Enemy spawn source completed | Source=%s | EnemyClass=%s | Requested=%d | Spawned=%d | Radius=%.1f"),
			*GetNameSafe(this),
			*GetNameSafe(EnemyClass.Get()),
			Count,
			SpawnedEnemyCount,
			SpawnRadius);
	}

	return SpawnedEnemyCount;
}

void ATunicEnemySpawnSource::CleanupSpawnedEnemies()
{
	for (const TWeakObjectPtr<ATunicEnemyCharacter>& EnemyPtr : SpawnedEnemies)
	{
		if (ATunicEnemyCharacter* EnemyCharacter = EnemyPtr.Get())
		{
			EnemyCharacter->Destroy();
		}
	}

	SpawnedEnemies.Reset();
}

FTransform ATunicEnemySpawnSource::ResolveSpawnTransform() const
{
	FTransform SpawnTransform = GetActorTransform();
	if (SpawnRadius <= UE_SMALL_NUMBER)
	{
		return SpawnTransform;
	}

	const auto ApplyRandomOffset = [this, &SpawnTransform](const FVector& RandomLocation)
	{
		SpawnTransform.SetLocation(RandomLocation);
		return SpawnTransform;
	};

	if (bProjectRandomSpawnToNavigation)
	{
		if (UWorld* World = GetWorld())
		{
			if (UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
			{
				constexpr int32 MaxProjectionAttempts = 8;
				for (int32 AttemptIndex = 0; AttemptIndex < MaxProjectionAttempts; ++AttemptIndex)
				{
					const FVector2D RandomOffset2D = FMath::RandPointInCircle(SpawnRadius);
					const FVector RandomLocation = GetActorLocation() + FVector(RandomOffset2D.X, RandomOffset2D.Y, 0.0f);

					FNavLocation ProjectedLocation;
					if (NavigationSystem->ProjectPointToNavigation(RandomLocation, ProjectedLocation, NavProjectionExtent))
					{
						SpawnTransform.SetLocation(ProjectedLocation.Location);
						return SpawnTransform;
					}
				}
			}
		}

		if (bLogSpawnSourceState)
		{
			UE_LOG(LogLpQuestSpawnSource, Warning, TEXT("Enemy spawn source NavMesh projection failed; using original transform | Source=%s | Radius=%.1f"),
				*GetNameSafe(this),
				SpawnRadius);
		}
		return SpawnTransform;
	}

	const FVector2D RandomOffset2D = FMath::RandPointInCircle(SpawnRadius);
	return ApplyRandomOffset(GetActorLocation() + FVector(RandomOffset2D.X, RandomOffset2D.Y, 0.0f));
}

void ATunicEnemySpawnSource::UpdateRadiusPreview()
{
	if (!RadiusPreview)
	{
		return;
	}

	RadiusPreview->SetSphereRadius(FMath::Max(0.0f, SpawnRadius));
	RadiusPreview->SetVisibility(SpawnRadius > UE_SMALL_NUMBER);
}

int32 ATunicFloorWaveEnemySpawnSource::GetCurrentSpawnFloorIndex() const
{
	return CurrentSpawnFloorIndex;
}

int32 ATunicFloorWaveEnemySpawnSource::SpawnForFloor(int32 FloorIndex)
{
	if (!HasAuthority())
	{
		return 0;
	}

	const int32 NormalizedFloorIndex = FMath::Max(1, FloorIndex);
	if (CurrentSpawnFloorIndex == NormalizedFloorIndex && GetSpawnedEnemyCount() > 0)
	{
		return 0;
	}

	CleanupSpawnedEnemies();
	CurrentSpawnFloorIndex = NormalizedFloorIndex;
	return SpawnEnemies();
}

void ATunicFloorWaveEnemySpawnSource::ResetForNextFloor()
{
	if (!HasAuthority())
	{
		return;
	}

	CleanupSpawnedEnemies();
	CurrentSpawnFloorIndex = 0;
}
