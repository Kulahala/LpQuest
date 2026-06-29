// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TunicGameMode.generated.h"

class ATunicPlayerState;
class ATunicPlayerCharacter;
class ATunicPortalActor;
class UGameplayEffect;

UCLASS(Blueprintable)
class LPQUEST_API ATunicGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATunicGameMode();

	virtual void BeginPlay() override;
	virtual void Logout(AController* Exiting) override;

	void EvaluatePartyWipe();
	bool TryStartPortalEvent(ATunicPlayerCharacter* InteractingPlayer, ATunicPortalActor* PortalActor);
	void MarkFloorTransitionReady();
	void HandleEnemyDeath(class ATunicEnemyCharacter* DeadEnemy);
	bool TrySelectRunUpgradeForPlayer(ATunicPlayerState* TunicPlayerState);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Run", meta = (ClampMin = "0.0", Units = "s", ToolTip = "Portal ready 后 floor transition stub 等待多久完成，单位秒。完成后楼层 +1、重置 Portal/Spawner，并回到 CombatActive。"))
	float FloorTransitionStubDelay = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Run|Rewards", meta = (ToolTip = "玩家消耗一次 pending run upgrade 时，服务器授予到 PlayerState-owned ASC 的默认升级 GameplayEffect。v1 默认 MaxHealth +20 并恢复 Health +20。"))
	TSubclassOf<UGameplayEffect> DefaultRunUpgradeGameplayEffectClass;

private:
	class ATunicEncounterSpawner* FindEncounterSpawner() const;
	void SpawnEncounterForCurrentFloor();
	void CompleteFloorTransitionStub();
	void GrantPendingRunUpgradeChoices(int32 LevelDelta);
};

