// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/TunicCharacterBase.h"
#include "Combat/TunicCombatHitWindowSourceInterface.h"
#include "Combat/TunicCombatTargetInterface.h"
#include "TunicEnemyCharacter.generated.h"

class FLifetimeProperty;
class AActor;
class UAnimMontage;
class UGameplayAbility;
class UGameplayEffect;
struct FOnAttributeChangeData;
struct FHitResult;

UCLASS(Blueprintable)
class LPQUEST_API ATunicEnemyCharacter : public ATunicCharacterBase, public ITunicCombatTargetInterface, public ITunicCombatHitWindowSourceInterface
{
	GENERATED_BODY()

public:
	ATunicEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Combat")
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Rewards")
	int32 GetExperienceReward() const;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat")
	void TryActivateEnemyMeleeAttack();

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat")
	void BeginEnemyMeleeHitWindow();

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat")
	void ProcessEnemyMeleeHitWindow();

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat")
	void EndEnemyMeleeHitWindow();

	void ExecuteEnemyMeleeAttackAbility();
	void HandleHitReaction(AActor* InstigatorActor);
	virtual bool IsCombatTargetAvailable() const override;
	virtual ETunicCombatTeam GetCombatTargetTeam() const override;
	virtual UTunicAbilitySystemComponent* GetCombatTargetAbilitySystemComponent() const override;
	virtual UTunicAttributeSet* GetCombatTargetAttributeSet() const override;
	virtual void HandleCombatTargetHitReaction(AActor* InstigatorActor) override;
	virtual void BeginCombatHitWindow(FName HitWindowName) override;
	virtual void ProcessCombatHitWindow(FName HitWindowName) override;
	virtual void EndCombatHitWindow(FName HitWindowName) override;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug")
	void SetAbilitySystemInitializationLoggingEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug")
	void SetAttributeDebugDrawEnabled(bool bEnabled);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Combat")
	void OnDeathStateChanged(bool bNewIsDead);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Combat")
	void OnHitReaction(AActor* InstigatorActor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bLogAbilitySystemInitialization = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bLogDeathState = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bLogHitReaction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bDrawAttributeDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Prototype AI")
	bool bEnablePrototypeAutoAttack = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Prototype AI", meta = (ClampMin = "0.0", Units = "cm"))
	float PrototypeAutoAttackRange = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Prototype AI", meta = (ClampMin = "0.05", Units = "s"))
	float PrototypeAutoAttackInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation")
	bool bRunEnemyMeleeHitWindowOnRequest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ClampMin = "0.0", Units = "cm"))
	float EnemyMeleeSweepRadius = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ClampMin = "0.0", Units = "cm"))
	float EnemyMeleeSweepHalfHeight = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (Units = "cm"))
	FVector EnemyMeleeSweepStartOffset = FVector(55.0f, 0.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (Units = "cm"))
	FVector EnemyMeleeSweepEndOffset = FVector(190.0f, 0.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Debug")
	bool bApplyEnemyMeleeDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Debug")
	bool bLogEnemyMeleeAttack = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Damage")
	TSubclassOf<UGameplayEffect> MeleeAttackDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Animation")
	TObjectPtr<UAnimMontage> MeleeAttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Presentation")
	TObjectPtr<UAnimMontage> DefaultHitReactionMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Presentation")
	TObjectPtr<UAnimMontage> DefaultDeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Abilities")
	TSubclassOf<UGameplayAbility> DefaultMeleeAttackAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Rewards", meta = (ClampMin = "0"))
	int32 ExperienceReward = 1;

private:
	UFUNCTION()
	void OnRep_IsDead();

	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void GrantDefaultAbilities();
	void SetDead(bool bNewIsDead);
	void ApplyDeathState();
	void LogEnemyAbilitySystemDebug() const;
	void DrawAttributeDebug() const;
	void UpdatePrototypeAutoAttack(float DeltaSeconds);
	AActor* FindPrototypeAutoAttackTarget() const;
	bool PlayMeleeAttackMontage();
	void CheckMeleeAttackMontageHitWindowTriggered(int32 MontageSerial);
	FVector GetEnemyMeleeSweepPoint(const FVector& LocalOffset) const;
	void HandleEnemyMeleeTargetHit(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget);
	void ApplyEnemyMeleeDamage(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget);
	void LogEnemyMeleeHitSweepDebug(const TArray<FHitResult>& HitResults, int32 ProcessedHitCount) const;
	void PlayPresentationMontage(UAnimMontage* MontageToPlay, bool bStopAllMontages);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitReaction(AActor* InstigatorActor);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayMeleeAttackMontage(UAnimMontage* MontageToPlay);

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	bool bDeathPresentationApplied = false;
	bool bEnemyMeleeHitWindowActive = false;
	int32 MeleeAttackMontageActivationSerial = 0;
	int32 MeleeAttackMontageHitWindowSerial = 0;
	float PrototypeAutoAttackElapsedTime = 0.0f;
	TSet<TWeakObjectPtr<AActor>> EnemyMeleeHitTargets;
};

