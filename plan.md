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
- Portal interaction uses one `ATunicPortalActor` with completion mode plus destination ID. CombatEvent mode locks the event to the interacted Portal, can spawn a configured existing enemy Blueprint as a test Boss, and blocks charging until that Boss is dead or gone; DirectFloorExit mode requires all living players in the Portal radius and enters the floor transition stub without `PortalEventActive`.

Most recent commits:

- `fd9ef57 [Feature] 分离传送门目的地与完成模式（Split Portal Destination and Completion Mode）`
- `096d973 [Refactor] 统一敌人死亡经验配置（Use Enemy Config XP Rewards）`
- `119c88c [Feature] 统一楼层敌人生成源（Unify Floor Enemy Spawn Source）`
- `f957a7b [Feature] 添加敌人死亡拾取掉落（Add Enemy Drop Pickup Source）`
- `bf3b338 [Feature] 添加拾取装备交互闭环（Add Pickup Equipment Interaction）`

Current stage:

- No active implementation stage after `Debug Draw Settings Foundation v1`.
- `Debug Draw Settings Foundation v1` is completed, user build / PIE validation passed, strict review passed, and the outcome is recorded in `docs/archive/stage-log.md`.
- Next stage should be chosen deliberately from the near-term TODOs below rather than inferred from this completed stage.

Near-term TODOs:

- `Enemy Variant Profile Cleanup v2` after current Portal work: when Guard / Wild / Spawn / Elite only differ by tuning, fold them toward one enemy class plus profile DataAssets instead of keeping separate Blueprints for each variant.
- `Portal Destination / Floor Route DataAsset v2`: when destination IDs, branch weights, floor types, safe/combat rules, map assets, room pools, or shop/hub rules start repeating, replace the current `FName PortalDestinationId` bridge with authored route/floor data.
- `Equipment DataAsset / Inventory Slots v2`: add only when multiple weapons, item stats, icons, descriptions, ability grants, or switching between several carried items become real scope.
- `Enemy Drop Profile / Equipment DataAsset v2`: add only when several enemies repeat drop setup, random drop pools, rarity, weights, ownership policy, or equipment stats become real scope.

Current accepted risks:

- Pressure spawn uses Portal-local logic rather than the floor-wave spawn source. Add `Portal Spawn Source Integration v2` or `Spawn Director / Spawn Profile v2` only when exploration spawns, portal pressure, Boss minions, or floor scaling repeat the same spawn ownership logic.
- Pressure spawn points are simple fixed Actor references on the Portal. Add richer spawn selection only when map layout, navigation safety, or enemy archetype rules need it.
- Boss v1 remains an existing enemy Blueprint spawned by Portal. Add Boss-specific class/component only when phase logic, Boss UI, arena mechanics, or dedicated Boss-only state appears.
- Portal Event RunState is still global, but the selected CombatEvent Portal owner is now server-only on GameMode. Add a replicated selected-owner presentation surface only if UI/visual lockout needs it.
- Player death v1 is still one-way. Revive, respawn, spectator/free-camera, party wipe resolution, and floor fail UI remain deferred.
- `FloorTransitionStub` remains intentionally active as the current placeholder floor loop. Rename or replace it only when real floor travel/loading exists.

Build command:

```powershell
& "D:\UE\UE_5.8\Engine\Build\BatchFiles\Build.bat" LpQuestEditor Win64 Development -Project="E:\UnRealEngine\LpQuest\LpQuest.uproject" -WaitMutex
```
