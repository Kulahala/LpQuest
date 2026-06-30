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

	if (PortalActor->GetPortalDestinationId().IsNone())
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Portal event start rejected: missing destination id | Player=%s | Portal=%s"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor));
		return false;
	}

	if (PortalActor->GetPortalCompletionMode() != ETunicPortalCompletionMode::CombatEvent)
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Portal event start rejected: portal is not CombatEvent mode | Player=%s | Portal=%s | Mode=%d"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor),
			static_cast<int32>(PortalActor->GetPortalCompletionMode()));
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

bool ATunicGameMode::TryUseDirectFloorExitPortal(ATunicPlayerCharacter* InteractingPlayer, ATunicPortalActor* PortalActor)
{
	if (!PortalActor || PortalActor->GetPortalCompletionMode() != ETunicPortalCompletionMode::DirectFloorExit)
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Direct floor exit rejected: invalid portal or wrong mode | Player=%s | Portal=%s"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor));
		return false;
	}

	if (!CanUsePortalForFloorTransition(InteractingPlayer, PortalActor, true))
	{
		return false;
	}

	if (!MarkFloorTransitionReady(PortalActor->GetPortalDestinationId()))
	{
		return false;
	}

	UE_LOG(LogLpQuestRunState, Display, TEXT("Direct floor exit accepted | Player=%s | Portal=%s | Destination=%s"),
		*GetNameSafe(InteractingPlayer),
		*GetNameSafe(PortalActor),
		*PortalActor->GetPortalDestinationId().ToString());
	return true;
}

bool ATunicGameMode::MarkFloorTransitionReady(FName DestinationId)
{
	ATunicGameState* TunicGameState = GetGameState<ATunicGameState>();
	if (!HasAuthority() || !TunicGameState || !(TunicGameState->IsPortalEventActive() || TunicGameState->GetRunState() == ETunicRunState::CombatActive) || DestinationId.IsNone())
	{
		return false;
	}

	PendingFloorDestinationId = DestinationId;
	TunicGameState->SetRunState(ETunicRunState::FloorTransitionReady);
	UE_LOG(LogLpQuestRunState, Display, TEXT("Run state changed to FloorTransitionReady | Destination=%s"),
		*PendingFloorDestinationId.ToString());

	if (FloorTransitionStubDelay <= UE_SMALL_NUMBER)
	{
		CompleteFloorTransitionStub();
		return true;
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ATunicGameMode::CompleteFloorTransitionStub, FloorTransitionStubDelay, false);
	return true;
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
	TunicGameState->SetCurrentFloorDestinationId(PendingFloorDestinationId);
	TunicGameState->SetRunState(ETunicRunState::CombatActive);
	SpawnFloorWavesForCurrentFloor();

	UE_LOG(LogLpQuestRunState, Display, TEXT("Floor transition stub completed | PreviousFloor=%d | NewFloor=%d | Destination=%s | ResetPortals=%d | ResetSpawnSources=%d"),
		PreviousFloorIndex,
		NewFloorIndex,
		*PendingFloorDestinationId.ToString(),
		ResetPortalCount,
		ResetSpawnSourceCount);

	PendingFloorDestinationId = TEXT("Next");
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

bool ATunicGameMode::CanUsePortalForFloorTransition(ATunicPlayerCharacter* InteractingPlayer, ATunicPortalActor* PortalActor, bool bRequireAllLivingPlayersInPortalRadius)
{
	const ATunicGameState* TunicGameState = GetGameState<ATunicGameState>();
	if (!HasAuthority() || !TunicGameState || !InteractingPlayer || !PortalActor)
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Portal floor transition rejected: missing authority, game state, player, or portal | Player=%s | Portal=%s"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor));
		return false;
	}

	if (InteractingPlayer->IsDead())
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Portal floor transition rejected: dead player | Player=%s | Portal=%s"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor));
		return false;
	}

	if (PortalActor->GetPortalDestinationId().IsNone())
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Portal floor transition rejected: missing destination id | Player=%s | Portal=%s"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor));
		return false;
	}

	if (TunicGameState->IsPartyWiped() || TunicGameState->IsFloorTransitionReady() || TunicGameState->IsPortalEventActive())
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Portal floor transition rejected: run state does not allow direct exit | Player=%s | Portal=%s | RunState=%d"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor),
			static_cast<int32>(TunicGameState->GetRunState()));
		return false;
	}

	const float InteractionRadius = FMath::Max(1.0f, PortalActor->GetInteractionRadius());
	const float DistanceSquared2D = FVector::DistSquared2D(InteractingPlayer->GetActorLocation(), PortalActor->GetActorLocation());
	if (DistanceSquared2D > FMath::Square(InteractionRadius))
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Portal floor transition rejected: player outside interaction radius | Player=%s | Portal=%s | Distance=%.1f | Radius=%.1f"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor),
			FMath::Sqrt(DistanceSquared2D),
			InteractionRadius);
		return false;
	}

	if (!bRequireAllLivingPlayersInPortalRadius)
	{
		return true;
	}

	const AGameStateBase* CurrentGameState = GameState;
	if (!CurrentGameState)
	{
		return false;
	}

	const float ActivationRadiusSquared = FMath::Square(FMath::Max(0.0f, PortalActor->GetActivationRadius()));
	const FVector PortalLocation = PortalActor->GetActorLocation();
	int32 RequiredLivingPlayerCount = 0;
	int32 PresentLivingPlayerCount = 0;

	for (const APlayerState* PlayerState : CurrentGameState->PlayerArray)
	{
		if (!PlayerState)
		{
			continue;
		}

		const ATunicPlayerCharacter* PlayerCharacter = PlayerState->GetPawn<ATunicPlayerCharacter>();
		if (!PlayerCharacter || PlayerCharacter->IsDead())
		{
			continue;
		}

		++RequiredLivingPlayerCount;
		if (FVector::DistSquared2D(PlayerCharacter->GetActorLocation(), PortalLocation) <= ActivationRadiusSquared)
		{
			++PresentLivingPlayerCount;
		}
	}

	const bool bAllLivingPlayersInRange = RequiredLivingPlayerCount > 0 && RequiredLivingPlayerCount == PresentLivingPlayerCount;
	if (!bAllLivingPlayersInRange)
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Portal floor transition rejected: not all living players in portal radius | Player=%s | Portal=%s | PresentLivingPlayers=%d/%d"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor),
			PresentLivingPlayerCount,
			RequiredLivingPlayerCount);
		return false;
	}

	return true;
}

