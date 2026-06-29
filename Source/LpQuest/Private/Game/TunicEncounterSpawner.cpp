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

bool ATunicEncounterSpawner::HasActiveEncounter() const
{
	return GetTotalEncounterEnemyCount() > 0;
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

int32 ATunicEncounterSpawner::GetPlacedEncounterEnemyCount() const
{
	int32 PlacedEnemyCount = 0;
	for (const TWeakObjectPtr<ATunicEnemyCharacter>& EnemyPtr : ActivePlacedEncounterEnemies)
	{
		if (EnemyPtr.IsValid())
		{
			++PlacedEnemyCount;
		}
	}

	return PlacedEnemyCount;
}

int32 ATunicEncounterSpawner::GetTotalEncounterEnemyCount() const
{
	return GetSpawnedEncounterEnemyCount() + GetPlacedEncounterEnemyCount();
}

int32 ATunicEncounterSpawner::GetAliveEncounterEnemyCount() const
{
	int32 AliveEnemyCount = 0;
	const auto CountAliveEnemy = [&AliveEnemyCount](const ATunicEnemyCharacter* EnemyCharacter)
	{
		if (EnemyCharacter && !EnemyCharacter->IsDead())
		{
			++AliveEnemyCount;
		}
	};

	for (const TWeakObjectPtr<ATunicEnemyCharacter>& EnemyPtr : SpawnedEncounterEnemies)
	{
		CountAliveEnemy(EnemyPtr.Get());
	}

	for (const TWeakObjectPtr<ATunicEnemyCharacter>& EnemyPtr : ActivePlacedEncounterEnemies)
	{
		CountAliveEnemy(EnemyPtr.Get());
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

	if (HasActiveEncounter() && CurrentEncounterFloorIndex == FloorIndex)
	{
		return;
	}

	CleanupSpawnedEnemies();
	RefreshActivePlacedEncounterEnemies();

	CurrentEncounterFloorIndex = FMath::Max(1, FloorIndex);

	const int32 SpawnPointCount = SpawnPoints.Num();
	int32 SpawnedEnemyCount = 0;
	int32 SpawnRequestIndex = 0;

	if (SpawnPointCount <= 0 && SpawnEntries.Num() > 0)
	{
		UE_LOG(LogLpQuestSpawner, Warning, TEXT("Encounter spawn skipped: no spawn points | Spawner=%s | Floor=%d"),
			*GetNameSafe(this),
			CurrentEncounterFloorIndex);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (SpawnPointCount > 0)
	{
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
	}

	bHasSpawnedEncounter = SpawnedEnemyCount > 0;

	if (bLogSpawnerState)
	{
		UE_LOG(LogLpQuestSpawner, Display, TEXT("Encounter activated | Spawner=%s | Floor=%d | SpawnPoints=%d | SpawnedEnemies=%d | PlacedEnemies=%d | TotalEnemies=%d"),
			*GetNameSafe(this),
			CurrentEncounterFloorIndex,
			SpawnPointCount,
			SpawnedEnemyCount,
			GetPlacedEncounterEnemyCount(),
			GetTotalEncounterEnemyCount());
	}

	if (HasActiveEncounter())
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
	ActivePlacedEncounterEnemies.Reset();
	bHasSpawnedEncounter = false;
	CurrentEncounterFloorIndex = 0;
}

bool ATunicEncounterSpawner::IsEncounterEnemy(const ATunicEnemyCharacter* EnemyCharacter) const
{
	if (!EnemyCharacter)
	{
		return false;
	}

	for (const TWeakObjectPtr<ATunicEnemyCharacter>& EnemyPtr : SpawnedEncounterEnemies)
	{
		if (EnemyPtr.Get() == EnemyCharacter)
		{
			return true;
		}
	}

	for (const TWeakObjectPtr<ATunicEnemyCharacter>& EnemyPtr : ActivePlacedEncounterEnemies)
	{
		if (EnemyPtr.Get() == EnemyCharacter)
		{
			return true;
		}
	}

	return false;
}

void ATunicEncounterSpawner::OnEncounterSpawned_Implementation(int32)
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

void ATunicEncounterSpawner::RefreshActivePlacedEncounterEnemies()
{
	ActivePlacedEncounterEnemies.Reset();

	for (const TObjectPtr<ATunicEnemyCharacter>& EnemyPtr : PlacedEncounterEnemies)
	{
		ATunicEnemyCharacter* EnemyCharacter = EnemyPtr.Get();
		if (!EnemyCharacter || EnemyCharacter->IsDead())
		{
			continue;
		}

		bool bAlreadyRegistered = false;
		for (const TWeakObjectPtr<ATunicEnemyCharacter>& ActiveEnemyPtr : ActivePlacedEncounterEnemies)
		{
			if (ActiveEnemyPtr.Get() == EnemyCharacter)
			{
				bAlreadyRegistered = true;
				break;
			}
		}

		if (!bAlreadyRegistered)
		{
			ActivePlacedEncounterEnemies.Add(EnemyCharacter);
		}
	}
}
