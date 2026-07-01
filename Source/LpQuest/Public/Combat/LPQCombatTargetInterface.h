// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/LPQCombatTypes.h"
#include "UObject/Interface.h"
#include "LPQCombatTargetInterface.generated.h"

class AActor;
class ULPQAbilitySystemComponent;
class ULPQAttributeSet;

UINTERFACE(MinimalAPI)
class ULPQCombatTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class LPQUEST_API ILPQCombatTargetInterface
{
	GENERATED_BODY()

public:
	virtual bool IsCombatTargetAvailable() const = 0;
	virtual ELPQCombatTeam GetCombatTargetTeam() const = 0;
	virtual ULPQAbilitySystemComponent* GetCombatTargetAbilitySystemComponent() const = 0;
	virtual ULPQAttributeSet* GetCombatTargetAttributeSet() const = 0;
	virtual void HandleCombatTargetHitReaction(AActor* InstigatorActor) = 0;
};
