// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/TunicPlayerCharacter.h"

#include "Ability/TunicAbilitySystemComponent.h"
#include "Ability/TunicAttributeSet.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Player/TunicPlayerState.h"

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
	OnDodgeRequested();
}

void ATunicPlayerCharacter::LightAttack(const FInputActionValue& Value)
{
	OnLightAttackRequested();
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
}

