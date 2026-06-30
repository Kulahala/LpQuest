// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/TunicEncounterSpawner.h"

#include "Character/TunicEnemyCharacter.h"

ATunicEncounterSpawner::ATunicEncounterSpawner()
{
	bReplicates = false;
	PrimaryActorTick.bCanEverTick = false;
}

void ATunicEncounterSpawner::BeginPlay()
{
	Super::BeginPlay();
}

bool ATunicEncounterSpawner::HasActiveEncounter() const
{
	return GetTotalEncounterEnemyCount() > 0;
}

int32 ATunicEncounterSpawner::GetPlacedEncounterEnemyCount() const
{
	int32 PlacedEnemyCount = 0;
	for (const TObjectPtr<ATunicEnemyCharacter>& EnemyPtr : PlacedEncounterEnemies)
	{
		const ATunicEnemyCharacter* EnemyCharacter = EnemyPtr.Get();
		if (EnemyCharacter && !EnemyCharacter->IsDead())
		{
			++PlacedEnemyCount;
		}
	}

	return PlacedEnemyCount;
}

int32 ATunicEncounterSpawner::GetTotalEncounterEnemyCount() const
{
	return GetPlacedEncounterEnemyCount();
}

int32 ATunicEncounterSpawner::GetAliveEncounterEnemyCount() const
{
	int32 AliveEnemyCount = 0;
	for (const TObjectPtr<ATunicEnemyCharacter>& EnemyPtr : PlacedEncounterEnemies)
	{
		const ATunicEnemyCharacter* EnemyCharacter = EnemyPtr.Get();
		if (EnemyCharacter && !EnemyCharacter->IsDead())
		{
			++AliveEnemyCount;
		}
	}

	return AliveEnemyCount;
}

bool ATunicEncounterSpawner::IsEncounterEnemy(const ATunicEnemyCharacter* EnemyCharacter) const
{
	if (!EnemyCharacter)
	{
		return false;
	}

	for (const TObjectPtr<ATunicEnemyCharacter>& EnemyPtr : PlacedEncounterEnemies)
	{
		if (EnemyPtr.Get() == EnemyCharacter)
		{
			return true;
		}
	}

	return false;
}
