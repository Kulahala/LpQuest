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
	EncounterCleared,
	FloorTransitionReady
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
	int32 GetCurrentFloorIndex() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run")
	bool IsCombatActive() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run")
	bool IsPartyWiped() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run")
	bool IsEncounterCleared() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Run")
	bool IsFloorTransitionReady() const;

	void SetRunState(ETunicRunState NewRunState);
	void SetCurrentFloorIndex(int32 NewFloorIndex);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Run")
	void OnRunStateChanged(ETunicRunState NewRunState);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Run")
	void OnFloorIndexChanged(int32 NewFloorIndex);

private:
	UFUNCTION()
	void OnRep_RunState();

	UFUNCTION()
	void OnRep_CurrentFloorIndex();

	UPROPERTY(ReplicatedUsing = OnRep_RunState)
	ETunicRunState RunState = ETunicRunState::CombatActive;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentFloorIndex)
	int32 CurrentFloorIndex = 1;
};

