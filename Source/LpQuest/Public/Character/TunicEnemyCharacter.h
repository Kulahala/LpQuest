// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/TunicCharacterBase.h"
#include "TunicEnemyCharacter.generated.h"

UCLASS(Blueprintable)
class LPQUEST_API ATunicEnemyCharacter : public ATunicCharacterBase
{
	GENERATED_BODY()

public:
	ATunicEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
};

