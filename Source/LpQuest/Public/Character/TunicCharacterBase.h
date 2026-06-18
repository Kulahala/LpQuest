// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "TunicCharacterBase.generated.h"

class UTunicAbilitySystemComponent;
class UTunicAttributeSet;
class UAbilitySystemComponent;

UCLASS(Abstract, Blueprintable)
class LPQUEST_API ATunicCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATunicCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Ability")
	UTunicAbilitySystemComponent* GetTunicAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Attributes")
	UTunicAttributeSet* GetAttributeSet() const;

protected:
	void SetAbilitySystemReferences(UTunicAbilitySystemComponent* InAbilitySystemComponent, UTunicAttributeSet* InAttributeSet);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Ability")
	TObjectPtr<UTunicAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Attributes")
	TObjectPtr<UTunicAttributeSet> AttributeSet;
};

