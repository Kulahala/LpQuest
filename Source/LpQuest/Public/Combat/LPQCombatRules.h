// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/LPQCombatTypes.h"

class AActor;
class ILPQCombatTargetInterface;

struct LPQUEST_API FLPQCombatRules
{
	static bool IsSelfHit(const AActor* SourceActor, const AActor* TargetActor);
	static ELPQCombatTeam GetSourceTeam(const AActor* SourceActor);
	static bool CanTriggerHitReaction(const AActor* SourceActor, const AActor* TargetActor, const ILPQCombatTargetInterface& Target);
	static bool CanApplyDamage(const AActor* SourceActor, const AActor* TargetActor, const ILPQCombatTargetInterface& Target);
};
