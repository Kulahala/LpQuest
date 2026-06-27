// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/TunicGameplayAbility_Dodge.h"

#include "Ability/TunicDodgeCooldownGameplayEffect.h"
#include "Ability/TunicDodgeInvulnerabilityGameplayEffect.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/GameplayAbilityTriggerType.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Character/TunicPlayerCharacter.h"
#include "GameFramework/RootMotionSource.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestDodgeAbility, Log, All);

namespace
{
	const FName TunicDodgeAbilityEventTagName(TEXT("GameplayEvent.Movement.Dodge"));
}

UTunicGameplayAbility_Dodge::UTunicGameplayAbility_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	CooldownGameplayEffectClass = UTunicDodgeCooldownGameplayEffect::StaticClass();
	DodgeInvulnerabilityGameplayEffectClass = UTunicDodgeInvulnerabilityGameplayEffect::StaticClass();

	FGameplayTagContainer DefaultAbilityTags;
	if (const FGameplayTag DodgeTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Movement.Dodge"), false); DodgeTag.IsValid())
	{
		DefaultAbilityTags.AddTag(DodgeTag);
	}
	SetAssetTags(DefaultAbilityTags);

	if (const FGameplayTag CooldownDodgeTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Dodge"), false); CooldownDodgeTag.IsValid())
	{
		ActivationBlockedTags.AddTag(CooldownDodgeTag);
	}
	if (const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"), false); DeadTag.IsValid())
	{
		ActivationBlockedTags.AddTag(DeadTag);
	}
	if (const FGameplayTag ActionLockedTag = FGameplayTag::RequestGameplayTag(TEXT("State.ActionLocked"), false); ActionLockedTag.IsValid())
	{
		ActivationBlockedTags.AddTag(ActionLockedTag);
	}

	if (const FGameplayTag DodgeEventTag = FGameplayTag::RequestGameplayTag(TunicDodgeAbilityEventTagName, false); DodgeEventTag.IsValid())
	{
		FAbilityTriggerData DodgeEventTrigger;
		DodgeEventTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		DodgeEventTrigger.TriggerTag = DodgeEventTag;
		AbilityTriggers.Add(DodgeEventTrigger);
	}
}

void UTunicGameplayAbility_Dodge::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ATunicPlayerCharacter* PlayerCharacter = ActorInfo ? Cast<ATunicPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!PlayerCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector DodgeDirection = ResolveDodgeDirection(TriggerEventData, PlayerCharacter);
	if (DodgeDirection.IsNearlyZero())
	{
		UE_LOG(LogLpQuestDodgeAbility, Warning, TEXT("Dodge ability canceled: invalid direction | Character=%s | IsAuthority=%s"),
			*GetNameSafe(PlayerCharacter),
			ActorInfo && ActorInfo->IsNetAuthority() ? TEXT("true") : TEXT("false"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const bool bStartedPredictedMovement = ApplyPredictedDodgeMovement(PlayerCharacter, DodgeDirection);
	if (!bStartedPredictedMovement)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ActorInfo && ActorInfo->IsNetAuthority() && DodgeInvulnerabilityGameplayEffectClass)
	{
		FGameplayEffectSpecHandle InvulnerabilitySpecHandle = MakeOutgoingGameplayEffectSpec(
			Handle,
			ActorInfo,
			ActivationInfo,
			DodgeInvulnerabilityGameplayEffectClass,
			GetAbilityLevel(Handle, ActorInfo));
		if (InvulnerabilitySpecHandle.IsValid())
		{
			InvulnerabilitySpecHandle.Data->SetDuration(FMath::Max(0.0f, DodgeInvulnerabilityDuration), true);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, InvulnerabilitySpecHandle);
		}
	}

	UE_LOG(LogLpQuestDodgeAbility, Display, TEXT("Dodge ability activated | Character=%s | OwnerActor=%s | AvatarActor=%s | IsAuthority=%s | IsLocallyControlled=%s | Direction=%s | Distance=%.1f | Duration=%.3f | CooldownActionLockDuration=%.3f | InvulnerabilityDuration=%.3f"),
		*GetNameSafe(PlayerCharacter),
		*GetNameSafe(ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr),
		*GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr),
		ActorInfo && ActorInfo->IsNetAuthority() ? TEXT("true") : TEXT("false"),
		PlayerCharacter->IsLocallyControlled() ? TEXT("true") : TEXT("false"),
		*DodgeDirection.ToCompactString(),
		PlayerCharacter->GetDodgeDistance(),
		PlayerCharacter->GetDodgeDuration(),
		DodgeCooldownActionLockDuration,
		DodgeInvulnerabilityDuration);

	PlayerCharacter->ExecuteDodgeAbility();
}

void UTunicGameplayAbility_Dodge::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!CooldownGameplayEffectClass)
	{
		return;
	}

	FGameplayEffectSpecHandle CooldownSpecHandle = MakeOutgoingGameplayEffectSpec(
		Handle,
		ActorInfo,
		ActivationInfo,
		CooldownGameplayEffectClass,
		GetAbilityLevel(Handle, ActorInfo));
	if (!CooldownSpecHandle.IsValid())
	{
		return;
	}

	CooldownSpecHandle.Data->SetDuration(FMath::Max(0.0f, DodgeCooldownActionLockDuration), true);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpecHandle);
}

void UTunicGameplayAbility_Dodge::HandleDodgeRootMotionFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

FVector UTunicGameplayAbility_Dodge::ResolveDodgeDirection(const FGameplayEventData* TriggerEventData, const ATunicPlayerCharacter* PlayerCharacter) const
{
	if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
	{
		const FGameplayAbilityTargetData* TargetData = TriggerEventData->TargetData.Get(0);
		if (TargetData && TargetData->HasOrigin() && TargetData->HasEndPoint())
		{
			const FVector Origin = TargetData->GetOrigin().GetLocation();
			const FVector EndPoint = TargetData->GetEndPoint();
			const FVector EventDirection = (EndPoint - Origin).GetSafeNormal2D();
			if (!EventDirection.IsNearlyZero())
			{
				return EventDirection;
			}
		}
	}

	return PlayerCharacter ? PlayerCharacter->GetDodgeDirection() : FVector::ZeroVector;
}

bool UTunicGameplayAbility_Dodge::ApplyPredictedDodgeMovement(ATunicPlayerCharacter* PlayerCharacter, const FVector& DodgeDirection)
{
	if (!PlayerCharacter)
	{
		return false;
	}

	const float DodgeDistance = PlayerCharacter->GetDodgeDistance();
	const float DodgeDuration = PlayerCharacter->GetDodgeDuration();
	if (DodgeDistance <= 0.0f || DodgeDuration <= UE_SMALL_NUMBER)
	{
		UE_LOG(LogLpQuestDodgeAbility, Warning, TEXT("Dodge predicted movement skipped: invalid tuning | Character=%s | Distance=%.1f | Duration=%.3f"),
			*GetNameSafe(PlayerCharacter),
			DodgeDistance,
			DodgeDuration);
		return false;
	}

	UAbilityTask_ApplyRootMotionConstantForce* DodgeRootMotionTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this,
		DodgeRootMotionTaskName,
		DodgeDirection,
		DodgeDistance / DodgeDuration,
		DodgeDuration,
		false,
		nullptr,
		ERootMotionFinishVelocityMode::SetVelocity,
		FVector::ZeroVector,
		0.0f,
		true);
	if (!DodgeRootMotionTask)
	{
		UE_LOG(LogLpQuestDodgeAbility, Warning, TEXT("Dodge predicted movement skipped: failed to create root motion task | Character=%s"),
			*GetNameSafe(PlayerCharacter));
		return false;
	}

	DodgeRootMotionTask->OnFinish.AddDynamic(this, &UTunicGameplayAbility_Dodge::HandleDodgeRootMotionFinished);
	DodgeRootMotionTask->ReadyForActivation();
	return true;
}
