// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/TunicEncounterSpawner.h"

#include "Character/TunicEnemyCharacter.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestSpawner, Log, All);

ATunicEncounterSpawner::ATunicEncounterSpawner()
{
	bReplicates = false;
	PrimaryActorTick.bCanEverTick = false;
}

void ATunicEncounterSpawner::BeginPlay()
{
	Super::BeginPlay();
}

bool ATunicEncounterSpawner::HasSpawnedEncounter() const
{
	return bHasSpawnedEncounter;
}

int32 ATunicEncounterSpawner::GetSpawnedEncounterEnemyCount() const
{
	int32 SpawnedEnemyCount = 0;
	for (const TWeakObjectPtr<ATunicEnemyCharacter>& EnemyPtr : SpawnedEncounterEnemies)
	{
		if (EnemyPtr.IsValid())
		{
			++SpawnedEnemyCount;
		}
	}

	return SpawnedEnemyCount;
}

int32 ATunicEncounterSpawner::GetAliveEncounterEnemyCount() const
{
	int32 AliveEnemyCount = 0;
	for (const TWeakObjectPtr<ATunicEnemyCharacter>& EnemyPtr : SpawnedEncounterEnemies)
	{
		const ATunicEnemyCharacter* EnemyCharacter = EnemyPtr.Get();
		if (EnemyCharacter && !EnemyCharacter->IsDead())
		{
			++AliveEnemyCount;
		}
	}

	return AliveEnemyCount;
}

int32 ATunicEncounterSpawner::GetCurrentEncounterFloorIndex() const
{
	return CurrentEncounterFloorIndex;
}

void ATunicEncounterSpawner::SpawnEncounterForFloor(int32 FloorIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bHasSpawnedEncounter && CurrentEncounterFloorIndex == FloorIndex)
	{
		return;
	}

	CleanupSpawnedEnemies();

	CurrentEncounterFloorIndex = FMath::Max(1, FloorIndex);

	const int32 SpawnPointCount = SpawnPoints.Num();
	int32 SpawnedEnemyCount = 0;
	int32 SpawnRequestIndex = 0;

	if (SpawnPointCount <= 0)
	{
		UE_LOG(LogLpQuestSpawner, Warning, TEXT("Encounter spawn skipped: no spawn points | Spawner=%s | Floor=%d"),
			*GetNameSafe(this),
			CurrentEncounterFloorIndex);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const FTunicEncounterSpawnEntry& SpawnEntry : SpawnEntries)
	{
		if (!SpawnEntry.EnemyClass || SpawnEntry.Count <= 0)
		{
			continue;
		}

		for (int32 EnemyIndex = 0; EnemyIndex < SpawnEntry.Count; ++EnemyIndex)
		{
			AActor* SpawnPoint = SpawnPoints[SpawnRequestIndex % SpawnPointCount].Get();
			++SpawnRequestIndex;

			if (!SpawnPoint)
			{
				continue;
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = this;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			ATunicEnemyCharacter* SpawnedEnemy = World->SpawnActor<ATunicEnemyCharacter>(SpawnEntry.EnemyClass, SpawnPoint->GetActorTransform(), SpawnParameters);
			if (!SpawnedEnemy)
			{
				continue;
			}

			SpawnedEncounterEnemies.Add(SpawnedEnemy);
			++SpawnedEnemyCount;
		}
	}

	bHasSpawnedEncounter = SpawnedEnemyCount > 0;

	if (bLogSpawnerState)
	{
		UE_LOG(LogLpQuestSpawner, Display, TEXT("Encounter spawned | Spawner=%s | Floor=%d | SpawnPoints=%d | SpawnedEnemies=%d"),
			*GetNameSafe(this),
			CurrentEncounterFloorIndex,
			SpawnPointCount,
			SpawnedEnemyCount);
	}

	if (bHasSpawnedEncounter)
	{
		OnEncounterSpawned(CurrentEncounterFloorIndex);
	}
}

void ATunicEncounterSpawner::ResetEncounterForNextFloor()
{
	if (!HasAuthority())
	{
		return;
	}

	CleanupSpawnedEnemies();
	bHasSpawnedEncounter = false;
	CurrentEncounterFloorIndex = 0;
}

bool ATunicEncounterSpawner::EvaluateEncounterClear(int32& OutTotalEnemyCount, int32& OutAliveEnemyCount)
{
	OutTotalEnemyCount = GetSpawnedEncounterEnemyCount();
	OutAliveEnemyCount = GetAliveEncounterEnemyCount();

	const bool bEncounterCleared = bHasSpawnedEncounter && OutTotalEnemyCount > 0 && OutAliveEnemyCount == 0;
	if (bLogSpawnerState)
	{
		UE_LOG(LogLpQuestSpawner, Display, TEXT("Spawner encounter clear evaluation | Spawner=%s | Floor=%d | TotalEnemies=%d | AliveEnemies=%d | Triggered=%s"),
			*GetNameSafe(this),
			CurrentEncounterFloorIndex,
			OutTotalEnemyCount,
			OutAliveEnemyCount,
			bEncounterCleared ? TEXT("true") : TEXT("false"));
	}

	if (bEncounterCleared)
	{
		OnEncounterCleared(CurrentEncounterFloorIndex);
	}

	return bEncounterCleared;
}

void ATunicEncounterSpawner::OnEncounterSpawned_Implementation(int32)
{
}

void ATunicEncounterSpawner::OnEncounterCleared_Implementation(int32)
{
}

void ATunicEncounterSpawner::CleanupSpawnedEnemies()
{
	for (const TWeakObjectPtr<ATunicEnemyCharacter>& EnemyPtr : SpawnedEncounterEnemies)
	{
		if (ATunicEnemyCharacter* EnemyCharacter = EnemyPtr.Get())
		{
			EnemyCharacter->Destroy();
		}
	}

	SpawnedEncounterEnemies.Reset();
}
