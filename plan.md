# plan.md - LpQuest Active Implementation Plan

## Current Status

LpQuest is the UE 5.8 clean project that replaces the earlier LowpolyQuest prototype. Gameplay C++ has been migrated into the `LpQuest` runtime module and adapted to the `LPQUEST_API` export macro.

## Completed

- Built `LpQuestEditor Win64 Development` with UE 5.8 successfully after migration.
- Fixed UE 5.8 deprecation warning by replacing direct `NetUpdateFrequency` assignment with `SetNetUpdateFrequency(100.0f)`.

- Created UE 5.8 C++ project shell: `E:\UnRealEngine\LpQuest`.
- Migrated gameplay C++ skeleton into `Source/LpQuest/Public` and `Source/LpQuest/Private`.
- Updated module dependencies in `LpQuest.Build.cs` for Enhanced Input, GAS, StateTree, AI, and UMG.
- Enabled required plugins in `LpQuest.uproject`: GameplayAbilities, GameplayStateTree, StateTree, AnimationLocomotionLibrary, and AnimationWarping.
- Created content directory skeleton under `Content/_Game` and `Content/_External/KayKit`.
- Rewrote project documentation for UE 5.8, multiplayer PvE, PlayerState-owned ASC, and short/long plan split.

## Next Short Plan

1. Open the UE 5.8 editor and confirm migrated C++ classes are visible.
2. Create minimal Editor assets for stage 1 validation:
   - `IA_Move`, `IA_Dodge`, `IA_LightAttack`, `IA_HeavyAttack`, `IA_Aim`, `IA_Interact`, `IA_SwitchWeapon`
   - `IMC_Player`
   - `BP_TunicPlayerCharacter`
   - `BP_TunicPlayerController`
   - `BP_TunicGameMode`
   - `LV_TestMap`
3. Validate single-player PIE movement and fixed camera.
4. Validate listen-server PIE with two players spawning and moving.
5. Add a small GAS debug path to confirm each player has PlayerState-owned ASC and replicated AttributeSet.

## Build Command

```powershell
& "D:\UE\UE_5.8\Engine\Build\BatchFiles\Build.bat" LpQuestEditor Win64 Development -Project="E:\UnRealEngine\LpQuest\LpQuest.uproject" -WaitMutex
```

## Current Risks

- Existing generated folders from the new project are local build products; do not treat them as source.
- No Blueprint or `.uasset` gameplay assets have been migrated yet.


