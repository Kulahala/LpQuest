// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/TunicEnemyAIController.h"

#include "AI/TunicEnemyPatrolPoint.h"
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
		RefreshCurrentCombatTargetFromPerception();
	}
}

void ATunicEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	CurrentCombatTarget.Reset();
	PatrolRoutePoints.Reset();
	CurrentPatrolPointIndex = 0;
	HomeLocation = InPawn ? InPawn->GetActorLocation() : FVector::ZeroVector;
	CurrentTargetLostTimeSeconds = 0.0;
	bCurrentTargetPendingForget = false;

	if (!HasAuthority())
	{
		return;
	}

	ConfigureSightPerception();
	RebuildPatrolRoute();
	EnsureControlledEnemyCanMove();
	StartEnemyStateTree();
}

void ATunicEnemyAIController::OnUnPossess()
{
	StopEnemyAILogic();
	PatrolRoutePoints.Reset();
	CurrentPatrolPointIndex = 0;
	Super::OnUnPossess();
}

UStateTreeAIComponent* ATunicEnemyAIController::GetEnemyStateTreeComponent() const
{
	return StateTreeAIComponent;
}

AActor* ATunicEnemyAIController::FindNearestAvailableCombatTarget() const
{
	if (!CanRunEnemyAI())
	{
		return nullptr;
	}

	if (AActor* PerceivedTarget = FindBestPerceivedCombatTarget())
	{
		return PerceivedTarget;
	}

	const ATunicEnemyCharacter* EnemyCharacter = GetControlledEnemy();
	const UWorld* World = GetWorld();
	const ATunicGameState* TunicGameState = World ? World->GetGameState<ATunicGameState>() : nullptr;
	if (!EnemyCharacter || !TunicGameState)
	{
		return nullptr;
	}

	AActor* BestTarget = nullptr;
	float BestDistanceSquared = FMath::Square(FMath::Max(0.0f, TargetSearchRadius));
	const FVector QueryLocation = EnemyCharacter->GetActorLocation();

	for (APlayerState* CandidatePlayerState : TunicGameState->PlayerArray)
	{
		ATunicPlayerCharacter* PlayerCharacter = CandidatePlayerState ? CandidatePlayerState->GetPawn<ATunicPlayerCharacter>() : nullptr;
		AActor* TargetActor = PlayerCharacter;
		if (!IsValidCombatTarget(TargetActor))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(QueryLocation, TargetActor->GetActorLocation());
		if (DistanceSquared <= BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = TargetActor;
		}
	}

	return BestTarget;
}

void ATunicEnemyAIController::RefreshCurrentCombatTargetFromPerception()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!CanRunEnemyAI())
	{
		ClearCurrentCombatTarget();
		return;
	}

	AActor* TargetActor = CurrentCombatTarget.Get();
	if (IsValidCombatTarget(TargetActor) && IsCombatTargetCurrentlyPerceived(TargetActor))
	{
		SetCurrentCombatTarget(TargetActor);
		return;
	}

	if (AActor* BestPerceivedTarget = FindBestPerceivedCombatTarget())
	{
		SetCurrentCombatTarget(BestPerceivedTarget);
		bCurrentTargetPendingForget = false;
		CurrentTargetLostTimeSeconds = 0.0;
		return;
	}

	if (!IsValidCombatTarget(TargetActor))
	{
		ClearCurrentCombatTarget();
		bCurrentTargetPendingForget = false;
		CurrentTargetLostTimeSeconds = 0.0;
		return;
	}

	const UWorld* World = GetWorld();
	const double CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
	if (!bCurrentTargetPendingForget)
	{
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

	CurrentCombatTarget = IsValidCombatTarget(NewTarget) ? NewTarget : nullptr;
	if (AActor* TargetActor = CurrentCombatTarget.Get())
	{
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
	StopMovement();
	ClearCurrentCombatTarget();

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
	for (const TWeakObjectPtr<AActor>& PatrolPoint : PatrolRoutePoints)
	{
		if (PatrolPoint.IsValid())
		{
			return true;
		}
	}

	return false;
}

AActor* ATunicEnemyAIController::GetCurrentPatrolTarget() const
{
	if (PatrolRoutePoints.IsEmpty())
	{
		return nullptr;
	}

	const int32 PatrolPointCount = PatrolRoutePoints.Num();
	for (int32 Offset = 0; Offset < PatrolPointCount; ++Offset)
	{
		const int32 CandidateIndex = (CurrentPatrolPointIndex + Offset) % PatrolPointCount;
		if (AActor* PatrolPoint = PatrolRoutePoints[CandidateIndex].Get())
		{
			return PatrolPoint;
		}
	}

	return nullptr;
}

bool ATunicEnemyAIController::AdvancePatrolTarget()
{
	if (!HasAuthority() || PatrolRoutePoints.IsEmpty())
	{
		return false;
	}

	CurrentPatrolPointIndex = (CurrentPatrolPointIndex + 1) % PatrolRoutePoints.Num();
	return GetCurrentPatrolTarget() != nullptr;
}

FVector ATunicEnemyAIController::GetHomeLocation() const
{
	return HomeLocation;
}

void ATunicEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!HasAuthority() || !Actor)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed() && IsValidCombatTarget(Actor))
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
		const UWorld* World = GetWorld();
		bCurrentTargetPendingForget = true;
		CurrentTargetLostTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
		RefreshCurrentCombatTargetFromPerception();
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
	PatrolRoutePoints.Reset();
	CurrentPatrolPointIndex = 0;

	UWorld* World = GetWorld();
	if (!HasAuthority() || !World || PatrolRouteId.IsNone())
	{
		return;
	}

	TArray<ATunicEnemyPatrolPoint*> MatchedPatrolPoints;
	for (TActorIterator<ATunicEnemyPatrolPoint> It(World); It; ++It)
	{
		ATunicEnemyPatrolPoint* PatrolPoint = *It;
		if (PatrolPoint && PatrolPoint->GetRouteId() == PatrolRouteId)
		{
			MatchedPatrolPoints.Add(PatrolPoint);
		}
	}

	MatchedPatrolPoints.Sort([](const ATunicEnemyPatrolPoint& Left, const ATunicEnemyPatrolPoint& Right)
	{
		if (Left.GetOrderIndex() == Right.GetOrderIndex())
		{
			return Left.GetName() < Right.GetName();
		}

		return Left.GetOrderIndex() < Right.GetOrderIndex();
	});

	for (ATunicEnemyPatrolPoint* PatrolPoint : MatchedPatrolPoints)
	{
		PatrolRoutePoints.Add(PatrolPoint);
	}

	if (PatrolRoutePoints.IsEmpty())
	{
		UE_LOG(LogLpQuestEnemyAI, Display, TEXT("Enemy AI patrol route empty | Controller=%s | Enemy=%s | PatrolRouteId=%s"),
			*GetNameSafe(this),
			*GetNameSafe(GetPawn()),
			*PatrolRouteId.ToString());
	}
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
