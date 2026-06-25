// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/TunicEnemyAIController.h"

#include "Character/TunicEnemyCharacter.h"
#include "Character/TunicPlayerCharacter.h"
#include "Combat/TunicCombatTargetInterface.h"
#include "Components/StateTreeAIComponent.h"
#include "Engine/World.h"
#include "Game/TunicGameState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "StateTree.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestEnemyAI, Log, All);

ATunicEnemyAIController::ATunicEnemyAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
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
	}
}

void ATunicEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	CurrentCombatTarget.Reset();

	if (!HasAuthority())
	{
		return;
	}

	EnsureControlledEnemyCanMove();
	StartEnemyStateTree();
}

void ATunicEnemyAIController::OnUnPossess()
{
	StopEnemyAILogic();
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
}

AActor* ATunicEnemyAIController::GetCurrentCombatTarget() const
{
	return CurrentCombatTarget.Get();
}

void ATunicEnemyAIController::ClearCurrentCombatTarget()
{
	CurrentCombatTarget.Reset();
	ClearFocus(EAIFocusPriority::Gameplay);
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
