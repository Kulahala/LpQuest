// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/LPQGameMode.h"

#include "AbilitySystemComponent.h"
#include "Ability/LPQRunUpgradeMaxHealthGameplayEffect.h"
#include "Character/LPQEnemyCharacter.h"
#include "Character/LPQPlayerCharacter.h"
#include "Controller/LPQPlayerController.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/LPQEnemySpawnSource.h"
#include "Game/LPQGameState.h"
#include "Game/LPQPortalActor.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Interaction/LPQPickupActor.h"
#include "Player/LPQPlayerState.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestRunState, Log, All);

ALPQGameMode::ALPQGameMode()
{
	DefaultPawnClass = ALPQPlayerCharacter::StaticClass();
	PlayerControllerClass = ALPQPlayerController::StaticClass();
	PlayerStateClass = ALPQPlayerState::StaticClass();
	GameStateClass = ALPQGameState::StaticClass();
	DefaultRunUpgradeGameplayEffectClass = ULPQRunUpgradeMaxHealthGameplayEffect::StaticClass();
}

void ALPQGameMode::BeginPlay()
{
	Super::BeginPlay();

	SpawnFloorWavesForCurrentFloor();
}

void ALPQGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	EvaluatePartyWipe();
}

void ALPQGameMode::EvaluatePartyWipe()
{
	ALPQGameState* LpQuestGameState = GetGameState<ALPQGameState>();
	if (!HasAuthority() || !LpQuestGameState || !LpQuestGameState->IsCombatActive())
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

			const ALPQPlayerCharacter* PlayerCharacter = PlayerState->GetPawn<ALPQPlayerCharacter>();
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
		LpQuestGameState->SetRunState(ELPQRunState::PartyWiped);
		UE_LOG(LogLpQuestRunState, Display, TEXT("Run state changed to PartyWiped"));
	}
}

void ALPQGameMode::HandleEnemyDeath(ALPQEnemyCharacter* DeadEnemy)
{
	ALPQGameState* LpQuestGameState = GetGameState<ALPQGameState>();
	if (!HasAuthority() || !LpQuestGameState || !LpQuestGameState->IsCombatActive() || !DeadEnemy)
	{
		return;
	}

	const int32 ExperienceReward = DeadEnemy->GetExperienceReward();

	if (ExperienceReward > 0)
	{
		const int32 OldSharedRunLevel = LpQuestGameState->GetSharedRunLevel();
		LpQuestGameState->AddSharedRunExperience(ExperienceReward, DeadEnemy);
		const int32 NewSharedRunLevel = LpQuestGameState->GetSharedRunLevel();
		if (NewSharedRunLevel > OldSharedRunLevel)
		{
			GrantPendingRunUpgradeChoices(NewSharedRunLevel - OldSharedRunLevel);
		}

		UE_LOG(LogLpQuestRunState, Display, TEXT("Shared XP awarded | Enemy=%s | Amount=%d | Source=enemy config | TotalSharedXP=%d"),
			*GetNameSafe(DeadEnemy),
			ExperienceReward,
			LpQuestGameState->GetSharedRunExperience());
	}
	else
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Shared XP skipped: enemy ExperienceReward is 0 | Enemy=%s"),
			*GetNameSafe(DeadEnemy));
	}

	SpawnEnemyDropPickup(DeadEnemy);
}

bool ALPQGameMode::TrySelectRunUpgradeForPlayer(ALPQPlayerState* LpQuestPlayerState)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (!LpQuestPlayerState)
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Run upgrade selection rejected: missing player state"));
		return false;
	}

	UAbilitySystemComponent* AbilitySystemComponent = LpQuestPlayerState->GetAbilitySystemComponent();
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Run upgrade selection rejected: missing ASC | PlayerState=%s"),
			*GetNameSafe(LpQuestPlayerState));
		return false;
	}

	if (!DefaultRunUpgradeGameplayEffectClass)
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Run upgrade selection rejected: missing default upgrade GE | PlayerState=%s"),
			*GetNameSafe(LpQuestPlayerState));
		return false;
	}

	if (LpQuestPlayerState->GetPendingRunUpgradeChoices() <= 0)
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Run upgrade selection rejected: no pending choices | PlayerState=%s"),
			*GetNameSafe(LpQuestPlayerState));
		return false;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(LpQuestPlayerState);

	FGameplayEffectSpecHandle UpgradeSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultRunUpgradeGameplayEffectClass, 1.0f, EffectContext);
	if (!UpgradeSpecHandle.IsValid())
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Run upgrade selection rejected: failed to create GE spec | PlayerState=%s | EffectClass=%s"),
			*GetNameSafe(LpQuestPlayerState),
			*GetNameSafe(DefaultRunUpgradeGameplayEffectClass.Get()));
		return false;
	}

	if (!LpQuestPlayerState->TryConsumePendingRunUpgradeChoice())
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Run upgrade selection rejected: pending choice consume failed | PlayerState=%s"),
			*GetNameSafe(LpQuestPlayerState));
		return false;
	}

	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*UpgradeSpecHandle.Data.Get());

	UE_LOG(LogLpQuestRunState, Display, TEXT("Run upgrade selected | PlayerState=%s | EffectClass=%s | RemainingPending=%d"),
		*GetNameSafe(LpQuestPlayerState),
		*GetNameSafe(DefaultRunUpgradeGameplayEffectClass.Get()),
		LpQuestPlayerState->GetPendingRunUpgradeChoices());

	return true;
}

bool ALPQGameMode::TryStartPortalEvent(ALPQPlayerCharacter* InteractingPlayer, ALPQPortalActor* PortalActor)
{
	ALPQGameState* LpQuestGameState = GetGameState<ALPQGameState>();
	if (!HasAuthority() || !LpQuestGameState || !InteractingPlayer || !PortalActor)
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

	if (PortalActor->GetPortalCompletionMode() != ELPQPortalCompletionMode::CombatEvent)
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

	if (LpQuestGameState->IsPartyWiped() || LpQuestGameState->IsFloorTransitionReady())
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Portal event start rejected: run state does not allow portal interaction | Player=%s | Portal=%s | RunState=%d"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor),
			static_cast<int32>(LpQuestGameState->GetRunState()));
		return false;
	}

	if (ALPQPortalActor* ExistingPortalEventOwner = ActivePortalEventOwner.Get())
	{
		if (ExistingPortalEventOwner == PortalActor)
		{
			return LpQuestGameState->IsPortalEventActive();
		}

		UE_LOG(LogLpQuestRunState, Display, TEXT("Portal event start rejected: another portal already owns the active event | Player=%s | RequestedPortal=%s | ActivePortal=%s"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor),
			*GetNameSafe(ExistingPortalEventOwner));
		return false;
	}

	if (LpQuestGameState->IsPortalEventActive())
	{
		UE_LOG(LogLpQuestRunState, Warning, TEXT("Portal event start rejected: PortalEventActive has no selected portal owner | Player=%s | Portal=%s"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor));
		return false;
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

	ActivePortalEventOwner = PortalActor;
	LpQuestGameState->SetRunState(ELPQRunState::PortalEventActive);
	UE_LOG(LogLpQuestRunState, Display, TEXT("Run state changed to PortalEventActive | Player=%s | Portal=%s"),
		*GetNameSafe(InteractingPlayer),
		*GetNameSafe(PortalActor));
	return true;
}

bool ALPQGameMode::TryUseDirectFloorExitPortal(ALPQPlayerCharacter* InteractingPlayer, ALPQPortalActor* PortalActor)
{
	if (ActivePortalEventOwner.IsValid())
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Direct floor exit rejected: a combat portal event is already selected | Player=%s | Portal=%s | ActivePortal=%s"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor),
			*GetNameSafe(ActivePortalEventOwner.Get()));
		return false;
	}

	if (!PortalActor || PortalActor->GetPortalCompletionMode() != ELPQPortalCompletionMode::DirectFloorExit)
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

bool ALPQGameMode::IsActivePortalEventOwner(const ALPQPortalActor* PortalActor) const
{
	return PortalActor && ActivePortalEventOwner.Get() == PortalActor;
}

bool ALPQGameMode::MarkFloorTransitionReady(FName DestinationId)
{
	ALPQGameState* LpQuestGameState = GetGameState<ALPQGameState>();
	if (!HasAuthority() || !LpQuestGameState || !(LpQuestGameState->IsPortalEventActive() || LpQuestGameState->GetRunState() == ELPQRunState::CombatActive) || DestinationId.IsNone())
	{
		return false;
	}

	PendingFloorDestinationId = DestinationId;
	LpQuestGameState->SetRunState(ELPQRunState::FloorTransitionReady);
	UE_LOG(LogLpQuestRunState, Display, TEXT("Run state changed to FloorTransitionReady | Destination=%s"),
		*PendingFloorDestinationId.ToString());

	if (FloorTransitionStubDelay <= UE_SMALL_NUMBER)
	{
		CompleteFloorTransitionStub();
		return true;
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ALPQGameMode::CompleteFloorTransitionStub, FloorTransitionStubDelay, false);
	return true;
}

void ALPQGameMode::CompleteFloorTransitionStub()
{
	ALPQGameState* LpQuestGameState = GetGameState<ALPQGameState>();
	if (!HasAuthority() || !LpQuestGameState || !LpQuestGameState->IsFloorTransitionReady())
	{
		return;
	}

	const int32 PreviousFloorIndex = LpQuestGameState->GetCurrentFloorIndex();
	const int32 NewFloorIndex = PreviousFloorIndex + 1;

	int32 ResetPortalCount = 0;
	for (TActorIterator<ALPQPortalActor> PortalIt(GetWorld()); PortalIt; ++PortalIt)
	{
		ALPQPortalActor* PortalActor = *PortalIt;
		if (!PortalActor)
		{
			continue;
		}

		PortalActor->ResetPortalForNextFloorStub();
		++ResetPortalCount;
	}

	int32 ResetSpawnSourceCount = 0;
	for (TActorIterator<ALPQFloorWaveEnemySpawnSource> SpawnSourceIt(GetWorld()); SpawnSourceIt; ++SpawnSourceIt)
	{
		ALPQFloorWaveEnemySpawnSource* SpawnSource = *SpawnSourceIt;
		if (!SpawnSource)
		{
			continue;
		}

		SpawnSource->ResetForNextFloor();
		++ResetSpawnSourceCount;
	}

	LpQuestGameState->SetCurrentFloorIndex(NewFloorIndex);
	LpQuestGameState->SetCurrentFloorDestinationId(PendingFloorDestinationId);
	LpQuestGameState->SetRunState(ELPQRunState::CombatActive);
	ActivePortalEventOwner.Reset();
	SpawnFloorWavesForCurrentFloor();

	UE_LOG(LogLpQuestRunState, Display, TEXT("Floor transition stub completed | PreviousFloor=%d | NewFloor=%d | Destination=%s | ResetPortals=%d | ResetSpawnSources=%d"),
		PreviousFloorIndex,
		NewFloorIndex,
		*PendingFloorDestinationId.ToString(),
		ResetPortalCount,
		ResetSpawnSourceCount);

	PendingFloorDestinationId = TEXT("Next");
}

void ALPQGameMode::SpawnFloorWavesForCurrentFloor()
{
	if (!HasAuthority())
	{
		return;
	}

	ALPQGameState* LpQuestGameState = GetGameState<ALPQGameState>();
	if (!LpQuestGameState || LpQuestGameState->GetRunState() != ELPQRunState::CombatActive)
	{
		return;
	}

	int32 SpawnSourceCount = 0;
	int32 SpawnedEnemyCount = 0;
	for (TActorIterator<ALPQFloorWaveEnemySpawnSource> SpawnSourceIt(GetWorld()); SpawnSourceIt; ++SpawnSourceIt)
	{
		ALPQFloorWaveEnemySpawnSource* SpawnSource = *SpawnSourceIt;
		if (!SpawnSource)
		{
			continue;
		}

		++SpawnSourceCount;
		SpawnedEnemyCount += SpawnSource->SpawnForFloor(LpQuestGameState->GetCurrentFloorIndex());
	}

	if (SpawnSourceCount <= 0)
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Floor wave spawn skipped: no floor wave enemy spawn sources | Floor=%d"),
			LpQuestGameState->GetCurrentFloorIndex());
		return;
	}

	UE_LOG(LogLpQuestRunState, Display, TEXT("Floor wave spawn completed | Floor=%d | SpawnSources=%d | SpawnedEnemies=%d"),
		LpQuestGameState->GetCurrentFloorIndex(),
		SpawnSourceCount,
		SpawnedEnemyCount);
}

void ALPQGameMode::GrantPendingRunUpgradeChoices(int32 LevelDelta)
{
	if (!HasAuthority() || LevelDelta <= 0 || !GameState)
	{
		return;
	}

	int32 GrantedPlayerCount = 0;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		ALPQPlayerState* LpQuestPlayerState = Cast<ALPQPlayerState>(PlayerState);
		if (!LpQuestPlayerState)
		{
			continue;
		}

		LpQuestPlayerState->AddPendingRunUpgradeChoices(LevelDelta);
		++GrantedPlayerCount;
	}

	UE_LOG(LogLpQuestRunState, Display, TEXT("Pending run upgrade choices granted | LevelDelta=%d | Players=%d"),
		LevelDelta,
		GrantedPlayerCount);
}

void ALPQGameMode::SpawnEnemyDropPickup(ALPQEnemyCharacter* DeadEnemy)
{
	if (!HasAuthority() || !DeadEnemy)
	{
		return;
	}

	TSubclassOf<ALPQPickupActor> DroppedPickupClass = DeadEnemy->GetDroppedPickupClass();
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

	ALPQPickupActor* SpawnedPickup = World->SpawnActor<ALPQPickupActor>(DroppedPickupClass, DeadEnemy->GetActorTransform(), SpawnParameters);
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

bool ALPQGameMode::CanUsePortalForFloorTransition(ALPQPlayerCharacter* InteractingPlayer, ALPQPortalActor* PortalActor, bool bRequireAllLivingPlayersInPortalRadius)
{
	const ALPQGameState* LpQuestGameState = GetGameState<ALPQGameState>();
	if (!HasAuthority() || !LpQuestGameState || !InteractingPlayer || !PortalActor)
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

	if (LpQuestGameState->IsPartyWiped() || LpQuestGameState->IsFloorTransitionReady() || LpQuestGameState->IsPortalEventActive())
	{
		UE_LOG(LogLpQuestRunState, Display, TEXT("Portal floor transition rejected: run state does not allow direct exit | Player=%s | Portal=%s | RunState=%d"),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(PortalActor),
			static_cast<int32>(LpQuestGameState->GetRunState()));
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

		const ALPQPlayerCharacter* PlayerCharacter = PlayerState->GetPawn<ALPQPlayerCharacter>();
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

