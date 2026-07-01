// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LPQCombatHitWindowSourceInterface.generated.h"

UINTERFACE(MinimalAPI)
class ULPQCombatHitWindowSourceInterface : public UInterface
{
	GENERATED_BODY()
};

class LPQUEST_API ILPQCombatHitWindowSourceInterface
{
	GENERATED_BODY()

public:
	virtual void BeginCombatHitWindow(FName HitWindowName) = 0;
	virtual void ProcessCombatHitWindow(FName HitWindowName) = 0;
	virtual void EndCombatHitWindow(FName HitWindowName) = 0;
};
