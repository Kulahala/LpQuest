// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunicEnemyPatrolPoint.generated.h"

class USceneComponent;
class UBillboardComponent;

UCLASS(Blueprintable)
class LPQUEST_API ATunicEnemyPatrolPoint : public AActor
{
	GENERATED_BODY()

public:
	ATunicEnemyPatrolPoint();

	UFUNCTION(BlueprintPure, Category = "Tunic|AI|Patrol")
	FName GetRouteId() const;

	UFUNCTION(BlueprintPure, Category = "Tunic|AI|Patrol")
	int32 GetOrderIndex() const;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tunic|AI|Patrol")
	FName RouteId = NAME_None;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tunic|AI|Patrol", meta = (ClampMin = "0"))
	int32 OrderIndex = 0;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|AI|Patrol", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRootComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tunic|AI|Patrol", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBillboardComponent> EditorBillboardComponent;
#endif
};
