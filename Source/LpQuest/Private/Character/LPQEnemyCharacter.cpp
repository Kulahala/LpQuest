// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/LPQEnemyCharacter.h"

#include "AI/LPQEnemyAIController.h"
#include "Ability/LPQAbilitySystemComponent.h"
#include "Ability/LPQAttributeSet.h"
#include "Ability/LPQDamageGameplayEffect.h"
#include "Ability/LPQGameplayAbility_EnemyMeleeAttack.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/LPQPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/LPQCombatRules.h"
#include "Combat/LPQCombatTargetInterface.h"
#include "Debug/LPQDebugSettings.h"
#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/LPQGameMode.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestEnemyGasDebug, Log, All);

namespace
{
	bool IsEnemyMeleeTargetInvulnerable(const ILPQCombatTargetInterface* CombatTarget)
	{
		const ULPQAbilitySystemComponent* TargetAbilitySystemComponent = CombatTarget ? CombatTarget->GetCombatTargetAbilitySystemComponent() : nullptr;
		const FGameplayTag InvulnerableTag = FGameplayTag::RequestGameplayTag(TEXT("State.Invulnerable"), false);
		return TargetAbilitySystemComponent && InvulnerableTag.IsValid() && TargetAbilitySystemComponent->HasMatchingGameplayTag(InvulnerableTag);
	}

	void GetEnemyMeleeTargetCollisionBounds(const AActor* TargetActor, FVector& OutOrigin, FVector& OutExtent)
	{
		OutOrigin = TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;
		OutExtent = FVector::ZeroVector;
		if (!TargetActor)
		{
			return;
		}

		const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
		const UCapsuleComponent* TargetCapsule = TargetCharacter ? TargetCharacter->GetCapsuleComponent() : nullptr;
		if (TargetCapsule)
		{
			OutOrigin = TargetCapsule->GetComponentLocation();
			OutExtent = FVector(TargetCapsule->GetScaledCapsuleRadius(), TargetCapsule->GetScaledCapsuleRadius(), TargetCapsule->GetScaledCapsuleHalfHeight());
			return;
		}

		TargetActor->GetActorBounds(true, OutOrigin, OutExtent);
	}
}

ALPQEnemyCharacter::ALPQEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ULPQAbilitySystemComponent* EnemyAbilitySystemComponent = CreateDefaultSubobject<ULPQAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	EnemyAbilitySystemComponent->SetIsReplicated(true);
	EnemyAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	ULPQAttributeSet* EnemyAttributeSet = CreateDefaultSubobject<ULPQAttributeSet>(TEXT("AttributeSet"));
	SetAbilitySystemReferences(EnemyAbilitySystemComponent, EnemyAttributeSet);

	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = ALPQEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->DefaultLandMovementMode = MOVE_Walking;
		MovementComponent->MaxWalkSpeed = 300.0f;
		MovementComponent->bOrientRotationToMovement = true;
		MovementComponent->bUseControllerDesiredRotation = false;
		MovementComponent->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
	}

	DefaultMeleeAttackAbilityClass = ULPQGameplayAbility_EnemyMeleeAttack::StaticClass();
	MeleeAttackDamageEffectClass = ULPQDamageGameplayEffect::StaticClass();
}

void ALPQEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALPQEnemyCharacter, bIsDead);
}

void ALPQEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	EnsureLiveEnemyMovementMode();
	DrawAttributeDebug();
}

void ALPQEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	EnsureLiveEnemyMovementMode();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ULPQAttributeSet::GetHealthAttribute()).AddUObject(this, &ALPQEnemyCharacter::HandleHealthChanged);
		GrantDefaultAbilities();
		LogEnemyAbilitySystemDebug();
	}
}

bool ALPQEnemyCharacter::IsDead() const
{
	return bIsDead;
}

int32 ALPQEnemyCharacter::GetExperienceReward() const
{
	return FMath::Max(0, ExperienceReward);
}

TSubclassOf<ALPQPickupActor> ALPQEnemyCharacter::GetDroppedPickupClass() const
{
	return DroppedPickupClass;
}

bool ALPQEnemyCharacter::TryActivateEnemyMeleeAttack()
{
	if (!HasAuthority() || bIsDead || !AbilitySystemComponent)
	{
		return false;
	}

	if (const FGameplayTag EnemyMeleeAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack.Enemy.Melee"), false); EnemyMeleeAttackTag.IsValid())
	{
		FGameplayTagContainer EnemyMeleeAttackTags;
		EnemyMeleeAttackTags.AddTag(EnemyMeleeAttackTag);
		if (AbilitySystemComponent->TryActivateAbilitiesByTag(EnemyMeleeAttackTags))
		{
			return true;
		}
	}

	if (DefaultMeleeAttackAbilityClass)
	{
		return AbilitySystemComponent->TryActivateAbilityByClass(DefaultMeleeAttackAbilityClass);
	}

	return false;
}

void ALPQEnemyCharacter::BeginEnemyMeleeHitWindow()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	bEnemyMeleeHitWindowActive = true;
	MeleeAttackMontageHitWindowSerial = MeleeAttackMontageActivationSerial;
	EnemyMeleeHitTargets.Reset();
}

void ALPQEnemyCharacter::ProcessEnemyMeleeHitWindow()
{
	if (!HasAuthority() || !bEnemyMeleeHitWindowActive || bIsDead)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float AttackRange = FMath::Max(0.0f, EnemyMeleeAttackRange);
	const float AttackAngleDegrees = FMath::Clamp(EnemyMeleeAttackAngleDegrees, 0.0f, 180.0f);
	const float AttackHalfHeight = FMath::Max(0.0f, EnemyMeleeAttackHalfHeight);
	if (AttackRange <= UE_SMALL_NUMBER || AttackAngleDegrees <= UE_SMALL_NUMBER)
	{
		UE_LOG(LogLpQuestEnemyGasDebug, Warning, TEXT("Enemy melee attack shape skipped: invalid range or angle | Character=%s | Range=%.1f | Angle=%.1f | HalfHeight=%.1f"),
			*GetNameSafe(this),
			AttackRange,
			AttackAngleDegrees,
			AttackHalfHeight);
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyMeleeAttackShapeOverlap), false, this);
	TArray<FOverlapResult> OverlapResults;
	const float OverlapQueryRadius = FMath::Sqrt(FMath::Square(AttackRange) + FMath::Square(AttackHalfHeight));
	World->OverlapMultiByChannel(
		OverlapResults,
		GetEnemyMeleeAttackShapeOrigin(),
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(OverlapQueryRadius),
		QueryParams);

	int32 ProcessedHitCount = 0;
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* TargetActor = OverlapResult.GetActor();
		ILPQCombatTargetInterface* CombatTarget = Cast<ILPQCombatTargetInterface>(TargetActor);
		if (!TargetActor || !CombatTarget || !CombatTarget->IsCombatTargetAvailable() || EnemyMeleeHitTargets.Contains(TargetActor) || FLPQCombatRules::IsSelfHit(this, TargetActor) || !IsActorInsideEnemyMeleeAttackShape(TargetActor))
		{
			continue;
		}

		EnemyMeleeHitTargets.Add(TargetActor);
		HandleEnemyMeleeTargetHit(TargetActor, CombatTarget);
		++ProcessedHitCount;
	}

	if (bDrawEnemyMeleeAttackShapeDebug && FLPQDebugSettings::ShouldDrawEnemyMelee())
	{
		MulticastDrawEnemyMeleeAttackShape(0.1f, GetEnemyMeleeAttackShapeOrigin(), GetEnemyMeleeAttackShapeForward(), EnemyMeleeAttackRange, EnemyMeleeAttackHalfHeight, EnemyMeleeAttackAngleDegrees);
	}
	LogEnemyMeleeAttackShapeDebug(OverlapResults, ProcessedHitCount);
}

void ALPQEnemyCharacter::EndEnemyMeleeHitWindow()
{
	if (!HasAuthority())
	{
		return;
	}

	bEnemyMeleeHitWindowActive = false;
	EnemyMeleeHitTargets.Reset();
}

void ALPQEnemyCharacter::ExecuteEnemyMeleeAttackAbility()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	if (bEnemyMeleeTelegraphActive)
	{
		if (bLogEnemyMeleeAttack && FLPQDebugSettings::ShouldLogCombat())
		{
			UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee attack request ignored: telegraph already active | Character=%s"),
				*GetNameSafe(this));
		}
		return;
	}

	if (bLogEnemyMeleeAttack && FLPQDebugSettings::ShouldLogCombat())
	{
		UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee attack request accepted on server | Character=%s | ASC=%s | AttributeSet=%s | Health=%.1f/%.1f | LocalRole=%d | RemoteRole=%d"),
			*GetNameSafe(this),
			*GetNameSafe(AbilitySystemComponent),
			*GetNameSafe(AttributeSet),
			AttributeSet ? AttributeSet->GetHealth() : 0.0f,
			AttributeSet ? AttributeSet->GetMaxHealth() : 0.0f,
			static_cast<int32>(GetLocalRole()),
			static_cast<int32>(GetRemoteRole()));
	}

	StartEnemyMeleeTelegraph();
}

bool ALPQEnemyCharacter::IsCombatTargetAvailable() const
{
	return !IsDead();
}

ELPQCombatTeam ALPQEnemyCharacter::GetCombatTargetTeam() const
{
	return ELPQCombatTeam::Enemy;
}

ULPQAbilitySystemComponent* ALPQEnemyCharacter::GetCombatTargetAbilitySystemComponent() const
{
	return GetTunicAbilitySystemComponent();
}

ULPQAttributeSet* ALPQEnemyCharacter::GetCombatTargetAttributeSet() const
{
	return GetAttributeSet();
}

void ALPQEnemyCharacter::HandleCombatTargetHitReaction(AActor* InstigatorActor)
{
	HandleHitReaction(InstigatorActor);
}

void ALPQEnemyCharacter::BeginCombatHitWindow(FName)
{
	BeginEnemyMeleeHitWindow();
}

void ALPQEnemyCharacter::ProcessCombatHitWindow(FName)
{
	ProcessEnemyMeleeHitWindow();
}

void ALPQEnemyCharacter::EndCombatHitWindow(FName)
{
	EndEnemyMeleeHitWindow();
}

void ALPQEnemyCharacter::SetAbilitySystemInitializationLoggingEnabled(bool bEnabled)
{
	bLogAbilitySystemInitialization = bEnabled;
}

void ALPQEnemyCharacter::SetAttributeDebugDrawEnabled(bool bEnabled)
{
	bDrawAttributeDebug = bEnabled;
}

void ALPQEnemyCharacter::OnDeathStateChanged_Implementation(bool)
{
}

void ALPQEnemyCharacter::OnHitReaction_Implementation(AActor*)
{
}

void ALPQEnemyCharacter::OnEnemyMeleeTelegraphStarted_Implementation(float, FVector, FVector, float, float)
{
}

void ALPQEnemyCharacter::OnRep_IsDead()
{
	ApplyDeathState();
}

void ALPQEnemyCharacter::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!HasAuthority() || bIsDead || ChangeData.NewValue > 0.0f)
	{
		return;
	}

	SetDead(true);
}

void ALPQEnemyCharacter::GrantDefaultAbilities()
{
	if (!HasAuthority() || !AbilitySystemComponent || !DefaultMeleeAttackAbilityClass)
	{
		return;
	}

	if (AbilitySystemComponent->FindAbilitySpecFromClass(DefaultMeleeAttackAbilityClass))
	{
		return;
	}

	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DefaultMeleeAttackAbilityClass, 1));
}

void ALPQEnemyCharacter::HandleHitReaction(AActor* InstigatorActor)
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	MulticastPlayHitReaction(InstigatorActor);
}

void ALPQEnemyCharacter::SetDead(bool bNewIsDead)
{
	if (!HasAuthority() || bIsDead == bNewIsDead)
	{
		return;
	}

	bIsDead = bNewIsDead;
	ApplyDeathState();

	if (bIsDead)
	{
		if (ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(GetController()))
		{
			EnemyAIController->StopEnemyAILogic();
		}

		if (ALPQGameMode* LpQuestGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALPQGameMode>() : nullptr)
		{
			LpQuestGameMode->HandleEnemyDeath(this);
		}
	}
}

void ALPQEnemyCharacter::ApplyDeathState()
{
	if (!bIsDead)
	{
		return;
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	ClearEnemyMeleeTelegraph();
	bEnemyMeleeHitWindowActive = false;
	EnemyMeleeHitTargets.Reset();

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

	if (bLogDeathState && FLPQDebugSettings::ShouldLogCombat())
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

	if (!bDeathPresentationApplied)
	{
		bDeathPresentationApplied = true;
		PlayPresentationMontage(DefaultDeathMontage, true);
		OnDeathStateChanged(bIsDead);
	}
}

void ALPQEnemyCharacter::LogEnemyAbilitySystemDebug() const
{
	if (!bLogAbilitySystemInitialization || !FLPQDebugSettings::ShouldLogCombat())
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

void ALPQEnemyCharacter::DrawAttributeDebug() const
{
	if (!bDrawAttributeDebug || !FLPQDebugSettings::ShouldDrawAttributes() || !AttributeSet)
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

void ALPQEnemyCharacter::StartEnemyMeleeTelegraph()
{
	if (!HasAuthority() || bIsDead || bEnemyMeleeTelegraphActive)
	{
		return;
	}

	if (ALPQEnemyAIController* EnemyAIController = Cast<ALPQEnemyAIController>(GetController()))
	{
		EnemyAIController->StopMovement();
	}

	const float TelegraphDuration = FMath::Max(0.0f, EnemyMeleeTelegraphDuration);
	const FVector ShapeOrigin = GetEnemyMeleeAttackShapeOrigin();
	const FVector ShapeForward = GetEnemyMeleeAttackShapeForward();

	bEnemyMeleeTelegraphActive = true;
	MulticastStartEnemyMeleeTelegraph(TelegraphDuration, ShapeOrigin, ShapeForward, EnemyMeleeAttackRange, EnemyMeleeAttackHalfHeight);

	if (bLogEnemyMeleeAttack && FLPQDebugSettings::ShouldLogCombat())
	{
		UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee telegraph started | Character=%s | Duration=%.3f | ShapeOrigin=%s | ShapeForward=%s | Range=%.1f | Angle=%.1f | HalfHeight=%.1f"),
			*GetNameSafe(this),
			TelegraphDuration,
			*ShapeOrigin.ToCompactString(),
			*ShapeForward.ToCompactString(),
			EnemyMeleeAttackRange,
			EnemyMeleeAttackAngleDegrees,
			EnemyMeleeAttackHalfHeight);
	}

	if (TelegraphDuration <= UE_SMALL_NUMBER)
	{
		FinishEnemyMeleeTelegraphAndAttack();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			EnemyMeleeTelegraphTimerHandle,
			this,
			&ALPQEnemyCharacter::FinishEnemyMeleeTelegraphAndAttack,
			TelegraphDuration,
			false);
	}
	else
	{
		FinishEnemyMeleeTelegraphAndAttack();
	}
}

void ALPQEnemyCharacter::FinishEnemyMeleeTelegraphAndAttack()
{
	if (!HasAuthority())
	{
		return;
	}

	ClearEnemyMeleeTelegraph();
	if (bIsDead)
	{
		return;
	}

	if (!PlayMeleeAttackMontage())
	{
		UE_LOG(LogLpQuestEnemyGasDebug, Warning, TEXT("Enemy melee attack skipped hit window: missing attack Montage | Character=%s | Montage=%s | Requires Combat Hit Window Notify"),
			*GetNameSafe(this),
			*GetNameSafe(MeleeAttackMontage.Get()));
	}
}

void ALPQEnemyCharacter::ClearEnemyMeleeTelegraph()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EnemyMeleeTelegraphTimerHandle);
	}

	bEnemyMeleeTelegraphActive = false;
}

FVector ALPQEnemyCharacter::GetEnemyMeleeAttackShapeOrigin() const
{
	const float CapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.0f;
	const FVector ActorLocation = GetActorLocation();
	const FVector FootCenter(ActorLocation.X, ActorLocation.Y, ActorLocation.Z - CapsuleHalfHeight + 5.0f);
	return FootCenter + GetEnemyMeleeAttackShapeForward() * EnemyMeleeAttackOriginForwardOffset;
}

FVector ALPQEnemyCharacter::GetEnemyMeleeAttackShapeForward() const
{
	const FVector Forward2D = GetActorForwardVector().GetSafeNormal2D();
	return Forward2D.IsNearlyZero() ? FVector::ForwardVector : Forward2D;
}

bool ALPQEnemyCharacter::IsActorInsideEnemyMeleeAttackShape(const AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return false;
	}

	FVector TargetOrigin = TargetActor->GetActorLocation();
	FVector TargetExtent = FVector::ZeroVector;
	GetEnemyMeleeTargetCollisionBounds(TargetActor, TargetOrigin, TargetExtent);

	const FVector ShapeOrigin = GetEnemyMeleeAttackShapeOrigin();
	const float ShapeHalfHeight = FMath::Max(0.0f, EnemyMeleeAttackHalfHeight);
	const float TargetBottom = TargetOrigin.Z - TargetExtent.Z;
	const float TargetTop = TargetOrigin.Z + TargetExtent.Z;
	const float ShapeBottom = ShapeOrigin.Z - ShapeHalfHeight;
	const float ShapeTop = ShapeOrigin.Z + ShapeHalfHeight;
	if (TargetTop < ShapeBottom || TargetBottom > ShapeTop)
	{
		return false;
	}

	const FVector ToLocation = TargetOrigin - ShapeOrigin;
	const FVector ToLocation2D(ToLocation.X, ToLocation.Y, 0.0f);
	const float Distance2D = ToLocation2D.Size();
	const float TargetRadius2D = FMath::Max(TargetExtent.X, TargetExtent.Y);
	if (Distance2D > FMath::Max(0.0f, EnemyMeleeAttackRange) + TargetRadius2D)
	{
		return false;
	}

	if (Distance2D <= UE_SMALL_NUMBER)
	{
		return true;
	}

	const float HalfAngleDegrees = FMath::Clamp(EnemyMeleeAttackAngleDegrees, 0.0f, 180.0f) * 0.5f;
	const float Dot = FVector::DotProduct(GetEnemyMeleeAttackShapeForward(), ToLocation2D / Distance2D);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
	const float AngleToleranceDegrees = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(TargetRadius2D / Distance2D, 0.0f, 1.0f)));
	return AngleDegrees <= HalfAngleDegrees + AngleToleranceDegrees;
}

void ALPQEnemyCharacter::DrawEnemyMeleeFanDebug(FVector ShapeOrigin, FVector ShapeForward, float ShapeRange, float ShapeHalfHeight, float ShapeAngleDegrees, float LifeTime, FColor DebugColor, const TCHAR* DebugLabel) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector DrawCenter = ShapeOrigin;
	const FVector Forward2D = ShapeForward.GetSafeNormal2D().IsNearlyZero() ? GetEnemyMeleeAttackShapeForward() : ShapeForward.GetSafeNormal2D();
	const float ArcRadius = FMath::Max(0.0f, ShapeRange);
	const float ArcAngleDegrees = FMath::Clamp(ShapeAngleDegrees, 1.0f, 180.0f);
	const float HalfAngleDegrees = ArcAngleDegrees * 0.5f;
	const int32 SegmentCount = FMath::Max(8, FMath::CeilToInt(ArcAngleDegrees / 6.0f));
	const int32 FillSegmentStep = FMath::Max(1, SegmentCount / 8);
	const bool bPersistentLines = false;

	FVector PreviousPoint = DrawCenter + Forward2D.RotateAngleAxis(-HalfAngleDegrees, FVector::UpVector) * ArcRadius;
	DrawDebugLine(World, DrawCenter, PreviousPoint, DebugColor, bPersistentLines, LifeTime, 0, 3.0f);
	DrawDebugLine(World, DrawCenter, DrawCenter + Forward2D * ArcRadius, DebugColor, bPersistentLines, LifeTime, 0, 1.5f);
	DrawDebugLine(World, DrawCenter, DrawCenter + Forward2D * (ArcRadius * 0.35f), DebugColor, bPersistentLines, LifeTime, 0, 5.0f);

	for (int32 SegmentIndex = 1; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
		const float AngleDegrees = FMath::Lerp(-HalfAngleDegrees, HalfAngleDegrees, Alpha);
		const FVector CurrentPoint = DrawCenter + Forward2D.RotateAngleAxis(AngleDegrees, FVector::UpVector) * ArcRadius;
		DrawDebugLine(World, PreviousPoint, CurrentPoint, DebugColor, bPersistentLines, LifeTime, 0, 3.0f);
		if (SegmentIndex % FillSegmentStep == 0 || SegmentIndex == SegmentCount)
		{
			DrawDebugLine(World, DrawCenter, CurrentPoint, DebugColor, bPersistentLines, LifeTime, 0, 1.25f);
		}
		PreviousPoint = CurrentPoint;
	}

	DrawDebugLine(World, DrawCenter, PreviousPoint, DebugColor, bPersistentLines, LifeTime, 0, 3.0f);
	DrawDebugString(World, DrawCenter + Forward2D * (ArcRadius * 0.5f) + FVector(0.0f, 0.0f, ShapeHalfHeight + 35.0f), FString(DebugLabel), nullptr, DebugColor, LifeTime, true);
}

void ALPQEnemyCharacter::DrawEnemyMeleeAttackShapeDebug(float LifeTime, FColor DebugColor, const TCHAR* DebugLabel) const
{
	DrawEnemyMeleeFanDebug(GetEnemyMeleeAttackShapeOrigin(), GetEnemyMeleeAttackShapeForward(), EnemyMeleeAttackRange, EnemyMeleeAttackHalfHeight, EnemyMeleeAttackAngleDegrees, LifeTime, DebugColor, DebugLabel);
}

void ALPQEnemyCharacter::DrawEnemyMeleeTelegraphDebug(FVector ShapeOrigin, FVector ShapeForward, float ShapeRange, float ShapeHalfHeight) const
{
	if (!bDrawEnemyMeleeTelegraphDebug || !FLPQDebugSettings::ShouldDrawEnemyMelee())
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	DrawEnemyMeleeFanDebug(ShapeOrigin, ShapeForward, ShapeRange, ShapeHalfHeight, EnemyMeleeAttackAngleDegrees, FMath::Max(0.0f, EnemyMeleeTelegraphDebugDuration), FColor::Orange, TEXT("Enemy Telegraph"));
}

bool ALPQEnemyCharacter::PlayMeleeAttackMontage()
{
	if (!HasAuthority() || !MeleeAttackMontage)
	{
		return false;
	}

	const int32 MontageSerial = ++MeleeAttackMontageActivationSerial;
	MeleeAttackMontageHitWindowSerial = 0;
	MulticastPlayMeleeAttackMontage(MeleeAttackMontage);

	if (UWorld* World = GetWorld())
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(this, &ALPQEnemyCharacter::CheckMeleeAttackMontageHitWindowTriggered, MontageSerial);
		World->GetTimerManager().SetTimer(
			TimerHandle,
			TimerDelegate,
			MeleeAttackMontage->GetPlayLength() + 0.1f,
			false);
	}

	return true;
}

void ALPQEnemyCharacter::CheckMeleeAttackMontageHitWindowTriggered(int32 MontageSerial)
{
	if (!HasAuthority() || !MeleeAttackMontage || MeleeAttackMontageActivationSerial != MontageSerial || MeleeAttackMontageHitWindowSerial == MontageSerial)
	{
		return;
	}

	UE_LOG(LogLpQuestEnemyGasDebug, Warning, TEXT("Enemy melee attack montage completed without hit-window notify | Character=%s | Montage=%s"),
		*GetNameSafe(this),
		*GetNameSafe(MeleeAttackMontage));
}

void ALPQEnemyCharacter::HandleEnemyMeleeTargetHit(AActor* TargetActor, ILPQCombatTargetInterface* CombatTarget)
{
	if (!TargetActor || !CombatTarget)
	{
		return;
	}

	const bool bCanApplyDamage = FLPQCombatRules::CanApplyDamage(this, TargetActor, *CombatTarget);
	const bool bCanTriggerHitReaction = FLPQCombatRules::CanTriggerHitReaction(this, TargetActor, *CombatTarget);
	const bool bIsTargetInvulnerable = IsEnemyMeleeTargetInvulnerable(CombatTarget);
	const ULPQAttributeSet* TargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();
	const float HealthBefore = TargetAttributeSet ? TargetAttributeSet->GetHealth() : 0.0f;

	if (bCanApplyDamage)
	{
		ApplyEnemyMeleeDamage(TargetActor, CombatTarget);
	}
	else if (bLogEnemyMeleeAttack && FLPQDebugSettings::ShouldLogCombat())
	{
		UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee hit target without damage | Character=%s | Target=%s | SourceTeam=%d | TargetTeam=%d"),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor),
			static_cast<int32>(FLPQCombatRules::GetSourceTeam(this)),
			static_cast<int32>(CombatTarget->GetCombatTargetTeam()));
	}

	const ULPQAttributeSet* UpdatedTargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();
	const float HealthAfter = UpdatedTargetAttributeSet ? UpdatedTargetAttributeSet->GetHealth() : HealthBefore;
	if (!bIsTargetInvulnerable && bCanTriggerHitReaction && CombatTarget->IsCombatTargetAvailable() && (!bCanApplyDamage || HealthAfter < HealthBefore) && HealthAfter > 0.0f)
	{
		CombatTarget->HandleCombatTargetHitReaction(this);
	}
}

void ALPQEnemyCharacter::ApplyEnemyMeleeDamage(AActor* TargetActor, ILPQCombatTargetInterface* CombatTarget)
{
	if (!bApplyEnemyMeleeDamage)
	{
		return;
	}

	if (!TargetActor || !CombatTarget || !CombatTarget->IsCombatTargetAvailable())
	{
		return;
	}

	ULPQAbilitySystemComponent* TargetAbilitySystemComponent = CombatTarget->GetCombatTargetAbilitySystemComponent();
	ULPQAttributeSet* TargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();
	if (!TargetAbilitySystemComponent || !TargetAttributeSet || !MeleeAttackDamageEffectClass)
	{
		UE_LOG(LogLpQuestEnemyGasDebug, Warning, TEXT("Enemy melee damage failed: missing target GAS data | Character=%s | Target=%s | TargetASC=%s | TargetAttributeSet=%s | EffectClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor),
			*GetNameSafe(TargetAbilitySystemComponent),
			*GetNameSafe(TargetAttributeSet),
			*GetNameSafe(MeleeAttackDamageEffectClass.Get()));
		return;
	}

	if (const FGameplayTag InvulnerableTag = FGameplayTag::RequestGameplayTag(TEXT("State.Invulnerable"), false);
		InvulnerableTag.IsValid() && TargetAbilitySystemComponent->HasMatchingGameplayTag(InvulnerableTag))
	{
		if (ALPQPlayerCharacter* TargetPlayerCharacter = Cast<ALPQPlayerCharacter>(TargetActor))
		{
			TargetPlayerCharacter->NotifyDodgeInvulnerabilitySuccess(this);
		}

		if (FLPQDebugSettings::ShouldLogCombat())
		{
			UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee damage skipped: target invulnerable | Character=%s | Target=%s | EffectClass=%s"),
				*GetNameSafe(this),
				*GetNameSafe(TargetActor),
				*GetNameSafe(MeleeAttackDamageEffectClass.Get()));
		}
		return;
	}

	const float HealthBefore = TargetAttributeSet->GetHealth();
	const ULPQAbilitySystemComponent* SourceAbilitySystemComponent = GetTunicAbilitySystemComponent();
	FGameplayEffectContextHandle EffectContext = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->MakeEffectContext() : TargetAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	TargetAbilitySystemComponent->BP_ApplyGameplayEffectToSelf(MeleeAttackDamageEffectClass, 1.0f, EffectContext);
	const float HealthAfter = TargetAttributeSet->GetHealth();

	if (FLPQDebugSettings::ShouldLogCombat())
	{
		UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee damage applied | Character=%s | Target=%s | EffectClass=%s | TargetHealth=%.1f->%.1f"),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor),
			*GetNameSafe(MeleeAttackDamageEffectClass.Get()),
			HealthBefore,
			HealthAfter);
	}
}

void ALPQEnemyCharacter::LogEnemyMeleeAttackShapeDebug(const TArray<FOverlapResult>& OverlapResults, int32 ProcessedHitCount) const
{
	if (!bLogEnemyMeleeAttack || !FLPQDebugSettings::ShouldLogCombat())
	{
		return;
	}

	UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee attack shape completed | Character=%s | ShapeOrigin=%s | Forward=%s | OverlapCount=%d | ProcessedHitCount=%d | Range=%.1f | Angle=%.1f | HalfHeight=%.1f"),
		*GetNameSafe(this),
		*GetEnemyMeleeAttackShapeOrigin().ToCompactString(),
		*GetEnemyMeleeAttackShapeForward().ToCompactString(),
		OverlapResults.Num(),
		ProcessedHitCount,
		EnemyMeleeAttackRange,
		EnemyMeleeAttackAngleDegrees,
		EnemyMeleeAttackHalfHeight);

	if (!bLogEnemyMeleeAttackShapeDetails)
	{
		return;
	}

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		const AActor* TargetActor = OverlapResult.GetActor();
		const ILPQCombatTargetInterface* CombatTarget = Cast<ILPQCombatTargetInterface>(TargetActor);
		if (!TargetActor || !CombatTarget)
		{
			continue;
		}

		const ULPQAbilitySystemComponent* TargetAbilitySystemComponent = CombatTarget->GetCombatTargetAbilitySystemComponent();
		const ULPQAttributeSet* TargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();
		FVector TargetOrigin = TargetActor->GetActorLocation();
		FVector TargetExtent = FVector::ZeroVector;
		GetEnemyMeleeTargetCollisionBounds(TargetActor, TargetOrigin, TargetExtent);

		const FVector ShapeOrigin = GetEnemyMeleeAttackShapeOrigin();
		const FVector ShapeForward = GetEnemyMeleeAttackShapeForward();
		const float ShapeRange = FMath::Max(0.0f, EnemyMeleeAttackRange);
		const float ShapeHalfHeight = FMath::Max(0.0f, EnemyMeleeAttackHalfHeight);
		const float ShapeHalfAngleDegrees = FMath::Clamp(EnemyMeleeAttackAngleDegrees, 0.0f, 180.0f) * 0.5f;
		const float TargetBottom = TargetOrigin.Z - TargetExtent.Z;
		const float TargetTop = TargetOrigin.Z + TargetExtent.Z;
		const float ShapeBottom = ShapeOrigin.Z - ShapeHalfHeight;
		const float ShapeTop = ShapeOrigin.Z + ShapeHalfHeight;
		const FVector ToTarget = TargetOrigin - ShapeOrigin;
		const FVector ToTarget2D(ToTarget.X, ToTarget.Y, 0.0f);
		const float Distance2D = ToTarget2D.Size();
		const float TargetRadius2D = FMath::Max(TargetExtent.X, TargetExtent.Y);
		const float Dot = Distance2D > UE_SMALL_NUMBER ? FVector::DotProduct(ShapeForward, ToTarget2D / Distance2D) : 1.0f;
		const float AngleDegrees = Distance2D > UE_SMALL_NUMBER ? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f))) : 0.0f;
		const float AngleToleranceDegrees = Distance2D > UE_SMALL_NUMBER ? FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(TargetRadius2D / Distance2D, 0.0f, 1.0f))) : 0.0f;
		const bool bHeightPass = TargetTop >= ShapeBottom && TargetBottom <= ShapeTop;
		const bool bRangePass = Distance2D <= ShapeRange + TargetRadius2D;
		const bool bAnglePass = Distance2D <= UE_SMALL_NUMBER || AngleDegrees <= ShapeHalfAngleDegrees + AngleToleranceDegrees;
		const bool bInShape = bHeightPass && bRangePass && bAnglePass;

		UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee attack shape candidate | Character=%s | Target=%s | TargetASC=%s | TargetAttributeSet=%s | TargetHealth=%.1f/%.1f | TargetStamina=%.1f/%.1f | InShape=%s | HeightPass=%s | RangePass=%s | AnglePass=%s | Distance2D=%.1f/%.1f+%.1f | Angle=%.1f/%.1f+%.1f | TargetZ=%.1f..%.1f | ShapeZ=%.1f..%.1f | TargetOrigin=%s | TargetExtent=%s | ShapeOrigin=%s | ShapeForward=%s | TargetLocalRole=%d | TargetRemoteRole=%d"),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor),
			*GetNameSafe(TargetAbilitySystemComponent),
			*GetNameSafe(TargetAttributeSet),
			TargetAttributeSet ? TargetAttributeSet->GetHealth() : 0.0f,
			TargetAttributeSet ? TargetAttributeSet->GetMaxHealth() : 0.0f,
			TargetAttributeSet ? TargetAttributeSet->GetStamina() : 0.0f,
			TargetAttributeSet ? TargetAttributeSet->GetMaxStamina() : 0.0f,
			bInShape ? TEXT("true") : TEXT("false"),
			bHeightPass ? TEXT("true") : TEXT("false"),
			bRangePass ? TEXT("true") : TEXT("false"),
			bAnglePass ? TEXT("true") : TEXT("false"),
			Distance2D,
			ShapeRange,
			TargetRadius2D,
			AngleDegrees,
			ShapeHalfAngleDegrees,
			AngleToleranceDegrees,
			TargetBottom,
			TargetTop,
			ShapeBottom,
			ShapeTop,
			*TargetOrigin.ToCompactString(),
			*TargetExtent.ToCompactString(),
			*ShapeOrigin.ToCompactString(),
			*ShapeForward.ToCompactString(),
			static_cast<int32>(TargetActor->GetLocalRole()),
			static_cast<int32>(TargetActor->GetRemoteRole()));
	}
}

void ALPQEnemyCharacter::EnsureLiveEnemyMovementMode()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!HasAuthority() || bIsDead || !MovementComponent || MovementComponent->MovementMode != MOVE_None)
	{
		return;
	}

	MovementComponent->SetMovementMode(MOVE_Walking);
}

void ALPQEnemyCharacter::PlayPresentationMontage(UAnimMontage* MontageToPlay, bool bStopAllMontages)
{
	if (!MontageToPlay)
	{
		return;
	}

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	UAnimInstance* AnimInstance = CharacterMesh ? CharacterMesh->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	AnimInstance->Montage_Play(MontageToPlay, 1.0f, bStopAllMontages ? EMontagePlayReturnType::MontageLength : EMontagePlayReturnType::Duration, 0.0f, bStopAllMontages);
}

void ALPQEnemyCharacter::MulticastPlayMeleeAttackMontage_Implementation(UAnimMontage* MontageToPlay)
{
	PlayPresentationMontage(MontageToPlay, false);
}

void ALPQEnemyCharacter::MulticastStartEnemyMeleeTelegraph_Implementation(float TelegraphDuration, FVector ShapeOrigin, FVector ShapeForward, float ShapeRange, float ShapeHalfHeight)
{
	DrawEnemyMeleeTelegraphDebug(ShapeOrigin, ShapeForward, ShapeRange, ShapeHalfHeight);
	OnEnemyMeleeTelegraphStarted(TelegraphDuration, ShapeOrigin, ShapeForward, ShapeRange, ShapeHalfHeight);
}

void ALPQEnemyCharacter::MulticastDrawEnemyMeleeAttackShape_Implementation(float LifeTime, FVector ShapeOrigin, FVector ShapeForward, float ShapeRange, float ShapeHalfHeight, float ShapeAngleDegrees)
{
	if (!FLPQDebugSettings::ShouldDrawEnemyMelee())
	{
		return;
	}

	DrawEnemyMeleeFanDebug(ShapeOrigin, ShapeForward, ShapeRange, ShapeHalfHeight, ShapeAngleDegrees, FMath::Max(0.0f, LifeTime), FColor::Red, TEXT("Enemy Hit Shape"));
}

void ALPQEnemyCharacter::MulticastPlayHitReaction_Implementation(AActor* InstigatorActor)
{
	if (bIsDead)
	{
		return;
	}

	if (bLogHitReaction && FLPQDebugSettings::ShouldLogCombat())
	{
		UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy hit reaction | Character=%s | Instigator=%s | Authority=%s | LocalRole=%d | RemoteRole=%d"),
			*GetNameSafe(this),
			*GetNameSafe(InstigatorActor),
			HasAuthority() ? TEXT("true") : TEXT("false"),
			static_cast<int32>(GetLocalRole()),
			static_cast<int32>(GetRemoteRole()));
	}

	PlayPresentationMontage(DefaultHitReactionMontage, false);
	OnHitReaction(InstigatorActor);
}

