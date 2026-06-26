// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/TunicEnemyPatrolPoint.h"

#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"

ATunicEnemyPatrolPoint::ATunicEnemyPatrolPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRootComponent;

#if WITH_EDITORONLY_DATA
	EditorBillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorBillboard"));
	if (EditorBillboardComponent)
	{
		EditorBillboardComponent->SetupAttachment(SceneRootComponent);
		EditorBillboardComponent->bHiddenInGame = true;
	}
#endif
}

FName ATunicEnemyPatrolPoint::GetRouteId() const
{
	return RouteId;
}

int32 ATunicEnemyPatrolPoint::GetOrderIndex() const
{
	return OrderIndex;
}
