// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TunicCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
	MainHand,
	Ranged,
	Spell,
	Utility
};

UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
	Sword,
	Bow,
	Staff,
	BossOnly
};

UENUM(BlueprintType)
enum class ECombatAction : uint8
{
	None,
	LightAttack,
	HeavyAttack,
	Dodge,
	BlockOrAim,
	Cast,
	Interact,
	HitReact,
	Death
};

UENUM(BlueprintType)
enum class EEnemyArchetype : uint8
{
	MeleeMinion,
	RangedMinion,
	Mage,
	Boss
};

UENUM(BlueprintType)
enum class ETunicElementType : uint8
{
	None,
	Fire,
	Ice,
	Lightning
};

UENUM(BlueprintType)
enum class ETunicCombatTeam : uint8
{
	Neutral,
	Player,
	Enemy
};

