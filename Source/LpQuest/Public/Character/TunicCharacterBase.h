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

	UFUNCTION(BlueprintPure, Category = "LPQ|Ability", meta = (ToolTip = "返回该角色当前缓存的 LPQ ASC。玩家实际 ASC 归 PlayerState 持有，敌人 ASC 归敌人角色持有。"))
	UTunicAbilitySystemComponent* GetTunicAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category = "LPQ|Attributes", meta = (ToolTip = "返回该角色当前缓存的 AttributeSet。不要从蓝图直接改属性，属性变化应走 GAS / GameplayEffect。"))
	UTunicAttributeSet* GetAttributeSet() const;

protected:
	void SetAbilitySystemReferences(UTunicAbilitySystemComponent* InAbilitySystemComponent, UTunicAttributeSet* InAttributeSet);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LPQ|Ability")
	TObjectPtr<UTunicAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LPQ|Attributes")
	TObjectPtr<UTunicAttributeSet> AttributeSet;
};

