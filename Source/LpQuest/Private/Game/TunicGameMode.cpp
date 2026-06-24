// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/TunicGameMode.h"

#include "Character/TunicPlayerCharacter.h"
#include "Controller/TunicPlayerController.h"
#include "Game/TunicGameState.h"
#include "Player/TunicPlayerState.h"

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
	if (!HasAuthority() || !TunicGameState || TunicGameState->IsPartyWiped())
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

