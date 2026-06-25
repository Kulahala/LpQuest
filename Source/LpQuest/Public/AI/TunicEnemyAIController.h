// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TunicEnemyAIController.generated.h"

class ATunicEnemyCharacter;
class UStateTree;
class UStateTreeAIComponent;

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
	void SetCurrentCombatTarget(AActor* NewTarget);

	UFUNCTION(BlueprintPure, Category = "Tunic|AI")
	AActor* GetCurrentCombatTarget() const;

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

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|AI")
	TObjectPtr<UStateTree> DefaultEnemyStateTree;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI", meta = (ClampMin = "0.0", Units = "cm"))
	float TargetSearchRadius = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|AI", meta = (ClampMin = "0.0", Units = "cm"))
	float AttackActivationRange = 260.0f;

private:
	bool CanRunEnemyAI() const;
	bool IsRunCombatActive() const;
	ATunicEnemyCharacter* GetControlledEnemy() const;
	bool IsValidCombatTarget(AActor* TargetActor) const;
	void EnsureControlledEnemyCanMove() const;
	void StartEnemyStateTree();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentCombatTarget;
};
