// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "TunicPlayerState.generated.h"

class UAbilitySystemComponent;
class UTunicAbilitySystemComponent;
class UTunicAttributeSet;

UCLASS(Blueprintable)
class LPQUEST_API ATunicPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATunicPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Ability")
	UTunicAbilitySystemComponent* GetTunicAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Attributes")
	UTunicAttributeSet* GetAttributeSet() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Ability")
	TObjectPtr<UTunicAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Attributes")
	TObjectPtr<UTunicAttributeSet> AttributeSet;
};

