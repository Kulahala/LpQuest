# plan.md - LpQuest Active Implementation Plan

## How To Read This File

Hard rule: before starting, replacing, reviewing, or committing a stage, first check whether the previous completed / rejected / superseded stage outcome is recorded in `docs/archive/stage-log.md`. If the previous stage is complete and missing from the archive, archive it before editing this file or implementing the new stage. Keep this file as the current working board, not the long-term stage history.

Read the `Active Snapshot` as the current handoff. Historical completed stage notes live in `docs/archive/stage-log.md`; use that archive only for context and do not treat old TODOs, old risks, old validation gaps, or old debugging notes as current truth without checking live source/assets and recent commits.

Reusable compiler/runtime/debugging lessons belong in server-memory MCP, not in this file. If a future stage hits a repeated error pattern, record the durable lesson in memory and keep `plan.md` limited to the current stage outcome.

## Active Snapshot

Current working state:

- Core loop is in place: server-authoritative player/enemy combat, enemy death, player death v1, RunState, Portal Event, Floor Stub, floor-wave spawn sources, enemy-configured shared XP, shared run Level, pending upgrade choices, and fixed run-local upgrade selection.
- Enemy AI uses `ATunicEnemyAIController` + StateTree + Sight/awareness policy + last-known investigation state + spline patrol route. StateTree owns intent flow; GameplayAbility / combat hit windows own final attack results.
- Enemy melee attack uses `GameplayAbility -> telegraph -> Montage -> AnimNotify hit window -> server attack shape query -> GameplayEffect damage`; missing Montage or missing hit-window Notify should fail visibly instead of applying fallback damage.
- Dodge movement is client-predicted through GAS `LocalPredicted` activation and `UAbilityTask_ApplyRootMotionConstantForce`; invulnerability, damage immunity, cooldown/action-lock, Health, and final correction remain server-authoritative.
- Ordinary floor waves now use placed `ATunicFloorWaveEnemySpawnSource` actors as the single authored source for enemy class, count, location, optional radius sampling, tracking, and cleanup.
- Enemy death XP uses each enemy's own `ExperienceReward`; `ExperienceReward=0` is the no-XP configuration. Ordinary enemy death does not move RunState out of `CombatActive`.
- Portal Event can be started through the unified `E` interact path. Portal can spawn one configured existing enemy Blueprint as a test Boss; charging is blocked until that Boss is dead or gone.

Most recent commits:

- `119c88c [Feature] 统一楼层敌人生成源（Unify Floor Enemy Spawn Source）`
- `f957a7b [Feature] 添加敌人死亡拾取掉落（Add Enemy Drop Pickup Source）`
- `bf3b338 [Feature] 添加拾取装备交互闭环（Add Pickup Equipment Interaction）`
- `ea5a693 [Refactor] 清理战斗命中兜底与命名（Clean Combat Hit Window Fallback and Naming）`
- `7fed449 [Refactor] 清理敌人旧近战扫掠字段（Clean Enemy Legacy Sweep Fields）`

Current stage:

- No active implementation stage after `Enemy Reward Ownership Cleanup v1`.
- `Enemy Reward Ownership Cleanup v1` is completed, user build / PIE validation passed, strict review passed, and the outcome is recorded in `docs/archive/stage-log.md`.
- Recent implemented foundation: all enemies that reach `ATunicGameMode::HandleEnemyDeath()` now use their own `ExperienceReward`; `ExperienceReward=0` is the no-XP configuration. `ATunicEncounterSpawner` and Portal pressure XP budget were removed.
- Next stage should be chosen deliberately from the near-term TODOs below rather than inferred from this completed stage.

Near-term TODOs:

- `Enemy Variant Profile Cleanup v2` after current Portal work: when Guard / Wild / Spawn / Elite only differ by tuning, fold them toward one enemy class plus profile DataAssets instead of keeping separate Blueprints for each variant.
- `Safe Travel Portal v1`: later, when safe zones / shop areas / hub return need pure interaction travel, add a separate lightweight interactable travel portal instead of subclassing the combat Portal Event actor or disabling most of its Boss/charging/pressure behavior.
- `Equipment DataAsset / Inventory Slots v2`: add only when multiple weapons, item stats, icons, descriptions, ability grants, or switching between several carried items become real scope.
- `Enemy Drop Profile / Equipment DataAsset v2`: add only when several enemies repeat drop setup, random drop pools, rarity, weights, ownership policy, or equipment stats become real scope.

Current accepted risks:

- Pressure spawn uses Portal-local logic rather than the floor-wave spawn source. Add `Portal Spawn Source Integration v2` or `Spawn Director / Spawn Profile v2` only when exploration spawns, portal pressure, Boss minions, or floor scaling repeat the same spawn ownership logic.
- Pressure spawn points are simple fixed Actor references on the Portal. Add richer spawn selection only when map layout, navigation safety, or enemy archetype rules need it.
- Boss v1 remains an existing enemy Blueprint spawned by Portal. Add Boss-specific class/component only when phase logic, Boss UI, arena mechanics, or dedicated Boss-only state appears.
- Portal Event state is still global. Add per-portal event ownership only when multiple active portals can coexist.
- Player death v1 is still one-way. Revive, respawn, spectator/free-camera, party wipe resolution, and floor fail UI remain deferred.
- `FloorTransitionStub` remains intentionally active as the current placeholder floor loop. Rename or replace it only when real floor travel/loading exists.

Build command:

```powershell
& "D:\UE\UE_5.8\Engine\Build\BatchFiles\Build.bat" LpQuestEditor Win64 Development -Project="E:\UnRealEngine\LpQuest\LpQuest.uproject" -WaitMutex
```
