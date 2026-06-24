// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/TunicGameMode.h"

#include "Character/TunicEnemyCharacter.h"
#include "Character/TunicPlayerCharacter.h"
#include "Controller/TunicPlayerController.h"
#include "EngineUtils.h"
#include "Game/TunicEncounterSpawner.h"
#include "Game/TunicGameState.h"
#include "Game/TunicPortalActor.h"
#include "Player/TunicPlayerState.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestRunState, Log, All);

ATunicGameMode::ATunicGameMode()
{
	DefaultPawnClass = ATunicPlayerCharacter::StaticClass();
	PlayerControllerClass = ATunicPlayerController::StaticClass();
	PlayerStateClass = ATunicPlayerState::StaticClass();
	GameStateClass = ATunicGameState::StaticClass();
}

void ATunicGameMode::BeginPlay()
{
	Super::BeginPlay();

	SpawnEncounterForCurrentFloor();
}

void ATunicGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	EvaluatePartyWipe();
}

void ATunicGameMode::EvaluatePartyWipe()
{
	ATunicGameState* TunicGameState = GetGameState<ATunicGameState>();
	if (!HasAuthority() || !TunicGameState || !TunicGameState->IsCombatActive())
	{
		return;
	}

	int32 ParticipatingPlayerCount = 0;
	int32 DeadPlayerCount = 0;
	int32 UnreadyPlayerCount = 0;

	if (GameState)
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (!PlayerState)
			{
				continue;
			}

			const ATunicPlayerCharacter* PlayerCharacter = PlayerState->GetPawn<ATunicPlayerCharacter>();
			if (!PlayerCharacter)
			{
				++UnreadyPlayerCount;
				continue;
			}

			++ParticipatingPlayerCount;
			if (PlayerCharacter->IsDead())
			{
				++DeadPlayerCount;
			}
		}
	}

	const bool bAllParticipatingPlayersDead = ParticipatingPlayerCount > 0 && DeadPlayerCount == ParticipatingPlayerCount;

	UE_LOG(LogLpQuestRunState, Display, TEXT("Party wipe evaluation | ParticipatingPlayers=%d | DeadPlayers=%d | UnreadyPlayers=%d | Triggered=%s"),
		ParticipatingPlayerCount,
		DeadPlayerCount,
		UnreadyPlayerCount,
		bAllParticipatingPlayersDead ? TEXT("true") : TEXT("false"));

	if (bAllParticipatingPlayersDead)
	{
		TunicGameState->SetRunState(ETunicRunState::PartyWiped);
		UE_LOG(LogLpQuestRunState, Display, TEXT("Run state changed to PartyWiped"));
	}
}

void ATunicGameMode::EvaluateEncounterClear()
{
	ATunicGameState* TunicGameState = GetGameState<ATunicGameState>();
	if (!HasAuthority() || !TunicGameState || !TunicGameState->IsCombatActive())
	{
		return;
	}

	ATunicEncounterSpawner* EncounterSpawner = FindEncounterSpawner();
	if (!EncounterSpawner || !EncounterSpawner->HasSpawnedEncounter())
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Encounter clear evaluation skipped: no active encounter spawner | Floor=%d"),
			TunicGameState->GetCurrentFloorIndex());
		return;
	}

	int32 TotalEnemyCount = 0;
	int32 AliveEnemyCount = 0;
	const bool bEncounterCleared = EncounterSpawner->EvaluateEncounterClear(TotalEnemyCount, AliveEnemyCount);

	UE_LOG(LogLpQuestRunState, Display, TEXT("Encounter clear evaluation | Floor=%d | Spawner=%s | TotalEnemies=%d | AliveEnemies=%d | Triggered=%s"),
		TunicGameState->GetCurrentFloorIndex(),
		*GetNameSafe(EncounterSpawner),
		TotalEnemyCount,
		AliveEnemyCount,
		bEncounterCleared ? TEXT("true") : TEXT("false"));

	if (bEncounterCleared)
	{
		TunicGameState->SetRunState(ETunicRunState::EncounterCleared);
		UE_LOG(LogLpQuestRunState, Display, TEXT("Run state changed to EncounterCleared"));
	}
}

void ATunicGameMode::HandleEnemyDeath(ATunicEnemyCharacter* DeadEnemy)
{
	ATunicGameState* TunicGameState = GetGameState<ATunicGameState>();
	if (!HasAuthority() || !TunicGameState || !TunicGameState->IsCombatActive() || !DeadEnemy)
	{
		return;
	}

	ATunicEncounterSpawner* EncounterSpawner = FindEncounterSpawner();
	const bool bIsEncounterEnemy = EncounterSpawner && EncounterSpawner->IsEncounterEnemy(DeadEnemy);
	if (bIsEncounterEnemy)
	{
		const int32 ExperienceReward = DeadEnemy->GetExperienceReward();
		TunicGameState->AddSharedRunExperience(ExperienceReward, DeadEnemy);
		UE_LOG(LogLpQuestRunState, Display, TEXT("Shared XP awarded | Enemy=%s | Amount=%d | TotalSharedXP=%d"),
			*GetNameSafe(DeadEnemy),
			ExperienceReward,
			TunicGameState->GetSharedRunExperience());
	}
	else
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Shared XP skipped: enemy is not part of active encounter | Enemy=%s"),
			*GetNameSafe(DeadEnemy));
	}

	EvaluateEncounterClear();
}

void ATunicGameMode::MarkFloorTransitionReady()
{
	ATunicGameState* TunicGameState = GetGameState<ATunicGameState>();
	if (!HasAuthority() || !TunicGameState || !TunicGameState->IsEncounterCleared())
	{
		return;
	}

	TunicGameState->SetRunState(ETunicRunState::FloorTransitionReady);
	UE_LOG(LogLpQuestRunState, Display, TEXT("Run state changed to FloorTransitionReady"));

	if (FloorTransitionStubDelay <= UE_SMALL_NUMBER)
	{
		CompleteFloorTransitionStub();
		return;
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ATunicGameMode::CompleteFloorTransitionStub, FloorTransitionStubDelay, false);
}

void ATunicGameMode::CompleteFloorTransitionStub()
{
	ATunicGameState* TunicGameState = GetGameState<ATunicGameState>();
	if (!HasAuthority() || !TunicGameState || !TunicGameState->IsFloorTransitionReady())
	{
		return;
	}

	const int32 PreviousFloorIndex = TunicGameState->GetCurrentFloorIndex();
	const int32 NewFloorIndex = PreviousFloorIndex + 1;

	int32 ResetPortalCount = 0;
	for (TActorIterator<ATunicPortalActor> PortalIt(GetWorld()); PortalIt; ++PortalIt)
	{
		ATunicPortalActor* PortalActor = *PortalIt;
		if (!PortalActor)
		{
			continue;
		}

		PortalActor->ResetPortalForNextFloorStub();
		++ResetPortalCount;
	}

	int32 ResetSpawnerCount = 0;
	for (TActorIterator<ATunicEncounterSpawner> SpawnerIt(GetWorld()); SpawnerIt; ++SpawnerIt)
	{
		ATunicEncounterSpawner* EncounterSpawner = *SpawnerIt;
		if (!EncounterSpawner)
		{
			continue;
		}

		EncounterSpawner->ResetEncounterForNextFloor();
		++ResetSpawnerCount;
	}

	TunicGameState->SetCurrentFloorIndex(NewFloorIndex);
	TunicGameState->SetRunState(ETunicRunState::CombatActive);
	SpawnEncounterForCurrentFloor();

	UE_LOG(LogLpQuestRunState, Display, TEXT("Floor transition stub completed | PreviousFloor=%d | NewFloor=%d | ResetPortals=%d | ResetSpawners=%d"),
		PreviousFloorIndex,
		NewFloorIndex,
		ResetPortalCount,
		ResetSpawnerCount);
}

ATunicEncounterSpawner* ATunicGameMode::FindEncounterSpawner() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ATunicEncounterSpawner> SpawnerIt(World); SpawnerIt; ++SpawnerIt)
	{
		ATunicEncounterSpawner* EncounterSpawner = *SpawnerIt;
		if (EncounterSpawner)
		{
			return EncounterSpawner;
		}
	}

	return nullptr;
}

void ATunicGameMode::SpawnEncounterForCurrentFloor()
{
	if (!HasAuthority())
	{
		return;
	}

	ATunicGameState* TunicGameState = GetGameState<ATunicGameState>();
	if (!TunicGameState || !TunicGameState->IsCombatActive())
	{
		return;
	}

	ATunicEncounterSpawner* EncounterSpawner = FindEncounterSpawner();
	if (!EncounterSpawner)
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Encounter spawn skipped: no encounter spawner | Floor=%d"),
			TunicGameState->GetCurrentFloorIndex());
		return;
	}

	EncounterSpawner->SpawnEncounterForFloor(TunicGameState->GetCurrentFloorIndex());
}

