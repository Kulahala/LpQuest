// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/LPQPickupActor.h"

#include "Character/LPQPlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/LPQDebugSettings.h"
#include "Engine/EngineTypes.h"
#include "Player/LPQPlayerState.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestPickup, Log, All);

namespace
{
	UStaticMesh* GetDefaultPickupSphereMesh()
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> PickupMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		return PickupMeshFinder.Object;
	}
}

ALPQPickupActor::ALPQPickupActor()
{
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(SceneRoot);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetRelativeScale3D(FVector(0.25f));
	PickupMesh->SetStaticMesh(GetDefaultPickupSphereMesh());
}

bool ALPQPickupActor::CanInteractWithTunicPlayer_Implementation(ALPQPlayerCharacter* InteractingPlayer)
{
	if (HasAuthority() && !bPickupConsumed && PickupId.IsNone())
	{
		UE_LOG(LogLpQuestPickup, Warning, TEXT("Pickup cannot interact: missing PickupId | Pickup=%s | Player=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InteractingPlayer));
	}

	return HasAuthority()
		&& !bPickupConsumed
		&& !PickupId.IsNone()
		&& InteractingPlayer
		&& !InteractingPlayer->IsDead()
		&& InteractingPlayer->GetPlayerState<ALPQPlayerState>();
}

void ALPQPickupActor::InteractWithTunicPlayer_Implementation(ALPQPlayerCharacter* InteractingPlayer)
{
	if (!HasAuthority() || bPickupConsumed)
	{
		return;
	}

	if (PickupId.IsNone())
	{
		UE_LOG(LogLpQuestPickup, Warning, TEXT("Pickup rejected: missing PickupId | Pickup=%s | Player=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InteractingPlayer));
		return;
	}

	if (!InteractingPlayer || InteractingPlayer->IsDead())
	{
		if (bLogPickupInteraction && FLPQDebugSettings::ShouldLogInteraction())
		{
			UE_LOG(LogLpQuestPickup, Display, TEXT("Pickup rejected: invalid or dead player | Pickup=%s | Player=%s | PickupId=%s"),
				*GetNameSafe(this),
				*GetNameSafe(InteractingPlayer),
				*PickupId.ToString());
		}
		return;
	}

	ALPQPlayerState* LpQuestPlayerState = InteractingPlayer->GetPlayerState<ALPQPlayerState>();
	if (!LpQuestPlayerState || !LpQuestPlayerState->SetCurrentEquipmentId(PickupId))
	{
		UE_LOG(LogLpQuestPickup, Warning, TEXT("Pickup rejected: failed to set CurrentEquipmentId | Pickup=%s | Player=%s | PlayerState=%s | PickupId=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(LpQuestPlayerState),
			*PickupId.ToString());
		return;
	}

	bPickupConsumed = true;
	if (bLogPickupInteraction && FLPQDebugSettings::ShouldLogInteraction())
	{
		UE_LOG(LogLpQuestPickup, Display, TEXT("Pickup consumed | Pickup=%s | Player=%s | PlayerState=%s | PickupId=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(LpQuestPlayerState),
			*PickupId.ToString());
	}

	OnPickedUp(InteractingPlayer);
	Destroy();
}

void ALPQPickupActor::OnPickedUp_Implementation(ALPQPlayerCharacter*)
{
}
