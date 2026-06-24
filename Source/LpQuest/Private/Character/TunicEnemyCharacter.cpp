// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/TunicEnemyCharacter.h"

#include "Ability/TunicAbilitySystemComponent.h"
#include "Ability/TunicAttributeSet.h"
#include "Ability/TunicDamageGameplayEffect.h"
#include "Ability/TunicGameplayAbility_EnemyMeleeAttack.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/TunicCombatRules.h"
#include "Combat/TunicCombatTargetInterface.h"
#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
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

ATunicEnemyCharacter::ATunicEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UTunicAbilitySystemComponent* EnemyAbilitySystemComponent = CreateDefaultSubobject<UTunicAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	EnemyAbilitySystemComponent->SetIsReplicated(true);
	EnemyAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	UTunicAttributeSet* EnemyAttributeSet = CreateDefaultSubobject<UTunicAttributeSet>(TEXT("AttributeSet"));
	SetAbilitySystemReferences(EnemyAbilitySystemComponent, EnemyAttributeSet);

	PrimaryActorTick.bCanEverTick = true;

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

	DrawAttributeDebug();
	UpdatePrototypeAutoAttack(DeltaSeconds);
}

void ATunicEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

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

void ATunicEnemyCharacter::TryActivateEnemyMeleeAttack()
{
	if (!HasAuthority() || bIsDead || !AbilitySystemComponent)
	{
		return;
	}

	if (const FGameplayTag EnemyMeleeAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack.Enemy.Melee"), false); EnemyMeleeAttackTag.IsValid())
	{
		FGameplayTagContainer EnemyMeleeAttackTags;
		EnemyMeleeAttackTags.AddTag(EnemyMeleeAttackTag);
		if (AbilitySystemComponent->TryActivateAbilitiesByTag(EnemyMeleeAttackTags))
		{
			return;
		}
	}

	if (DefaultMeleeAttackAbilityClass)
	{
		AbilitySystemComponent->TryActivateAbilityByClass(DefaultMeleeAttackAbilityClass);
	}
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

	const FVector SweepStart = GetEnemyMeleeSweepPoint(EnemyMeleeSweepStartOffset);
	const FVector SweepEnd = GetEnemyMeleeSweepPoint(EnemyMeleeSweepEndOffset);
	const FCollisionShape SweepShape = FCollisionShape::MakeCapsule(EnemyMeleeSweepRadius, EnemyMeleeSweepHalfHeight);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyMeleeHitSweep), false, this);
	TArray<FHitResult> HitResults;
	World->SweepMultiByChannel(
		HitResults,
		SweepStart,
		SweepEnd,
		FQuat::Identity,
		ECC_Pawn,
		SweepShape,
		QueryParams);

	int32 ProcessedHitCount = 0;
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* TargetActor = HitResult.GetActor();
		ITunicCombatTargetInterface* CombatTarget = Cast<ITunicCombatTargetInterface>(TargetActor);
		if (!TargetActor || !CombatTarget || !CombatTarget->IsCombatTargetAvailable() || EnemyMeleeHitTargets.Contains(TargetActor) || FTunicCombatRules::IsSelfHit(this, TargetActor))
		{
			continue;
		}

		EnemyMeleeHitTargets.Add(TargetActor);
		HandleEnemyMeleeTargetHit(TargetActor, CombatTarget);
		++ProcessedHitCount;
	}

	LogEnemyMeleeHitSweepDebug(HitResults, ProcessedHitCount);
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

	const bool bPlayedMontage = PlayMeleeAttackMontage();
	if (bRunEnemyMeleeHitWindowOnRequest && !bPlayedMontage)
	{
		BeginEnemyMeleeHitWindow();
		ProcessEnemyMeleeHitWindow();
		EndEnemyMeleeHitWindow();
	}
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
		if (ATunicGameMode* TunicGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ATunicGameMode>() : nullptr)
		{
			TunicGameMode->EvaluateEncounterClear();
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

FVector ATunicEnemyCharacter::GetEnemyMeleeSweepPoint(const FVector& LocalOffset) const
{
	return GetActorLocation() + GetActorRotation().RotateVector(LocalOffset);
}

void ATunicEnemyCharacter::HandleEnemyMeleeTargetHit(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget)
{
	if (!TargetActor || !CombatTarget)
	{
		return;
	}

	const bool bCanApplyDamage = FTunicCombatRules::CanApplyDamage(this, TargetActor, *CombatTarget);
	const bool bCanTriggerHitReaction = FTunicCombatRules::CanTriggerHitReaction(this, TargetActor, *CombatTarget);
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
	if (bCanTriggerHitReaction && CombatTarget->IsCombatTargetAvailable() && (!bCanApplyDamage || HealthAfter < HealthBefore) && HealthAfter > 0.0f)
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

void ATunicEnemyCharacter::LogEnemyMeleeHitSweepDebug(const TArray<FHitResult>& HitResults, int32 ProcessedHitCount) const
{
	if (!bLogEnemyMeleeAttack)
	{
		return;
	}

	UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee hit sweep completed | Character=%s | SweepStart=%s | SweepEnd=%s | HitCount=%d | ProcessedHitCount=%d | Radius=%.1f | HalfHeight=%.1f"),
		*GetNameSafe(this),
		*GetEnemyMeleeSweepPoint(EnemyMeleeSweepStartOffset).ToCompactString(),
		*GetEnemyMeleeSweepPoint(EnemyMeleeSweepEndOffset).ToCompactString(),
		HitResults.Num(),
		ProcessedHitCount,
		EnemyMeleeSweepRadius,
		EnemyMeleeSweepHalfHeight);

	for (const FHitResult& HitResult : HitResults)
	{
		const AActor* TargetActor = HitResult.GetActor();
		const ITunicCombatTargetInterface* CombatTarget = Cast<ITunicCombatTargetInterface>(TargetActor);
		if (!TargetActor || !CombatTarget)
		{
			continue;
		}

		const UTunicAbilitySystemComponent* TargetAbilitySystemComponent = CombatTarget->GetCombatTargetAbilitySystemComponent();
		const UTunicAttributeSet* TargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();
		UE_LOG(LogLpQuestEnemyGasDebug, Display, TEXT("Enemy melee hit sweep target | Character=%s | Target=%s | TargetASC=%s | TargetAttributeSet=%s | TargetHealth=%.1f/%.1f | TargetStamina=%.1f/%.1f | ImpactPoint=%s | TargetLocalRole=%d | TargetRemoteRole=%d"),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor),
			*GetNameSafe(TargetAbilitySystemComponent),
			*GetNameSafe(TargetAttributeSet),
			TargetAttributeSet ? TargetAttributeSet->GetHealth() : 0.0f,
			TargetAttributeSet ? TargetAttributeSet->GetMaxHealth() : 0.0f,
			TargetAttributeSet ? TargetAttributeSet->GetStamina() : 0.0f,
			TargetAttributeSet ? TargetAttributeSet->GetMaxStamina() : 0.0f,
			*HitResult.ImpactPoint.ToCompactString(),
			static_cast<int32>(TargetActor->GetLocalRole()),
			static_cast<int32>(TargetActor->GetRemoteRole()));
	}
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

