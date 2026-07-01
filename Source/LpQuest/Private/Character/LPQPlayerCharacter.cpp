// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/LPQPlayerCharacter.h"

#include "Ability/LPQAbilitySystemComponent.h"
#include "Ability/LPQAttributeSet.h"
#include "Ability/LPQDamageGameplayEffect.h"
#include "Ability/LPQGameplayAbility_Dodge.h"
#include "Ability/LPQGameplayAbility_LightAttack.h"
#include "Ability/LPQStaminaRegenGameplayEffect.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/LPQCombatRules.h"
#include "Combat/LPQCombatTargetInterface.h"
#include "Debug/LPQDebugSettings.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Game/LPQGameMode.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Interaction/LPQInteractableInterface.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "Player/LPQPlayerState.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestGasDebug, Log, All);

namespace
{
	const FName LPQPlayerDodgeEventTagName(TEXT("GameplayEvent.Movement.Dodge"));

	bool IsPlayerLightAttackTargetInvulnerable(const ILPQCombatTargetInterface* CombatTarget)
	{
		const ULPQAbilitySystemComponent* TargetAbilitySystemComponent = CombatTarget ? CombatTarget->GetCombatTargetAbilitySystemComponent() : nullptr;
		const FGameplayTag InvulnerableTag = FGameplayTag::RequestGameplayTag(TEXT("State.Invulnerable"), false);
		return TargetAbilitySystemComponent && InvulnerableTag.IsValid() && TargetAbilitySystemComponent->HasMatchingGameplayTag(InvulnerableTag);
	}
}

ALPQPlayerCharacter::ALPQPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1300.0f;
	CameraBoom->SetRelativeRotation(FRotator(-60.0f, -45.0f, 0.0f));
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

	LightAttackDamageEffectClass = ULPQDamageGameplayEffect::StaticClass();
	LightAttackAbilityClass = ULPQGameplayAbility_LightAttack::StaticClass();
	DodgeAbilityClass = ULPQGameplayAbility_Dodge::StaticClass();
	StaminaRegenEffectClass = ULPQStaminaRegenGameplayEffect::StaticClass();
}

void ALPQPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitializePlayerAbilitySystem();
}

void ALPQPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitializePlayerAbilitySystem();
}

void ALPQPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALPQPlayerCharacter, bIsDead);
}

void ALPQPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALPQPlayerCharacter::Move);
	}

	if (DodgeAction)
	{
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &ALPQPlayerCharacter::StartDodge);
	}

	if (LightAttackAction)
	{
		EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Started, this, &ALPQPlayerCharacter::LightAttack);
	}

	if (HeavyAttackAction)
	{
		EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &ALPQPlayerCharacter::HeavyAttack);
	}

	if (AimAction)
	{
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ALPQPlayerCharacter::StartAim);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ALPQPlayerCharacter::StopAim);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &ALPQPlayerCharacter::StopAim);
	}

	if (InteractAction)
	{
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ALPQPlayerCharacter::Interact);
	}

	if (SwitchWeaponAction)
	{
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &ALPQPlayerCharacter::SwitchWeapon);
	}
}

void ALPQPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsDead)
	{
		UpdateFacingToMouse(DeltaSeconds, false);
	}
	DrawAttributeDebug();
}

bool ALPQPlayerCharacter::IsCombatTargetAvailable() const
{
	return !bIsDead && GetTunicAbilitySystemComponent() && GetAttributeSet();
}

ELPQCombatTeam ALPQPlayerCharacter::GetCombatTargetTeam() const
{
	return ELPQCombatTeam::Player;
}

ULPQAbilitySystemComponent* ALPQPlayerCharacter::GetCombatTargetAbilitySystemComponent() const
{
	return GetTunicAbilitySystemComponent();
}

ULPQAttributeSet* ALPQPlayerCharacter::GetCombatTargetAttributeSet() const
{
	return GetAttributeSet();
}

void ALPQPlayerCharacter::HandleCombatTargetHitReaction(AActor* InstigatorActor)
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	MulticastPlayHitReaction(InstigatorActor);
}

void ALPQPlayerCharacter::BeginCombatHitWindow(FName)
{
	BeginLightAttackHitWindow();
}

void ALPQPlayerCharacter::ProcessCombatHitWindow(FName)
{
	ProcessLightAttackHitWindow();
}

void ALPQPlayerCharacter::EndCombatHitWindow(FName)
{
	EndLightAttackHitWindow();
}

void ALPQPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (bIsDead || !Controller || MovementVector.IsNearlyZero())
	{
		return;
	}

	FVector ScreenUpDirection = FVector::ForwardVector;
	FVector ScreenRightDirection = FVector::RightVector;
	GetFixedViewMovementDirections(ScreenUpDirection, ScreenRightDirection);

	const FVector MovementWorldDirection = ((ScreenUpDirection * -MovementVector.Y) + (ScreenRightDirection * -MovementVector.X)).GetSafeNormal2D();
	if (!MovementWorldDirection.IsNearlyZero())
	{
		LastNonZeroMovementInputWorldDirection = MovementWorldDirection;
		LastMovementInputDirectionUpdateTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	}

	AddMovementInput(ScreenUpDirection, -MovementVector.Y);
	AddMovementInput(ScreenRightDirection, -MovementVector.X);
}

void ALPQPlayerCharacter::StartDodge(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	RequestDodge();
}

void ALPQPlayerCharacter::LightAttack(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	UpdateFacingToMouse(0.0f, true);
	RequestLightAttack();
}

void ALPQPlayerCharacter::HeavyAttack(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	OnHeavyAttackRequested();
}

void ALPQPlayerCharacter::StartAim(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	OnAimStarted();
}

void ALPQPlayerCharacter::StopAim(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	OnAimStopped();
}

void ALPQPlayerCharacter::Interact(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	OnInteractRequested();
	ServerRequestInteract();
}

void ALPQPlayerCharacter::SwitchWeapon(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	OnSwitchWeaponRequested();
}

void ALPQPlayerCharacter::OnDodgeRequested_Implementation()
{
}

void ALPQPlayerCharacter::OnLightAttackRequested_Implementation()
{
}

void ALPQPlayerCharacter::OnHitReaction_Implementation(AActor*)
{
}

void ALPQPlayerCharacter::OnDeathStateChanged_Implementation(bool)
{
}

void ALPQPlayerCharacter::OnHeavyAttackRequested_Implementation()
{
}

void ALPQPlayerCharacter::OnAimStarted_Implementation()
{
}

void ALPQPlayerCharacter::OnAimStopped_Implementation()
{
}

void ALPQPlayerCharacter::OnInteractRequested_Implementation()
{
}

void ALPQPlayerCharacter::OnSwitchWeaponRequested_Implementation()
{
}

void ALPQPlayerCharacter::SetAbilitySystemInitializationLoggingEnabled(bool bEnabled)
{
	bLogAbilitySystemInitialization = bEnabled;
}

void ALPQPlayerCharacter::SetDodgeRequestLoggingEnabled(bool bEnabled)
{
	bLogDodgeRequests = bEnabled;
}

void ALPQPlayerCharacter::SetLightAttackRequestLoggingEnabled(bool bEnabled)
{
	bLogLightAttackRequests = bEnabled;
}

void ALPQPlayerCharacter::SetLightAttackHitSweepLoggingEnabled(bool bEnabled)
{
	bLogLightAttackHitSweep = bEnabled;
}

void ALPQPlayerCharacter::SetAttributeDebugDrawEnabled(bool bEnabled)
{
	bDrawAttributeDebug = bEnabled;
}

bool ALPQPlayerCharacter::GetMouseFacingYaw(float& OutYaw) const
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

void ALPQPlayerCharacter::GetFixedViewMovementDirections(FVector& OutScreenUpDirection, FVector& OutScreenRightDirection) const
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

void ALPQPlayerCharacter::ServerSetFacingYaw_Implementation(float NewYaw, bool bSnap)
{
	if (bIsDead)
	{
		return;
	}

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

void ALPQPlayerCharacter::UpdateFacingToMouse(float DeltaSeconds, bool bForceServerUpdate)
{
	if (bIsDead || !bFaceMouseOnAttack || !IsLocallyControlled())
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

void ALPQPlayerCharacter::InitializePlayerAbilitySystem()
{
	ALPQPlayerState* LpQuestPlayerState = GetPlayerState<ALPQPlayerState>();
	if (!LpQuestPlayerState)
	{
		return;
	}

	ULPQAbilitySystemComponent* PlayerAbilitySystemComponent = LpQuestPlayerState->GetTunicAbilitySystemComponent();
	ULPQAttributeSet* PlayerAttributeSet = LpQuestPlayerState->GetAttributeSet();
	if (!PlayerAbilitySystemComponent || !PlayerAttributeSet)
	{
		return;
	}

	PlayerAbilitySystemComponent->InitAbilityActorInfo(LpQuestPlayerState, this);
	SetAbilitySystemReferences(PlayerAbilitySystemComponent, PlayerAttributeSet);
	BindHealthChangeDelegate(PlayerAbilitySystemComponent);
	if (HasAuthority() && !bIsDead && PlayerAttributeSet->GetHealth() <= 0.0f)
	{
		SetDead(true);
	}
	GrantDefaultAbilities(PlayerAbilitySystemComponent);
	ApplyDefaultEffects(PlayerAbilitySystemComponent);
	LogPlayerAbilitySystemDebug(LpQuestPlayerState, PlayerAbilitySystemComponent, PlayerAttributeSet);
}

void ALPQPlayerCharacter::BindHealthChangeDelegate(ULPQAbilitySystemComponent* PlayerAbilitySystemComponent)
{
	if (!HasAuthority() || !PlayerAbilitySystemComponent)
	{
		return;
	}

	FOnGameplayAttributeValueChange& HealthChangeDelegate = PlayerAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ULPQAttributeSet::GetHealthAttribute());
	HealthChangeDelegate.RemoveAll(this);
	HealthChangeDelegate.AddUObject(this, &ALPQPlayerCharacter::HandleHealthChanged);
}

void ALPQPlayerCharacter::GrantDefaultAbilities(ULPQAbilitySystemComponent* PlayerAbilitySystemComponent)
{
	if (!HasAuthority() || !PlayerAbilitySystemComponent)
	{
		return;
	}

	if (LightAttackAbilityClass && !PlayerAbilitySystemComponent->FindAbilitySpecFromClass(LightAttackAbilityClass))
	{
		PlayerAbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(LightAttackAbilityClass, 1));
	}

	if (DodgeAbilityClass && !PlayerAbilitySystemComponent->FindAbilitySpecFromClass(DodgeAbilityClass))
	{
		PlayerAbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DodgeAbilityClass, 1));
	}
}

void ALPQPlayerCharacter::ApplyDefaultEffects(ULPQAbilitySystemComponent* PlayerAbilitySystemComponent)
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

void ALPQPlayerCharacter::LogPlayerAbilitySystemDebug(const ALPQPlayerState* LpQuestPlayerState, const ULPQAbilitySystemComponent* PlayerAbilitySystemComponent, const ULPQAttributeSet* PlayerAttributeSet) const
{
	if (!bLogAbilitySystemInitialization || !FLPQDebugSettings::ShouldLogCombat())
	{
		return;
	}

	const AActor* AscOwnerActor = PlayerAbilitySystemComponent ? PlayerAbilitySystemComponent->GetOwnerActor() : nullptr;
	const AActor* AscAvatarActor = PlayerAbilitySystemComponent ? PlayerAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;

	UE_LOG(LogLpQuestGasDebug, Display, TEXT("Player ASC initialized | Character=%s | PlayerState=%s | ASC=%s | OwnerActor=%s | AvatarActor=%s | AttributeSet=%s | Health=%.1f/%.1f | Stamina=%.1f/%.1f | Authority=%s | LocalRole=%d | RemoteRole=%d"),
		*GetNameSafe(this),
		*GetNameSafe(LpQuestPlayerState),
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

void ALPQPlayerCharacter::RequestLightAttack()
{
	if (bIsDead)
	{
		return;
	}

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

void ALPQPlayerCharacter::ServerRequestLightAttack_Implementation()
{
	if (bIsDead)
	{
		return;
	}

	HandleLightAttackRequest();
}

void ALPQPlayerCharacter::ServerRequestInteract_Implementation()
{
	if (bIsDead)
	{
		return;
	}

	AActor* InteractableActor = FindBestInteractableActor();
	if (!InteractableActor)
	{
		if (bLogInteractionRequests && FLPQDebugSettings::ShouldLogInteraction())
		{
			UE_LOG(LogLpQuestGasDebug, Display, TEXT("Interact request found no target | Character=%s | Radius=%.1f"),
				*GetNameSafe(this),
				InteractionRadius);
		}
		return;
	}

	if (bLogInteractionRequests && FLPQDebugSettings::ShouldLogInteraction())
	{
		UE_LOG(LogLpQuestGasDebug, Display, TEXT("Interact request accepted | Character=%s | Target=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InteractableActor));
	}

	ILPQInteractableInterface::Execute_InteractWithTunicPlayer(InteractableActor, this);
}

void ALPQPlayerCharacter::HandleLightAttackRequest()
{
	if (!HasAuthority() || bIsDead)
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

void ALPQPlayerCharacter::ExecuteLightAttackAbility()
{
	if (bIsDead)
	{
		return;
	}

	HandleLightAttackRequest();
}

bool ALPQPlayerCharacter::TryActivateLightAttackAbility()
{
	if (bIsDead)
	{
		return false;
	}

	ULPQAbilitySystemComponent* PlayerAbilitySystemComponent = GetTunicAbilitySystemComponent();
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

bool ALPQPlayerCharacter::PlayLightAttackMontage()
{
	if (!HasAuthority() || bIsDead || !LightAttackMontage)
	{
		return false;
	}

	const int32 MontageSerial = ++LightAttackMontageActivationSerial;
	LightAttackMontageHitWindowSerial = 0;
	MulticastPlayLightAttackMontage(LightAttackMontage);

	if (UWorld* World = GetWorld())
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(this, &ALPQPlayerCharacter::CheckLightAttackMontageHitWindowTriggered, MontageSerial);
		World->GetTimerManager().SetTimer(
			TimerHandle,
			TimerDelegate,
			LightAttackMontage->GetPlayLength() + 0.1f,
			false);
	}

	return true;
}

void ALPQPlayerCharacter::MulticastPlayLightAttackMontage_Implementation(UAnimMontage* MontageToPlay)
{
	if (bIsDead || !MontageToPlay)
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

void ALPQPlayerCharacter::MulticastPlayHitReaction_Implementation(AActor* InstigatorActor)
{
	if (bIsDead)
	{
		return;
	}

	if (bLogHitReaction && FLPQDebugSettings::ShouldLogCombat())
	{
		UE_LOG(LogLpQuestGasDebug, Display, TEXT("Player hit reaction | Character=%s | Instigator=%s | Authority=%s | LocalRole=%d | RemoteRole=%d"),
			*GetNameSafe(this),
			*GetNameSafe(InstigatorActor),
			HasAuthority() ? TEXT("true") : TEXT("false"),
			static_cast<int32>(GetLocalRole()),
			static_cast<int32>(GetRemoteRole()));
	}

	if (DefaultHitReactionMontage)
	{
		USkeletalMeshComponent* CharacterMesh = GetMesh();
		UAnimInstance* AnimInstance = CharacterMesh ? CharacterMesh->GetAnimInstance() : nullptr;
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(DefaultHitReactionMontage);
		}
	}

	OnHitReaction(InstigatorActor);
}

void ALPQPlayerCharacter::ClientShowDodgeInvulnerabilitySuccess_Implementation(AActor* InstigatorActor)
{
	if (!bShowDodgeInvulnerabilityDebugMessage || !FLPQDebugSettings::ShouldLogCombat())
	{
		return;
	}

	UE_LOG(LogLpQuestGasDebug, Display, TEXT("Dodge invulnerability success | Character=%s | Instigator=%s"),
		*GetNameSafe(this),
		*GetNameSafe(InstigatorActor));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.2f,
			FColor::Cyan,
			FString::Printf(TEXT("Dodge Invulnerable! Blocked hit from %s"), *GetNameSafe(InstigatorActor)));
	}
}

void ALPQPlayerCharacter::CheckLightAttackMontageHitWindowTriggered(int32 MontageSerial)
{
	if (!HasAuthority() || !LightAttackMontage || LightAttackMontageActivationSerial != MontageSerial || LightAttackMontageHitWindowSerial == MontageSerial)
	{
		return;
	}

	UE_LOG(LogLpQuestGasDebug, Warning, TEXT("Light attack montage completed without hit-window notify | Character=%s | Montage=%s"),
		*GetNameSafe(this),
		*GetNameSafe(LightAttackMontage));
}

void ALPQPlayerCharacter::LogLightAttackRequestDebug() const
{
	LogServerInputRequestDebug(TEXT("Light attack"), bLogLightAttackRequests);
}

void ALPQPlayerCharacter::DrawAttributeDebug() const
{
	if (!bDrawAttributeDebug || !FLPQDebugSettings::ShouldDrawAttributes() || !AttributeSet)
	{
		return;
	}

	const FString DebugText = FString::Printf(
		TEXT("Player%s\nHP %.0f/%.0f\nSTA %.0f/%.0f"),
		bIsDead ? TEXT(" DEAD") : TEXT(""),
		AttributeSet->GetHealth(),
		AttributeSet->GetMaxHealth(),
		AttributeSet->GetStamina(),
		AttributeSet->GetMaxStamina());

	DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.0f, 0.0f, 130.0f), DebugText, nullptr, bIsDead ? FColor::Silver : FColor::Cyan, 0.0f, true, 1.1f);
}

void ALPQPlayerCharacter::BeginLightAttackHitWindow()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	bLightAttackHitWindowActive = true;
	LightAttackMontageHitWindowSerial = LightAttackMontageActivationSerial;
	LightAttackHitTargets.Reset();
}

void ALPQPlayerCharacter::ProcessLightAttackHitWindow()
{
	if (!HasAuthority() || !bLightAttackHitWindowActive || bIsDead)
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

	int32 ProcessedHitCount = 0;
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* TargetActor = HitResult.GetActor();
		ILPQCombatTargetInterface* CombatTarget = Cast<ILPQCombatTargetInterface>(TargetActor);
		if (!TargetActor || !CombatTarget || !CombatTarget->IsCombatTargetAvailable() || LightAttackHitTargets.Contains(TargetActor) || FLPQCombatRules::IsSelfHit(this, TargetActor))
		{
			continue;
		}

		LightAttackHitTargets.Add(TargetActor);
		HandleLightAttackTargetHit(TargetActor, CombatTarget);
		++ProcessedHitCount;
	}

	LogLightAttackHitSweepDebug(HitResults, ProcessedHitCount);
}

void ALPQPlayerCharacter::EndLightAttackHitWindow()
{
	if (!HasAuthority())
	{
		return;
	}

	bLightAttackHitWindowActive = false;
	LightAttackHitTargets.Reset();
}

FVector ALPQPlayerCharacter::GetLightAttackSweepPoint(const FVector& LocalOffset) const
{
	return GetActorLocation() + GetActorRotation().RotateVector(LocalOffset);
}

void ALPQPlayerCharacter::LogLightAttackHitSweepDebug(const TArray<FHitResult>& HitResults, int32 ProcessedHitCount) const
{
	if (!bLogLightAttackHitSweep || !FLPQDebugSettings::ShouldLogCombat())
	{
		return;
	}

	UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack hit sweep completed | Character=%s | SweepStart=%s | SweepEnd=%s | HitCount=%d | ProcessedHitCount=%d | Radius=%.1f | HalfHeight=%.1f"),
		*GetNameSafe(this),
		*GetLightAttackSweepPoint(LightAttackSweepStartOffset).ToCompactString(),
		*GetLightAttackSweepPoint(LightAttackSweepEndOffset).ToCompactString(),
		HitResults.Num(),
		ProcessedHitCount,
		LightAttackSweepRadius,
		LightAttackSweepHalfHeight);

	if (HitResults.IsEmpty())
	{
		return;
	}

	for (const FHitResult& HitResult : HitResults)
	{
		const AActor* TargetActor = HitResult.GetActor();
		const ILPQCombatTargetInterface* CombatTarget = Cast<ILPQCombatTargetInterface>(TargetActor);
		if (!TargetActor || !CombatTarget)
		{
			continue;
		}

		const ULPQAbilitySystemComponent* TargetAbilitySystemComponent = CombatTarget->GetCombatTargetAbilitySystemComponent();
		const ULPQAttributeSet* TargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();

		if (FLPQDebugSettings::ShouldLogCombat())
		{
			UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack hit sweep target | Character=%s | Target=%s | TargetASC=%s | TargetAttributeSet=%s | TargetHealth=%.1f/%.1f | TargetStamina=%.1f/%.1f | ImpactPoint=%s | TargetLocalRole=%d | TargetRemoteRole=%d"),
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
}

void ALPQPlayerCharacter::ApplyLightAttackDamage(AActor* TargetActor, ILPQCombatTargetInterface* CombatTarget)
{
	if (!bApplyLightAttackDamage)
	{
		return;
	}

	if (!TargetActor || !CombatTarget)
	{
		if (FLPQDebugSettings::ShouldLogCombat())
		{
			UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack damage skipped: no target | Character=%s"),
				*GetNameSafe(this));
		}
		return;
	}

	if (!CombatTarget->IsCombatTargetAvailable())
	{
		if (FLPQDebugSettings::ShouldLogCombat())
		{
			UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack damage skipped: target unavailable | Character=%s | Target=%s"),
				*GetNameSafe(this),
				*GetNameSafe(TargetActor));
		}
		return;
	}

	ULPQAbilitySystemComponent* TargetAbilitySystemComponent = CombatTarget->GetCombatTargetAbilitySystemComponent();
	ULPQAttributeSet* TargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();
	if (!TargetAbilitySystemComponent || !TargetAttributeSet || !LightAttackDamageEffectClass)
	{
		UE_LOG(LogLpQuestGasDebug, Warning, TEXT("Light attack damage failed: missing target GAS data | Character=%s | Target=%s | TargetASC=%s | TargetAttributeSet=%s | EffectClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor),
			*GetNameSafe(TargetAbilitySystemComponent),
			*GetNameSafe(TargetAttributeSet),
			*GetNameSafe(LightAttackDamageEffectClass.Get()));
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
			UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack damage skipped: target invulnerable | Character=%s | Target=%s | EffectClass=%s"),
				*GetNameSafe(this),
				*GetNameSafe(TargetActor),
				*GetNameSafe(LightAttackDamageEffectClass.Get()));
		}
		return;
	}

	const float HealthBefore = TargetAttributeSet->GetHealth();
	const ULPQAbilitySystemComponent* SourceAbilitySystemComponent = GetTunicAbilitySystemComponent();
	FGameplayEffectContextHandle EffectContext = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->MakeEffectContext() : TargetAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	TargetAbilitySystemComponent->BP_ApplyGameplayEffectToSelf(LightAttackDamageEffectClass, 1.0f, EffectContext);
	const float HealthAfter = TargetAttributeSet->GetHealth();

	if (FLPQDebugSettings::ShouldLogCombat())
	{
		UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack damage applied | Character=%s | Target=%s | EffectClass=%s | TargetHealth=%.1f->%.1f"),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor),
			*GetNameSafe(LightAttackDamageEffectClass.Get()),
			HealthBefore,
			HealthAfter);
	}
}

void ALPQPlayerCharacter::HandleLightAttackTargetHit(AActor* TargetActor, ILPQCombatTargetInterface* CombatTarget)
{
	if (!TargetActor || !CombatTarget)
	{
		return;
	}

	const bool bCanApplyDamage = FLPQCombatRules::CanApplyDamage(this, TargetActor, *CombatTarget);
	const bool bCanTriggerHitReaction = FLPQCombatRules::CanTriggerHitReaction(this, TargetActor, *CombatTarget);
	const bool bIsTargetInvulnerable = IsPlayerLightAttackTargetInvulnerable(CombatTarget);
	const ULPQAttributeSet* TargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();
	const float HealthBefore = TargetAttributeSet ? TargetAttributeSet->GetHealth() : 0.0f;

	if (bCanApplyDamage)
	{
		ApplyLightAttackDamage(TargetActor, CombatTarget);
	}
	else if (FLPQDebugSettings::ShouldLogCombat())
	{
		UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack hit target without damage | Character=%s | Target=%s | SourceTeam=%d | TargetTeam=%d"),
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

void ALPQPlayerCharacter::RequestDodge()
{
	if (bIsDead)
	{
		return;
	}

	const bool bShouldUseAbility = GetTunicAbilitySystemComponent() && DodgeAbilityClass;
	if (bShouldUseAbility)
	{
		if (!TryActivateDodgeAbility(GetDodgeDirection()) && bLogDodgeRequests)
		{
			UE_LOG(LogLpQuestGasDebug, Warning, TEXT("Dodge request did not activate an ability | Character=%s"),
				*GetNameSafe(this));
		}
		return;
	}

	if (bLogDodgeRequests)
	{
		UE_LOG(LogLpQuestGasDebug, Warning, TEXT("Dodge request skipped: missing ASC or DodgeAbilityClass | Character=%s | HasASC=%s | DodgeAbilityClass=%s"),
			*GetNameSafe(this),
			GetTunicAbilitySystemComponent() ? TEXT("true") : TEXT("false"),
			*GetNameSafe(DodgeAbilityClass.Get()));
	}
}

void ALPQPlayerCharacter::ExecuteDodgeAbility()
{
	if (bIsDead)
	{
		return;
	}

	LogDodgeRequestDebug();

	const bool bShouldPlayLocalPresentation = IsLocallyControlled();
	if (bShouldPlayLocalPresentation)
	{
		MulticastPlayDodgeMontage(DodgeMontage);
		bLocalDodgePresentationActive = true;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(LocalDodgePresentationTimerHandle);
			World->GetTimerManager().SetTimer(
				LocalDodgePresentationTimerHandle,
				this,
				&ALPQPlayerCharacter::EndPredictedDodgePresentation,
				FMath::Max(0.0f, DodgeDuration + 0.10f),
				false);
		}
	}
	else if (HasAuthority())
	{
		PlayDodgeMontage();
	}

	OnDodgeRequested();
}

void ALPQPlayerCharacter::NotifyDodgeInvulnerabilitySuccess(AActor* InstigatorActor)
{
	if (!HasAuthority())
	{
		return;
	}

	ClientShowDodgeInvulnerabilitySuccess(InstigatorActor);
}

bool ALPQPlayerCharacter::TryActivateDodgeAbility(const FVector& DodgeDirection)
{
	if (bIsDead)
	{
		return false;
	}

	ULPQAbilitySystemComponent* PlayerAbilitySystemComponent = GetTunicAbilitySystemComponent();
	if (!PlayerAbilitySystemComponent)
	{
		return false;
	}

	const FGameplayTag DodgeEventTag = FGameplayTag::RequestGameplayTag(LPQPlayerDodgeEventTagName, false);
	const FVector NormalizedDodgeDirection = DodgeDirection.GetSafeNormal2D();
	if (DodgeEventTag.IsValid() && !NormalizedDodgeDirection.IsNearlyZero())
	{
		FGameplayEventData DodgeEventData;
		DodgeEventData.EventTag = DodgeEventTag;
		DodgeEventData.Instigator = this;
		DodgeEventData.Target = this;

		FGameplayAbilityTargetData_LocationInfo* DodgeDirectionTargetData = new FGameplayAbilityTargetData_LocationInfo();
		const FVector Origin = GetActorLocation();
		DodgeDirectionTargetData->SourceLocation.LiteralTransform = FTransform(Origin);
		DodgeDirectionTargetData->SourceLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
		DodgeDirectionTargetData->TargetLocation.LiteralTransform = FTransform(Origin + NormalizedDodgeDirection * 100.0f);
		DodgeDirectionTargetData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
		DodgeEventData.TargetData.Add(DodgeDirectionTargetData);

		const int32 TriggeredAbilityCount = PlayerAbilitySystemComponent->HandleGameplayEvent(DodgeEventTag, &DodgeEventData);
		if (bLogDodgeRequests && FLPQDebugSettings::ShouldLogCombat())
		{
			UE_LOG(LogLpQuestGasDebug, Display, TEXT("Dodge gameplay event sent | Character=%s | Direction=%s | TriggeredAbilityCount=%d"),
				*GetNameSafe(this),
				*NormalizedDodgeDirection.ToCompactString(),
				TriggeredAbilityCount);
		}
		return TriggeredAbilityCount > 0;
	}

	return false;
}

bool ALPQPlayerCharacter::PlayDodgeMontage()
{
	if (!HasAuthority() || bIsDead || !DodgeMontage)
	{
		return false;
	}

	MulticastPlayDodgeMontage(DodgeMontage);
	return true;
}

void ALPQPlayerCharacter::MulticastPlayDodgeMontage_Implementation(UAnimMontage* MontageToPlay)
{
	if (bIsDead || !MontageToPlay)
	{
		return;
	}

	if (IsLocallyControlled() && !HasAuthority() && bLocalDodgePresentationActive)
	{
		if (bLogDodgeRequests && FLPQDebugSettings::ShouldLogCombat())
		{
			UE_LOG(LogLpQuestGasDebug, Display, TEXT("Dodge multicast presentation skipped on owning client: local presentation already active | Character=%s"),
				*GetNameSafe(this));
		}
		return;
	}

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
		{
			AnimInstance->Montage_Play(MontageToPlay, FMath::Max(0.01f, DodgeMontagePlayRate));
		}
	}
}

void ALPQPlayerCharacter::EndPredictedDodgePresentation()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LocalDodgePresentationTimerHandle);
	}

	bLocalDodgePresentationActive = false;
}

FVector ALPQPlayerCharacter::GetDodgeDirection() const
{
	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	const bool bHasRecentMovementInputDirection =
		!LastNonZeroMovementInputWorldDirection.IsNearlyZero() &&
		LastMovementInputDirectionUpdateTime >= 0.0f &&
		(CurrentTime - LastMovementInputDirectionUpdateTime) <= DodgeMovementInputDirectionGraceTime;

	if (bHasRecentMovementInputDirection)
	{
		return LastNonZeroMovementInputWorldDirection.GetSafeNormal2D();
	}

	const FVector ForwardDirection = GetActorForwardVector().GetSafeNormal2D();
	return ForwardDirection.IsNearlyZero() ? FVector::ForwardVector : ForwardDirection;
}

float ALPQPlayerCharacter::GetDodgeDistance() const
{
	return DodgeDistance;
}

float ALPQPlayerCharacter::GetDodgeDuration() const
{
	return DodgeDuration;
}

void ALPQPlayerCharacter::LogDodgeRequestDebug() const
{
	LogServerInputRequestDebug(TEXT("Dodge"), bLogDodgeRequests);
}

void ALPQPlayerCharacter::LogServerInputRequestDebug(const TCHAR* RequestName, bool bShouldLog) const
{
	if (!bShouldLog || !FLPQDebugSettings::ShouldLogCombat())
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

AActor* ALPQPlayerCharacter::FindBestInteractableActor()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const float InteractionRadiusSquared = FMath::Square(FMath::Max(1.0f, InteractionRadius));
	const FVector PlayerLocation = GetActorLocation();
	AActor* BestInteractableActor = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		AActor* CandidateActor = *ActorIt;
		if (!CandidateActor || CandidateActor == this || !CandidateActor->GetClass()->ImplementsInterface(ULPQInteractableInterface::StaticClass()))
		{
			continue;
		}

		const float DistanceSquared2D = FVector::DistSquared2D(PlayerLocation, CandidateActor->GetActorLocation());
		if (DistanceSquared2D > InteractionRadiusSquared || DistanceSquared2D >= BestDistanceSquared)
		{
			continue;
		}

		if (!ILPQInteractableInterface::Execute_CanInteractWithTunicPlayer(CandidateActor, this))
		{
			continue;
		}

		BestInteractableActor = CandidateActor;
		BestDistanceSquared = DistanceSquared2D;
	}

	return BestInteractableActor;
}

bool ALPQPlayerCharacter::IsDead() const
{
	return bIsDead;
}

void ALPQPlayerCharacter::OnRep_IsDead()
{
	ApplyDeathState();
}

void ALPQPlayerCharacter::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!HasAuthority() || bIsDead || ChangeData.NewValue > 0.0f)
	{
		return;
	}

	SetDead(true);
}

void ALPQPlayerCharacter::SetDead(bool bNewIsDead)
{
	if (!HasAuthority() || bIsDead == bNewIsDead)
	{
		return;
	}

	bIsDead = bNewIsDead;
	ApplyDeathState();

	if (bIsDead)
	{
		if (ALPQGameMode* LpQuestGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALPQGameMode>() : nullptr)
		{
			LpQuestGameMode->EvaluatePartyWipe();
		}
	}
}

void ALPQPlayerCharacter::ApplyDeathState()
{
	if (!bIsDead)
	{
		return;
	}

	bLightAttackHitWindowActive = false;
	LightAttackHitTargets.Reset();
	EndPredictedDodgePresentation();

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	ULPQAbilitySystemComponent* PlayerAbilitySystemComponent = GetTunicAbilitySystemComponent();
	if (HasAuthority() && PlayerAbilitySystemComponent)
	{
		const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"), false);
		if (DeadTag.IsValid())
		{
			PlayerAbilitySystemComponent->SetLooseGameplayTagCount(DeadTag, 1, EGameplayTagReplicationState::TagAndCountToAll);
		}
	}

	if (bLogDeathState && FLPQDebugSettings::ShouldLogCombat())
	{
		const ULPQAttributeSet* PlayerAttributeSet = GetAttributeSet();
		UE_LOG(LogLpQuestGasDebug, Display, TEXT("Player entered death state | Character=%s | PlayerState=%s | ASC=%s | AttributeSet=%s | Health=%.1f/%.1f | Authority=%s | LocalRole=%d | RemoteRole=%d"),
			*GetNameSafe(this),
			*GetNameSafe(GetPlayerState()),
			*GetNameSafe(PlayerAbilitySystemComponent),
			*GetNameSafe(PlayerAttributeSet),
			PlayerAttributeSet ? PlayerAttributeSet->GetHealth() : 0.0f,
			PlayerAttributeSet ? PlayerAttributeSet->GetMaxHealth() : 0.0f,
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

void ALPQPlayerCharacter::PlayPresentationMontage(UAnimMontage* MontageToPlay, bool bStopAllMontages)
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

