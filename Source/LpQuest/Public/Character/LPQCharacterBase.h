// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "LPQCharacterBase.generated.h"

class ULPQAbilitySystemComponent;
class ULPQAttributeSet;
class UAbilitySystemComponent;

UCLASS(Abstract, Blueprintable)
class LPQUEST_API ALPQCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ALPQCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "LPQ|Ability", meta = (ToolTip = "返回该角色当前缓存的 LPQ ASC。玩家实际 ASC 归 PlayerState 持有，敌人 ASC 归敌人角色持有。"))
	ULPQAbilitySystemComponent* GetTunicAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category = "LPQ|Attributes", meta = (ToolTip = "返回该角色当前缓存的 AttributeSet。不要从蓝图直接改属性，属性变化应走 GAS / GameplayEffect。"))
	ULPQAttributeSet* GetAttributeSet() const;

protected:
	void SetAbilitySystemReferences(ULPQAbilitySystemComponent* InAbilitySystemComponent, ULPQAttributeSet* InAttributeSet);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LPQ|Ability")
	TObjectPtr<ULPQAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LPQ|Attributes")
	TObjectPtr<ULPQAttributeSet> AttributeSet;
};

