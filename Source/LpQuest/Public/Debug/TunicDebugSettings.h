// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct LPQUEST_API FTunicDebugSettings
{
	static bool ShouldDrawAttributes();
	static bool ShouldDrawEnemyMelee();
	static bool ShouldDrawPatrolRoutes();
	static bool ShouldLogCombat();
	static bool ShouldLogPortal();
	static bool ShouldLogInteraction();
	static bool ShouldLogSpawnSource();
};
