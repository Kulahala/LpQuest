// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunicEncounterSpawner.generated.h"

class ATunicEnemyCharacter;

USTRUCT(BlueprintType)
struct LPQUEST_API FTunicEncounterSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Spawner")
	TSubclassOf<ATunicEnemyCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Spawner", meta = (ClampMin = "0"))
	int32 Count = 1;
};

UCLASS(Blueprintable)
class LPQUEST_API ATunicEncounterSpawner : public AActor
{
	GENERATED_BODY()

public:
	ATunicEncounterSpawner();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner")
	bool HasSpawnedEncounter() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner")
	int32 GetSpawnedEncounterEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner")
	int32 GetAliveEncounterEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Spawner")
	int32 GetCurrentEncounterFloorIndex() const;

	void SpawnEncounterForFloor(int32 FloorIndex);
	void ResetEncounterForNextFloor();
	bool EvaluateEncounterClear(int32& OutTotalEnemyCount, int32& OutAliveEnemyCount);
	bool IsEncounterEnemy(const ATunicEnemyCharacter* EnemyCharacter) const;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tunic|Spawner")
	TArray<TObjectPtr<AActor>> SpawnPoints;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tunic|Spawner")
	TArray<FTunicEncounterSpawnEntry> SpawnEntries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bLogSpawnerState = true;

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Spawner")
	void OnEncounterSpawned(int32 FloorIndex);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Spawner")
	void OnEncounterCleared(int32 FloorIndex);

private:
	void CleanupSpawnedEnemies();

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ATunicEnemyCharacter>> SpawnedEncounterEnemies;

	int32 CurrentEncounterFloorIndex = 0;
	bool bHasSpawnedEncounter = false;
};
