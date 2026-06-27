// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/TunicEnemyAIController.h"

#include "AI/TunicEnemyPatrolRoute.h"
#include "Character/TunicEnemyCharacter.h"
#include "Character/TunicPlayerCharacter.h"
#include "Combat/TunicCombatTargetInterface.h"
#include "Components/StateTreeAIComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Game/TunicGameState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "StateTree.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestEnemyAI, Log, All);

ATunicEnemyAIController::ATunicEnemyAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EnemyPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("EnemyPerceptionComponent"));
	if (EnemyPerceptionComponent)
	{
		SetPerceptionComponent(*EnemyPerceptionComponent);
		EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ATunicEnemyAIController::HandleTargetPerceptionUpdated);
	}

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	ConfigureSightPerception();

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAIComponent"));
	BrainComponent = StateTreeAIComponent;
	if (StateTreeAIComponent)
	{
		StateTreeAIComponent->SetStartLogicAutomatically(false);
	}

	PrimaryActorTick.bCanEverTick = true;
}

void ATunicEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	if (StateTreeAIComponent && StateTreeAIComponent->IsRunning() && !CanRunEnemyAI())
	{
		StopEnemyAILogic();
		return;
	}

	if (CanRunEnemyAI())
	{
		RefreshCurrentCombatTargetFromAwareness();
		UpdateIdleScan(DeltaSeconds);
	}
}

void ATunicEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	CurrentCombatTarget.Reset();
	PatrolRouteActor.Reset();
	CurrentPatrolPointIndex = 0;
	HomeLocation = InPawn ? InPawn->GetActorLocation() : FVector::ZeroVector;
	LastKnownTargetLocation = FVector::ZeroVector;
	CurrentTargetLostTimeSeconds = 0.0;
	bCurrentTargetPendingForget = false;
	bHasLastKnownTargetLocation = false;
	bCombatSpawnAggroReady = AwarenessPolicy != ETunicEnemyAwarenessPolicy::CombatSpawnAggro;

	if (!HasAuthority())
	{
		return;
	}

	ConfigureSightPerception();
	RebuildPatrolRoute();
	EnsureControlledEnemyCanMove();
	StartEnemyStateTree();

	if (AwarenessPolicy == ETunicEnemyAwarenessPolicy::CombatSpawnAggro)
	{
		const float Delay = FMath::Max(0.0f, CombatSpawnAggroDelay);
		if (Delay <= 0.0f)
		{
			HandleCombatSpawnAggroDelayElapsed();
		}
		else if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(CombatSpawnAggroTimerHandle, this, &ATunicEnemyAIController::HandleCombatSpawnAggroDelayElapsed, Delay, false);
		}
	}
}

void ATunicEnemyAIController::OnUnPossess()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CombatSpawnAggroTimerHandle);
	}

	StopEnemyAILogic();
	PatrolRouteActor.Reset();
	CurrentPatrolPointIndex = 0;
	ClearLastKnownTargetLocation();
	Super::OnUnPossess();
}

UStateTreeAIComponent* ATunicEnemyAIController::GetEnemyStateTreeComponent() const
{
	return StateTreeAIComponent;
}

void ATunicEnemyAIController::RefreshCurrentCombatTargetFromAwareness()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!CanRunEnemyAI())
	{
		ClearCurrentCombatTarget();
		ClearLastKnownTargetLocation();
		return;
	}

	AActor* TargetActor = CurrentCombatTarget.Get();
	if (IsValidCombatTarget(TargetActor) && IsCombatTargetCurrentlyPerceived(TargetActor))
	{
		SetCurrentCombatTarget(TargetActor);
		return;
	}

	if ((AwarenessPolicy == ETunicEnemyAwarenessPolicy::SightAndProximity || (AwarenessPolicy == ETunicEnemyAwarenessPolicy::CombatSpawnAggro && bCombatSpawnAggroReady)) && IsCombatTargetWithinProximity(TargetActor, ProximityAggroRadius))
	{
		SetCurrentCombatTarget(TargetActor);
		return;
	}

	if (AwarenessPolicy != ETunicEnemyAwarenessPolicy::CombatSpawnAggro || bCombatSpawnAggroReady)
	{
		if (AActor* BestPerceivedTarget = FindBestPerceivedCombatTarget())
		{
			SetCurrentCombatTarget(BestPerceivedTarget);
			bCurrentTargetPendingForget = false;
			CurrentTargetLostTimeSeconds = 0.0;
			return;
		}
	}

	if ((AwarenessPolicy == ETunicEnemyAwarenessPolicy::SightAndProximity || (AwarenessPolicy == ETunicEnemyAwarenessPolicy::CombatSpawnAggro && bCombatSpawnAggroReady)) && !IsValidCombatTarget(TargetActor))
	{
		if (AActor* BestProximityTarget = FindBestProximityCombatTarget(ProximityAggroRadius))
		{
			SetCurrentCombatTarget(BestProximityTarget);
			bCurrentTargetPendingForget = false;
			CurrentTargetLostTimeSeconds = 0.0;
			return;
		}
	}

	if (!IsValidCombatTarget(TargetActor))
	{
		if (TargetActor)
		{
			ClearLastKnownTargetLocation();
		}
		ClearCurrentCombatTarget();
		bCurrentTargetPendingForget = false;
		CurrentTargetLostTimeSeconds = 0.0;
		return;
	}

	const UWorld* World = GetWorld();
	const double CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
	if (!bCurrentTargetPendingForget)
	{
		RecordLastKnownTargetLocation(TargetActor);
		bCurrentTargetPendingForget = true;
		CurrentTargetLostTimeSeconds = CurrentTimeSeconds;
	}

	if (CurrentTimeSeconds - CurrentTargetLostTimeSeconds >= FMath::Max(0.0f, AggroForgetDelay))
	{
		ClearCurrentCombatTarget();
		bCurrentTargetPendingForget = false;
		CurrentTargetLostTimeSeconds = 0.0;
	}
}

void ATunicEnemyAIController::SetCurrentCombatTarget(AActor* NewTarget)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!CanRunEnemyAI())
	{
		CurrentCombatTarget.Reset();
		ClearFocus(EAIFocusPriority::Gameplay);
		ClearLastKnownTargetLocation();
		bCurrentTargetPendingForget = false;
		CurrentTargetLostTimeSeconds = 0.0;
		return;
	}

	CurrentCombatTarget = IsValidCombatTarget(NewTarget) ? NewTarget : nullptr;
	if (AActor* TargetActor = CurrentCombatTarget.Get())
	{
		ClearLastKnownTargetLocation();
		SetFocus(TargetActor);
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}

	bCurrentTargetPendingForget = false;
	CurrentTargetLostTimeSeconds = 0.0;
}

AActor* ATunicEnemyAIController::GetCurrentCombatTarget() const
{
	return CurrentCombatTarget.Get();
}

bool ATunicEnemyAIController::HasCurrentCombatTarget() const
{
	return IsCurrentTargetValid();
}

void ATunicEnemyAIController::ClearCurrentCombatTarget()
{
	CurrentCombatTarget.Reset();
	ClearFocus(EAIFocusPriority::Gameplay);
	bCurrentTargetPendingForget = false;
	CurrentTargetLostTimeSeconds = 0.0;
}

bool ATunicEnemyAIController::IsCurrentTargetValid() const
{
	return IsValidCombatTarget(CurrentCombatTarget.Get());
}

bool ATunicEnemyAIController::IsCurrentTargetInAttackRange() const
{
	const ATunicEnemyCharacter* EnemyCharacter = GetControlledEnemy();
	const AActor* TargetActor = CurrentCombatTarget.Get();
	if (!EnemyCharacter || !IsValidCombatTarget(CurrentCombatTarget.Get()))
	{
		return false;
	}

	return FVector::DistSquared2D(EnemyCharacter->GetActorLocation(), TargetActor->GetActorLocation()) <= FMath::Square(FMath::Max(0.0f, AttackActivationRange));
}

bool ATunicEnemyAIController::TryActivateCurrentTargetMeleeAttack()
{
	if (!CanRunEnemyAI() || !IsCurrentTargetInAttackRange())
	{
		return false;
	}

	ATunicEnemyCharacter* EnemyCharacter = GetControlledEnemy();
	AActor* TargetActor = CurrentCombatTarget.Get();
	if (!EnemyCharacter || !TargetActor)
	{
		return false;
	}

	const FVector ToTarget = TargetActor->GetActorLocation() - EnemyCharacter->GetActorLocation();
	const FVector ToTarget2D(ToTarget.X, ToTarget.Y, 0.0f);
	if (!ToTarget2D.IsNearlyZero())
	{
		EnemyCharacter->SetActorRotation(ToTarget2D.Rotation());
	}

	const bool bActivated = EnemyCharacter->TryActivateEnemyMeleeAttack();
	return bActivated;
}

void ATunicEnemyAIController::StopEnemyAILogic()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CombatSpawnAggroTimerHandle);
	}

	StopMovement();
	ClearCurrentCombatTarget();
	ClearLastKnownTargetLocation();

	if (StateTreeAIComponent && StateTreeAIComponent->IsRunning())
	{
		StateTreeAIComponent->StopLogic(TEXT("Enemy AI stopped"));
	}
}

float ATunicEnemyAIController::GetAttackActivationRange() const
{
	return FMath::Max(0.0f, AttackActivationRange);
}

bool ATunicEnemyAIController::HasPatrolRoute() const
{
	const ATunicEnemyPatrolRoute* PatrolRoute = PatrolRouteActor.Get();
	return PatrolRoute && PatrolRoute->GetPatrolPointCount() > 0;
}

FVector ATunicEnemyAIController::GetCurrentPatrolLocation() const
{
	if (const ATunicEnemyPatrolRoute* PatrolRoute = PatrolRouteActor.Get())
	{
		return PatrolRoute->GetPatrolPointLocation(CurrentPatrolPointIndex);
	}

	return HomeLocation;
}

bool ATunicEnemyAIController::IsCurrentPatrolTargetStop() const
{
	if (const ATunicEnemyPatrolRoute* PatrolRoute = PatrolRouteActor.Get())
	{
		return PatrolRoute->IsPatrolPointStop(CurrentPatrolPointIndex);
	}

	return false;
}

float ATunicEnemyAIController::GetCurrentPatrolStopHoldDuration() const
{
	if (const ATunicEnemyPatrolRoute* PatrolRoute = PatrolRouteActor.Get())
	{
		return PatrolRoute->GetPatrolStopHoldDuration(CurrentPatrolPointIndex);
	}

	return 0.0f;
}

bool ATunicEnemyAIController::AdvancePatrolTarget()
{
	if (!HasAuthority())
	{
		return false;
	}

	if (const ATunicEnemyPatrolRoute* PatrolRoute = PatrolRouteActor.Get())
	{
		const int32 PointCount = PatrolRoute->GetPatrolPointCount();
		if (PointCount <= 0)
		{
			return false;
		}

		if (PatrolRoute->IsLoopRoute())
		{
			CurrentPatrolPointIndex = (CurrentPatrolPointIndex + 1) % PointCount;
		}
		else
		{
			CurrentPatrolPointIndex = FMath::Min(CurrentPatrolPointIndex + 1, PointCount - 1);
		}

		return true;
	}

	return false;
}

FVector ATunicEnemyAIController::GetHomeLocation() const
{
	return HomeLocation;
}

bool ATunicEnemyAIController::HasLastKnownTargetLocation() const
{
	return bHasLastKnownTargetLocation;
}

FVector ATunicEnemyAIController::GetLastKnownTargetLocation() const
{
	return bHasLastKnownTargetLocation ? LastKnownTargetLocation : HomeLocation;
}

void ATunicEnemyAIController::ClearLastKnownTargetLocation()
{
	if (!HasAuthority())
	{
		return;
	}

	LastKnownTargetLocation = FVector::ZeroVector;
	bHasLastKnownTargetLocation = false;
}

float ATunicEnemyAIController::GetInvestigationDuration() const
{
	return FMath::Max(0.0f, InvestigationDuration);
}

float ATunicEnemyAIController::GetInvestigationAcceptanceRadius() const
{
	return FMath::Max(0.0f, InvestigationAcceptanceRadius);
}

void ATunicEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!HasAuthority() || !Actor)
	{
		return;
	}

	if (!CanRunEnemyAI())
	{
		StopEnemyAILogic();
		return;
	}

	if (Stimulus.WasSuccessfullySensed() && IsValidCombatTarget(Actor) && (AwarenessPolicy != ETunicEnemyAwarenessPolicy::CombatSpawnAggro || bCombatSpawnAggroReady))
	{
		AActor* TargetActor = CurrentCombatTarget.Get();
		if (!IsValidCombatTarget(TargetActor) || !IsCombatTargetCurrentlyPerceived(TargetActor) || TargetActor == Actor)
		{
			SetCurrentCombatTarget(Actor);
		}
		return;
	}

	if (Actor == CurrentCombatTarget.Get())
	{
		RecordLastKnownTargetLocation(Actor);
		const UWorld* World = GetWorld();
		bCurrentTargetPendingForget = true;
		CurrentTargetLostTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
		RefreshCurrentCombatTargetFromAwareness();
	}
}

bool ATunicEnemyAIController::CanRunEnemyAI() const
{
	const ATunicEnemyCharacter* EnemyCharacter = GetControlledEnemy();
	return HasAuthority() && EnemyCharacter && !EnemyCharacter->IsDead() && IsRunCombatActive();
}

bool ATunicEnemyAIController::IsRunCombatActive() const
{
	const UWorld* World = GetWorld();
	const ATunicGameState* TunicGameState = World ? World->GetGameState<ATunicGameState>() : nullptr;
	return TunicGameState && TunicGameState->IsCombatActive();
}

ATunicEnemyCharacter* ATunicEnemyAIController::GetControlledEnemy() const
{
	return Cast<ATunicEnemyCharacter>(GetPawn());
}

bool ATunicEnemyAIController::IsValidCombatTarget(AActor* TargetActor) const
{
	const ITunicCombatTargetInterface* CombatTarget = Cast<ITunicCombatTargetInterface>(TargetActor);
	return TargetActor && CombatTarget && CombatTarget->IsCombatTargetAvailable() && CombatTarget->GetCombatTargetTeam() == ETunicCombatTeam::Player;
}

bool ATunicEnemyAIController::IsCombatTargetCurrentlyPerceived(AActor* TargetActor) const
{
	if (!IsValidCombatTarget(TargetActor) || !EnemyPerceptionComponent)
	{
		return false;
	}

	TArray<AActor*> PerceivedActors;
	EnemyPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	return PerceivedActors.Contains(TargetActor);
}

bool ATunicEnemyAIController::IsCombatTargetWithinProximity(AActor* TargetActor, float SearchRadius) const
{
	const ATunicEnemyCharacter* EnemyCharacter = GetControlledEnemy();
	if (!EnemyCharacter || !IsValidCombatTarget(TargetActor))
	{
		return false;
	}

	return FVector::DistSquared2D(EnemyCharacter->GetActorLocation(), TargetActor->GetActorLocation()) <= FMath::Square(FMath::Max(0.0f, SearchRadius));
}

AActor* ATunicEnemyAIController::FindBestPerceivedCombatTarget() const
{
	if (!CanRunEnemyAI() || !EnemyPerceptionComponent)
	{
		return nullptr;
	}

	const ATunicEnemyCharacter* EnemyCharacter = GetControlledEnemy();
	if (!EnemyCharacter)
	{
		return nullptr;
	}

	TArray<AActor*> PerceivedActors;
	EnemyPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	AActor* BestTarget = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	const FVector QueryLocation = EnemyCharacter->GetActorLocation();

	for (AActor* CandidateActor : PerceivedActors)
	{
		if (!IsValidCombatTarget(CandidateActor))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(QueryLocation, CandidateActor->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = CandidateActor;
		}
	}

	return BestTarget;
}

AActor* ATunicEnemyAIController::FindBestProximityCombatTarget(float SearchRadius) const
{
	if (!CanRunEnemyAI())
	{
		return nullptr;
	}

	const ATunicEnemyCharacter* EnemyCharacter = GetControlledEnemy();
	const UWorld* World = GetWorld();
	const ATunicGameState* TunicGameState = World ? World->GetGameState<ATunicGameState>() : nullptr;
	if (!EnemyCharacter || !TunicGameState)
	{
		return nullptr;
	}

	AActor* BestTarget = nullptr;
	float BestDistanceSquared = FMath::Square(FMath::Max(0.0f, SearchRadius));
	const FVector QueryLocation = EnemyCharacter->GetActorLocation();

	for (APlayerState* CandidatePlayerState : TunicGameState->PlayerArray)
	{
		ATunicPlayerCharacter* PlayerCharacter = CandidatePlayerState ? CandidatePlayerState->GetPawn<ATunicPlayerCharacter>() : nullptr;
		AActor* CandidateTarget = PlayerCharacter;
		if (!IsValidCombatTarget(CandidateTarget))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(QueryLocation, CandidateTarget->GetActorLocation());
		if (DistanceSquared <= BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = CandidateTarget;
		}
	}

	return BestTarget;
}

void ATunicEnemyAIController::RecordLastKnownTargetLocation(AActor* TargetActor)
{
	if (!HasAuthority() || !IsValidCombatTarget(TargetActor))
	{
		return;
	}

	LastKnownTargetLocation = TargetActor->GetActorLocation();
	bHasLastKnownTargetLocation = true;
}

void ATunicEnemyAIController::HandleCombatSpawnAggroDelayElapsed()
{
	if (!HasAuthority() || AwarenessPolicy != ETunicEnemyAwarenessPolicy::CombatSpawnAggro)
	{
		return;
	}

	bCombatSpawnAggroReady = true;

	if (!CanRunEnemyAI() || IsCurrentTargetValid())
	{
		return;
	}

	if (AActor* BestProximityTarget = FindBestProximityCombatTarget(ProximityAggroRadius))
	{
		SetCurrentCombatTarget(BestProximityTarget);
	}
}

void ATunicEnemyAIController::UpdateIdleScan(float DeltaSeconds)
{
	if (!bEnableIdleScan || IdleScanYawRate <= 0.0f || IsCurrentTargetValid())
	{
		return;
	}

	ATunicEnemyCharacter* EnemyCharacter = GetControlledEnemy();
	if (!EnemyCharacter || EnemyCharacter->IsDead())
	{
		return;
	}

	if (!bHasLastKnownTargetLocation && HasPatrolRoute())
	{
		return;
	}

	const FVector ScanAnchorLocation = bHasLastKnownTargetLocation ? LastKnownTargetLocation : HomeLocation;
	const float ScanTolerance = bHasLastKnownTargetLocation ? InvestigationAcceptanceRadius : IdleScanHomeTolerance;
	if (FVector::DistSquared2D(EnemyCharacter->GetActorLocation(), ScanAnchorLocation) > FMath::Square(FMath::Max(0.0f, ScanTolerance)))
	{
		return;
	}

	FRotator NewRotation = EnemyCharacter->GetActorRotation();
	NewRotation.Yaw += IdleScanYawRate * DeltaSeconds;
	EnemyCharacter->SetActorRotation(NewRotation);
}

void ATunicEnemyAIController::EnsureControlledEnemyCanMove() const
{
	ATunicEnemyCharacter* EnemyCharacter = GetControlledEnemy();
	UCharacterMovementComponent* MovementComponent = EnemyCharacter ? EnemyCharacter->GetCharacterMovement() : nullptr;
	if (!HasAuthority() || !EnemyCharacter || EnemyCharacter->IsDead() || !MovementComponent)
	{
		return;
	}

	if (MovementComponent->MovementMode == MOVE_None)
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
	}
}

void ATunicEnemyAIController::ConfigureSightPerception()
{
	if (!EnemyPerceptionComponent || !SightConfig)
	{
		return;
	}

	SightConfig->SightRadius = FMath::Max(0.0f, SightRadius);
	SightConfig->LoseSightRadius = FMath::Max(SightConfig->SightRadius, LoseSightRadius);
	SightConfig->PeripheralVisionAngleDegrees = FMath::Clamp(PeripheralVisionAngleDegrees, 0.0f, 180.0f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	EnemyPerceptionComponent->ConfigureSense(*SightConfig);
	EnemyPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	EnemyPerceptionComponent->RequestStimuliListenerUpdate();
}

void ATunicEnemyAIController::RebuildPatrolRoute()
{
	PatrolRouteActor.Reset();
	CurrentPatrolPointIndex = 0;

	UWorld* World = GetWorld();
	if (!HasAuthority() || !World || PatrolRouteId.IsNone())
	{
		if (HasAuthority() && PatrolRouteId.IsNone())
		{
			UE_LOG(LogLpQuestEnemyAI, Verbose, TEXT("Enemy AI patrol route disabled: PatrolRouteId is None | Controller=%s | Enemy=%s"),
				*GetNameSafe(this),
				*GetNameSafe(GetPawn()));
		}
		return;
	}

	TArray<FString> AvailableRouteIds;
	for (TActorIterator<ATunicEnemyPatrolRoute> It(World); It; ++It)
	{
		ATunicEnemyPatrolRoute* PatrolRoute = *It;
		if (!PatrolRoute)
		{
			continue;
		}

		AvailableRouteIds.Add(FString::Printf(TEXT("%s(%s:%d)"),
			*GetNameSafe(PatrolRoute),
			*PatrolRoute->GetRouteId().ToString(),
			PatrolRoute->GetPatrolPointCount()));

		if (PatrolRoute && PatrolRoute->GetRouteId() == PatrolRouteId && PatrolRoute->GetPatrolPointCount() > 0)
		{
			PatrolRouteActor = PatrolRoute;
			UE_LOG(LogLpQuestEnemyAI, Display, TEXT("Enemy AI patrol route bound | Controller=%s | Enemy=%s | PatrolRoute=%s | PatrolRouteId=%s | PointCount=%d | Loop=%s"),
				*GetNameSafe(this),
				*GetNameSafe(GetPawn()),
				*GetNameSafe(PatrolRoute),
				*PatrolRouteId.ToString(),
				PatrolRoute->GetPatrolPointCount(),
				PatrolRoute->IsLoopRoute() ? TEXT("true") : TEXT("false"));
			return;
		}
	}

	UE_LOG(LogLpQuestEnemyAI, Display, TEXT("Enemy AI patrol route empty | Controller=%s | Enemy=%s | PatrolRouteId=%s | AvailableRoutes=[%s]"),
		*GetNameSafe(this),
		*GetNameSafe(GetPawn()),
		*PatrolRouteId.ToString(),
		*FString::Join(AvailableRouteIds, TEXT(", ")));
}

void ATunicEnemyAIController::StartEnemyStateTree()
{
	EnsureControlledEnemyCanMove();

	if (!CanRunEnemyAI())
	{
		return;
	}

	if (!StateTreeAIComponent)
	{
		UE_LOG(LogLpQuestEnemyAI, Warning, TEXT("Enemy AI cannot start: missing StateTreeAIComponent | Controller=%s | Enemy=%s"),
			*GetNameSafe(this),
			*GetNameSafe(GetPawn()));
		return;
	}

	if (!DefaultEnemyStateTree)
	{
		UE_LOG(LogLpQuestEnemyAI, Warning, TEXT("Enemy AI idle: no DefaultEnemyStateTree configured | Controller=%s | Enemy=%s"),
			*GetNameSafe(this),
			*GetNameSafe(GetPawn()));
		return;
	}

	StateTreeAIComponent->SetStateTree(DefaultEnemyStateTree);
	StateTreeAIComponent->StartLogic();
}
