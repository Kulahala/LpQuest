// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TunicGameState.generated.h"

class FLifetimeProperty;

UENUM(BlueprintType)
enum class ETunicRunState : uint8
{
	CombatActive,
	PartyWiped,
	EncounterCleared
};

UCLASS(Blueprintable)
class LPQUEST_API ATunicGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ATunicGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run")
	ETunicRunState GetRunState() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run")
	bool IsCombatActive() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run")
	bool IsPartyWiped() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run")
	bool IsEncounterCleared() const;

	void SetRunState(ETunicRunState NewRunState);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Run")
	void OnRunStateChanged(ETunicRunState NewRunState);

private:
	UFUNCTION()
	void OnRep_RunState();

	UPROPERTY(ReplicatedUsing = OnRep_RunState)
	ETunicRunState RunState = ETunicRunState::CombatActive;
};

