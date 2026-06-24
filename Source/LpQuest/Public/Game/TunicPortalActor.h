// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunicPortalActor.generated.h"

class FLifetimeProperty;
class USceneComponent;
class USphereComponent;

UCLASS(Blueprintable)
class LPQUEST_API ATunicPortalActor : public AActor
{
	GENERATED_BODY()

public:
	ATunicPortalActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Portal")
	bool IsPortalActive() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Portal")
	bool IsPortalCharging() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Portal")
	bool IsPortalReady() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Portal")
	float GetActivationProgress() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Portal")
	int32 GetRequiredLivingPlayerCount() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|Portal")
	int32 GetPresentLivingPlayerCount() const;

	void ResetPortalForNextFloorStub();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Portal")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|Portal")
	TObjectPtr<USphereComponent> PortalRadiusPreview;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Portal", meta = (ClampMin = "1.0", Units = "cm"))
	float ActivationRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Portal", meta = (ClampMin = "0.0", Units = "s"))
	float ChargeDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bLogPortalState = true;

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Portal")
	void OnPortalActivated();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Portal")
	void OnPortalChargingStateChanged(bool bNewIsCharging);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Portal")
	void OnPortalChargeChanged(float NewProgress);

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Portal")
	void OnPortalReady();

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Portal")
	void OnPortalReset();

private:
	void TryActivatePortalFromRunState();
	void EvaluatePortalCharge(float DeltaSeconds);
	void CompletePortal();
	bool CountLivingPlayersInRange(int32& OutRequiredLivingPlayerCount, int32& OutPresentLivingPlayerCount) const;
	void UpdatePortalRadiusPreview();
	void SetPortalActive(bool bNewIsPortalActive);
	void SetPortalCharging(bool bNewIsCharging);
	void SetPortalReady(bool bNewIsReady);
	void SetActivationProgress(float NewActivationProgress);
	void SetPortalPlayerCounts(int32 NewRequiredLivingPlayerCount, int32 NewPresentLivingPlayerCount);

	UFUNCTION()
	void OnRep_IsPortalActive();

	UFUNCTION()
	void OnRep_IsPortalCharging();

	UFUNCTION()
	void OnRep_IsPortalReady();

	UFUNCTION()
	void OnRep_ActivationProgress();

	UFUNCTION()
	void OnRep_PortalResetSerial();

	UPROPERTY(ReplicatedUsing = OnRep_IsPortalActive)
	bool bIsPortalActive = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsPortalCharging)
	bool bIsPortalCharging = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsPortalReady)
	bool bIsPortalReady = false;

	UPROPERTY(ReplicatedUsing = OnRep_ActivationProgress)
	float ActivationProgress = 0.0f;

	UPROPERTY(Replicated)
	int32 RequiredLivingPlayerCount = 0;

	UPROPERTY(Replicated)
	int32 PresentLivingPlayerCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_PortalResetSerial)
	int32 PortalResetSerial = 0;
};
