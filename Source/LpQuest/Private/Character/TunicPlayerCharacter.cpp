// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/TunicPlayerCharacter.h"

#include "Ability/TunicAbilitySystemComponent.h"
#include "Ability/TunicAttributeSet.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
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
	OnLightAttackRequested();
}

void ATunicPlayerCharacter::LogLightAttackRequestDebug() const
{
	LogServerInputRequestDebug(TEXT("Light attack"), bLogLightAttackRequests);
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

