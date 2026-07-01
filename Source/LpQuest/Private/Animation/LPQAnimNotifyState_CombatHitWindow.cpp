// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/LPQAnimNotifyState_CombatHitWindow.h"

#include "Combat/LPQCombatHitWindowSourceInterface.h"
#include "Components/SkeletalMeshComponent.h"

void ULPQAnimNotifyState_CombatHitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	ILPQCombatHitWindowSourceInterface* HitWindowSource = MeshComp ? Cast<ILPQCombatHitWindowSourceInterface>(MeshComp->GetOwner()) : nullptr;
	if (!HitWindowSource)
	{
		return;
	}

	HitWindowSource->BeginCombatHitWindow(HitWindowName);
}

void ULPQAnimNotifyState_CombatHitWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	ILPQCombatHitWindowSourceInterface* HitWindowSource = MeshComp ? Cast<ILPQCombatHitWindowSourceInterface>(MeshComp->GetOwner()) : nullptr;
	if (!HitWindowSource)
	{
		return;
	}

	HitWindowSource->ProcessCombatHitWindow(HitWindowName);
}

void ULPQAnimNotifyState_CombatHitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	ILPQCombatHitWindowSourceInterface* HitWindowSource = MeshComp ? Cast<ILPQCombatHitWindowSourceInterface>(MeshComp->GetOwner()) : nullptr;
	if (!HitWindowSource)
	{
		return;
	}

	HitWindowSource->EndCombatHitWindow(HitWindowName);
}

FString ULPQAnimNotifyState_CombatHitWindow::GetNotifyName_Implementation() const
{
	return TEXT("Combat Hit Window");
}
