// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/TunicGameMode.h"

#include "AbilitySystemComponent.h"
#include "Ability/TunicRunUpgradeMaxHealthGameplayEffect.h"
#include "Character/TunicEnemyCharacter.h"
#include "Character/TunicPlayerCharacter.h"
#include "Controller/TunicPlayerController.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/TunicEnemySpawnSource.h"
#include "Game/TunicGameState.h"
#include "Game/TunicPortalActor.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Interaction/TunicPickupActor.h"
#include "Player/TunicPlayerState.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestRunState, Log, All);

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

	SpawnFloorWavesForCurrentFloor();
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

void ATunicGameMode::HandleEnemyDeath(ATunicEnemyCharacter* DeadEnemy)
{
	ATunicGameState* TunicGameState = GetGameState<ATunicGameState>();
	if (!HasAuthority() || !TunicGameState || !TunicGameState->IsCombatActive() || !DeadEnemy)
	{
		return;
	}

	const int32 ExperienceReward = DeadEnemy->GetExperienceReward();

	if (ExperienceReward > 0)
	{
		const int32 OldSharedRunLevel = TunicGameState->GetSharedRunLevel();
		TunicGameState->AddSharedRunExperience(ExperienceReward, DeadEnemy);
		const int32 NewSharedRunLevel = TunicGameState->GetSharedRunLevel();
		if (NewSharedRunLevel > OldSharedRunLevel)
		{
			GrantPendingRunUpgradeChoices(NewSharedRunLevel - OldSharedRunLevel);
		}

		UE_LOG(LogLpQuestRunState, Display, TEXT("Shared XP awarded | Enemy=%s | Amount=%d | Source=enemy config | TotalSharedXP=%d"),
			*GetNameSafe(DeadEnemy),
			ExperienceReward,
			TunicGameState->GetSharedRunExperience());
	}
	else
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Shared XP skipped: enemy ExperienceReward is 0 | Enemy=%s"),
			*GetNameSafe(DeadEnemy));
	}

	SpawnEnemyDropPickup(DeadEnemy);
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

	int32 ResetSpawnSourceCount = 0;
	for (TActorIterator<ATunicFloorWaveEnemySpawnSource> SpawnSourceIt(GetWorld()); SpawnSourceIt; ++SpawnSourceIt)
	{
		ATunicFloorWaveEnemySpawnSource* SpawnSource = *SpawnSourceIt;
		if (!SpawnSource)
		{
			continue;
		}

		SpawnSource->ResetForNextFloor();
		++ResetSpawnSourceCount;
	}

	TunicGameState->SetCurrentFloorIndex(NewFloorIndex);
	TunicGameState->SetRunState(ETunicRunState::CombatActive);
	SpawnFloorWavesForCurrentFloor();

	UE_LOG(LogLpQuestRunState, Display, TEXT("Floor transition stub completed | PreviousFloor=%d | NewFloor=%d | ResetPortals=%d | ResetSpawnSources=%d"),
		PreviousFloorIndex,
		NewFloorIndex,
		ResetPortalCount,
		ResetSpawnSourceCount);
}

void ATunicGameMode::SpawnFloorWavesForCurrentFloor()
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

	int32 SpawnSourceCount = 0;
	int32 SpawnedEnemyCount = 0;
	for (TActorIterator<ATunicFloorWaveEnemySpawnSource> SpawnSourceIt(GetWorld()); SpawnSourceIt; ++SpawnSourceIt)
	{
		ATunicFloorWaveEnemySpawnSource* SpawnSource = *SpawnSourceIt;
		if (!SpawnSource)
		{
			continue;
		}

		++SpawnSourceCount;
		SpawnedEnemyCount += SpawnSource->SpawnForFloor(TunicGameState->GetCurrentFloorIndex());
	}

	if (SpawnSourceCount <= 0)
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Floor wave spawn skipped: no floor wave enemy spawn sources | Floor=%d"),
			TunicGameState->GetCurrentFloorIndex());
		return;
	}

	UE_LOG(LogLpQuestRunState, Display, TEXT("Floor wave spawn completed | Floor=%d | SpawnSources=%d | SpawnedEnemies=%d"),
		TunicGameState->GetCurrentFloorIndex(),
		SpawnSourceCount,
		SpawnedEnemyCount);
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

void ATunicGameMode::SpawnEnemyDropPickup(ATunicEnemyCharacter* DeadEnemy)
{
	if (!HasAuthority() || !DeadEnemy)
	{
		return;
	}

	TSubclassOf<ATunicPickupActor> DroppedPickupClass = DeadEnemy->GetDroppedPickupClass();
	if (!DroppedPickupClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = DeadEnemy;
	SpawnParameters.Instigator = DeadEnemy->GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ATunicPickupActor* SpawnedPickup = World->SpawnActor<ATunicPickupActor>(DroppedPickupClass, DeadEnemy->GetActorTransform(), SpawnParameters);
	if (!SpawnedPickup)
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Enemy drop spawn failed | Enemy=%s | PickupClass=%s"),
			*GetNameSafe(DeadEnemy),
			*GetNameSafe(DroppedPickupClass.Get()));
		return;
	}

	UE_LOG(LogLpQuestRunState, Display, TEXT("Enemy drop spawned | Enemy=%s | Pickup=%s | PickupClass=%s"),
		*GetNameSafe(DeadEnemy),
		*GetNameSafe(SpawnedPickup),
		*GetNameSafe(DroppedPickupClass.Get()));
}

