// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "TimerManager.h"
#include "TunicEnemyAIController.generated.h"

class ATunicEnemyCharacter;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UStateTree;
class UStateTreeAIComponent;

UENUM(BlueprintType)
enum class ETunicEnemyAwarenessPolicy : uint8
{
	SightOnlyPatrol,
	SightAndProximity,
	CombatSpawnAggro
};

UCLASS(Blueprintable)
class LPQUEST_API ATunicEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ATunicEnemyAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI")
	UStateTreeAIComponent* GetEnemyStateTreeComponent() const;

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI")
	AActor* FindNearestAvailableCombatTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI")
	void RefreshCurrentCombatTargetFromPerception();

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI")
	void RefreshCurrentCombatTargetFromAwareness();

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI")
	void SetCurrentCombatTarget(AActor* NewTarget);

	UFUNCTION(BlueprintPure, Category = "Tunic|AI")
	AActor* GetCurrentCombatTarget() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI")
	bool HasCurrentCombatTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI")
	void ClearCurrentCombatTarget();

	UFUNCTION(BlueprintPure, Category = "Tunic|AI")
	bool IsCurrentTargetValid() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI")
	bool IsCurrentTargetInAttackRange() const;

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI")
	bool TryActivateCurrentTargetMeleeAttack();

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI")
	void StopEnemyAILogic();

	UFUNCTION(BlueprintPure, Category = "Tunic|AI")
	float GetAttackActivationRange() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI")
	bool HasPatrolRoute() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI")
	AActor* GetCurrentPatrolTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Tunic|AI")
	bool AdvancePatrolTarget();

	UFUNCTION(BlueprintPure, Category = "Tunic|AI")
	FVector GetHomeLocation() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|AI")
	TObjectPtr<UStateTree> DefaultEnemyStateTree;

	UPROPERTY()
	float TargetSearchRadius_DEPRECATED = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI", meta = (ClampMin = "0.0", Units = "cm"))
	float AttackActivationRange = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Perception", meta = (ClampMin = "0.0", Units = "cm"))
	float SightRadius = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Perception", meta = (ClampMin = "0.0", Units = "cm"))
	float LoseSightRadius = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Perception", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float PeripheralVisionAngleDegrees = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Aggro", meta = (ClampMin = "0.0", Units = "s"))
	float AggroForgetDelay = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Aggro")
	ETunicEnemyAwarenessPolicy AwarenessPolicy = ETunicEnemyAwarenessPolicy::SightOnlyPatrol;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Aggro", meta = (ClampMin = "0.0", Units = "cm"))
	float ProximityAggroRadius = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Aggro", meta = (ClampMin = "0.0", Units = "s"))
	float CombatSpawnAggroDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Patrol")
	FName PatrolRouteId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Idle", meta = (ClampMin = "0.0", Units = "deg/s"))
	float IdleScanYawRate = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Idle", meta = (ClampMin = "0.0", Units = "cm"))
	float IdleScanHomeTolerance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI|Idle")
	bool bEnableIdleScan = true;

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	bool CanRunEnemyAI() const;
	bool IsRunCombatActive() const;
	ATunicEnemyCharacter* GetControlledEnemy() const;
	bool IsValidCombatTarget(AActor* TargetActor) const;
	bool IsCombatTargetCurrentlyPerceived(AActor* TargetActor) const;
	bool IsCombatTargetWithinProximity(AActor* TargetActor, float SearchRadius) const;
	AActor* FindBestPerceivedCombatTarget() const;
	AActor* FindBestProximityCombatTarget(float SearchRadius) const;
	void HandleCombatSpawnAggroDelayElapsed();
	void UpdateIdleScan(float DeltaSeconds);
	void EnsureControlledEnemyCanMove() const;
	void ConfigureSightPerception();
	void RebuildPatrolRoute();
	void StartEnemyStateTree();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> EnemyPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentCombatTarget;

	TArray<TWeakObjectPtr<AActor>> PatrolRoutePoints;
	int32 CurrentPatrolPointIndex = 0;
	FVector HomeLocation = FVector::ZeroVector;
	double CurrentTargetLostTimeSeconds = 0.0;
	bool bCurrentTargetPendingForget = false;
	bool bCombatSpawnAggroReady = false;
	FTimerHandle CombatSpawnAggroTimerHandle;
};
