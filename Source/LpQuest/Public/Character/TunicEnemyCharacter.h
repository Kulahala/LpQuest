// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/TunicCharacterBase.h"
#include "TunicEnemyCharacter.generated.h"

class FLifetimeProperty;
struct FOnAttributeChangeData;

UCLASS(Blueprintable)
class LPQUEST_API ATunicEnemyCharacter : public ATunicCharacterBase
{
	GENERATED_BODY()

public:
	ATunicEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Tunic|Combat")
	bool IsDead() const;

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug")
	void SetAbilitySystemInitializationLoggingEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Tunic|Debug")
	void SetAttributeDebugDrawEnabled(bool bEnabled);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Tunic|Combat")
	void OnDeathStateChanged(bool bNewIsDead);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bLogAbilitySystemInitialization = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bLogDeathState = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunic|Debug")
	bool bDrawAttributeDebug = true;

private:
	UFUNCTION()
	void OnRep_IsDead();

	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void SetDead(bool bNewIsDead);
	void ApplyDeathState();
	void LogEnemyAbilitySystemDebug() const;
	void DrawAttributeDebug() const;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	bool bDeathPresentationApplied = false;
};

