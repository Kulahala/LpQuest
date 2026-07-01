// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LPQGameMode.generated.h"

class ALPQPlayerState;
class ALPQPlayerCharacter;
class ALPQPortalActor;
class ALPQEnemyCharacter;
class UGameplayEffect;

UCLASS(Blueprintable)
class LPQUEST_API ALPQGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALPQGameMode();

	virtual void BeginPlay() override;
	virtual void Logout(AController* Exiting) override;

	void EvaluatePartyWipe();
	bool TryStartPortalEvent(ALPQPlayerCharacter* InteractingPlayer, ALPQPortalActor* PortalActor);
	bool TryUseDirectFloorExitPortal(ALPQPlayerCharacter* InteractingPlayer, ALPQPortalActor* PortalActor);
	bool IsActivePortalEventOwner(const ALPQPortalActor* PortalActor) const;
	bool MarkFloorTransitionReady(FName DestinationId);
	void HandleEnemyDeath(ALPQEnemyCharacter* DeadEnemy);
	bool TrySelectRunUpgradeForPlayer(ALPQPlayerState* LpQuestPlayerState);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LPQ|Run", meta = (ClampMin = "0.0", Units = "s", ToolTip = "Portal ready 后 floor transition stub 等待多久完成，单位秒。完成后楼层 +1、重置 Portal 和 floor-wave spawn source，并回到 CombatActive。"))
	float FloorTransitionStubDelay = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LPQ|Run|Rewards", meta = (ToolTip = "玩家消耗一次 pending run upgrade 时，服务器授予到 PlayerState-owned ASC 的默认升级 GameplayEffect。v1 默认 MaxHealth +20 并恢复 Health +20。"))
	TSubclassOf<UGameplayEffect> DefaultRunUpgradeGameplayEffectClass;

private:
	void SpawnFloorWavesForCurrentFloor();
	void CompleteFloorTransitionStub();
	void GrantPendingRunUpgradeChoices(int32 LevelDelta);
	void SpawnEnemyDropPickup(ALPQEnemyCharacter* DeadEnemy);
	bool CanUsePortalForFloorTransition(ALPQPlayerCharacter* InteractingPlayer, ALPQPortalActor* PortalActor, bool bRequireAllLivingPlayersInPortalRadius);

	FName PendingFloorDestinationId = TEXT("Next");
	TWeakObjectPtr<ALPQPortalActor> ActivePortalEventOwner;
};

