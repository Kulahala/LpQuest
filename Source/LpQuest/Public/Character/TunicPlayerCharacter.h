// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/TunicCharacterBase.h"
#include "TunicPlayerCharacter.generated.h"

class UCameraComponent;
class UGameplayAbility;
class UGameplayEffect;
class UInputAction;
class UTunicAbilitySystemComponent;
struct FInputActionValue;
struct FHitResult;
class USpringArmComponent;
class ATunicEnemyCharacter;

UCLASS(Blueprintable)
class LPQUEST_API ATunicPlayerCharacter : public ATunicCharacterBase
{
	GENERATED_BODY()

public:
	ATunicPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat")
	void BeginLightAttackHitWindow();

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat")
	void ProcessLightAttackHitWindow();

	UFUNCTION(BlueprintCallable, Category = "Tunic|Combat")
	void EndLightAttackHitWindow();

	void ExecuteLightAttackAbility();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input")
	TObjectPtr<UInputAction> DodgeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input")
	TObjectPtr<UInputAction> LightAttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input")
	TObjectPtr<UInputAction> HeavyAttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Input")
	TObjectPtr<UInputAction> SwitchWeaponAction;

	void Move(const FInputActionValue& Value);
	void StartDodge(const FInputActionValue& Value);
	void LightAttack(const FInputActionValue& Value);
	void HeavyAttack(const FInputActionValue& Value);
	void StartAim(const FInputActionValue& Value);
	void StopAim(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void SwitchWeapon(const FInputActionValue& Value);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input")
	void OnDodgeRequested();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input")
	void OnLightAttackRequested();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input")
	void OnHeavyAttackRequested();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input")
	void OnAimStarted();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input")
	void OnAimStopped();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input")
	void OnInteractRequested();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Input")
	void OnSwitchWeaponRequested();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bLogAbilitySystemInitialization = true;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug")
	void SetAbilitySystemInitializationLoggingEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bLogDodgeRequests = true;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug")
	void SetDodgeRequestLoggingEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bLogLightAttackRequests = true;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug")
	void SetLightAttackRequestLoggingEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bLogLightAttackHitSweep = true;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug")
	void SetLightAttackHitSweepLoggingEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug", meta = (DeprecatedFunction, DeprecationMessage = "Use SetLightAttackHitSweepLoggingEnabled."))
	void SetLightAttackTargetQueryLoggingEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation")
	bool bRunLightAttackHitWindowOnRequest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ClampMin = "0.0", Units = "cm"))
	float LightAttackSweepRadius = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (ClampMin = "0.0", Units = "cm"))
	float LightAttackSweepHalfHeight = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (Units = "cm"))
	FVector LightAttackSweepStartOffset = FVector(60.0f, 0.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Hit Confirmation", meta = (Units = "cm"))
	FVector LightAttackSweepEndOffset = FVector(225.0f, 0.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Combat|Debug")
	bool bApplyLightAttackDebugDamage = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Debug")
	TSubclassOf<UGameplayEffect> LightAttackDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Abilities")
	TSubclassOf<UGameplayAbility> LightAttackAbilityClass;

private:
	void InitializePlayerAbilitySystem();
	void GrantDefaultAbilities(UTunicAbilitySystemComponent* PlayerAbilitySystemComponent);
	void LogPlayerAbilitySystemDebug(const class ATunicPlayerState* TunicPlayerState, const class UTunicAbilitySystemComponent* PlayerAbilitySystemComponent, const class UTunicAttributeSet* PlayerAttributeSet) const;
	void RequestDodge();
	void HandleDodgeRequest();
	void LogDodgeRequestDebug() const;
	void RequestLightAttack();
	void HandleLightAttackRequest();
	bool TryActivateLightAttackAbility();
	void LogLightAttackRequestDebug() const;
	FVector GetLightAttackSweepPoint(const FVector& LocalOffset) const;
	void LogLightAttackHitSweepDebug(const TArray<FHitResult>& HitResults, int32 AppliedHitCount) const;
	void ApplyLightAttackDebugDamage(ATunicEnemyCharacter* TargetEnemy) const;
	void LogServerInputRequestDebug(const TCHAR* RequestName, bool bShouldLog) const;

	UFUNCTION(Server, Reliable)
	void ServerRequestDodge();

	UFUNCTION(Server, Reliable)
	void ServerRequestLightAttack();

	bool bLightAttackHitWindowActive = false;
	TSet<TWeakObjectPtr<ATunicEnemyCharacter>> LightAttackHitTargets;
};

