// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/TunicPlayerCharacter.h"

#include "Ability/TunicAbilitySystemComponent.h"
#include "Ability/TunicAttributeSet.h"
#include "Ability/TunicDamageGameplayEffect.h"
#include "Ability/TunicGameplayAbility_Dodge.h"
#include "Ability/TunicGameplayAbility_LightAttack.h"
#include "Ability/TunicStaminaRegenGameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/TunicCombatRules.h"
#include "Combat/TunicCombatTargetInterface.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/RootMotionSource.h"
#include "GameFramework/SpringArmComponent.h"
#include "Game/TunicGameMode.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "Player/TunicPlayerState.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLpQuestGasDebug, Log, All);

namespace
{
	const FName TunicDodgeManualMovementRootMotionSourceName(TEXT("TunicDodgeManualMovement"));

	bool IsTunicCombatTargetInvulnerable(const ITunicCombatTargetInterface* CombatTarget)
	{
		const UTunicAbilitySystemComponent* TargetAbilitySystemComponent = CombatTarget ? CombatTarget->GetCombatTargetAbilitySystemComponent() : nullptr;
		const FGameplayTag InvulnerableTag = FGameplayTag::RequestGameplayTag(TEXT("State.Invulnerable"), false);
		return TargetAbilitySystemComponent && InvulnerableTag.IsValid() && TargetAbilitySystemComponent->HasMatchingGameplayTag(InvulnerableTag);
	}
}

ATunicPlayerCharacter::ATunicPlayerCharacter(const FObjectInitializer& ObjectInitializer)
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

	LightAttackDamageEffectClass = UTunicDamageGameplayEffect::StaticClass();
	LightAttackAbilityClass = UTunicGameplayAbility_LightAttack::StaticClass();
	DodgeAbilityClass = UTunicGameplayAbility_Dodge::StaticClass();
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

void ATunicPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATunicPlayerCharacter, bIsDead);
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

	if (!bIsDead)
	{
		UpdateFacingToMouse(DeltaSeconds, false);
	}
	DrawAttributeDebug();
}

bool ATunicPlayerCharacter::IsCombatTargetAvailable() const
{
	return !bIsDead && GetTunicAbilitySystemComponent() && GetAttributeSet();
}

ETunicCombatTeam ATunicPlayerCharacter::GetCombatTargetTeam() const
{
	return ETunicCombatTeam::Player;
}

UTunicAbilitySystemComponent* ATunicPlayerCharacter::GetCombatTargetAbilitySystemComponent() const
{
	return GetTunicAbilitySystemComponent();
}

UTunicAttributeSet* ATunicPlayerCharacter::GetCombatTargetAttributeSet() const
{
	return GetAttributeSet();
}

void ATunicPlayerCharacter::HandleCombatTargetHitReaction(AActor* InstigatorActor)
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	MulticastPlayHitReaction(InstigatorActor);
}

void ATunicPlayerCharacter::BeginCombatHitWindow(FName)
{
	BeginLightAttackHitWindow();
}

void ATunicPlayerCharacter::ProcessCombatHitWindow(FName)
{
	ProcessLightAttackHitWindow();
}

void ATunicPlayerCharacter::EndCombatHitWindow(FName)
{
	EndLightAttackHitWindow();
}

void ATunicPlayerCharacter::Move(const FInputActionValue& Value)
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

void ATunicPlayerCharacter::StartDodge(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	RequestDodge();
}

void ATunicPlayerCharacter::LightAttack(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	UpdateFacingToMouse(0.0f, true);
	RequestLightAttack();
}

void ATunicPlayerCharacter::HeavyAttack(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	OnHeavyAttackRequested();
}

void ATunicPlayerCharacter::StartAim(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	OnAimStarted();
}

void ATunicPlayerCharacter::StopAim(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	OnAimStopped();
}

void ATunicPlayerCharacter::Interact(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	OnInteractRequested();
}

void ATunicPlayerCharacter::SwitchWeapon(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	OnSwitchWeaponRequested();
}

void ATunicPlayerCharacter::OnDodgeRequested_Implementation()
{
}

void ATunicPlayerCharacter::OnLightAttackRequested_Implementation()
{
}

void ATunicPlayerCharacter::OnHitReaction_Implementation(AActor*)
{
}

void ATunicPlayerCharacter::OnDeathStateChanged_Implementation(bool)
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

void ATunicPlayerCharacter::UpdateFacingToMouse(float DeltaSeconds, bool bForceServerUpdate)
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
	BindHealthChangeDelegate(PlayerAbilitySystemComponent);
	if (HasAuthority() && !bIsDead && PlayerAttributeSet->GetHealth() <= 0.0f)
	{
		SetDead(true);
	}
	GrantDefaultAbilities(PlayerAbilitySystemComponent);
	ApplyDefaultEffects(PlayerAbilitySystemComponent);
	LogPlayerAbilitySystemDebug(TunicPlayerState, PlayerAbilitySystemComponent, PlayerAttributeSet);
}

void ATunicPlayerCharacter::BindHealthChangeDelegate(UTunicAbilitySystemComponent* PlayerAbilitySystemComponent)
{
	if (!HasAuthority() || !PlayerAbilitySystemComponent)
	{
		return;
	}

	FOnGameplayAttributeValueChange& HealthChangeDelegate = PlayerAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UTunicAttributeSet::GetHealthAttribute());
	HealthChangeDelegate.RemoveAll(this);
	HealthChangeDelegate.AddUObject(this, &ATunicPlayerCharacter::HandleHealthChanged);
}

void ATunicPlayerCharacter::GrantDefaultAbilities(UTunicAbilitySystemComponent* PlayerAbilitySystemComponent)
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

void ATunicPlayerCharacter::ServerRequestLightAttack_Implementation()
{
	if (bIsDead)
	{
		return;
	}

	HandleLightAttackRequest();
}

void ATunicPlayerCharacter::HandleLightAttackRequest()
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

void ATunicPlayerCharacter::ExecuteLightAttackAbility()
{
	if (bIsDead)
	{
		return;
	}

	HandleLightAttackRequest();
}

bool ATunicPlayerCharacter::TryActivateLightAttackAbility()
{
	if (bIsDead)
	{
		return false;
	}

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
		TimerDelegate.BindUObject(this, &ATunicPlayerCharacter::CheckLightAttackMontageHitWindowTriggered, MontageSerial);
		World->GetTimerManager().SetTimer(
			TimerHandle,
			TimerDelegate,
			LightAttackMontage->GetPlayLength() + 0.1f,
			false);
	}

	return true;
}

void ATunicPlayerCharacter::MulticastPlayLightAttackMontage_Implementation(UAnimMontage* MontageToPlay)
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

void ATunicPlayerCharacter::MulticastPlayHitReaction_Implementation(AActor* InstigatorActor)
{
	if (bIsDead)
	{
		return;
	}

	if (bLogHitReaction)
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

void ATunicPlayerCharacter::ClientShowDodgeInvulnerabilitySuccess_Implementation(AActor* InstigatorActor)
{
	if (!bShowDodgeInvulnerabilityDebugMessage)
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

void ATunicPlayerCharacter::CheckLightAttackMontageHitWindowTriggered(int32 MontageSerial)
{
	if (!HasAuthority() || !LightAttackMontage || LightAttackMontageActivationSerial != MontageSerial || LightAttackMontageHitWindowSerial == MontageSerial)
	{
		return;
	}

	UE_LOG(LogLpQuestGasDebug, Warning, TEXT("Light attack montage completed without hit-window notify | Character=%s | Montage=%s"),
		*GetNameSafe(this),
		*GetNameSafe(LightAttackMontage));
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
		TEXT("Player%s\nHP %.0f/%.0f\nSTA %.0f/%.0f"),
		bIsDead ? TEXT(" DEAD") : TEXT(""),
		AttributeSet->GetHealth(),
		AttributeSet->GetMaxHealth(),
		AttributeSet->GetStamina(),
		AttributeSet->GetMaxStamina());

	DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.0f, 0.0f, 130.0f), DebugText, nullptr, bIsDead ? FColor::Silver : FColor::Cyan, 0.0f, true, 1.1f);
}

void ATunicPlayerCharacter::BeginLightAttackHitWindow()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	bLightAttackHitWindowActive = true;
	LightAttackMontageHitWindowSerial = LightAttackMontageActivationSerial;
	LightAttackHitTargets.Reset();
}

void ATunicPlayerCharacter::ProcessLightAttackHitWindow()
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
		ITunicCombatTargetInterface* CombatTarget = Cast<ITunicCombatTargetInterface>(TargetActor);
		if (!TargetActor || !CombatTarget || !CombatTarget->IsCombatTargetAvailable() || LightAttackHitTargets.Contains(TargetActor) || FTunicCombatRules::IsSelfHit(this, TargetActor))
		{
			continue;
		}

		LightAttackHitTargets.Add(TargetActor);
		HandleLightAttackTargetHit(TargetActor, CombatTarget);
		++ProcessedHitCount;
	}

	LogLightAttackHitSweepDebug(HitResults, ProcessedHitCount);
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

void ATunicPlayerCharacter::LogLightAttackHitSweepDebug(const TArray<FHitResult>& HitResults, int32 ProcessedHitCount) const
{
	if (!bLogLightAttackHitSweep)
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
		const ITunicCombatTargetInterface* CombatTarget = Cast<ITunicCombatTargetInterface>(TargetActor);
		if (!TargetActor || !CombatTarget)
		{
			continue;
		}

		const UTunicAbilitySystemComponent* TargetAbilitySystemComponent = CombatTarget->GetCombatTargetAbilitySystemComponent();
		const UTunicAttributeSet* TargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();

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

void ATunicPlayerCharacter::ApplyLightAttackDebugDamage(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget)
{
	if (!bApplyLightAttackDebugDamage)
	{
		return;
	}

	if (!TargetActor || !CombatTarget)
	{
		UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack debug damage skipped: no target | Character=%s"),
			*GetNameSafe(this));
		return;
	}

	if (!CombatTarget->IsCombatTargetAvailable())
	{
		UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack debug damage skipped: target unavailable | Character=%s | Target=%s"),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor));
		return;
	}

	UTunicAbilitySystemComponent* TargetAbilitySystemComponent = CombatTarget->GetCombatTargetAbilitySystemComponent();
	UTunicAttributeSet* TargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();
	if (!TargetAbilitySystemComponent || !TargetAttributeSet || !LightAttackDamageEffectClass)
	{
		UE_LOG(LogLpQuestGasDebug, Warning, TEXT("Light attack debug damage failed: missing target GAS data | Character=%s | Target=%s | TargetASC=%s | TargetAttributeSet=%s | EffectClass=%s"),
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
		if (ATunicPlayerCharacter* TargetPlayerCharacter = Cast<ATunicPlayerCharacter>(TargetActor))
		{
			TargetPlayerCharacter->NotifyDodgeInvulnerabilitySuccess(this);
		}

		UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack debug damage skipped: target invulnerable | Character=%s | Target=%s | EffectClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(TargetActor),
			*GetNameSafe(LightAttackDamageEffectClass.Get()));
		return;
	}

	const float HealthBefore = TargetAttributeSet->GetHealth();
	const UTunicAbilitySystemComponent* SourceAbilitySystemComponent = GetTunicAbilitySystemComponent();
	FGameplayEffectContextHandle EffectContext = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->MakeEffectContext() : TargetAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	TargetAbilitySystemComponent->BP_ApplyGameplayEffectToSelf(LightAttackDamageEffectClass, 1.0f, EffectContext);
	const float HealthAfter = TargetAttributeSet->GetHealth();

	UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack debug damage applied | Character=%s | Target=%s | EffectClass=%s | TargetHealth=%.1f->%.1f"),
		*GetNameSafe(this),
		*GetNameSafe(TargetActor),
		*GetNameSafe(LightAttackDamageEffectClass.Get()),
		HealthBefore,
		HealthAfter);
}

void ATunicPlayerCharacter::HandleLightAttackTargetHit(AActor* TargetActor, ITunicCombatTargetInterface* CombatTarget)
{
	if (!TargetActor || !CombatTarget)
	{
		return;
	}

	const bool bCanApplyDamage = FTunicCombatRules::CanApplyDamage(this, TargetActor, *CombatTarget);
	const bool bCanTriggerHitReaction = FTunicCombatRules::CanTriggerHitReaction(this, TargetActor, *CombatTarget);
	const bool bIsTargetInvulnerable = IsTunicCombatTargetInvulnerable(CombatTarget);
	const UTunicAttributeSet* TargetAttributeSet = CombatTarget->GetCombatTargetAttributeSet();
	const float HealthBefore = TargetAttributeSet ? TargetAttributeSet->GetHealth() : 0.0f;

	if (bCanApplyDamage)
	{
		ApplyLightAttackDebugDamage(TargetActor, CombatTarget);
	}
	else
	{
		UE_LOG(LogLpQuestGasDebug, Display, TEXT("Light attack hit target without damage | Character=%s | Target=%s | SourceTeam=%d | TargetTeam=%d"),
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

void ATunicPlayerCharacter::RequestDodge()
{
	if (bIsDead)
	{
		return;
	}

	const bool bShouldUseAbility = GetTunicAbilitySystemComponent() && DodgeAbilityClass;
	if (bShouldUseAbility)
	{
		if (HasAuthority())
		{
			TryActivateDodgeAbility();
		}
		else
		{
			ServerRequestDodge(GetDodgeDirection());
		}
		return;
	}

	if (HasAuthority())
	{
		HandleDodgeRequest();
		return;
	}

	ServerRequestDodge(GetDodgeDirection());
}

void ATunicPlayerCharacter::ServerRequestDodge_Implementation(FVector RequestedDodgeDirection)
{
	if (bIsDead)
	{
		return;
	}

	const FVector NormalizedRequestedDirection = RequestedDodgeDirection.GetSafeNormal2D();
	if (!NormalizedRequestedDirection.IsNearlyZero())
	{
		LastNonZeroMovementInputWorldDirection = NormalizedRequestedDirection;
		LastMovementInputDirectionUpdateTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	}

	const bool bShouldUseAbility = GetTunicAbilitySystemComponent() && DodgeAbilityClass;
	if (bShouldUseAbility)
	{
		TryActivateDodgeAbility();
		return;
	}

	HandleDodgeRequest();
}

void ATunicPlayerCharacter::HandleDodgeRequest()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	LogDodgeRequestDebug();
	PlayDodgeMontage();
	StartManualDodgeMovement();
	OnDodgeRequested();
}

void ATunicPlayerCharacter::ExecuteDodgeAbility()
{
	if (bIsDead)
	{
		return;
	}

	HandleDodgeRequest();
}

void ATunicPlayerCharacter::NotifyDodgeInvulnerabilitySuccess(AActor* InstigatorActor)
{
	if (!HasAuthority())
	{
		return;
	}

	ClientShowDodgeInvulnerabilitySuccess(InstigatorActor);
}

bool ATunicPlayerCharacter::TryActivateDodgeAbility()
{
	if (bIsDead)
	{
		return false;
	}

	UTunicAbilitySystemComponent* PlayerAbilitySystemComponent = GetTunicAbilitySystemComponent();
	if (!PlayerAbilitySystemComponent)
	{
		return false;
	}

	if (const FGameplayTag DodgeTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Movement.Dodge"), false); DodgeTag.IsValid())
	{
		FGameplayTagContainer DodgeAbilityTags;
		DodgeAbilityTags.AddTag(DodgeTag);
		if (PlayerAbilitySystemComponent->TryActivateAbilitiesByTag(DodgeAbilityTags))
		{
			return true;
		}
	}

	return DodgeAbilityClass ? PlayerAbilitySystemComponent->TryActivateAbilityByClass(DodgeAbilityClass) : false;
}

bool ATunicPlayerCharacter::PlayDodgeMontage()
{
	if (!HasAuthority() || bIsDead || !DodgeMontage)
	{
		return false;
	}

	MulticastPlayDodgeMontage(DodgeMontage);
	return true;
}

void ATunicPlayerCharacter::MulticastPlayDodgeMontage_Implementation(UAnimMontage* MontageToPlay)
{
	if (bIsDead || !MontageToPlay)
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
		{
			AnimInstance->Montage_Play(MontageToPlay);
		}
	}
}

bool ATunicPlayerCharacter::StartManualDodgeMovement()
{
	if (!HasAuthority() || bIsDead)
	{
		return false;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		UE_LOG(LogLpQuestGasDebug, Warning, TEXT("Dodge manual movement skipped: missing movement component | Character=%s"),
			*GetNameSafe(this));
		return false;
	}

	if (DodgeDuration <= UE_SMALL_NUMBER || DodgeDistance <= 0.0f)
	{
		UE_LOG(LogLpQuestGasDebug, Warning, TEXT("Dodge manual movement skipped: invalid tuning | Character=%s | Distance=%.1f | Duration=%.3f"),
			*GetNameSafe(this),
			DodgeDistance,
			DodgeDuration);
		return false;
	}

	const FVector DodgeDirection = GetDodgeDirection();
	if (DodgeDirection.IsNearlyZero())
	{
		UE_LOG(LogLpQuestGasDebug, Warning, TEXT("Dodge manual movement skipped: no valid direction | Character=%s"),
			*GetNameSafe(this));
		return false;
	}

	MovementComponent->RemoveRootMotionSource(TunicDodgeManualMovementRootMotionSourceName);

	TSharedPtr<FRootMotionSource_ConstantForce> DodgeRootMotionSource = MakeShared<FRootMotionSource_ConstantForce>();
	DodgeRootMotionSource->InstanceName = TunicDodgeManualMovementRootMotionSourceName;
	DodgeRootMotionSource->AccumulateMode = ERootMotionAccumulateMode::Override;
	DodgeRootMotionSource->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
	DodgeRootMotionSource->Priority = static_cast<uint16>(FMath::Clamp(DodgeRootMotionSourcePriority, 0, static_cast<int32>(MAX_uint16)));
	DodgeRootMotionSource->Duration = DodgeDuration;
	DodgeRootMotionSource->Force = DodgeDirection * (DodgeDistance / DodgeDuration);

	const uint16 RootMotionSourceId = MovementComponent->ApplyRootMotionSource(DodgeRootMotionSource);

	UE_LOG(LogLpQuestGasDebug, Display, TEXT("Dodge manual movement started | Character=%s | Direction=%s | Distance=%.1f | Duration=%.3f | RootMotionSourceId=%u"),
		*GetNameSafe(this),
		*DodgeDirection.ToCompactString(),
		DodgeDistance,
		DodgeDuration,
		RootMotionSourceId);

	return true;
}

FVector ATunicPlayerCharacter::GetDodgeDirection() const
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

bool ATunicPlayerCharacter::IsDead() const
{
	return bIsDead;
}

void ATunicPlayerCharacter::OnRep_IsDead()
{
	ApplyDeathState();
}

void ATunicPlayerCharacter::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!HasAuthority() || bIsDead || ChangeData.NewValue > 0.0f)
	{
		return;
	}

	SetDead(true);
}

void ATunicPlayerCharacter::SetDead(bool bNewIsDead)
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
			TunicGameMode->EvaluatePartyWipe();
		}
	}
}

void ATunicPlayerCharacter::ApplyDeathState()
{
	if (!bIsDead)
	{
		return;
	}

	bLightAttackHitWindowActive = false;
	LightAttackHitTargets.Reset();

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	UTunicAbilitySystemComponent* PlayerAbilitySystemComponent = GetTunicAbilitySystemComponent();
	if (HasAuthority() && PlayerAbilitySystemComponent)
	{
		const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"), false);
		if (DeadTag.IsValid())
		{
			PlayerAbilitySystemComponent->SetLooseGameplayTagCount(DeadTag, 1, EGameplayTagReplicationState::TagAndCountToAll);
		}
	}

	if (bLogDeathState)
	{
		const UTunicAttributeSet* PlayerAttributeSet = GetAttributeSet();
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

void ATunicPlayerCharacter::PlayPresentationMontage(UAnimMontage* MontageToPlay, bool bStopAllMontages)
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

