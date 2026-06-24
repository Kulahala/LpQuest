// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/TunicGameMode.h"

#include "Character/TunicEnemyCharacter.h"
#include "Character/TunicPlayerCharacter.h"
#include "Controller/TunicPlayerController.h"
#include "EngineUtils.h"
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

	const int32 CurrentFloorIndex = TunicGameState->GetCurrentFloorIndex();
	if (EvaluatedFloorIndex != CurrentFloorIndex)
	{
		EvaluatedFloorIndex = CurrentFloorIndex;
		bHasSeenLivingEnemyThisFloor = false;
	}

	int32 TotalEnemyCount = 0;
	int32 AliveEnemyCount = 0;

	for (TActorIterator<ATunicEnemyCharacter> EnemyIt(GetWorld()); EnemyIt; ++EnemyIt)
	{
		const ATunicEnemyCharacter* EnemyCharacter = *EnemyIt;
		if (!EnemyCharacter)
		{
			continue;
		}

		++TotalEnemyCount;
		if (!EnemyCharacter->IsDead())
		{
			++AliveEnemyCount;
		}
	}

	if (AliveEnemyCount > 0)
	{
		bHasSeenLivingEnemyThisFloor = true;
	}

	const bool bEncounterCleared = bHasSeenLivingEnemyThisFloor && TotalEnemyCount > 0 && AliveEnemyCount == 0;

	UE_LOG(LogLpQuestRunState, Display, TEXT("Encounter clear evaluation | Floor=%d | TotalEnemies=%d | AliveEnemies=%d | SeenLivingEnemyThisFloor=%s | Triggered=%s"),
		CurrentFloorIndex,
		TotalEnemyCount,
		AliveEnemyCount,
		bHasSeenLivingEnemyThisFloor ? TEXT("true") : TEXT("false"),
		bEncounterCleared ? TEXT("true") : TEXT("false"));

	if (bEncounterCleared)
	{
		TunicGameState->SetRunState(ETunicRunState::EncounterCleared);
		UE_LOG(LogLpQuestRunState, Display, TEXT("Run state changed to EncounterCleared"));
	}
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

	TunicGameState->SetCurrentFloorIndex(NewFloorIndex);
	TunicGameState->SetRunState(ETunicRunState::CombatActive);

	UE_LOG(LogLpQuestRunState, Display, TEXT("Floor transition stub completed | PreviousFloor=%d | NewFloor=%d | ResetPortals=%d"),
		PreviousFloorIndex,
		NewFloorIndex,
		ResetPortalCount);
}

