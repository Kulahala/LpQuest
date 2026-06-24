// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "TunicPlayerState.generated.h"

class UAbilitySystemComponent;
class UTunicAbilitySystemComponent;
class UTunicAttributeSet;
class FLifetimeProperty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTunicPendingRunUpgradeChoicesChangedSignature, int32, PendingChoices);

UCLASS(Blueprintable)
class LPQUEST_API ATunicPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATunicPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Ability")
	UTunicAbilitySystemComponent* GetTunicAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Attributes")
	UTunicAttributeSet* GetAttributeSet() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run")
	int32 GetPendingRunUpgradeChoices() const;

	UPROPERTY(BlueprintAssignable, Category = "Tunic|Run")
	FTunicPendingRunUpgradeChoicesChangedSignature OnPendingRunUpgradeChoicesChangedEvent;

	void AddPendingRunUpgradeChoices(int32 Amount);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Run")
	void OnPendingRunUpgradeChoicesChanged(int32 PendingChoices);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Ability")
	TObjectPtr<UTunicAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Attributes")
	TObjectPtr<UTunicAttributeSet> AttributeSet;

private:
	UFUNCTION()
	void OnRep_PendingRunUpgradeChoices();

	UPROPERTY(ReplicatedUsing = OnRep_PendingRunUpgradeChoices)
	int32 PendingRunUpgradeChoices = 0;
};

