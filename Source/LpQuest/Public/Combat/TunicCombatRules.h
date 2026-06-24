// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/TunicCombatTypes.h"

class AActor;
class ITunicCombatTargetInterface;

struct LPQUEST_API FTunicCombatRules
{
	static bool IsSelfHit(const AActor* SourceActor, const AActor* TargetActor);
	static ETunicCombatTeam GetSourceTeam(const AActor* SourceActor);
	static bool CanTriggerHitReaction(const AActor* SourceActor, const AActor* TargetActor, const ITunicCombatTargetInterface& Target);
	static bool CanApplyDamage(const AActor* SourceActor, const AActor* TargetActor, const ITunicCombatTargetInterface& Target);
};
