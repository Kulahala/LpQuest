// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/TunicPlayerCharacter.h"

#include "Ability/TunicAbilitySystemComponent.h"
#include "Ability/TunicAttributeSet.h"
#include "Ability/TunicDamageGameplayEffect.h"
#include "Ability/TunicGameplayAbility_LightAttack.h"
#include "Ability/TunicStaminaRegenGameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Character/TunicEnemyCharacter.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
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
	CameraBoom->bDoCollisionTest = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	PrimaryActorTick.bCanEverTick = true;

	LightAttackDamageEffectClass = UTunicDamageGameplayEffect::StaticClass();
	LightAttackAbilityClass = UTunicGameplayAbility_LightAttack::StaticClass();
	StaminaRegenEffectClass = UTunicStaminaRegenGameplayEffect::StaticClass();
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

void ATunicPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateFacingToMouse(DeltaSeconds, false);
	DrawAttributeDebug();
}

void ATunicPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (!Controller || MovementVector.IsNearlyZero())
	{
		return;
	}

	FVector ScreenUpDirection = FVector::ForwardVector;
	FVector ScreenRightDirection = FVector::RightVector;
	GetFixedViewMovementDirections(ScreenUpDirection, ScreenRightDirection);

	AddMovementInput(ScreenUpDirection, -MovementVector.Y);
	AddMovementInput(ScreenRightDirection, -MovementVector.X);
}

void ATunicPlayerCharacter::StartDodge(const FInputActionValue& Value)
{
	RequestDodge();
}

void ATunicPlayerCharacter::LightAttack(const FInputActionValue& Value)
{
	UpdateFacingToMouse(0.0f, true);
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

void ATunicPlayerCharacter::SetLightAttackHitSweepLoggingEnabled(bool bEnabled)
{
	bLogLightAttackHitSweep = bEnabled;
}

void ATunicPlayerCharacter::SetAttributeDebugDrawEnabled(bool bEnabled)
{
	bDrawAttributeDebug = bEnabled;
}

void ATunicPlayerCharacter::SetLightAttackTargetQueryLoggingEnabled(bool bEnabled)
{
	SetLightAttackHitSweepLoggingEnabled(bEnabled);
}

bool ATunicPlayerCharacter::GetMouseFacingYaw(float& OutYaw) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return false;
	}

	FVector CursorWorldLocation = FVector::ZeroVector;
	FVector CursorWorldDirection = FVector::ZeroVector;
	if (!PlayerController->DeprojectMousePositionToWorld(CursorWorldLocation, CursorWorldDirection) || FMath::IsNearlyZero(CursorWorldDirection.Z))
	{
		return false;
	}

	FVector AimPoint = FVector::ZeroVector;
	FHitResult CursorHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MouseFacingTrace), false, this);
	QueryParams.AddIgnoredActor(this);

	UWorld* World = GetWorld();
	if (World && World->LineTraceSingleByChannel(CursorHit, CursorWorldLocation, CursorWorldLocation + CursorWorldDirection * 100000.0f, ECC_Visibility, QueryParams) && CursorHit.bBlockingHit)
	{
		AimPoint = CursorHit.ImpactPoint;
	}
	else
	{
		const float PlaneDistance = (GetActorLocation().Z - CursorWorldLocation.Z) / CursorWorldDirection.Z;
		if (PlaneDistance < 0.0f)
		{
			return false;
		}

		AimPoint = CursorWorldLocation + CursorWorldDirection * PlaneDistance;
	}

	const FVector FacingDirection = AimPoint - GetActorLocation();
	const FVector FacingDirection2D(FacingDirection.X, FacingDirection.Y, 0.0f);
	if (FacingDirection2D.IsNearlyZero())
	{
		return false;
	}

	OutYaw = FacingDirection2D.Rotation().Yaw;
	return true;
}

void ATunicPlayerCharacter::GetFixedViewMovementDirections(FVector& OutScreenUpDirection, FVector& OutScreenRightDirection) const
{
	const UCameraComponent* MovementCamera = FollowCamera ? FollowCamera.Get() : nullptr;
	if (!MovementCamera)
	{
		return;
	}

	OutScreenUpDirection = FVector::VectorPlaneProject(MovementCamera->GetUpVector(), FVector::UpVector).GetSafeNormal();
	OutScreenRightDirection = FVector::VectorPlaneProject(MovementCamera->GetRightVector(), FVector::UpVector).GetSafeNormal();

	if (OutScreenUpDirection.IsNearlyZero() || OutScreenRightDirection.IsNearlyZero())
	{
		OutScreenUpDirection = FVector::ForwardVector;
		OutScreenRightDirection = FVector::RightVector;
	}
}

void ATunicPlayerCharacter::ServerSetFacingYaw_Implementation(float NewYaw, bool bSnap)
{
	if (!FMath::IsFinite(NewYaw))
	{
		return;
	}

	if (bSnap)
	{
		SetActorRotation(FRotator(0.0f, FRotator::NormalizeAxis(NewYaw), 0.0f));
		return;
	}

	SetActorRotation(FRotator(0.0f, FRotator::NormalizeAxis(NewYaw), 0.0f));
}

void ATunicPlayerCharacter::UpdateFacingToMouse(float DeltaSeconds, bool bForceServerUpdate)
{
	if (!bFaceMouseOnAttack || !IsLocallyControlled())
	{
		return;
	}

	TimeSinceLastMouseFacingServerUpdate += FMath::Max(0.0f, DeltaSeconds);

	float NewYaw = 0.0f;
	if (!GetMouseFacingYaw(NewYaw))
	{
		return;
	}

	SetActorRotation(FRotator(0.0f, FRotator::NormalizeAxis(NewYaw), 0.0f));
	if (!HasAuthority())
	{
		if (bForceServerUpdate || TimeSinceLastMouseFacingServerUpdate >= MouseFacingServerUpdateInterval)
		{
			ServerSetFacingYaw(NewYaw, true);
			TimeSinceLastMouseFacingServerUpdate = 0.0f;
		}
	}
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
	GrantDefaultAbilities(PlayerAbilitySystemComponent);
	ApplyDefaultEffects(PlayerAbilitySystemComponent);
	LogPlayerAbilitySystemDebug(TunicPlayerState, PlayerAbilitySystemComponent, PlayerAttributeSet);
}

void ATunicPlayerCharacter::GrantDefaultAbilities(UTunicAbilitySystemComponent* PlayerAbilitySystemComponent)
{
	if (!HasAuthority() || !PlayerAbilitySystemComponent || !LightAttackAbilityClass)
	{
		return;
	}

	if (PlayerAbilitySystemComponent->FindAbilitySpecFromClass(LightAttackAbilityClass))
	{
		return;
	}

	PlayerAbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(LightAttackAbilityClass, 1));
}

void ATunicPlayerCharacter::ApplyDefaultEffects(UTunicAbilitySystemComponent* PlayerAbilitySystemComponent)
{
	if (!HasAuthority() || !PlayerAbilitySystemComponent || !StaminaRegenEffectClass)
	{
		return;
	}

	if (StaminaRegenEffectHandle.IsValid())
	{
		return;
	}

	const TArray<FActiveGameplayEffectHandle> ActiveEffectHandles = PlayerAbilitySystemComponent->GetActiveEffects(FGameplayEffectQuery());
	for (const FActiveGameplayEffectHandle& ActiveEffectHandle : ActiveEffectHandles)
	{
		const UGameplayEffect* ActiveEffect = PlayerAbilitySystemComponent->GetGameplayEffectDefForHandle(ActiveEffectHandle);
		if (ActiveEffect && ActiveEffect->GetClass() == StaminaRegenEffectClass)
		{
			StaminaRegenEffectHandle = ActiveEffectHandle;
			return;
		}
	}

	FGameplayEffectContextHandle EffectContext = PlayerAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	StaminaRegenEffectHandle = PlayerAbilitySystemComponent->ApplyGameplayEffectToSelf(StaminaRegenEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, EffectContext);
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
	const bool bShouldUseAbility = GetTunicAbilitySystemComponent() && LightAttackAbilityClass;
	if (bShouldUseAbility)
	{
		TryActivateLightAttackAbility();
		return;
	}

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
	const bool bPlayedMontage = PlayLightAttackMontage();
	if (bRunLightAttackHitWindowOnRequest && !bPlayedMontage)
	{
		BeginLightAttackHitWindow();
		ProcessLightAttackHitWindow();
		EndLightAttackHitWindow();
	}
	OnLightAttackRequested();
}

void ATunicPlayerCharacter::ExecuteLightAttackAbility()
{
	HandleLightAttackRequest();
}

bool ATunicPlayerCharacter::TryActivateLightAttackAbility()
{
	UTunicAbilitySystemComponent* PlayerAbilitySystemComponent = GetTunicAbilitySystemComponent();
	if (!PlayerAbilitySystemComponent)
	{
		return false;
	}

	if (const FGameplayTag LightAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack.Sword.Light"), false); LightAttackTag.IsValid())
	{
		FGameplayTagContainer LightAttackAbilityTags;
		LightAttackAbilityTags.AddTag(LightAttackTag);
		if (PlayerAbilitySystemComponent->TryActivateAbilitiesByTag(LightAttackAbilityTags))
		{
			return true;
		}
	}

	return LightAttackAbilityClass ? PlayerAbilitySystemComponent->TryActivateAbilityByClass(LightAttackAbilityClass) : false;
}

bool ATunicPlayerCharacter::PlayLightAttackMontage()
{
	if (!HasAuthority() || !LightAttackMontage)
	{
		return false;
	}

	MulticastPlayLightAttackMontage(LightAttackMontage);
	return true;
}

void ATunicPlayerCharacter::MulticastPlayLightAttackMontage_Implementation(UAnimMontage* MontageToPlay)
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

	AnimInstance->Montage_Play(MontageToPlay);
}

void ATunicPlayerCharacter::LogLightAttackRequestDebug() const
{
	LogServerInputRequestDebug(TEXT("Light attack"), bLogLightAttackRequests);
}

void ATunicPlayerCharacter::DrawAttributeDebug() const
{
	if (!bDrawAttributeDebug || !AttributeSet)
	{
		return;
	}

	const FString DebugText = FString::Printf(
		TEXT("Player\nHP %.0f/%.0f\nSTA %.0f/%.0f"),
		AttributeSet->GetHealth(),
		AttributeSet->GetMaxHealth(),
		AttributeSet->GetStamina(),
		AttributeSet->GetMaxStamina());

	DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.0f, 0.0f, 130.0f), DebugText, nullptr, FColor::Cyan, 0.0f, true, 1.1f);
}

void ATunicPlayerCharacter::BeginLightAttackHitWindow()
{
	if (!HasAuthority())
	{
		return;
	}

	bLightAttackHitWindowActive = true;
	LightAttackHitTargets.Reset();
}

void ATunicPlayerCharacter::ProcessLightAttackHitWindow()
{
	if (!HasAuthority() || !bLightAttackHitWindowActive)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector SweepStart = GetLightAttackSweepPoint(LightAttackSweepStartOffset);
	const FVector SweepEnd = GetLightAttackSweepPoint(LightAttackSweepEndOffset);
	const FCollisionShape SweepShape = FCollisionShape::MakeCapsule(LightAttackSweepRadius, LightAttackSweepHalfHeight);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LightAttackHitSweep), false, this);
	TArray<FHitResult> HitResults;
	World->SweepMultiByChannel(
		HitResults,
		SweepStart,
		SweepEnd,
		FQuat::Identity,
		ECC_Pawn,
		SweepShape,
		QueryParams);

	int32 AppliedHitCount = 0;
	for (const FHitResult& HitResult : HitResults)
	{
		ATunicEnemyCharacter* EnemyCharacter = Cast<ATunicEnemyCharacter>(HitResult.GetActor());
		if (!EnemyCharacter || EnemyCharacter->IsDead() || LightAttackHitTargets.Contains(EnemyCharacter))
		{
			continue;
		}

		LightAttackHitTargets.Add(EnemyCharacter);
		ApplyLightAttackDebugDamage(EnemyCharacter);
		++AppliedHitCount;
	}

	LogLightAttackHitSweepDebug(HitResults, AppliedHitCount);
}

void ATunicPlayerCharacter::EndLightAttackHitWindow()
{
	if (!HasAuthority())
	{
		return;
	}

	bLightAttackHitWindowActive = false;
	LightAttackHitTargets.Reset();
}

FVector ATunicPlayerCharacter::GetLightAttackSweepPoint(const FVector& LocalOffset) const
{
	return GetActorLocation() + GetActorRotation().RotateVector(LocalOffset);
}

void ATunicPlayerCharacter::LogLightAttackHitSweepDebug(const TArray<FHitResult>& HitResults, int32 AppliedHitCount) const
{
	if (!bLogLightAttackHitSweep)
	{
		return;
	}

	UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack hit sweep completed | Character=%s | SweepStart=%s | SweepEnd=%s | HitCount=%d | AppliedHitCount=%d | Radius=%.1f | HalfHeight=%.1f"),
		*GetNameSafe(this),
		*GetLightAttackSweepPoint(LightAttackSweepStartOffset).ToCompactString(),
		*GetLightAttackSweepPoint(LightAttackSweepEndOffset).ToCompactString(),
		HitResults.Num(),
		AppliedHitCount,
		LightAttackSweepRadius,
		LightAttackSweepHalfHeight);

	if (HitResults.IsEmpty())
	{
		return;
	}

	for (const FHitResult& HitResult : HitResults)
	{
		const ATunicEnemyCharacter* TargetEnemy = Cast<ATunicEnemyCharacter>(HitResult.GetActor());
		if (!TargetEnemy)
		{
			continue;
		}

		const UTunicAbilitySystemComponent* TargetAbilitySystemComponent = TargetEnemy->GetTunicAbilitySystemComponent();
		const UTunicAttributeSet* TargetAttributeSet = TargetEnemy->GetAttributeSet();

		UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack hit sweep target | Character=%s | Target=%s | TargetASC=%s | TargetAttributeSet=%s | TargetHealth=%.1f/%.1f | TargetStamina=%.1f/%.1f | ImpactPoint=%s | TargetLocalRole=%d | TargetRemoteRole=%d"),
			*GetNameSafe(this),
			*GetNameSafe(TargetEnemy),
			*GetNameSafe(TargetAbilitySystemComponent),
			*GetNameSafe(TargetAttributeSet),
			TargetAttributeSet ? TargetAttributeSet->GetHealth() : 0.0f,
			TargetAttributeSet ? TargetAttributeSet->GetMaxHealth() : 0.0f,
			TargetAttributeSet ? TargetAttributeSet->GetStamina() : 0.0f,
			TargetAttributeSet ? TargetAttributeSet->GetMaxStamina() : 0.0f,
			*HitResult.ImpactPoint.ToCompactString(),
			static_cast<int32>(TargetEnemy->GetLocalRole()),
			static_cast<int32>(TargetEnemy->GetRemoteRole()));
	}
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

