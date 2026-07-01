// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/LPQPortalActor.h"

#include "Character/LPQEnemyCharacter.h"
#include "Character/LPQPlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/LPQDebugSettings.h"
#include "Engine/World.h"
#include "Game/LPQGameMode.h"
#include "Game/LPQGameState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestPortal, Log, All);

namespace
{
	UStaticMesh* GetDefaultPortalMarkerSphereMesh()
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> MarkerMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		return MarkerMeshFinder.Object;
	}
}

ALPQPortalActor::ALPQPortalActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PortalRadiusPreview = CreateDefaultSubobject<USphereComponent>(TEXT("PortalRadiusPreview"));
	PortalRadiusPreview->SetupAttachment(SceneRoot);
	PortalRadiusPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalRadiusPreview->SetSphereRadius(ActivationRadius);

	PortalVisualMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalVisualMarker"));
	PortalVisualMarker->SetupAttachment(SceneRoot);
	PortalVisualMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalVisualMarker->SetHiddenInGame(false);
	PortalVisualMarker->SetRelativeScale3D(FVector(0.5f));
	PortalVisualMarker->SetStaticMesh(GetDefaultPortalMarkerSphereMesh());
}

void ALPQPortalActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdatePortalRadiusPreview();
}

void ALPQPortalActor::BeginPlay()
{
	Super::BeginPlay();

	UpdatePortalRadiusPreview();
}

void ALPQPortalActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || bIsPortalReady || PortalCompletionMode != ELPQPortalCompletionMode::CombatEvent)
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

void ALPQPortalActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALPQPortalActor, bIsPortalActive);
	DOREPLIFETIME(ALPQPortalActor, bIsPortalCharging);
	DOREPLIFETIME(ALPQPortalActor, bIsPortalReady);
	DOREPLIFETIME(ALPQPortalActor, ActivationProgress);
	DOREPLIFETIME(ALPQPortalActor, RequiredLivingPlayerCount);
	DOREPLIFETIME(ALPQPortalActor, PresentLivingPlayerCount);
	DOREPLIFETIME(ALPQPortalActor, PortalResetSerial);
}

bool ALPQPortalActor::IsPortalActive() const
{
	return bIsPortalActive;
}

bool ALPQPortalActor::IsPortalCharging() const
{
	return bIsPortalCharging;
}

bool ALPQPortalActor::IsPortalReady() const
{
	return bIsPortalReady;
}

float ALPQPortalActor::GetActivationProgress() const
{
	return ActivationProgress;
}

int32 ALPQPortalActor::GetRequiredLivingPlayerCount() const
{
	return RequiredLivingPlayerCount;
}

int32 ALPQPortalActor::GetPresentLivingPlayerCount() const
{
	return PresentLivingPlayerCount;
}

float ALPQPortalActor::GetInteractionRadius() const
{
	return InteractionRadius;
}

float ALPQPortalActor::GetActivationRadius() const
{
	return ActivationRadius;
}

ELPQPortalCompletionMode ALPQPortalActor::GetPortalCompletionMode() const
{
	return PortalCompletionMode;
}

FName ALPQPortalActor::GetPortalDestinationId() const
{
	return PortalDestinationId;
}

void ALPQPortalActor::ResetPortalForNextFloorStub()
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
	if (bLogPortalState && FLPQDebugSettings::ShouldLogPortal())
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal reset for next floor stub | Portal=%s | ResetSerial=%d"),
			*GetNameSafe(this),
			PortalResetSerial);
	}
	OnPortalReset();
}

void ALPQPortalActor::OnPortalActivated_Implementation()
{
}

void ALPQPortalActor::OnPortalChargingStateChanged_Implementation(bool)
{
}

void ALPQPortalActor::OnPortalChargeChanged_Implementation(float)
{
}

void ALPQPortalActor::OnPortalReady_Implementation()
{
}

void ALPQPortalActor::OnPortalReset_Implementation()
{
}

bool ALPQPortalActor::CanInteractWithTunicPlayer_Implementation(ALPQPlayerCharacter* InteractingPlayer)
{
	if (!HasAuthority() || !InteractingPlayer || InteractingPlayer->IsDead() || bIsPortalReady)
	{
		return false;
	}

	if (PortalDestinationId.IsNone())
	{
		if (bLogPortalState)
		{
			UE_LOG(LogLpQuestPortal, Warning, TEXT("Portal interaction rejected: missing destination id | Portal=%s | Player=%s"),
				*GetNameSafe(this),
				*GetNameSafe(InteractingPlayer));
		}
		return false;
	}

	const ALPQGameState* LpQuestGameState = GetWorld() ? GetWorld()->GetGameState<ALPQGameState>() : nullptr;
	if (!LpQuestGameState || LpQuestGameState->IsPartyWiped() || LpQuestGameState->IsFloorTransitionReady())
	{
		return false;
	}

	if (ALPQGameMode* LpQuestGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALPQGameMode>() : nullptr)
	{
		if (LpQuestGameState->IsPortalEventActive() && !LpQuestGameMode->IsActivePortalEventOwner(this))
		{
			return false;
		}
	}

	const float InteractionRadiusSquared = FMath::Square(FMath::Max(1.0f, InteractionRadius));
	return FVector::DistSquared2D(InteractingPlayer->GetActorLocation(), GetActorLocation()) <= InteractionRadiusSquared;
}

void ALPQPortalActor::InteractWithTunicPlayer_Implementation(ALPQPlayerCharacter* InteractingPlayer)
{
	if (ALPQGameMode* LpQuestGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALPQGameMode>() : nullptr)
	{
		if (PortalCompletionMode == ELPQPortalCompletionMode::DirectFloorExit)
		{
			LpQuestGameMode->TryUseDirectFloorExitPortal(InteractingPlayer, this);
			return;
		}

		LpQuestGameMode->TryStartPortalEvent(InteractingPlayer, this);
	}
}

void ALPQPortalActor::TryActivatePortalFromRunState()
{
	if (bIsPortalActive)
	{
		return;
	}

	const ALPQGameState* LpQuestGameState = GetWorld() ? GetWorld()->GetGameState<ALPQGameState>() : nullptr;
	if (!LpQuestGameState || !LpQuestGameState->IsPortalEventActive())
	{
		return;
	}

	const ALPQGameMode* LpQuestGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALPQGameMode>() : nullptr;
	if (!LpQuestGameMode || !LpQuestGameMode->IsActivePortalEventOwner(this))
	{
		return;
	}

	SetPortalActive(true);
	ResetPortalPressureState();
	SpawnPortalBossIfNeeded();
}

void ALPQPortalActor::EvaluatePortalCharge(float DeltaSeconds)
{
	const ALPQGameState* LpQuestGameState = GetWorld() ? GetWorld()->GetGameState<ALPQGameState>() : nullptr;
	if (!LpQuestGameState || !LpQuestGameState->IsPortalEventActive())
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

void ALPQPortalActor::CompletePortal()
{
	if (bIsPortalReady)
	{
		return;
	}

	ALPQGameMode* LpQuestGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALPQGameMode>() : nullptr;
	if (!LpQuestGameMode || !LpQuestGameMode->IsActivePortalEventOwner(this))
	{
		return;
	}

	SetActivationProgress(1.0f);
	SetPortalCharging(false);
	SetPortalReady(true);
	CleanupPortalPressureEnemies();

	const bool bTransitionStarted = LpQuestGameMode->MarkFloorTransitionReady(PortalDestinationId);
	if (!bTransitionStarted)
	{
		SetPortalReady(false);
	}
}

void ALPQPortalActor::SpawnPortalBossIfNeeded()
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

	ALPQEnemyCharacter* SpawnedBoss = World->SpawnActor<ALPQEnemyCharacter>(PortalBossEnemyClass, SpawnTransform, SpawnParameters);
	SpawnedPortalBossEnemy = SpawnedBoss;
	bPortalBossSpawnFailed = !SpawnedBoss;

	if (SpawnedBoss && bLogPortalState && FLPQDebugSettings::ShouldLogPortal())
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal boss spawned | Portal=%s | Boss=%s | BossClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(SpawnedBoss),
			*GetNameSafe(PortalBossEnemyClass.Get()));
	}
	else if (!SpawnedBoss && bLogPortalState)
	{
		UE_LOG(LogLpQuestPortal, Warning, TEXT("Portal boss spawn failed | Portal=%s | BossClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(PortalBossEnemyClass.Get()));
	}
}

bool ALPQPortalActor::IsPortalBossDefeated() const
{
	if (!PortalBossEnemyClass)
	{
		return true;
	}

	if (bPortalBossSpawnFailed)
	{
		return false;
	}

	const ALPQEnemyCharacter* SpawnedBoss = SpawnedPortalBossEnemy.Get();
	return !SpawnedBoss || SpawnedBoss->IsDead();
}

void ALPQPortalActor::CleanupPortalBoss()
{
	if (ALPQEnemyCharacter* SpawnedBoss = SpawnedPortalBossEnemy.Get())
	{
		SpawnedBoss->Destroy();
	}

	SpawnedPortalBossEnemy.Reset();
	bPortalBossSpawnFailed = false;
}

void ALPQPortalActor::ResetPortalPressureState()
{
	SpawnedPortalPressureEnemies.Reset();
	PortalPressureSpawnTimer = FMath::Max(0.1f, PortalPressureSpawnInterval);
	PortalPressureSpawnPointIndex = 0;
}

void ALPQPortalActor::TickPortalPressureSpawns(float DeltaSeconds)
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

bool ALPQPortalActor::ShouldSpawnPortalPressure() const
{
	const ALPQGameState* LpQuestGameState = GetWorld() ? GetWorld()->GetGameState<ALPQGameState>() : nullptr;
	return HasAuthority()
		&& bIsPortalActive
		&& !bIsPortalReady
		&& LpQuestGameState
		&& LpQuestGameState->IsPortalEventActive()
		&& IsPortalBossDefeated()
		&& PortalPressureEnemyClass
		&& MaxAlivePortalPressureEnemies > 0
		&& GetAlivePortalPressureEnemyCount() < MaxAlivePortalPressureEnemies;
}

void ALPQPortalActor::SpawnPortalPressureEnemy()
{
	UWorld* World = GetWorld();
	if (!World || !PortalPressureEnemyClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ALPQEnemyCharacter* SpawnedEnemy = World->SpawnActor<ALPQEnemyCharacter>(PortalPressureEnemyClass, GetNextPortalPressureSpawnTransform(), SpawnParameters);
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
	if (bLogPortalState && FLPQDebugSettings::ShouldLogPortal())
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal pressure enemy spawned | Portal=%s | Enemy=%s | Alive=%d/%d"),
			*GetNameSafe(this),
			*GetNameSafe(SpawnedEnemy),
			GetAlivePortalPressureEnemyCount(),
			MaxAlivePortalPressureEnemies);
	}
}

FTransform ALPQPortalActor::GetNextPortalPressureSpawnTransform()
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

	if (bLogPortalState && FLPQDebugSettings::ShouldLogPortal())
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal pressure spawn using portal transform fallback | Portal=%s"),
			*GetNameSafe(this));
	}
	return GetActorTransform();
}

int32 ALPQPortalActor::GetAlivePortalPressureEnemyCount() const
{
	int32 AliveCount = 0;
	for (const TWeakObjectPtr<ALPQEnemyCharacter>& PressureEnemy : SpawnedPortalPressureEnemies)
	{
		const ALPQEnemyCharacter* EnemyCharacter = PressureEnemy.Get();
		if (EnemyCharacter && !EnemyCharacter->IsDead())
		{
			++AliveCount;
		}
	}
	return AliveCount;
}

void ALPQPortalActor::CleanupPortalPressureEnemies()
{
	for (const TWeakObjectPtr<ALPQEnemyCharacter>& PressureEnemy : SpawnedPortalPressureEnemies)
	{
		if (ALPQEnemyCharacter* EnemyCharacter = PressureEnemy.Get())
		{
			EnemyCharacter->Destroy();
		}
	}

	SpawnedPortalPressureEnemies.Reset();
}

bool ALPQPortalActor::CountLivingPlayersInRange(int32& OutRequiredLivingPlayerCount, int32& OutPresentLivingPlayerCount) const
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

		const ALPQPlayerCharacter* PlayerCharacter = PlayerState->GetPawn<ALPQPlayerCharacter>();
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

void ALPQPortalActor::UpdatePortalRadiusPreview()
{
	if (PortalRadiusPreview)
	{
		PortalRadiusPreview->SetSphereRadius(FMath::Max(1.0f, ActivationRadius));
	}
}

void ALPQPortalActor::SetPortalActive(bool bNewIsPortalActive)
{
	if (bIsPortalActive == bNewIsPortalActive)
	{
		return;
	}

	bIsPortalActive = bNewIsPortalActive;
	if (bIsPortalActive)
	{
		if (bLogPortalState && FLPQDebugSettings::ShouldLogPortal())
		{
			UE_LOG(LogLpQuestPortal, Display, TEXT("Portal activated | Portal=%s"), *GetNameSafe(this));
		}
		OnPortalActivated();
	}
}

void ALPQPortalActor::SetPortalCharging(bool bNewIsCharging)
{
	if (bIsPortalCharging == bNewIsCharging)
	{
		return;
	}

	bIsPortalCharging = bNewIsCharging;
	if (bLogPortalState && FLPQDebugSettings::ShouldLogPortal())
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

void ALPQPortalActor::SetPortalReady(bool bNewIsReady)
{
	if (bIsPortalReady == bNewIsReady)
	{
		return;
	}

	bIsPortalReady = bNewIsReady;
	if (bIsPortalReady)
	{
		if (bLogPortalState && FLPQDebugSettings::ShouldLogPortal())
		{
			UE_LOG(LogLpQuestPortal, Display, TEXT("Portal ready | Portal=%s"), *GetNameSafe(this));
		}
		OnPortalReady();
	}
}

void ALPQPortalActor::SetActivationProgress(float NewActivationProgress)
{
	const float ClampedProgress = FMath::Clamp(NewActivationProgress, 0.0f, 1.0f);
	if (FMath::IsNearlyEqual(ActivationProgress, ClampedProgress, 0.001f))
	{
		return;
	}

	ActivationProgress = ClampedProgress;
	OnPortalChargeChanged(ActivationProgress);
}

void ALPQPortalActor::SetPortalPlayerCounts(int32 NewRequiredLivingPlayerCount, int32 NewPresentLivingPlayerCount)
{
	if (RequiredLivingPlayerCount == NewRequiredLivingPlayerCount && PresentLivingPlayerCount == NewPresentLivingPlayerCount)
	{
		return;
	}

	RequiredLivingPlayerCount = NewRequiredLivingPlayerCount;
	PresentLivingPlayerCount = NewPresentLivingPlayerCount;

	if (bLogPortalState && FLPQDebugSettings::ShouldLogPortal())
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal player count updated | Portal=%s | PresentLivingPlayers=%d/%d"),
			*GetNameSafe(this),
			PresentLivingPlayerCount,
			RequiredLivingPlayerCount);
	}
}

void ALPQPortalActor::OnRep_IsPortalActive()
{
	if (bIsPortalActive)
	{
		OnPortalActivated();
	}
}

void ALPQPortalActor::OnRep_IsPortalCharging()
{
	OnPortalChargingStateChanged(bIsPortalCharging);
}

void ALPQPortalActor::OnRep_IsPortalReady()
{
	if (bIsPortalReady)
	{
		OnPortalReady();
	}
}

void ALPQPortalActor::OnRep_ActivationProgress()
{
	OnPortalChargeChanged(ActivationProgress);
}

void ALPQPortalActor::OnRep_PortalResetSerial()
{
	OnPortalReset();
}
