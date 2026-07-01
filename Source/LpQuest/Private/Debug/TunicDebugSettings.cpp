// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/TunicDebugSettings.h"

#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarLPQDebugDrawAttributes(
		TEXT("LPQ.Debug.Draw.Attributes"),
		1,
		TEXT("Enables LPQ runtime attribute debug drawing. 0 = off, non-zero = on."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarLPQDebugDrawEnemyMelee(
		TEXT("LPQ.Debug.Draw.EnemyMelee"),
		1,
		TEXT("Enables LPQ enemy melee telegraph and hit-shape debug drawing. 0 = off, non-zero = on."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarLPQDebugDrawPatrolRoutes(
		TEXT("LPQ.Debug.Draw.PatrolRoutes"),
		1,
		TEXT("Enables LPQ runtime patrol route debug drawing. 0 = off, non-zero = on."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarLPQDebugLogCombat(
		TEXT("LPQ.Debug.Log.Combat"),
		1,
		TEXT("Enables LPQ combat debug logs. 0 = off, non-zero = on."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarLPQDebugLogPortal(
		TEXT("LPQ.Debug.Log.Portal"),
		1,
		TEXT("Enables LPQ portal debug logs. 0 = off, non-zero = on."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarLPQDebugLogInteraction(
		TEXT("LPQ.Debug.Log.Interaction"),
		1,
		TEXT("Enables LPQ interaction and pickup debug logs. 0 = off, non-zero = on."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarLPQDebugLogSpawnSource(
		TEXT("LPQ.Debug.Log.SpawnSource"),
		1,
		TEXT("Enables LPQ spawn source debug logs. 0 = off, non-zero = on."),
		ECVF_Default);
}

bool FLPQDebugSettings::ShouldDrawAttributes()
{
	return CVarLPQDebugDrawAttributes.GetValueOnAnyThread() != 0;
}

bool FLPQDebugSettings::ShouldDrawEnemyMelee()
{
	return CVarLPQDebugDrawEnemyMelee.GetValueOnAnyThread() != 0;
}

bool FLPQDebugSettings::ShouldDrawPatrolRoutes()
{
	return CVarLPQDebugDrawPatrolRoutes.GetValueOnAnyThread() != 0;
}

bool FLPQDebugSettings::ShouldLogCombat()
{
	return CVarLPQDebugLogCombat.GetValueOnAnyThread() != 0;
}

bool FLPQDebugSettings::ShouldLogPortal()
{
	return CVarLPQDebugLogPortal.GetValueOnAnyThread() != 0;
}

bool FLPQDebugSettings::ShouldLogInteraction()
{
	return CVarLPQDebugLogInteraction.GetValueOnAnyThread() != 0;
}

bool FLPQDebugSettings::ShouldLogSpawnSource()
{
	return CVarLPQDebugLogSpawnSource.GetValueOnAnyThread() != 0;
}
