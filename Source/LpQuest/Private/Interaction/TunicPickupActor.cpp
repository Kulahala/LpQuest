// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/TunicPickupActor.h"

#include "Character/TunicPlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/TunicDebugSettings.h"
#include "Engine/EngineTypes.h"
#include "Player/TunicPlayerState.h"
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

ATunicPickupActor::ATunicPickupActor()
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

bool ATunicPickupActor::CanInteractWithTunicPlayer_Implementation(ATunicPlayerCharacter* InteractingPlayer)
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
		&& InteractingPlayer->GetPlayerState<ATunicPlayerState>();
}

void ATunicPickupActor::InteractWithTunicPlayer_Implementation(ATunicPlayerCharacter* InteractingPlayer)
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
		if (bLogPickupInteraction && FTunicDebugSettings::ShouldLogInteraction())
		{
			UE_LOG(LogLpQuestPickup, Display, TEXT("Pickup rejected: invalid or dead player | Pickup=%s | Player=%s | PickupId=%s"),
				*GetNameSafe(this),
				*GetNameSafe(InteractingPlayer),
				*PickupId.ToString());
		}
		return;
	}

	ATunicPlayerState* TunicPlayerState = InteractingPlayer->GetPlayerState<ATunicPlayerState>();
	if (!TunicPlayerState || !TunicPlayerState->SetCurrentEquipmentId(PickupId))
	{
		UE_LOG(LogLpQuestPickup, Warning, TEXT("Pickup rejected: failed to set CurrentEquipmentId | Pickup=%s | Player=%s | PlayerState=%s | PickupId=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(TunicPlayerState),
			*PickupId.ToString());
		return;
	}

	bPickupConsumed = true;
	if (bLogPickupInteraction && FTunicDebugSettings::ShouldLogInteraction())
	{
		UE_LOG(LogLpQuestPickup, Display, TEXT("Pickup consumed | Pickup=%s | Player=%s | PlayerState=%s | PickupId=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InteractingPlayer),
			*GetNameSafe(TunicPlayerState),
			*PickupId.ToString());
	}

	OnPickedUp(InteractingPlayer);
	Destroy();
}

void ATunicPickupActor::OnPickedUp_Implementation(ATunicPlayerCharacter*)
{
}
