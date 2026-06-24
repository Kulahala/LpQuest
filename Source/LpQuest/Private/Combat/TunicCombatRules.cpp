// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/TunicCombatRules.h"

#include "Combat/TunicCombatTargetInterface.h"
#include "GameFramework/Actor.h"

bool FTunicCombatRules::IsSelfHit(const AActor* SourceActor, const AActor* TargetActor)
{
	return SourceActor && TargetActor && SourceActor == TargetActor;
}

ETunicCombatTeam FTunicCombatRules::GetSourceTeam(const AActor* SourceActor)
{
	if (const ITunicCombatTargetInterface* CombatTarget = Cast<ITunicCombatTargetInterface>(SourceActor))
	{
		return CombatTarget->GetCombatTargetTeam();
	}

	return ETunicCombatTeam::Neutral;
}

bool FTunicCombatRules::CanTriggerHitReaction(const AActor* SourceActor, const AActor* TargetActor, const ITunicCombatTargetInterface& Target)
{
	if (IsSelfHit(SourceActor, TargetActor))
	{
		return false;
	}

	const ETunicCombatTeam SourceTeam = GetSourceTeam(SourceActor);
	const ETunicCombatTeam TargetTeam = Target.GetCombatTargetTeam();

	if (SourceTeam != ETunicCombatTeam::Neutral && SourceTeam == TargetTeam)
	{
		return true;
	}

	return CanApplyDamage(SourceActor, TargetActor, Target);
}

bool FTunicCombatRules::CanApplyDamage(const AActor* SourceActor, const AActor* TargetActor, const ITunicCombatTargetInterface& Target)
{
	if (IsSelfHit(SourceActor, TargetActor))
	{
		return false;
	}

	const ETunicCombatTeam SourceTeam = GetSourceTeam(SourceActor);
	const ETunicCombatTeam TargetTeam = Target.GetCombatTargetTeam();

	return (SourceTeam == ETunicCombatTeam::Player && TargetTeam == ETunicCombatTeam::Enemy)
		|| (SourceTeam == ETunicCombatTeam::Enemy && TargetTeam == ETunicCombatTeam::Player);
}
