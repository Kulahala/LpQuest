// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/TunicAnimNotifyState_LightAttackHitWindow.h"

#include "Character/TunicPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UTunicAnimNotifyState_LightAttackHitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	ATunicPlayerCharacter* PlayerCharacter = MeshComp ? Cast<ATunicPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->BeginLightAttackHitWindow();
}

void UTunicAnimNotifyState_LightAttackHitWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	ATunicPlayerCharacter* PlayerCharacter = MeshComp ? Cast<ATunicPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->ProcessLightAttackHitWindow();
}

void UTunicAnimNotifyState_LightAttackHitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	ATunicPlayerCharacter* PlayerCharacter = MeshComp ? Cast<ATunicPlayerCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->EndLightAttackHitWindow();
}

FString UTunicAnimNotifyState_LightAttackHitWindow::GetNotifyName_Implementation() const
{
	return TEXT("Light Attack Hit Window");
}
