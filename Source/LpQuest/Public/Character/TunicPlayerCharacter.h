// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/TunicCharacterBase.h"
#include "TunicPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class USpringArmComponent;

UCLASS(Blueprintable)
class LPQUEST_API ATunicPlayerCharacter : public ATunicCharacterBase
{
	GENERATED_BODY()

public:
	ATunicPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

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

private:
	void InitializePlayerAbilitySystem();
	void LogPlayerAbilitySystemDebug(const class ATunicPlayerState* TunicPlayerState, const class UTunicAbilitySystemComponent* PlayerAbilitySystemComponent, const class UTunicAttributeSet* PlayerAttributeSet) const;
	void RequestDodge();
	void HandleDodgeRequest();
	void LogDodgeRequestDebug() const;
	void RequestLightAttack();
	void HandleLightAttackRequest();
	void LogLightAttackRequestDebug() const;
	void LogServerInputRequestDebug(const TCHAR* RequestName, bool bShouldLog) const;

	UFUNCTION(Server, Reliable)
	void ServerRequestDodge();

	UFUNCTION(Server, Reliable)
	void ServerRequestLightAttack();
};

