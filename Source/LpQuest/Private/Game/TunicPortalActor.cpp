// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/TunicPortalActor.h"

#include "Character/TunicPlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Game/TunicGameMode.h"
#include "Game/TunicGameState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestPortal, Log, All);

ATunicPortalActor::ATunicPortalActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PortalRadiusPreview = CreateDefaultSubobject<USphereComponent>(TEXT("PortalRadiusPreview"));
	PortalRadiusPreview->SetupAttachment(SceneRoot);
	PortalRadiusPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalRadiusPreview->SetSphereRadius(ActivationRadius);
}

void ATunicPortalActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdatePortalRadiusPreview();
}

void ATunicPortalActor::BeginPlay()
{
	Super::BeginPlay();

	UpdatePortalRadiusPreview();
}

void ATunicPortalActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || bIsPortalReady)
	{
		return;
	}

	TryActivatePortalFromRunState();
	if (bIsPortalActive)
	{
		EvaluatePortalCharge(DeltaSeconds);
	}
}

void ATunicPortalActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATunicPortalActor, bIsPortalActive);
	DOREPLIFETIME(ATunicPortalActor, bIsPortalCharging);
	DOREPLIFETIME(ATunicPortalActor, bIsPortalReady);
	DOREPLIFETIME(ATunicPortalActor, ActivationProgress);
	DOREPLIFETIME(ATunicPortalActor, RequiredLivingPlayerCount);
	DOREPLIFETIME(ATunicPortalActor, PresentLivingPlayerCount);
	DOREPLIFETIME(ATunicPortalActor, PortalResetSerial);
}

bool ATunicPortalActor::IsPortalActive() const
{
	return bIsPortalActive;
}

bool ATunicPortalActor::IsPortalCharging() const
{
	return bIsPortalCharging;
}

bool ATunicPortalActor::IsPortalReady() const
{
	return bIsPortalReady;
}

float ATunicPortalActor::GetActivationProgress() const
{
	return ActivationProgress;
}

int32 ATunicPortalActor::GetRequiredLivingPlayerCount() const
{
	return RequiredLivingPlayerCount;
}

int32 ATunicPortalActor::GetPresentLivingPlayerCount() const
{
	return PresentLivingPlayerCount;
}

float ATunicPortalActor::GetInteractionRadius() const
{
	return InteractionRadius;
}

void ATunicPortalActor::ResetPortalForNextFloorStub()
{
	if (!HasAuthority())
	{
		return;
	}

	SetPortalCharging(false);
	SetPortalReady(false);
	SetPortalActive(false);
	SetActivationProgress(0.0f);
	SetPortalPlayerCounts(0, 0);

	++PortalResetSerial;
	if (bLogPortalState)
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal reset for next floor stub | Portal=%s | ResetSerial=%d"),
			*GetNameSafe(this),
			PortalResetSerial);
	}
	OnPortalReset();
}

void ATunicPortalActor::OnPortalActivated_Implementation()
{
}

void ATunicPortalActor::OnPortalChargingStateChanged_Implementation(bool)
{
}

void ATunicPortalActor::OnPortalChargeChanged_Implementation(float)
{
}

void ATunicPortalActor::OnPortalReady_Implementation()
{
}

void ATunicPortalActor::OnPortalReset_Implementation()
{
}

bool ATunicPortalActor::CanInteractWithTunicPlayer_Implementation(ATunicPlayerCharacter* InteractingPlayer)
{
	if (!HasAuthority() || !InteractingPlayer || InteractingPlayer->IsDead() || bIsPortalReady)
	{
		return false;
	}

	const ATunicGameState* TunicGameState = GetWorld() ? GetWorld()->GetGameState<ATunicGameState>() : nullptr;
	if (!TunicGameState || TunicGameState->IsPartyWiped() || TunicGameState->IsFloorTransitionReady())
	{
		return false;
	}

	const float InteractionRadiusSquared = FMath::Square(FMath::Max(1.0f, InteractionRadius));
	return FVector::DistSquared2D(InteractingPlayer->GetActorLocation(), GetActorLocation()) <= InteractionRadiusSquared;
}

void ATunicPortalActor::InteractWithTunicPlayer_Implementation(ATunicPlayerCharacter* InteractingPlayer)
{
	if (ATunicGameMode* TunicGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ATunicGameMode>() : nullptr)
	{
		TunicGameMode->TryStartPortalEvent(InteractingPlayer, this);
	}
}

void ATunicPortalActor::TryActivatePortalFromRunState()
{
	if (bIsPortalActive)
	{
		return;
	}

	const ATunicGameState* TunicGameState = GetWorld() ? GetWorld()->GetGameState<ATunicGameState>() : nullptr;
	if (!TunicGameState || !TunicGameState->IsPortalEventActive())
	{
		return;
	}

	SetPortalActive(true);
}

void ATunicPortalActor::EvaluatePortalCharge(float DeltaSeconds)
{
	const ATunicGameState* TunicGameState = GetWorld() ? GetWorld()->GetGameState<ATunicGameState>() : nullptr;
	if (!TunicGameState || !TunicGameState->IsPortalEventActive())
	{
		SetPortalCharging(false);
		return;
	}

	int32 NewRequiredLivingPlayerCount = 0;
	int32 NewPresentLivingPlayerCount = 0;
	const bool bAllLivingPlayersInRange = CountLivingPlayersInRange(NewRequiredLivingPlayerCount, NewPresentLivingPlayerCount);
	SetPortalPlayerCounts(NewRequiredLivingPlayerCount, NewPresentLivingPlayerCount);

	if (bAllLivingPlayersInRange)
	{
		SetPortalCharging(true);
		const float ProgressDelta = ChargeDuration <= UE_SMALL_NUMBER ? 1.0f : DeltaSeconds / ChargeDuration;
		SetActivationProgress(ActivationProgress + ProgressDelta);
		if (ActivationProgress >= 1.0f)
		{
			CompletePortal();
		}
		return;
	}

	SetPortalCharging(false);
}

void ATunicPortalActor::CompletePortal()
{
	if (bIsPortalReady)
	{
		return;
	}

	SetActivationProgress(1.0f);
	SetPortalCharging(false);
	SetPortalReady(true);

	if (ATunicGameMode* TunicGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ATunicGameMode>() : nullptr)
	{
		TunicGameMode->MarkFloorTransitionReady();
	}
}

bool ATunicPortalActor::CountLivingPlayersInRange(int32& OutRequiredLivingPlayerCount, int32& OutPresentLivingPlayerCount) const
{
	OutRequiredLivingPlayerCount = 0;
	OutPresentLivingPlayerCount = 0;

	const AGameStateBase* CurrentGameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!CurrentGameState)
	{
		return false;
	}

	const float ActivationRadiusSquared = FMath::Square(FMath::Max(0.0f, ActivationRadius));
	const FVector PortalLocation = GetActorLocation();

	for (const APlayerState* PlayerState : CurrentGameState->PlayerArray)
	{
		if (!PlayerState)
		{
			continue;
		}

		const ATunicPlayerCharacter* PlayerCharacter = PlayerState->GetPawn<ATunicPlayerCharacter>();
		if (!PlayerCharacter || PlayerCharacter->IsDead())
		{
			continue;
		}

		++OutRequiredLivingPlayerCount;
		if (FVector::DistSquared2D(PlayerCharacter->GetActorLocation(), PortalLocation) <= ActivationRadiusSquared)
		{
			++OutPresentLivingPlayerCount;
		}
	}

	return OutRequiredLivingPlayerCount > 0 && OutPresentLivingPlayerCount == OutRequiredLivingPlayerCount;
}

void ATunicPortalActor::UpdatePortalRadiusPreview()
{
	if (PortalRadiusPreview)
	{
		PortalRadiusPreview->SetSphereRadius(FMath::Max(1.0f, ActivationRadius));
	}
}

void ATunicPortalActor::SetPortalActive(bool bNewIsPortalActive)
{
	if (bIsPortalActive == bNewIsPortalActive)
	{
		return;
	}

	bIsPortalActive = bNewIsPortalActive;
	if (bIsPortalActive)
	{
		if (bLogPortalState)
		{
			UE_LOG(LogLpQuestPortal, Display, TEXT("Portal activated | Portal=%s"), *GetNameSafe(this));
		}
		OnPortalActivated();
	}
}

void ATunicPortalActor::SetPortalCharging(bool bNewIsCharging)
{
	if (bIsPortalCharging == bNewIsCharging)
	{
		return;
	}

	bIsPortalCharging = bNewIsCharging;
	if (bLogPortalState)
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal charging %s | Portal=%s | PresentLivingPlayers=%d/%d | Progress=%.2f"),
			bIsPortalCharging ? TEXT("started") : TEXT("paused"),
			*GetNameSafe(this),
			PresentLivingPlayerCount,
			RequiredLivingPlayerCount,
			ActivationProgress);
	}
	OnPortalChargingStateChanged(bIsPortalCharging);
}

void ATunicPortalActor::SetPortalReady(bool bNewIsReady)
{
	if (bIsPortalReady == bNewIsReady)
	{
		return;
	}

	bIsPortalReady = bNewIsReady;
	if (bIsPortalReady)
	{
		if (bLogPortalState)
		{
			UE_LOG(LogLpQuestPortal, Display, TEXT("Portal ready | Portal=%s"), *GetNameSafe(this));
		}
		OnPortalReady();
	}
}

void ATunicPortalActor::SetActivationProgress(float NewActivationProgress)
{
	const float ClampedProgress = FMath::Clamp(NewActivationProgress, 0.0f, 1.0f);
	if (FMath::IsNearlyEqual(ActivationProgress, ClampedProgress, 0.001f))
	{
		return;
	}

	ActivationProgress = ClampedProgress;
	OnPortalChargeChanged(ActivationProgress);
}

void ATunicPortalActor::SetPortalPlayerCounts(int32 NewRequiredLivingPlayerCount, int32 NewPresentLivingPlayerCount)
{
	if (RequiredLivingPlayerCount == NewRequiredLivingPlayerCount && PresentLivingPlayerCount == NewPresentLivingPlayerCount)
	{
		return;
	}

	RequiredLivingPlayerCount = NewRequiredLivingPlayerCount;
	PresentLivingPlayerCount = NewPresentLivingPlayerCount;

	if (bLogPortalState)
	{
		UE_LOG(LogLpQuestPortal, Display, TEXT("Portal player count updated | Portal=%s | PresentLivingPlayers=%d/%d"),
			*GetNameSafe(this),
			PresentLivingPlayerCount,
			RequiredLivingPlayerCount);
	}
}

void ATunicPortalActor::OnRep_IsPortalActive()
{
	if (bIsPortalActive)
	{
		OnPortalActivated();
	}
}

void ATunicPortalActor::OnRep_IsPortalCharging()
{
	OnPortalChargingStateChanged(bIsPortalCharging);
}

void ATunicPortalActor::OnRep_IsPortalReady()
{
	if (bIsPortalReady)
	{
		OnPortalReady();
	}
}

void ATunicPortalActor::OnRep_ActivationProgress()
{
	OnPortalChargeChanged(ActivationProgress);
}

void ATunicPortalActor::OnRep_PortalResetSerial()
{
	OnPortalReset();
}
