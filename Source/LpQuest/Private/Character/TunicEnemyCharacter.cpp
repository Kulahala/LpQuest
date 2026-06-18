// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/TunicEnemyCharacter.h"

#include "Ability/TunicAbilitySystemComponent.h"
#include "Ability/TunicAttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestEnemyGasDebug, Log, All);

ATunicEnemyCharacter::ATunicEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UTunicAbilitySystemComponent* EnemyAbilitySystemComponent = CreateDefaultSubobject<UTunicAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	EnemyAbilitySystemComponent->SetIsReplicated(true);
	EnemyAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	UTunicAttributeSet* EnemyAttributeSet = CreateDefaultSubobject<UTunicAttributeSet>(TEXT("AttributeSet"));
	SetAbilitySystemReferences(EnemyAbilitySystemComponent, EnemyAttributeSet);

	PrimaryActorTick.bCanEverTick = true;
}

void ATunicEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATunicEnemyCharacter, bIsDead);
}

void ATunicEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DrawAttributeDebug();
}

void ATunicEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UTunicAttributeSet::GetHealthAttribute()).AddUObject(this, &ATunicEnemyCharacter::HandleHealthChanged);
		LogEnemyAbilitySystemDebug();
	}
}

bool ATunicEnemyCharacter::IsDead() const
{
	return bIsDead;
}

void ATunicEnemyCharacter::SetAbilitySystemInitializationLoggingEnabled(bool bEnabled)
{
	bLogAbilitySystemInitialization = bEnabled;
}

void ATunicEnemyCharacter::SetAttributeDebugDrawEnabled(bool bEnabled)
{
	bDrawAttributeDebug = bEnabled;
}

void ATunicEnemyCharacter::OnDeathStateChanged_Implementation(bool bNewIsDead)
{
	if (!bNewIsDead || bDeathPresentationApplied)
	{
		return;
	}

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh)
	{
		return;
	}

	bDeathPresentationApplied = true;
	CharacterMesh->SetRelativeRotation(CharacterMesh->GetRelativeRotation() + FRotator(0.0f, 0.0f, 90.0f));
	CharacterMesh->SetRelativeScale3D(FVector(1.0f, 0.35f, 1.0f));
}

void ATunicEnemyCharacter::OnRep_IsDead()
{
	ApplyDeathState();
}

void ATunicEnemyCharacter::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!HasAuthority() || bIsDead || ChangeData.NewValue > 0.0f)
	{
		return;
	}

	SetDead(true);
}

void ATunicEnemyCharacter::SetDead(bool bNewIsDead)
{
	if (!HasAuthority() || bIsDead == bNewIsDead)
	{
		return;
	}

	bIsDead = bNewIsDead;
	ApplyDeathState();
}

void ATunicEnemyCharacter::ApplyDeathState()
{
	if (!bIsDead)
	{
		return;
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	if (HasAuthority() && AbilitySystemComponent)
	{
		const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"), false);
		if (DeadTag.IsValid())
		{
			AbilitySystemComponent->SetLooseGameplayTagCount(DeadTag, 1, EGameplayTagReplicationState::TagAndCountToAll);
		}
	}

	if (bLogDeathState)
	{
		UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy entered death state | Character=%s | ASC=%s | AttributeSet=%s | Health=%.1f/%.1f | Authority=%s | LocalRole=%d | RemoteRole=%d"),
			*GetNameSafe(this),
			*GetNameSafe(AbilitySystemComponent),
			*GetNameSafe(AttributeSet),
			AttributeSet ? AttributeSet->GetHealth() : 0.0f,
			AttributeSet ? AttributeSet->GetMaxHealth() : 0.0f,
			HasAuthority() ? TEXT("true") : TEXT("false"),
			static_cast<int32>(GetLocalRole()),
			static_cast<int32>(GetRemoteRole()));
	}

	OnDeathStateChanged(bIsDead);
}

void ATunicEnemyCharacter::LogEnemyAbilitySystemDebug() const
{
	if (!bLogAbilitySystemInitialization)
	{
		return;
	}

	const AActor* AscOwnerActor = AbilitySystemComponent ? AbilitySystemComponent->GetOwnerActor() : nullptr;
	const AActor* AscAvatarActor = AbilitySystemComponent ? AbilitySystemComponent->GetAvatarActor_Direct() : nullptr;

	UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy ASC initialized | Character=%s | ASC=%s | OwnerActor=%s | AvatarActor=%s | AttributeSet=%s | Health=%.1f/%.1f | Stamina=%.1f/%.1f | Authority=%s | LocalRole=%d | RemoteRole=%d"),
		*GetNameSafe(this),
		*GetNameSafe(AbilitySystemComponent),
		*GetNameSafe(AscOwnerActor),
		*GetNameSafe(AscAvatarActor),
		*GetNameSafe(AttributeSet),
		AttributeSet ? AttributeSet->GetHealth() : 0.0f,
		AttributeSet ? AttributeSet->GetMaxHealth() : 0.0f,
		AttributeSet ? AttributeSet->GetStamina() : 0.0f,
		AttributeSet ? AttributeSet->GetMaxStamina() : 0.0f,
		HasAuthority() ? TEXT("true") : TEXT("false"),
		static_cast<int32>(GetLocalRole()),
		static_cast<int32>(GetRemoteRole()));
}

void ATunicEnemyCharacter::DrawAttributeDebug() const
{
	if (!bDrawAttributeDebug || !AttributeSet)
	{
		return;
	}

	const FString DebugText = FString::Printf(
		TEXT("Enemy%s\nHP %.0f/%.0f\nSTA %.0f/%.0f"),
		bIsDead ? TEXT(" DEAD") : TEXT(""),
		AttributeSet->GetHealth(),
		AttributeSet->GetMaxHealth(),
		AttributeSet->GetStamina(),
		AttributeSet->GetMaxStamina());

	DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.0f, 0.0f, 115.0f), DebugText, nullptr, bIsDead ? FColor::Silver : FColor::Red, 0.0f, true, 1.0f);
}

