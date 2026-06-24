// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Character/TunicCharacterBase.h"
#include "Combat/TunicCombatHitWindowSourceInterface.h"
#include "Combat/TunicCombatTargetInterface.h"
#include "TunicPlayerCharacter.generated.h"

class UCameraComponent;
class UGameplayAbility;
class UGameplayEffect;
class UInputAction;
class UAnimMontage;
class UTunicAbilitySystemComponent;
class AActor;
struct FInputActionValue;
struct FHitResult;
class USpringArmComponent;

UCLASS(Blueprintable)
class LPQUEST_API ATunicPlayerCharacter : public ATunicCharacterBase, public ITunicCombatTargetInterface, public ITunicCombatHitWindowSourceInterface
{
	GENERATED_BODY()

public:
	ATunicPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsCombatTargetAvailable() const override;
	virtual ETunicCombatTeam GetCombatTargetTeam() const override;
	virtual UTunicAbilitySystemComponent* GetCombatTargetAbilitySystemComponent() const override;
	virtual UTunicAttributeSet* GetCombatTargetAttributeSet() const override;
	virtual void HandleCombatTargetHitReaction(AActor* InstigatorActor) override;
	virtual void BeginCombatHitWindow(FName HitWindowName) override;
	virtual void ProcessCombatHitWindow(FName HitWindowName) override;
	virtual void EndCombatHitWindow(FName HitWindowName) override;

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

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Combat")
	void OnHitReaction(AActor* InstigatorActor);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bLogHitReaction = true;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug")
	void SetLightAttackRequestLoggingEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bLogLightAttackHitSweep = true;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug")
	void SetLightAttackHitSweepLoggingEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bDrawAttributeDebug = true;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug")
	void SetAttributeDebugDrawEnabled(bool bEnabled);

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Animation")
	TObjectPtr<UAnimMontage> LightAttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Combat|Presentation")
	TObjectPtr<UAnimMontage> DefaultHitReactionMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Abilities")
	TSubclassOf<UGameplayAbility> LightAttackAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tunic|Abilities")
	TSubclassOf<UGameplayEffect> StaminaRegenEffectClass;

private:
	bool GetMouseFacingYaw(float& OutYaw) const;
	void GetFixedViewMovementDirections(FVector& OutScreenUpDirection, FVector& OutScreenRightDirection) const;
	void UpdateFacingToMouse(float DeltaSeconds, bool bForceServerUpdate);
	void InitializePlayerAbilitySystem();
	void GrantDefaultAbilities(UTunicAbilitySystemComponent* PlayerAbilitySystemComponent);
	void ApplyDefaultEffects(UTunicAbilitySystemComponent* PlayerAbilitySystemComponent);
	void LogPlayerAbilitySystemDebug(const class ATunicPlayerState* TunicPlayerState, const class UTunicAbilitySystemComponent* PlayerAbilitySystemComponent, const class UTunicAttributeSet* PlayerAttributeSet) const;
	void RequestDodge();
	void HandleDodgeRequest();
	void LogDodgeRequestDebug() const;
	void RequestLightAttack();
	void HandleLightAttackRequest();
	bool TryActivateLightAttackAbility();
	bool PlayLightAttackMontage();
	void CheckLightAttackMontageHitWindowTriggered(int32 MontageSerial);
	void LogLightAttackRequestDebug() const;
	void DrawAttributeDebug() const;
	FVector GetLightAttackSweepPoint(const FVector& LocalOffset) const;
	void LogLightAttackHitSweepDebug(const TArray<FHitResult>& HitResults, int32 ProcessedHitCount) const;
	void ApplyLightAttackDebugDamage(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget);
	void HandleLightAttackTargetHit(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget);
	void LogServerInputRequestDebug(const TCHAR* RequestName, bool bShouldLog) const;

	UFUNCTION(Server, Reliable)
	void ServerRequestDodge();

	UFUNCTION(Server, Reliable)
	void ServerRequestLightAttack();

	UFUNCTION(Server, Unreliable)
	void ServerSetFacingYaw(float NewYaw, bool bSnap);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayLightAttackMontage(UAnimMontage* MontageToPlay);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitReaction(AActor* InstigatorActor);

	UPROPERTY(EditAnywhere, Category = "Tunic|Combat|Facing")
	bool bFaceMouseOnAttack = true;

	UPROPERTY(EditAnywhere, Category = "Tunic|Combat|Facing", meta = (ClampMin = "0.01", Units = "s"))
	float MouseFacingServerUpdateInterval = 0.05f;

	FActiveGameplayEffectHandle StaminaRegenEffectHandle;
	float TimeSinceLastMouseFacingServerUpdate = 0.0f;
	bool bLightAttackHitWindowActive = false;
	int32 LightAttackMontageActivationSerial = 0;
	int32 LightAttackMontageHitWindowSerial = 0;
	TSet<TWeakObjectPtr<AActor>> LightAttackHitTargets;
};

