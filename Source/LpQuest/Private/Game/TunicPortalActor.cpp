// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/TunicPortalActor.h"

#include "Character/TunicEnemyCharacter.h"
#include "Character/TunicPlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Game/TunicGameMode.h"
#include "Game/TunicGameState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestPortal, Log, All);

ATunicPortalActor::ATunicPortalActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PortalRadiusPreview = CreateDefaultSubobject<USphereComponent>(TEXT("PortalRadiusPreview"));
	PortalRadiusPreview->SetupAttachment(SceneRoot);
	PortalRadiusPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalRadiusPreview->SetSphereRadius(ActivationRadius);
}

void ATunicPortalActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdatePortalRadiusPreview();
}

void ATunicPortalActor::BeginPlay()
{
	Super::BeginPlay();

	UpdatePortalRadiusPreview();
}

void ATunicPortalActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || bIsPortalReady)
	{
		return;
	}

	TryActivatePortalFromRunState();
	if (bIsPortalActive)
	{
		EvaluatePortalCharge(DeltaSeconds);
		TickPortalPressureSpawns(DeltaSeconds);
	}
}

void ATunicPortalActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATunicPortalActor, bIsPortalActive);
	DOREPLIFETIME(ATunicPortalActor, bIsPortalCharging);
	DOREPLIFETIME(ATunicPortalActor, bIsPortalReady);
	DOREPLIFETIME(ATunicPortalActor, ActivationProgress);
	DOREPLIFETIME(ATunicPortalActor, RequiredLivingPlayerCount);
	DOREPLIFETIME(ATunicPortalActor, PresentLivingPlayerCount);
	DOREPLIFETIME(ATunicPortalActor, PortalResetSerial);
}

bool ATunicPortalActor::IsPortalActive() const
{
	return bIsPortalActive;
}

bool ATunicPortalActor::IsPortalCharging() const
{
	return bIsPortalCharging;
}

bool ATunicPortalActor::IsPortalReady() const
{
	return bIsPortalReady;
}

float ATunicPortalActor::GetActivationProgress() const
{
	return ActivationProgress;
}

int32 ATunicPortalActor::GetRequiredLivingPlayerCount() const
{
	return RequiredLivingPlayerCount;
}

int32 ATunicPortalActor::GetPresentLivingPlayerCount() const
{
	return PresentLivingPlayerCount;
}

float ATunicPortalActor::GetInteractionRadius() const
{
	return InteractionRadius;
}

bool ATunicPortalActor::OwnsPortalBossEnemy(const ATunicEnemyCharacter* EnemyCharacter) const
{
	return EnemyCharacter && SpawnedPortalBossEnemy.Get() == EnemyCharacter;
}

int32 ATunicPortalActor::ConsumePortalPressureExperienceReward(ATunicEnemyCharacter* DeadEnemy)
{
	if (!HasAuthority() || !DeadEnemy || !OwnsPortalPressureEnemy(DeadEnemy) || HasPortalPressureEnemyBeenRewarded(DeadEnemy))
	{
		return 0;
	}

	RewardedPortalPressureEnemies.Add(DeadEnemy);
	if (RemainingPortalPressureExperienceBudget <= 0)
	{
		if (bLogPortalState)
		{
			UE_LOG(LogLpQuestPortal, Display, TEXT("Portal pressure XP skipped: budget exhausted | Portal=%s | Enemy=%s"),
				*GetNameSafe(this),
				*GetNameSafe(DeadEnemy));
		}
		return 0;
	}

	const int32 RewardAmount = FMath::Min(DeadEnemy->GetExperienceReward(), RemainingPortalPressureExperienceBudget);
	RemainingPortalPressureExperienceBudget -= RewardAmount;

	if (bLogPortalState)
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal pressure XP consumed | Portal=%s | Enemy=%s | Amount=%d | RemainingBudget=%d"),
			*GetNameSafe(this),
			*GetNameSafe(DeadEnemy),
			RewardAmount,
			RemainingPortalPressureExperienceBudget);
	}

	return RewardAmount;
}

void ATunicPortalActor::ResetPortalForNextFloorStub()
{
	if (!HasAuthority())
	{
		return;
	}

	SetPortalCharging(false);
	SetPortalReady(false);
	SetPortalActive(false);
	SetActivationProgress(0.0f);
	SetPortalPlayerCounts(0, 0);
	CleanupPortalBoss();
	CleanupPortalPressureEnemies();
	ResetPortalPressureState();

	++PortalResetSerial;
	if (bLogPortalState)
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal reset for next floor stub | Portal=%s | ResetSerial=%d"),
			*GetNameSafe(this),
			PortalResetSerial);
	}
	OnPortalReset();
}

void ATunicPortalActor::OnPortalActivated_Implementation()
{
}

void ATunicPortalActor::OnPortalChargingStateChanged_Implementation(bool)
{
}

void ATunicPortalActor::OnPortalChargeChanged_Implementation(float)
{
}

void ATunicPortalActor::OnPortalReady_Implementation()
{
}

void ATunicPortalActor::OnPortalReset_Implementation()
{
}

bool ATunicPortalActor::CanInteractWithTunicPlayer_Implementation(ATunicPlayerCharacter* InteractingPlayer)
{
	if (!HasAuthority() || !InteractingPlayer || InteractingPlayer->IsDead() || bIsPortalReady)
	{
		return false;
	}

	const ATunicGameState* TunicGameState = GetWorld() ? GetWorld()->GetGameState<ATunicGameState>() : nullptr;
	if (!TunicGameState || TunicGameState->IsPartyWiped() || TunicGameState->IsFloorTransitionReady())
	{
		return false;
	}

	const float InteractionRadiusSquared = FMath::Square(FMath::Max(1.0f, InteractionRadius));
	return FVector::DistSquared2D(InteractingPlayer->GetActorLocation(), GetActorLocation()) <= InteractionRadiusSquared;
}

void ATunicPortalActor::InteractWithTunicPlayer_Implementation(ATunicPlayerCharacter* InteractingPlayer)
{
	if (ATunicGameMode* TunicGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ATunicGameMode>() : nullptr)
	{
		TunicGameMode->TryStartPortalEvent(InteractingPlayer, this);
	}
}

void ATunicPortalActor::TryActivatePortalFromRunState()
{
	if (bIsPortalActive)
	{
		return;
	}

	const ATunicGameState* TunicGameState = GetWorld() ? GetWorld()->GetGameState<ATunicGameState>() : nullptr;
	if (!TunicGameState || !TunicGameState->IsPortalEventActive())
	{
		return;
	}

	SetPortalActive(true);
	ResetPortalPressureState();
	SpawnPortalBossIfNeeded();
}

void ATunicPortalActor::EvaluatePortalCharge(float DeltaSeconds)
{
	const ATunicGameState* TunicGameState = GetWorld() ? GetWorld()->GetGameState<ATunicGameState>() : nullptr;
	if (!TunicGameState || !TunicGameState->IsPortalEventActive())
	{
		SetPortalCharging(false);
		return;
	}

	if (!IsPortalBossDefeated())
	{
		SetPortalCharging(false);
		return;
	}

	int32 NewRequiredLivingPlayerCount = 0;
	int32 NewPresentLivingPlayerCount = 0;
	const bool bAllLivingPlayersInRange = CountLivingPlayersInRange(NewRequiredLivingPlayerCount, NewPresentLivingPlayerCount);
	SetPortalPlayerCounts(NewRequiredLivingPlayerCount, NewPresentLivingPlayerCount);

	if (bAllLivingPlayersInRange)
	{
		SetPortalCharging(true);
		const float ProgressDelta = ChargeDuration <= UE_SMALL_NUMBER ? 1.0f : DeltaSeconds / ChargeDuration;
		SetActivationProgress(ActivationProgress + ProgressDelta);
		if (ActivationProgress >= 1.0f)
		{
			CompletePortal();
		}
		return;
	}

	SetPortalCharging(false);
}

void ATunicPortalActor::CompletePortal()
{
	if (bIsPortalReady)
	{
		return;
	}

	SetActivationProgress(1.0f);
	SetPortalCharging(false);
	SetPortalReady(true);
	CleanupPortalPressureEnemies();

	if (ATunicGameMode* TunicGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ATunicGameMode>() : nullptr)
	{
		TunicGameMode->MarkFloorTransitionReady();
	}
}

void ATunicPortalActor::SpawnPortalBossIfNeeded()
{
	if (!HasAuthority() || SpawnedPortalBossEnemy.IsValid())
	{
		return;
	}

	if (!PortalBossEnemyClass)
	{
		if (bLogPortalState)
		{
			UE_LOG(LogLpQuestPortal, Display, TEXT("Portal boss spawn skipped: no boss class configured | Portal=%s"),
				*GetNameSafe(this));
		}
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		bPortalBossSpawnFailed = true;
		return;
	}

	const FTransform SpawnTransform = PortalBossSpawnPoint ? PortalBossSpawnPoint->GetActorTransform() : GetActorTransform();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ATunicEnemyCharacter* SpawnedBoss = World->SpawnActor<ATunicEnemyCharacter>(PortalBossEnemyClass, SpawnTransform, SpawnParameters);
	SpawnedPortalBossEnemy = SpawnedBoss;
	bPortalBossSpawnFailed = !SpawnedBoss;

	if (bLogPortalState)
	{
		if (SpawnedBoss)
		{
			UE_LOG(LogLpQuestPortal, Display, TEXT("Portal boss spawned | Portal=%s | Boss=%s | BossClass=%s"),
				*GetNameSafe(this),
				*GetNameSafe(SpawnedBoss),
				*GetNameSafe(PortalBossEnemyClass.Get()));
		}
		else
		{
			UE_LOG(LogLpQuestPortal, Warning, TEXT("Portal boss spawn failed | Portal=%s | BossClass=%s"),
				*GetNameSafe(this),
				*GetNameSafe(PortalBossEnemyClass.Get()));
		}
	}
}

bool ATunicPortalActor::IsPortalBossDefeated() const
{
	if (!PortalBossEnemyClass)
	{
		return true;
	}

	if (bPortalBossSpawnFailed)
	{
		return false;
	}

	const ATunicEnemyCharacter* SpawnedBoss = SpawnedPortalBossEnemy.Get();
	return !SpawnedBoss || SpawnedBoss->IsDead();
}

void ATunicPortalActor::CleanupPortalBoss()
{
	if (ATunicEnemyCharacter* SpawnedBoss = SpawnedPortalBossEnemy.Get())
	{
		SpawnedBoss->Destroy();
	}

	SpawnedPortalBossEnemy.Reset();
	bPortalBossSpawnFailed = false;
}

void ATunicPortalActor::ResetPortalPressureState()
{
	SpawnedPortalPressureEnemies.Reset();
	RewardedPortalPressureEnemies.Reset();
	PortalPressureSpawnTimer = FMath::Max(0.1f, PortalPressureSpawnInterval);
	PortalPressureSpawnPointIndex = 0;
	RemainingPortalPressureExperienceBudget = FMath::Max(0, PortalPressureExperienceBudget);
}

void ATunicPortalActor::TickPortalPressureSpawns(float DeltaSeconds)
{
	if (!ShouldSpawnPortalPressure())
	{
		return;
	}

	PortalPressureSpawnTimer -= DeltaSeconds;
	if (PortalPressureSpawnTimer > 0.0f)
	{
		return;
	}

	SpawnPortalPressureEnemy();
	PortalPressureSpawnTimer = FMath::Max(0.1f, PortalPressureSpawnInterval);
}

bool ATunicPortalActor::ShouldSpawnPortalPressure() const
{
	const ATunicGameState* TunicGameState = GetWorld() ? GetWorld()->GetGameState<ATunicGameState>() : nullptr;
	return HasAuthority()
		&& bIsPortalActive
		&& !bIsPortalReady
		&& TunicGameState
		&& TunicGameState->IsPortalEventActive()
		&& IsPortalBossDefeated()
		&& PortalPressureEnemyClass
		&& MaxAlivePortalPressureEnemies > 0
		&& GetAlivePortalPressureEnemyCount() < MaxAlivePortalPressureEnemies;
}

void ATunicPortalActor::SpawnPortalPressureEnemy()
{
	UWorld* World = GetWorld();
	if (!World || !PortalPressureEnemyClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ATunicEnemyCharacter* SpawnedEnemy = World->SpawnActor<ATunicEnemyCharacter>(PortalPressureEnemyClass, GetNextPortalPressureSpawnTransform(), SpawnParameters);
	if (!SpawnedEnemy)
	{
		if (bLogPortalState)
		{
			UE_LOG(LogLpQuestPortal, Warning, TEXT("Portal pressure spawn failed | Portal=%s | EnemyClass=%s"),
				*GetNameSafe(this),
				*GetNameSafe(PortalPressureEnemyClass.Get()));
		}
		return;
	}

	SpawnedPortalPressureEnemies.Add(SpawnedEnemy);
	if (bLogPortalState)
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal pressure enemy spawned | Portal=%s | Enemy=%s | Alive=%d/%d | RemainingXPBudget=%d"),
			*GetNameSafe(this),
			*GetNameSafe(SpawnedEnemy),
			GetAlivePortalPressureEnemyCount(),
			MaxAlivePortalPressureEnemies,
			RemainingPortalPressureExperienceBudget);
	}
}

FTransform ATunicPortalActor::GetNextPortalPressureSpawnTransform()
{
	const int32 SpawnPointCount = PortalPressureSpawnPoints.Num();
	if (SpawnPointCount > 0)
	{
		for (int32 AttemptIndex = 0; AttemptIndex < SpawnPointCount; ++AttemptIndex)
		{
			const int32 SpawnPointIndex = (PortalPressureSpawnPointIndex + AttemptIndex) % SpawnPointCount;
			if (AActor* SpawnPoint = PortalPressureSpawnPoints[SpawnPointIndex])
			{
				PortalPressureSpawnPointIndex = (SpawnPointIndex + 1) % SpawnPointCount;
				return SpawnPoint->GetActorTransform();
			}
		}
	}

	if (bLogPortalState)
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal pressure spawn using portal transform fallback | Portal=%s"),
			*GetNameSafe(this));
	}
	return GetActorTransform();
}

int32 ATunicPortalActor::GetAlivePortalPressureEnemyCount() const
{
	int32 AliveCount = 0;
	for (const TWeakObjectPtr<ATunicEnemyCharacter>& PressureEnemy : SpawnedPortalPressureEnemies)
	{
		const ATunicEnemyCharacter* EnemyCharacter = PressureEnemy.Get();
		if (EnemyCharacter && !EnemyCharacter->IsDead())
		{
			++AliveCount;
		}
	}
	return AliveCount;
}

bool ATunicPortalActor::OwnsPortalPressureEnemy(const ATunicEnemyCharacter* EnemyCharacter) const
{
	if (!EnemyCharacter)
	{
		return false;
	}

	for (const TWeakObjectPtr<ATunicEnemyCharacter>& PressureEnemy : SpawnedPortalPressureEnemies)
	{
		if (PressureEnemy.Get() == EnemyCharacter)
		{
			return true;
		}
	}
	return false;
}

bool ATunicPortalActor::HasPortalPressureEnemyBeenRewarded(const ATunicEnemyCharacter* EnemyCharacter) const
{
	if (!EnemyCharacter)
	{
		return false;
	}

	for (const TWeakObjectPtr<ATunicEnemyCharacter>& RewardedEnemy : RewardedPortalPressureEnemies)
	{
		if (RewardedEnemy.Get() == EnemyCharacter)
		{
			return true;
		}
	}
	return false;
}

void ATunicPortalActor::CleanupPortalPressureEnemies()
{
	for (const TWeakObjectPtr<ATunicEnemyCharacter>& PressureEnemy : SpawnedPortalPressureEnemies)
	{
		if (ATunicEnemyCharacter* EnemyCharacter = PressureEnemy.Get())
		{
			EnemyCharacter->Destroy();
		}
	}

	SpawnedPortalPressureEnemies.Reset();
	RewardedPortalPressureEnemies.Reset();
}

bool ATunicPortalActor::CountLivingPlayersInRange(int32& OutRequiredLivingPlayerCount, int32& OutPresentLivingPlayerCount) const
{
	OutRequiredLivingPlayerCount = 0;
	OutPresentLivingPlayerCount = 0;

	const AGameStateBase* CurrentGameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!CurrentGameState)
	{
		return false;
	}

	const float ActivationRadiusSquared = FMath::Square(FMath::Max(0.0f, ActivationRadius));
	const FVector PortalLocation = GetActorLocation();

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

		++OutRequiredLivingPlayerCount;
		if (FVector::DistSquared2D(PlayerCharacter->GetActorLocation(), PortalLocation) <= ActivationRadiusSquared)
		{
			++OutPresentLivingPlayerCount;
		}
	}

	return OutRequiredLivingPlayerCount > 0 && OutPresentLivingPlayerCount == OutRequiredLivingPlayerCount;
}

void ATunicPortalActor::UpdatePortalRadiusPreview()
{
	if (PortalRadiusPreview)
	{
		PortalRadiusPreview->SetSphereRadius(FMath::Max(1.0f, ActivationRadius));
	}
}

void ATunicPortalActor::SetPortalActive(bool bNewIsPortalActive)
{
	if (bIsPortalActive == bNewIsPortalActive)
	{
		return;
	}

	bIsPortalActive = bNewIsPortalActive;
	if (bIsPortalActive)
	{
		if (bLogPortalState)
		{
			UE_LOG(LogLpQuestPortal, Display, TEXT("Portal activated | Portal=%s"), *GetNameSafe(this));
		}
		OnPortalActivated();
	}
}

void ATunicPortalActor::SetPortalCharging(bool bNewIsCharging)
{
	if (bIsPortalCharging == bNewIsCharging)
	{
		return;
	}

	bIsPortalCharging = bNewIsCharging;
	if (bLogPortalState)
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal charging %s | Portal=%s | PresentLivingPlayers=%d/%d | Progress=%.2f"),
			bIsPortalCharging ? TEXT("started") : TEXT("paused"),
			*GetNameSafe(this),
			PresentLivingPlayerCount,
			RequiredLivingPlayerCount,
			ActivationProgress);
	}
	OnPortalChargingStateChanged(bIsPortalCharging);
}

void ATunicPortalActor::SetPortalReady(bool bNewIsReady)
{
	if (bIsPortalReady == bNewIsReady)
	{
		return;
	}

	bIsPortalReady = bNewIsReady;
	if (bIsPortalReady)
	{
		if (bLogPortalState)
		{
			UE_LOG(LogLpQuestPortal, Display, TEXT("Portal ready | Portal=%s"), *GetNameSafe(this));
		}
		OnPortalReady();
	}
}

void ATunicPortalActor::SetActivationProgress(float NewActivationProgress)
{
	const float ClampedProgress = FMath::Clamp(NewActivationProgress, 0.0f, 1.0f);
	if (FMath::IsNearlyEqual(ActivationProgress, ClampedProgress, 0.001f))
	{
		return;
	}

	ActivationProgress = ClampedProgress;
	OnPortalChargeChanged(ActivationProgress);
}

void ATunicPortalActor::SetPortalPlayerCounts(int32 NewRequiredLivingPlayerCount, int32 NewPresentLivingPlayerCount)
{
	if (RequiredLivingPlayerCount == NewRequiredLivingPlayerCount && PresentLivingPlayerCount == NewPresentLivingPlayerCount)
	{
		return;
	}

	RequiredLivingPlayerCount = NewRequiredLivingPlayerCount;
	PresentLivingPlayerCount = NewPresentLivingPlayerCount;

	if (bLogPortalState)
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal player count updated | Portal=%s | PresentLivingPlayers=%d/%d"),
			*GetNameSafe(this),
			PresentLivingPlayerCount,
			RequiredLivingPlayerCount);
	}
}

void ATunicPortalActor::OnRep_IsPortalActive()
{
	if (bIsPortalActive)
	{
		OnPortalActivated();
	}
}

void ATunicPortalActor::OnRep_IsPortalCharging()
{
	OnPortalChargingStateChanged(bIsPortalCharging);
}

void ATunicPortalActor::OnRep_IsPortalReady()
{
	if (bIsPortalReady)
	{
		OnPortalReady();
	}
}

void ATunicPortalActor::OnRep_ActivationProgress()
{
	OnPortalChargeChanged(ActivationProgress);
}

void ATunicPortalActor::OnRep_PortalResetSerial()
{
	OnPortalReset();
}
