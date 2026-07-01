// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/LPQCombatRules.h"

#include "Combat/LPQCombatTargetInterface.h"
#include "GameFramework/Actor.h"

bool FLPQCombatRules::IsSelfHit(const AActor* SourceActor, const AActor* TargetActor)
{
	return SourceActor && TargetActor && SourceActor == TargetActor;
}

ELPQCombatTeam FLPQCombatRules::GetSourceTeam(const AActor* SourceActor)
{
	if (const ILPQCombatTargetInterface* CombatTarget = Cast<ILPQCombatTargetInterface>(SourceActor))
	{
		return CombatTarget->GetCombatTargetTeam();
	}

	return ELPQCombatTeam::Neutral;
}

bool FLPQCombatRules::CanTriggerHitReaction(const AActor* SourceActor, const AActor* TargetActor, const ILPQCombatTargetInterface& Target)
{
	if (IsSelfHit(SourceActor, TargetActor))
	{
		return false;
	}

	const ELPQCombatTeam SourceTeam = GetSourceTeam(SourceActor);
	const ELPQCombatTeam TargetTeam = Target.GetCombatTargetTeam();

	if (SourceTeam != ELPQCombatTeam::Neutral && SourceTeam == TargetTeam)
	{
		return true;
	}

	return CanApplyDamage(SourceActor, TargetActor, Target);
}

bool FLPQCombatRules::CanApplyDamage(const AActor* SourceActor, const AActor* TargetActor, const ILPQCombatTargetInterface& Target)
{
	if (IsSelfHit(SourceActor, TargetActor))
	{
		return false;
	}

	const ELPQCombatTeam SourceTeam = GetSourceTeam(SourceActor);
	const ELPQCombatTeam TargetTeam = Target.GetCombatTargetTeam();

	return (SourceTeam == ELPQCombatTeam::Player && TargetTeam == ELPQCombatTeam::Enemy)
		|| (SourceTeam == ELPQCombatTeam::Enemy && TargetTeam == ELPQCombatTeam::Player);
}
