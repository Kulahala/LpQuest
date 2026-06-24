// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TunicGameMode.generated.h"

UCLASS(Blueprintable)
class LPQUEST_API ATunicGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATunicGameMode();

	virtual void Logout(AController* Exiting) override;

	void EvaluatePartyWipe();
	void EvaluateEncounterClear();
	void MarkFloorTransitionReady();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Run", meta = (ClampMin = "0.0", Units = "s"))
	float FloorTransitionStubDelay = 2.0f;

private:
	void CompleteFloorTransitionStub();

	int32 EvaluatedFloorIndex = 1;
	bool bHasSeenLivingEnemyThisFloor = false;
};

