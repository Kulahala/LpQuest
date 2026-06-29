// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/TunicGameMode.h"

#include "AbilitySystemComponent.h"
#include "Ability/TunicRunUpgradeMaxHealthGameplayEffect.h"
#include "Character/TunicEnemyCharacter.h"
#include "Character/TunicPlayerCharacter.h"
#include "Controller/TunicPlayerController.h"
#include "EngineUtils.h"
#include "Game/TunicEncounterSpawner.h"
#include "Game/TunicGameState.h"
#include "Game/TunicPortalActor.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Player/TunicPlayerState.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestRunState, Log, All);

namespace
{
struct FTunicResolvedExperienceReward
{
	int32 Amount = 0;
	const TCHAR* Source = TEXT("none");
	bool bIsEncounterEnemy = false;
};

FTunicResolvedExperienceReward ResolveEnemyExperienceReward(UWorld* World, ATunicEncounterSpawner* EncounterSpawner, ATunicEnemyCharacter* DeadEnemy)
{
	FTunicResolvedExperienceReward Result;
	if (!World || !DeadEnemy)
	{
		return Result;
	}

	Result.bIsEncounterEnemy = EncounterSpawner && EncounterSpawner->IsEncounterEnemy(DeadEnemy);
	if (Result.bIsEncounterEnemy)
	{
		Result.Amount = DeadEnemy->GetExperienceReward();
		Result.Source = TEXT("encounter");
		return Result;
	}

	for (TActorIterator<ATunicPortalActor> PortalIt(World); PortalIt; ++PortalIt)
	{
		ATunicPortalActor* PortalActor = *PortalIt;
		if (!PortalActor || !PortalActor->IsPortalActive())
		{
			continue;
		}

		Result.Amount = PortalActor->ConsumePortalPressureExperienceReward(DeadEnemy);
		if (Result.Amount > 0)
		{
			Result.Source = TEXT("portal pressure");
			return Result;
		}

		if (PortalActor->OwnsPortalBossEnemy(DeadEnemy))
		{
			Result.Amount = DeadEnemy->GetExperienceReward();
			Result.Source = TEXT("portal boss");
			return Result;
		}
	}

	return Result;
}
}

ATunicGameMode::ATunicGameMode()
{
	DefaultPawnClass = ATunicPlayerCharacter::StaticClass();
	PlayerControllerClass = ATunicPlayerController::StaticClass();
	PlayerStateClass = ATunicPlayerState::StaticClass();
	GameStateClass = ATunicGameState::StaticClass();
	DefaultRunUpgradeGameplayEffectClass = UTunicRunUpgradeMaxHealthGameplayEffect::StaticClass();
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
	if (!HasAuthority() || !TunicGameState || TunicGameState->GetRunState() != ETunicRunState::CombatActive)
	{
		return;
	}

	ATunicEncounterSpawner* EncounterSpawner = FindEncounterSpawner();
	if (!EncounterSpawner || !EncounterSpawner->HasActiveEncounter())
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
}

void ATunicGameMode::HandleEnemyDeath(ATunicEnemyCharacter* DeadEnemy)
{
	ATunicGameState* TunicGameState = GetGameState<ATunicGameState>();
	if (!HasAuthority() || !TunicGameState || !TunicGameState->IsCombatActive() || !DeadEnemy)
	{
		return;
	}

	ATunicEncounterSpawner* EncounterSpawner = FindEncounterSpawner();
	const FTunicResolvedExperienceReward ResolvedReward = ResolveEnemyExperienceReward(GetWorld(), EncounterSpawner, DeadEnemy);

	if (ResolvedReward.Amount > 0)
	{
		const int32 OldSharedRunLevel = TunicGameState->GetSharedRunLevel();
		TunicGameState->AddSharedRunExperience(ResolvedReward.Amount, DeadEnemy);
		const int32 NewSharedRunLevel = TunicGameState->GetSharedRunLevel();
		if (NewSharedRunLevel > OldSharedRunLevel)
		{
			GrantPendingRunUpgradeChoices(NewSharedRunLevel - OldSharedRunLevel);
		}

		UE_LOG(LogLpQuestRunState, Display, TEXT("Shared XP awarded | Enemy=%s | Amount=%d | Source=%s | TotalSharedXP=%d"),
			*GetNameSafe(DeadEnemy),
			ResolvedReward.Amount,
			ResolvedReward.Source,
			TunicGameState->GetSharedRunExperience());
	}
	else if (!ResolvedReward.bIsEncounterEnemy)
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Shared XP skipped: enemy has no active reward source | Enemy=%s"),
			*GetNameSafe(DeadEnemy));
	}

	// Encounter clear is now a legacy/debug query; ordinary enemy death should not stop remaining AI.
}

bool ATunicGameMode::TrySelectRunUpgradeForPlayer(ATunicPlayerState* TunicPlayerState)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (!TunicPlayerState)
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Run upgrade selection rejected: missing player state"));
		return false;
	}

	UAbilitySystemComponent* AbilitySystemComponent = TunicPlayerState->GetAbilitySystemComponent();
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Run upgrade selection rejected: missing ASC | PlayerState=%s"),
			*GetNameSafe(TunicPlayerState));
		return false;
	}

	if (!DefaultRunUpgradeGameplayEffectClass)
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Run upgrade selection rejected: missing default upgrade GE | PlayerState=%s"),
			*GetNameSafe(TunicPlayerState));
		return false;
	}

	if (TunicPlayerState->GetPendingRunUpgradeChoices() <= 0)
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Run upgrade selection rejected: no pending choices | PlayerState=%s"),
			*GetNameSafe(TunicPlayerState));
		return false;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(TunicPlayerState);

	FGameplayEffectSpecHandle UpgradeSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultRunUpgradeGameplayEffectClass, 1.0f, EffectContext);
	if (!UpgradeSpecHandle.IsValid())
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Run upgrade selection rejected: failed to create GE spec | PlayerState=%s | EffectClass=%s"),
			*GetNameSafe(TunicPlayerState),
			*GetNameSafe(DefaultRunUpgradeGameplayEffectClass.Get()));
		return false;
	}

	if (!TunicPlayerState->TryConsumePendingRunUpgradeChoice())
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Run upgrade selection rejected: pending choice consume failed | PlayerState=%s"),
			*GetNameSafe(TunicPlayerState));
		return false;
	}

	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*UpgradeSpecHandle.Data.Get());

	UE_LOG(LogLpQuestRunState, Display, TEXT("Run upgrade selected | PlayerState=%s | EffectClass=%s | RemainingPending=%d"),
		*GetNameSafe(TunicPlayerState),
		*GetNameSafe(DefaultRunUpgradeGameplayEffectClass.Get()),
		TunicPlayerState->GetPendingRunUpgradeChoices());

	return true;
}

bool ATunicGameMode::TryStartPortalEvent(ATunicPlayerCharacter* InteractingPlayer, ATunicPortalActor* PortalActor)
{
	ATunicGameState* TunicGameState = GetGameState<ATunicGameState>();
	if (!HasAuthority() || !TunicGameState || !InteractingPlayer || !PortalActor)
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Portal event start rejected: missing authority, game state, player, or portal | Player=%s | Portal=%s"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor));
		return false;
	}

	if (InteractingPlayer->IsDead())
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Portal event start rejected: dead player | Player=%s | Portal=%s"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor));
		return false;
	}

	if (TunicGameState->IsPartyWiped() || TunicGameState->IsFloorTransitionReady())
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Portal event start rejected: run state does not allow portal interaction | Player=%s | Portal=%s | RunState=%d"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor),
			static_cast<int32>(TunicGameState->GetRunState()));
		return false;
	}

	if (TunicGameState->IsPortalEventActive())
	{
		return true;
	}

	const float InteractionRadius = FMath::Max(1.0f, PortalActor->GetInteractionRadius());
	const float DistanceSquared2D = FVector::DistSquared2D(InteractingPlayer->GetActorLocation(), PortalActor->GetActorLocation());
	if (DistanceSquared2D > FMath::Square(InteractionRadius))
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Portal event start rejected: player outside interaction radius | Player=%s | Portal=%s | Distance=%.1f | Radius=%.1f"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor),
			FMath::Sqrt(DistanceSquared2D),
			InteractionRadius);
		return false;
	}

	TunicGameState->SetRunState(ETunicRunState::PortalEventActive);
	UE_LOG(LogLpQuestRunState, Display, TEXT("Run state changed to PortalEventActive | Player=%s | Portal=%s"),
		*GetNameSafe(InteractingPlayer),
		*GetNameSafe(PortalActor));
	return true;
}

void ATunicGameMode::MarkFloorTransitionReady()
{
	ATunicGameState* TunicGameState = GetGameState<ATunicGameState>();
	if (!HasAuthority() || !TunicGameState || !TunicGameState->IsPortalEventActive())
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
	if (!TunicGameState || TunicGameState->GetRunState() != ETunicRunState::CombatActive)
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

void ATunicGameMode::GrantPendingRunUpgradeChoices(int32 LevelDelta)
{
	if (!HasAuthority() || LevelDelta <= 0 || !GameState)
	{
		return;
	}

	int32 GrantedPlayerCount = 0;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		ATunicPlayerState* TunicPlayerState = Cast<ATunicPlayerState>(PlayerState);
		if (!TunicPlayerState)
		{
			continue;
		}

		TunicPlayerState->AddPendingRunUpgradeChoices(LevelDelta);
		++GrantedPlayerCount;
	}

	UE_LOG(LogLpQuestRunState, Display, TEXT("Pending run upgrade choices granted | LevelDelta=%d | Players=%d"),
		LevelDelta,
		GrantedPlayerCount);
}

