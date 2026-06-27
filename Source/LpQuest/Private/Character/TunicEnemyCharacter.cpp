// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/TunicEnemyCharacter.h"

#include "AI/TunicEnemyAIController.h"
#include "Ability/TunicAbilitySystemComponent.h"
#include "Ability/TunicAttributeSet.h"
#include "Ability/TunicDamageGameplayEffect.h"
#include "Ability/TunicGameplayAbility_EnemyMeleeAttack.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Character/TunicPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/TunicCombatRules.h"
#include "Combat/TunicCombatTargetInterface.h"
#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/TunicGameMode.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestEnemyGasDebug, Log, All);

namespace
{
	bool IsEnemyMeleeTargetInvulnerable(const ITunicCombatTargetInterface* CombatTarget)
	{
		const UTunicAbilitySystemComponent* TargetAbilitySystemComponent = CombatTarget ? CombatTarget->GetCombatTargetAbilitySystemComponent() : nullptr;
		const FGameplayTag InvulnerableTag = FGameplayTag::RequestGameplayTag(TEXT("State.Invulnerable"), false);
		return TargetAbilitySystemComponent && InvulnerableTag.IsValid() && TargetAbilitySystemComponent->HasMatchingGameplayTag(InvulnerableTag);
	}
}

ATunicEnemyCharacter::ATunicEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UTunicAbilitySystemComponent* EnemyAbilitySystemComponent = CreateDefaultSubobject<UTunicAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	EnemyAbilitySystemComponent->SetIsReplicated(true);
	EnemyAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	UTunicAttributeSet* EnemyAttributeSet = CreateDefaultSubobject<UTunicAttributeSet>(TEXT("AttributeSet"));
	SetAbilitySystemReferences(EnemyAbilitySystemComponent, EnemyAttributeSet);

	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = ATunicEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->DefaultLandMovementMode = MOVE_Walking;
		MovementComponent->MaxWalkSpeed = 300.0f;
		MovementComponent->bOrientRotationToMovement = true;
		MovementComponent->bUseControllerDesiredRotation = false;
		MovementComponent->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
	}

	DefaultMeleeAttackAbilityClass = UTunicGameplayAbility_EnemyMeleeAttack::StaticClass();
	MeleeAttackDamageEffectClass = UTunicDamageGameplayEffect::StaticClass();
}

void ATunicEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATunicEnemyCharacter, bIsDead);
}

void ATunicEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	EnsureLiveEnemyMovementMode();
	DrawAttributeDebug();
	UpdatePrototypeAutoAttack(DeltaSeconds);
}

void ATunicEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	EnsureLiveEnemyMovementMode();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UTunicAttributeSet::GetHealthAttribute()).AddUObject(this, &ATunicEnemyCharacter::HandleHealthChanged);
		GrantDefaultAbilities();
		LogEnemyAbilitySystemDebug();
	}
}

bool ATunicEnemyCharacter::IsDead() const
{
	return bIsDead;
}

int32 ATunicEnemyCharacter::GetExperienceReward() const
{
	return FMath::Max(0, ExperienceReward);
}

bool ATunicEnemyCharacter::TryActivateEnemyMeleeAttack()
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

void ATunicEnemyCharacter::BeginEnemyMeleeHitWindow()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	bEnemyMeleeHitWindowActive = true;
	MeleeAttackMontageHitWindowSerial = MeleeAttackMontageActivationSerial;
	EnemyMeleeHitTargets.Reset();
}

void ATunicEnemyCharacter::ProcessEnemyMeleeHitWindow()
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

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyMeleeHitSweep), false, this);
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
		ITunicCombatTargetInterface* CombatTarget = Cast<ITunicCombatTargetInterface>(TargetActor);
		if (!TargetActor || !CombatTarget || !CombatTarget->IsCombatTargetAvailable() || EnemyMeleeHitTargets.Contains(TargetActor) || FTunicCombatRules::IsSelfHit(this, TargetActor) || !IsActorInsideEnemyMeleeAttackShape(TargetActor))
		{
			continue;
		}

		EnemyMeleeHitTargets.Add(TargetActor);
		HandleEnemyMeleeTargetHit(TargetActor, CombatTarget);
		++ProcessedHitCount;
	}

	if (bDrawEnemyMeleeAttackShapeDebug)
	{
		MulticastDrawEnemyMeleeAttackShape(0.1f, GetEnemyMeleeAttackShapeOrigin(), GetEnemyMeleeAttackShapeForward(), EnemyMeleeAttackRange, EnemyMeleeAttackHalfHeight, EnemyMeleeAttackAngleDegrees);
	}
	LogEnemyMeleeAttackShapeDebug(OverlapResults, ProcessedHitCount);
}

void ATunicEnemyCharacter::EndEnemyMeleeHitWindow()
{
	if (!HasAuthority())
	{
		return;
	}

	bEnemyMeleeHitWindowActive = false;
	EnemyMeleeHitTargets.Reset();
}

void ATunicEnemyCharacter::ExecuteEnemyMeleeAttackAbility()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	if (bEnemyMeleeTelegraphActive)
	{
		if (bLogEnemyMeleeAttack)
		{
			UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee attack request ignored: telegraph already active | Character=%s"),
				*GetNameSafe(this));
		}
		return;
	}

	if (bLogEnemyMeleeAttack)
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

bool ATunicEnemyCharacter::IsCombatTargetAvailable() const
{
	return !IsDead();
}

ETunicCombatTeam ATunicEnemyCharacter::GetCombatTargetTeam() const
{
	return ETunicCombatTeam::Enemy;
}

UTunicAbilitySystemComponent* ATunicEnemyCharacter::GetCombatTargetAbilitySystemComponent() const
{
	return GetTunicAbilitySystemComponent();
}

UTunicAttributeSet* ATunicEnemyCharacter::GetCombatTargetAttributeSet() const
{
	return GetAttributeSet();
}

void ATunicEnemyCharacter::HandleCombatTargetHitReaction(AActor* InstigatorActor)
{
	HandleHitReaction(InstigatorActor);
}

void ATunicEnemyCharacter::BeginCombatHitWindow(FName)
{
	BeginEnemyMeleeHitWindow();
}

void ATunicEnemyCharacter::ProcessCombatHitWindow(FName)
{
	ProcessEnemyMeleeHitWindow();
}

void ATunicEnemyCharacter::EndCombatHitWindow(FName)
{
	EndEnemyMeleeHitWindow();
}

void ATunicEnemyCharacter::SetAbilitySystemInitializationLoggingEnabled(bool bEnabled)
{
	bLogAbilitySystemInitialization = bEnabled;
}

void ATunicEnemyCharacter::SetAttributeDebugDrawEnabled(bool bEnabled)
{
	bDrawAttributeDebug = bEnabled;
}

void ATunicEnemyCharacter::OnDeathStateChanged_Implementation(bool)
{
}

void ATunicEnemyCharacter::OnHitReaction_Implementation(AActor*)
{
}

void ATunicEnemyCharacter::OnEnemyMeleeTelegraphStarted_Implementation(float, FVector, FVector, float, float)
{
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

void ATunicEnemyCharacter::GrantDefaultAbilities()
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

void ATunicEnemyCharacter::HandleHitReaction(AActor* InstigatorActor)
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	MulticastPlayHitReaction(InstigatorActor);
}

void ATunicEnemyCharacter::SetDead(bool bNewIsDead)
{
	if (!HasAuthority() || bIsDead == bNewIsDead)
	{
		return;
	}

	bIsDead = bNewIsDead;
	ApplyDeathState();

	if (bIsDead)
	{
		if (ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(GetController()))
		{
			EnemyAIController->StopEnemyAILogic();
		}

		if (ATunicGameMode* TunicGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ATunicGameMode>() : nullptr)
		{
			TunicGameMode->HandleEnemyDeath(this);
		}
	}
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

	if (!bDeathPresentationApplied)
	{
		bDeathPresentationApplied = true;
		PlayPresentationMontage(DefaultDeathMontage, true);
		OnDeathStateChanged(bIsDead);
	}
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

void ATunicEnemyCharacter::UpdatePrototypeAutoAttack(float DeltaSeconds)
{
	if (!HasAuthority() || !bEnablePrototypeAutoAttack || bIsDead)
	{
		return;
	}

	PrototypeAutoAttackElapsedTime += FMath::Max(0.0f, DeltaSeconds);
	if (PrototypeAutoAttackElapsedTime < PrototypeAutoAttackInterval)
	{
		return;
	}

	PrototypeAutoAttackElapsedTime = 0.0f;
	AActor* TargetActor = FindPrototypeAutoAttackTarget();
	if (!TargetActor)
	{
		return;
	}

	const FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	const FVector ToTarget2D(ToTarget.X, ToTarget.Y, 0.0f);
	if (!ToTarget2D.IsNearlyZero())
	{
		SetActorRotation(ToTarget2D.Rotation());
	}

	TryActivateEnemyMeleeAttack();
}

AActor* ATunicEnemyCharacter::FindPrototypeAutoAttackTarget() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector QueryLocation = GetActorLocation();
	const FCollisionShape QueryShape = FCollisionShape::MakeSphere(PrototypeAutoAttackRange);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyPrototypeAutoAttack), false, this);
	TArray<FOverlapResult> OverlapResults;
	World->OverlapMultiByChannel(
		OverlapResults,
		QueryLocation,
		FQuat::Identity,
		ECC_Pawn,
		QueryShape,
		QueryParams);

	AActor* BestTarget = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* TargetActor = OverlapResult.GetActor();
		const ITunicCombatTargetInterface* CombatTarget = Cast<ITunicCombatTargetInterface>(TargetActor);
		if (!TargetActor || !CombatTarget || !CombatTarget->IsCombatTargetAvailable() || CombatTarget->GetCombatTargetTeam() != ETunicCombatTeam::Player)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(QueryLocation, TargetActor->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = TargetActor;
		}
	}

	return BestTarget;
}

void ATunicEnemyCharacter::StartEnemyMeleeTelegraph()
{
	if (!HasAuthority() || bIsDead || bEnemyMeleeTelegraphActive)
	{
		return;
	}

	if (ATunicEnemyAIController* EnemyAIController = Cast<ATunicEnemyAIController>(GetController()))
	{
		EnemyAIController->StopMovement();
	}

	const float TelegraphDuration = FMath::Max(0.0f, EnemyMeleeTelegraphDuration);
	const FVector ShapeOrigin = GetEnemyMeleeAttackShapeOrigin();
	const FVector ShapeForward = GetEnemyMeleeAttackShapeForward();

	bEnemyMeleeTelegraphActive = true;
	MulticastStartEnemyMeleeTelegraph(TelegraphDuration, ShapeOrigin, ShapeForward, EnemyMeleeAttackRange, EnemyMeleeAttackHalfHeight);

	if (bLogEnemyMeleeAttack)
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
			&ATunicEnemyCharacter::FinishEnemyMeleeTelegraphAndAttack,
			TelegraphDuration,
			false);
	}
	else
	{
		FinishEnemyMeleeTelegraphAndAttack();
	}
}

void ATunicEnemyCharacter::FinishEnemyMeleeTelegraphAndAttack()
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

	const bool bPlayedMontage = PlayMeleeAttackMontage();
	if (bRunEnemyMeleeHitWindowOnRequest && !bPlayedMontage)
	{
		BeginEnemyMeleeHitWindow();
		ProcessEnemyMeleeHitWindow();
		EndEnemyMeleeHitWindow();
	}
}

void ATunicEnemyCharacter::ClearEnemyMeleeTelegraph()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EnemyMeleeTelegraphTimerHandle);
	}

	bEnemyMeleeTelegraphActive = false;
}

FVector ATunicEnemyCharacter::GetEnemyMeleeAttackShapeOrigin() const
{
	const float CapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.0f;
	const FVector ActorLocation = GetActorLocation();
	const FVector FootCenter(ActorLocation.X, ActorLocation.Y, ActorLocation.Z - CapsuleHalfHeight + 5.0f);
	return FootCenter + GetEnemyMeleeAttackShapeForward() * EnemyMeleeAttackOriginForwardOffset;
}

FVector ATunicEnemyCharacter::GetEnemyMeleeAttackShapeForward() const
{
	const FVector Forward2D = GetActorForwardVector().GetSafeNormal2D();
	return Forward2D.IsNearlyZero() ? FVector::ForwardVector : Forward2D;
}

bool ATunicEnemyCharacter::IsActorInsideEnemyMeleeAttackShape(const AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return false;
	}

	FVector TargetOrigin = TargetActor->GetActorLocation();
	FVector TargetExtent = FVector::ZeroVector;
	TargetActor->GetActorBounds(false, TargetOrigin, TargetExtent);

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
	if (Distance2D > FMath::Max(0.0f, EnemyMeleeAttackRange))
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
	return AngleDegrees <= HalfAngleDegrees;
}

void ATunicEnemyCharacter::DrawEnemyMeleeFanDebug(FVector ShapeOrigin, FVector ShapeForward, float ShapeRange, float ShapeHalfHeight, float ShapeAngleDegrees, float LifeTime, FColor DebugColor, const TCHAR* DebugLabel) const
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

void ATunicEnemyCharacter::DrawEnemyMeleeAttackShapeDebug(float LifeTime, FColor DebugColor, const TCHAR* DebugLabel) const
{
	DrawEnemyMeleeFanDebug(GetEnemyMeleeAttackShapeOrigin(), GetEnemyMeleeAttackShapeForward(), EnemyMeleeAttackRange, EnemyMeleeAttackHalfHeight, EnemyMeleeAttackAngleDegrees, LifeTime, DebugColor, DebugLabel);
}

void ATunicEnemyCharacter::DrawEnemyMeleeTelegraphDebug(FVector ShapeOrigin, FVector ShapeForward, float ShapeRange, float ShapeHalfHeight) const
{
	if (!bDrawEnemyMeleeTelegraphDebug)
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

bool ATunicEnemyCharacter::PlayMeleeAttackMontage()
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
		TimerDelegate.BindUObject(this, &ATunicEnemyCharacter::CheckMeleeAttackMontageHitWindowTriggered, MontageSerial);
		World->GetTimerManager().SetTimer(
			TimerHandle,
			TimerDelegate,
			MeleeAttackMontage->GetPlayLength() + 0.1f,
			false);
	}

	return true;
}

void ATunicEnemyCharacter::CheckMeleeAttackMontageHitWindowTriggered(int32 MontageSerial)
{
	if (!HasAuthority() || !MeleeAttackMontage || MeleeAttackMontageActivationSerial != MontageSerial || MeleeAttackMontageHitWindowSerial == MontageSerial)
	{
		return;
	}

	UE_LOG(LogLpQuestEnemyGasDebug, Warning, TEXT("Enemy melee attack montage completed without hit-window notify | Character=%s | Montage=%s"),
		*GetNameSafe(this),
		*GetNameSafe(MeleeAttackMontage));
}

void ATunicEnemyCharacter::HandleEnemyMeleeTargetHit(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget)
{
	if (!TargetActor || !CombatTarget)
	{
		return;
	}

	const bool bCanApplyDamage = FTunicCombatRules::CanApplyDamage(this, TargetActor, *CombatTarget);
	const bool bCanTriggerHitReaction = FTunicCombatRules::CanTriggerHitReaction(this, TargetActor, *CombatTarget);
	const bool bIsTargetInvulnerable = IsEnemyMeleeTargetInvulnerable(CombatTarget);
	const UTunicAttributeSet* TargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();
	const float HealthBefore = TargetAttributeSet ? TargetAttributeSet->GetHealth() : 0.0f;

	if (bCanApplyDamage)
	{
		ApplyEnemyMeleeDamage(TargetActor, CombatTarget);
	}
	else if (bLogEnemyMeleeAttack)
	{
		UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee hit target without damage | Character=%s | Target=%s | SourceTeam=%d | TargetTeam=%d"),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor),
			static_cast<int32>(FTunicCombatRules::GetSourceTeam(this)),
			static_cast<int32>(CombatTarget->GetCombatTargetTeam()));
	}

	const UTunicAttributeSet* UpdatedTargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();
	const float HealthAfter = UpdatedTargetAttributeSet ? UpdatedTargetAttributeSet->GetHealth() : HealthBefore;
	if (!bIsTargetInvulnerable && bCanTriggerHitReaction && CombatTarget->IsCombatTargetAvailable() && (!bCanApplyDamage || HealthAfter < HealthBefore) && HealthAfter > 0.0f)
	{
		CombatTarget->HandleCombatTargetHitReaction(this);
	}
}

void ATunicEnemyCharacter::ApplyEnemyMeleeDamage(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget)
{
	if (!bApplyEnemyMeleeDamage)
	{
		return;
	}

	if (!TargetActor || !CombatTarget || !CombatTarget->IsCombatTargetAvailable())
	{
		return;
	}

	UTunicAbilitySystemComponent* TargetAbilitySystemComponent = CombatTarget->GetCombatTargetAbilitySystemComponent();
	UTunicAttributeSet* TargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();
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
		if (ATunicPlayerCharacter* TargetPlayerCharacter = Cast<ATunicPlayerCharacter>(TargetActor))
		{
			TargetPlayerCharacter->NotifyDodgeInvulnerabilitySuccess(this);
		}

		UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee damage skipped: target invulnerable | Character=%s | Target=%s | EffectClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor),
			*GetNameSafe(MeleeAttackDamageEffectClass.Get()));
		return;
	}

	const float HealthBefore = TargetAttributeSet->GetHealth();
	const UTunicAbilitySystemComponent* SourceAbilitySystemComponent = GetTunicAbilitySystemComponent();
	FGameplayEffectContextHandle EffectContext = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->MakeEffectContext() : TargetAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	TargetAbilitySystemComponent->BP_ApplyGameplayEffectToSelf(MeleeAttackDamageEffectClass, 1.0f, EffectContext);
	const float HealthAfter = TargetAttributeSet->GetHealth();

	UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee damage applied | Character=%s | Target=%s | EffectClass=%s | TargetHealth=%.1f->%.1f"),
		*GetNameSafe(this),
		*GetNameSafe(TargetActor),
		*GetNameSafe(MeleeAttackDamageEffectClass.Get()),
		HealthBefore,
		HealthAfter);
}

void ATunicEnemyCharacter::LogEnemyMeleeAttackShapeDebug(const TArray<FOverlapResult>& OverlapResults, int32 ProcessedHitCount) const
{
	if (!bLogEnemyMeleeAttack)
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

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		const AActor* TargetActor = OverlapResult.GetActor();
		const ITunicCombatTargetInterface* CombatTarget = Cast<ITunicCombatTargetInterface>(TargetActor);
		if (!TargetActor || !CombatTarget)
		{
			continue;
		}

		const UTunicAbilitySystemComponent* TargetAbilitySystemComponent = CombatTarget->GetCombatTargetAbilitySystemComponent();
		const UTunicAttributeSet* TargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();
		UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee attack shape candidate | Character=%s | Target=%s | TargetASC=%s | TargetAttributeSet=%s | TargetHealth=%.1f/%.1f | TargetStamina=%.1f/%.1f | InShape=%s | TargetLocalRole=%d | TargetRemoteRole=%d"),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor),
			*GetNameSafe(TargetAbilitySystemComponent),
			*GetNameSafe(TargetAttributeSet),
			TargetAttributeSet ? TargetAttributeSet->GetHealth() : 0.0f,
			TargetAttributeSet ? TargetAttributeSet->GetMaxHealth() : 0.0f,
			TargetAttributeSet ? TargetAttributeSet->GetStamina() : 0.0f,
			TargetAttributeSet ? TargetAttributeSet->GetMaxStamina() : 0.0f,
			IsActorInsideEnemyMeleeAttackShape(TargetActor) ? TEXT("true") : TEXT("false"),
			static_cast<int32>(TargetActor->GetLocalRole()),
			static_cast<int32>(TargetActor->GetRemoteRole()));
	}
}

void ATunicEnemyCharacter::EnsureLiveEnemyMovementMode()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!HasAuthority() || bIsDead || !MovementComponent || MovementComponent->MovementMode != MOVE_None)
	{
		return;
	}

	MovementComponent->SetMovementMode(MOVE_Walking);
}

void ATunicEnemyCharacter::PlayPresentationMontage(UAnimMontage* MontageToPlay, bool bStopAllMontages)
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

void ATunicEnemyCharacter::MulticastPlayMeleeAttackMontage_Implementation(UAnimMontage* MontageToPlay)
{
	PlayPresentationMontage(MontageToPlay, false);
}

void ATunicEnemyCharacter::MulticastStartEnemyMeleeTelegraph_Implementation(float TelegraphDuration, FVector ShapeOrigin, FVector ShapeForward, float ShapeRange, float ShapeHalfHeight)
{
	DrawEnemyMeleeTelegraphDebug(ShapeOrigin, ShapeForward, ShapeRange, ShapeHalfHeight);
	OnEnemyMeleeTelegraphStarted(TelegraphDuration, ShapeOrigin, ShapeForward, ShapeRange, ShapeHalfHeight);
}

void ATunicEnemyCharacter::MulticastDrawEnemyMeleeAttackShape_Implementation(float LifeTime, FVector ShapeOrigin, FVector ShapeForward, float ShapeRange, float ShapeHalfHeight, float ShapeAngleDegrees)
{
	DrawEnemyMeleeFanDebug(ShapeOrigin, ShapeForward, ShapeRange, ShapeHalfHeight, ShapeAngleDegrees, FMath::Max(0.0f, LifeTime), FColor::Red, TEXT("Enemy Hit Shape"));
}

void ATunicEnemyCharacter::MulticastPlayHitReaction_Implementation(AActor* InstigatorActor)
{
	if (bIsDead)
	{
		return;
	}

	if (bLogHitReaction)
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

