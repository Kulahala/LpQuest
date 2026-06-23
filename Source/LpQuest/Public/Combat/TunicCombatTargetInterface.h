// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TunicCombatTargetInterface.generated.h"

class AActor;
class UTunicAbilitySystemComponent;
class UTunicAttributeSet;

UINTERFACE(MinimalAPI)
class UTunicCombatTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class LPQUEST_API ITunicCombatTargetInterface
{
	GENERATED_BODY()

public:
	virtual bool IsCombatTargetAvailable() const = 0;
	virtual UTunicAbilitySystemComponent* GetCombatTargetAbilitySystemComponent() const = 0;
	virtual UTunicAttributeSet* GetCombatTargetAttributeSet() const = 0;
	virtual void HandleCombatTargetHitReaction(AActor* InstigatorActor) = 0;
};
