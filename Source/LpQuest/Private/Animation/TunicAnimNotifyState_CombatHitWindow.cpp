// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/TunicAnimNotifyState_CombatHitWindow.h"

#include "Combat/TunicCombatHitWindowSourceInterface.h"
#include "Components/SkeletalMeshComponent.h"

void UTunicAnimNotifyState_CombatHitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	ITunicCombatHitWindowSourceInterface* HitWindowSource = MeshComp ? Cast<ITunicCombatHitWindowSourceInterface>(MeshComp->GetOwner()) : nullptr;
	if (!HitWindowSource)
	{
		return;
	}

	HitWindowSource->BeginCombatHitWindow(HitWindowName);
}

void UTunicAnimNotifyState_CombatHitWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	ITunicCombatHitWindowSourceInterface* HitWindowSource = MeshComp ? Cast<ITunicCombatHitWindowSourceInterface>(MeshComp->GetOwner()) : nullptr;
	if (!HitWindowSource)
	{
		return;
	}

	HitWindowSource->ProcessCombatHitWindow(HitWindowName);
}

void UTunicAnimNotifyState_CombatHitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	ITunicCombatHitWindowSourceInterface* HitWindowSource = MeshComp ? Cast<ITunicCombatHitWindowSourceInterface>(MeshComp->GetOwner()) : nullptr;
	if (!HitWindowSource)
	{
		return;
	}

	HitWindowSource->EndCombatHitWindow(HitWindowName);
}

FString UTunicAnimNotifyState_CombatHitWindow::GetNotifyName_Implementation() const
{
	return TEXT("Combat Hit Window");
}
