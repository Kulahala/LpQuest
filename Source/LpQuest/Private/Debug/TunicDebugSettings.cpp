// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/TunicDebugSettings.h"

#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarTunicDebugDrawAttributes(
		TEXT("Tunic.Debug.Draw.Attributes"),
		1,
		TEXT("Enables Tunic runtime attribute debug drawing. 0 = off, non-zero = on."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarTunicDebugDrawEnemyMelee(
		TEXT("Tunic.Debug.Draw.EnemyMelee"),
		1,
		TEXT("Enables Tunic enemy melee telegraph and hit-shape debug drawing. 0 = off, non-zero = on."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarTunicDebugDrawPatrolRoutes(
		TEXT("Tunic.Debug.Draw.PatrolRoutes"),
		1,
		TEXT("Enables Tunic runtime patrol route debug drawing. 0 = off, non-zero = on."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarTunicDebugLogCombat(
		TEXT("Tunic.Debug.Log.Combat"),
		1,
		TEXT("Enables Tunic combat debug logs. 0 = off, non-zero = on."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarTunicDebugLogPortal(
		TEXT("Tunic.Debug.Log.Portal"),
		1,
		TEXT("Enables Tunic portal debug logs. 0 = off, non-zero = on."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarTunicDebugLogInteraction(
		TEXT("Tunic.Debug.Log.Interaction"),
		1,
		TEXT("Enables Tunic interaction and pickup debug logs. 0 = off, non-zero = on."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarTunicDebugLogSpawnSource(
		TEXT("Tunic.Debug.Log.SpawnSource"),
		1,
		TEXT("Enables Tunic spawn source debug logs. 0 = off, non-zero = on."),
		ECVF_Default);
}

bool FTunicDebugSettings::ShouldDrawAttributes()
{
	return CVarTunicDebugDrawAttributes.GetValueOnAnyThread() != 0;
}

bool FTunicDebugSettings::ShouldDrawEnemyMelee()
{
	return CVarTunicDebugDrawEnemyMelee.GetValueOnAnyThread() != 0;
}

bool FTunicDebugSettings::ShouldDrawPatrolRoutes()
{
	return CVarTunicDebugDrawPatrolRoutes.GetValueOnAnyThread() != 0;
}

bool FTunicDebugSettings::ShouldLogCombat()
{
	return CVarTunicDebugLogCombat.GetValueOnAnyThread() != 0;
}

bool FTunicDebugSettings::ShouldLogPortal()
{
	return CVarTunicDebugLogPortal.GetValueOnAnyThread() != 0;
}

bool FTunicDebugSettings::ShouldLogInteraction()
{
	return CVarTunicDebugLogInteraction.GetValueOnAnyThread() != 0;
}

bool FTunicDebugSettings::ShouldLogSpawnSource()
{
	return CVarTunicDebugLogSpawnSource.GetValueOnAnyThread() != 0;
}
