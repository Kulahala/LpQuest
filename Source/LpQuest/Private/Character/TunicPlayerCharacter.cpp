// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/TunicPlayerCharacter.h"

#include "Ability/TunicAbilitySystemComponent.h"
#include "Ability/TunicAttributeSet.h"
#include "Ability/TunicDamageGameplayEffect.h"
#include "Camera/CameraComponent.h"
#include "Character/TunicEnemyCharacter.h"
#include "EnhancedInputComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Player/TunicPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestGasDebug, Log, All);

ATunicPlayerCharacter::ATunicPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 900.0f;
	CameraBoom->SetRelativeRotation(FRotator(-55.0f, -45.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	LightAttackDamageEffectClass = UTunicDamageGameplayEffect::StaticClass();
}

void ATunicPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitializePlayerAbilitySystem();
}

void ATunicPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitializePlayerAbilitySystem();
}

void ATunicPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATunicPlayerCharacter::Move);
	}

	if (DodgeAction)
	{
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &ATunicPlayerCharacter::StartDodge);
	}

	if (LightAttackAction)
	{
		EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Started, this, &ATunicPlayerCharacter::LightAttack);
	}

	if (HeavyAttackAction)
	{
		EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &ATunicPlayerCharacter::HeavyAttack);
	}

	if (AimAction)
	{
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ATunicPlayerCharacter::StartAim);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ATunicPlayerCharacter::StopAim);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &ATunicPlayerCharacter::StopAim);
	}

	if (InteractAction)
	{
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ATunicPlayerCharacter::Interact);
	}

	if (SwitchWeaponAction)
	{
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &ATunicPlayerCharacter::SwitchWeapon);
	}
}

void ATunicPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (!Controller || MovementVector.IsNearlyZero())
	{
		return;
	}

	const float CameraYaw = CameraBoom ? CameraBoom->GetComponentRotation().Yaw : GetActorRotation().Yaw;
	const FRotator YawRotation(0.0f, CameraYaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, -MovementVector.Y);
	AddMovementInput(RightDirection, -MovementVector.X);
}

void ATunicPlayerCharacter::StartDodge(const FInputActionValue& Value)
{
	RequestDodge();
}

void ATunicPlayerCharacter::LightAttack(const FInputActionValue& Value)
{
	RequestLightAttack();
}

void ATunicPlayerCharacter::HeavyAttack(const FInputActionValue& Value)
{
	OnHeavyAttackRequested();
}

void ATunicPlayerCharacter::StartAim(const FInputActionValue& Value)
{
	OnAimStarted();
}

void ATunicPlayerCharacter::StopAim(const FInputActionValue& Value)
{
	OnAimStopped();
}

void ATunicPlayerCharacter::Interact(const FInputActionValue& Value)
{
	OnInteractRequested();
}

void ATunicPlayerCharacter::SwitchWeapon(const FInputActionValue& Value)
{
	OnSwitchWeaponRequested();
}

void ATunicPlayerCharacter::OnDodgeRequested_Implementation()
{
}

void ATunicPlayerCharacter::OnLightAttackRequested_Implementation()
{
}

void ATunicPlayerCharacter::OnHeavyAttackRequested_Implementation()
{
}

void ATunicPlayerCharacter::OnAimStarted_Implementation()
{
}

void ATunicPlayerCharacter::OnAimStopped_Implementation()
{
}

void ATunicPlayerCharacter::OnInteractRequested_Implementation()
{
}

void ATunicPlayerCharacter::OnSwitchWeaponRequested_Implementation()
{
}

void ATunicPlayerCharacter::SetAbilitySystemInitializationLoggingEnabled(bool bEnabled)
{
	bLogAbilitySystemInitialization = bEnabled;
}

void ATunicPlayerCharacter::SetDodgeRequestLoggingEnabled(bool bEnabled)
{
	bLogDodgeRequests = bEnabled;
}

void ATunicPlayerCharacter::SetLightAttackRequestLoggingEnabled(bool bEnabled)
{
	bLogLightAttackRequests = bEnabled;
}

void ATunicPlayerCharacter::SetLightAttackTargetQueryLoggingEnabled(bool bEnabled)
{
	bLogLightAttackTargetQuery = bEnabled;
}

void ATunicPlayerCharacter::InitializePlayerAbilitySystem()
{
	ATunicPlayerState* TunicPlayerState = GetPlayerState<ATunicPlayerState>();
	if (!TunicPlayerState)
	{
		return;
	}

	UTunicAbilitySystemComponent* PlayerAbilitySystemComponent = TunicPlayerState->GetTunicAbilitySystemComponent();
	UTunicAttributeSet* PlayerAttributeSet = TunicPlayerState->GetAttributeSet();
	if (!PlayerAbilitySystemComponent || !PlayerAttributeSet)
	{
		return;
	}

	PlayerAbilitySystemComponent->InitAbilityActorInfo(TunicPlayerState, this);
	SetAbilitySystemReferences(PlayerAbilitySystemComponent, PlayerAttributeSet);
	LogPlayerAbilitySystemDebug(TunicPlayerState, PlayerAbilitySystemComponent, PlayerAttributeSet);
}

void ATunicPlayerCharacter::LogPlayerAbilitySystemDebug(const ATunicPlayerState* TunicPlayerState, const UTunicAbilitySystemComponent* PlayerAbilitySystemComponent, const UTunicAttributeSet* PlayerAttributeSet) const
{
	if (!bLogAbilitySystemInitialization)
	{
		return;
	}

	const AActor* AscOwnerActor = PlayerAbilitySystemComponent ? PlayerAbilitySystemComponent->GetOwnerActor() : nullptr;
	const AActor* AscAvatarActor = PlayerAbilitySystemComponent ? PlayerAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;

	UE_LOG(LogLpQuestGasDebug, Display, TEXT("Player ASC initialized | Character=%s | PlayerState=%s | ASC=%s | OwnerActor=%s | AvatarActor=%s | AttributeSet=%s | Health=%.1f/%.1f | Stamina=%.1f/%.1f | Authority=%s | LocalRole=%d | RemoteRole=%d"),
		*GetNameSafe(this),
		*GetNameSafe(TunicPlayerState),
		*GetNameSafe(PlayerAbilitySystemComponent),
		*GetNameSafe(AscOwnerActor),
		*GetNameSafe(AscAvatarActor),
		*GetNameSafe(PlayerAttributeSet),
		PlayerAttributeSet ? PlayerAttributeSet->GetHealth() : 0.0f,
		PlayerAttributeSet ? PlayerAttributeSet->GetMaxHealth() : 0.0f,
		PlayerAttributeSet ? PlayerAttributeSet->GetStamina() : 0.0f,
		PlayerAttributeSet ? PlayerAttributeSet->GetMaxStamina() : 0.0f,
		HasAuthority() ? TEXT("true") : TEXT("false"),
		static_cast<int32>(GetLocalRole()),
		static_cast<int32>(GetRemoteRole()));
}

void ATunicPlayerCharacter::RequestLightAttack()
{
	if (HasAuthority())
	{
		HandleLightAttackRequest();
		return;
	}

	ServerRequestLightAttack();
}

void ATunicPlayerCharacter::ServerRequestLightAttack_Implementation()
{
	HandleLightAttackRequest();
}

void ATunicPlayerCharacter::HandleLightAttackRequest()
{
	if (!HasAuthority())
	{
		return;
	}

	LogLightAttackRequestDebug();
	ATunicEnemyCharacter* TargetEnemy = FindLightAttackDebugTarget();
	if (bLogLightAttackTargetQuery)
	{
		LogLightAttackTargetDebug(TargetEnemy);
	}
	ApplyLightAttackDebugDamage(TargetEnemy);
	OnLightAttackRequested();
}

void ATunicPlayerCharacter::LogLightAttackRequestDebug() const
{
	LogServerInputRequestDebug(TEXT("Light attack"), bLogLightAttackRequests);
}

ATunicEnemyCharacter* ATunicPlayerCharacter::FindLightAttackDebugTarget() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector QueryCenter = GetActorLocation();
	const FCollisionShape QueryShape = FCollisionShape::MakeSphere(LightAttackTargetQueryRange);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LightAttackDebugTargetQuery), false, this);
	TArray<FOverlapResult> OverlapResults;
	World->OverlapMultiByChannel(
		OverlapResults,
		QueryCenter,
		FQuat::Identity,
		ECC_Pawn,
		QueryShape,
		QueryParams);

	ATunicEnemyCharacter* ClosestEnemy = nullptr;
	float ClosestDistanceSquared = UE_BIG_NUMBER;

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		ATunicEnemyCharacter* EnemyCharacter = Cast<ATunicEnemyCharacter>(OverlapResult.GetActor());
		if (EnemyCharacter && !EnemyCharacter->IsDead())
		{
			const float DistanceSquared = FVector::DistSquared(GetActorLocation(), EnemyCharacter->GetActorLocation());
			if (DistanceSquared < ClosestDistanceSquared)
			{
				ClosestEnemy = EnemyCharacter;
				ClosestDistanceSquared = DistanceSquared;
			}
		}
	}

	return ClosestEnemy;
}

void ATunicPlayerCharacter::LogLightAttackTargetDebug(const ATunicEnemyCharacter* TargetEnemy) const
{
	if (!bLogLightAttackTargetQuery)
	{
		return;
	}

	if (!TargetEnemy)
	{
		UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack target query found no enemy | Character=%s | QueryRange=%.1f"),
			*GetNameSafe(this),
			LightAttackTargetQueryRange);
		return;
	}

	const UTunicAbilitySystemComponent* TargetAbilitySystemComponent = TargetEnemy->GetTunicAbilitySystemComponent();
	const UTunicAttributeSet* TargetAttributeSet = TargetEnemy->GetAttributeSet();

	UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack target query found enemy | Character=%s | Target=%s | TargetASC=%s | TargetAttributeSet=%s | TargetHealth=%.1f/%.1f | TargetStamina=%.1f/%.1f | TargetDistance=%.1f | TargetLocalRole=%d | TargetRemoteRole=%d"),
		*GetNameSafe(this),
		*GetNameSafe(TargetEnemy),
		*GetNameSafe(TargetAbilitySystemComponent),
		*GetNameSafe(TargetAttributeSet),
		TargetAttributeSet ? TargetAttributeSet->GetHealth() : 0.0f,
		TargetAttributeSet ? TargetAttributeSet->GetMaxHealth() : 0.0f,
		TargetAttributeSet ? TargetAttributeSet->GetStamina() : 0.0f,
		TargetAttributeSet ? TargetAttributeSet->GetMaxStamina() : 0.0f,
		FVector::Dist(GetActorLocation(), TargetEnemy->GetActorLocation()),
		static_cast<int32>(TargetEnemy->GetLocalRole()),
		static_cast<int32>(TargetEnemy->GetRemoteRole()));
}

void ATunicPlayerCharacter::ApplyLightAttackDebugDamage(ATunicEnemyCharacter* TargetEnemy) const
{
	if (!bApplyLightAttackDebugDamage)
	{
		return;
	}

	if (!TargetEnemy)
	{
		UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack debug damage skipped: no target | Character=%s"),
			*GetNameSafe(this));
		return;
	}

	if (TargetEnemy->IsDead())
	{
		UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack debug damage skipped: target is dead | Character=%s | Target=%s"),
			*GetNameSafe(this),
			*GetNameSafe(TargetEnemy));
		return;
	}

	UTunicAbilitySystemComponent* TargetAbilitySystemComponent = TargetEnemy->GetTunicAbilitySystemComponent();
	UTunicAttributeSet* TargetAttributeSet = TargetEnemy->GetAttributeSet();
	if (!TargetAbilitySystemComponent || !TargetAttributeSet || !LightAttackDamageEffectClass)
	{
		UE_LOG(LogLpQuestGasDebug, Warning, TEXT("Light attack debug damage failed: missing target GAS data | Character=%s | Target=%s | TargetASC=%s | TargetAttributeSet=%s | EffectClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(TargetEnemy),
			*GetNameSafe(TargetAbilitySystemComponent),
			*GetNameSafe(TargetAttributeSet),
			*GetNameSafe(LightAttackDamageEffectClass.Get()));
		return;
	}

	const float HealthBefore = TargetAttributeSet->GetHealth();
	const UTunicAbilitySystemComponent* SourceAbilitySystemComponent = GetTunicAbilitySystemComponent();
	FGameplayEffectContextHandle EffectContext = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->MakeEffectContext() : TargetAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	TargetAbilitySystemComponent->BP_ApplyGameplayEffectToSelf(LightAttackDamageEffectClass, 1.0f, EffectContext);

	UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack debug damage applied | Character=%s | Target=%s | EffectClass=%s | TargetHealth=%.1f->%.1f"),
		*GetNameSafe(this),
		*GetNameSafe(TargetEnemy),
		*GetNameSafe(LightAttackDamageEffectClass.Get()),
		HealthBefore,
		TargetAttributeSet->GetHealth());
}

void ATunicPlayerCharacter::RequestDodge()
{
	if (HasAuthority())
	{
		HandleDodgeRequest();
		return;
	}

	ServerRequestDodge();
}

void ATunicPlayerCharacter::ServerRequestDodge_Implementation()
{
	HandleDodgeRequest();
}

void ATunicPlayerCharacter::HandleDodgeRequest()
{
	if (!HasAuthority())
	{
		return;
	}

	LogDodgeRequestDebug();
	OnDodgeRequested();
}

void ATunicPlayerCharacter::LogDodgeRequestDebug() const
{
	LogServerInputRequestDebug(TEXT("Dodge"), bLogDodgeRequests);
}

void ATunicPlayerCharacter::LogServerInputRequestDebug(const TCHAR* RequestName, bool bShouldLog) const
{
	if (!bShouldLog)
	{
		return;
	}

	UE_LOG(LogLpQuestGasDebug, Display, TEXT("%s request accepted on server | Character=%s | PlayerState=%s | ASC=%s | AttributeSet=%s | Health=%.1f/%.1f | Stamina=%.1f/%.1f | LocalRole=%d | RemoteRole=%d"),
		RequestName,
		*GetNameSafe(this),
		*GetNameSafe(GetPlayerState()),
		*GetNameSafe(GetTunicAbilitySystemComponent()),
		*GetNameSafe(GetAttributeSet()),
		GetAttributeSet() ? GetAttributeSet()->GetHealth() : 0.0f,
		GetAttributeSet() ? GetAttributeSet()->GetMaxHealth() : 0.0f,
		GetAttributeSet() ? GetAttributeSet()->GetStamina() : 0.0f,
		GetAttributeSet() ? GetAttributeSet()->GetMaxStamina() : 0.0f,
		static_cast<int32>(GetLocalRole()),
		static_cast<int32>(GetRemoteRole()));
}

