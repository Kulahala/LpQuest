// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TunicGameMode.generated.h"

UCLASS(Blueprintable)
class LPQUEST_API ATunicGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATunicGameMode();

	virtual void BeginPlay() override;
	virtual void Logout(AController* Exiting) override;

	void EvaluatePartyWipe();
	void EvaluateEncounterClear();
	void MarkFloorTransitionReady();
	void HandleEnemyDeath(class ATunicEnemyCharacter* DeadEnemy);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Run", meta = (ClampMin = "0.0", Units = "s", ToolTip = "Portal ready 后 floor transition stub 等待多久完成，单位秒。完成后楼层 +1、重置 Portal/Spawner，并回到 CombatActive。"))
	float FloorTransitionStubDelay = 2.0f;

private:
	class ATunicEncounterSpawner* FindEncounterSpawner() const;
	void SpawnEncounterForCurrentFloor();
	void CompleteFloorTransitionStub();
	void GrantPendingRunUpgradeChoices(int32 LevelDelta);
};

