// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "TunicAnimNotifyState_CombatHitWindow.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;
struct FAnimNotifyEventReference;

UCLASS(Const, CollapseCategories, meta = (DisplayName = "LPQ Combat Hit Window", ToolTip = "在 Montage 上标记战斗命中窗口。开始/结束窗口本身不直接造成伤害，命中由对应角色服务器逻辑确认。"))
class LPQUEST_API UTunicAnimNotifyState_CombatHitWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LPQ|Combat", meta = (AllowPrivateAccess = "true", ToolTip = "命中窗口名称。角色会按名称转发到对应的 Begin/Process/End CombatHitWindow 逻辑。"))
	FName HitWindowName = FName(TEXT("Default"));
};
