# plan.md - LpQuest Active Implementation Plan

## How To Read This File

Hard rule: before starting, replacing, reviewing, or committing a stage, first check whether the previous completed / rejected / superseded stage outcome is recorded in `docs/archive/stage-log.md`. If the previous stage is complete and missing from the archive, archive it before editing this file or implementing the new stage. Keep this file as the current working board, not the long-term stage history.

Read the `Active Snapshot` as the current handoff. Historical completed stage notes live in `docs/archive/stage-log.md`; use that archive only for context and do not treat old TODOs, old risks, old validation gaps, or old debugging notes as current truth without checking live source/assets and recent commits.

Reusable compiler/runtime/debugging lessons belong in server-memory MCP, not in this file. If a future stage hits a repeated error pattern, record the durable lesson in memory and keep `plan.md` limited to the current stage outcome.

## Active Snapshot

Current working state:

- Core loop is in place: server-authoritative player/enemy combat, enemy death, player death v1, RunState, Portal Event, Floor Stub, floor-wave spawn sources, enemy-configured shared XP, shared run Level, pending upgrade choices, and fixed run-local upgrade selection.
- Enemy AI uses `ALPQEnemyAIController` + StateTree + Sight/awareness policy + last-known investigation state + spline patrol route. StateTree owns intent flow; GameplayAbility / combat hit windows own final attack results.
- Enemy melee attack uses `GameplayAbility -> telegraph -> Montage -> AnimNotify hit window -> server attack shape query -> GameplayEffect damage`; missing Montage or missing hit-window Notify should fail visibly instead of applying fallback damage.
- Dodge movement is client-predicted through GAS `LocalPredicted` activation and `UAbilityTask_ApplyRootMotionConstantForce`; invulnerability, damage immunity, cooldown/action-lock, Health, and final correction remain server-authoritative.
- Ordinary floor waves now use placed `ALPQFloorWaveEnemySpawnSource` actors as the single authored source for enemy class, count, location, optional radius sampling, tracking, and cleanup.
- Enemy death XP uses each enemy's own `ExperienceReward`; `ExperienceReward=0` is the no-XP configuration. Ordinary enemy death does not move RunState out of `CombatActive`.
- Portal interaction uses one `ALPQPortalActor` with completion mode plus destination ID. CombatEvent mode locks the event to the interacted Portal, can spawn a configured existing enemy Blueprint as a test Boss, and blocks charging until that Boss is dead or gone; DirectFloorExit mode requires all living players in the Portal radius and enters the floor transition stub without `PortalEventActive`.

Most recent commits:

- `f7e85e2 [Refactor] 重命名LPQ蓝图资产（Rename LPQ Blueprint Assets）`
- `28fcda2 [Refactor] 重命名LPQ反射类型（Rename LPQ Reflected Types）`
- `7ceb483 [Refactor] 清理LPQ表层命名（Clean LPQ Naming Surface）`
- `176f9d8 [Chore] 启用AIAssistant插件（Enable AIAssistant Plugin）`

Current stage:

- No active implementation stage after `LPQ Redirected Asset Resave v3.1`.
- `LPQ Redirected Asset Resave v3.1` is completed and ready to archive / commit.
- Current `Content/_Game` assets no longer serialize old Tunic-prefixed native class paths; old native names remain only in `[CoreRedirects]` and known Blueprint-facing API names.
- Next stage should be chosen deliberately from the near-term TODOs below.

Near-term TODOs:

- `Enemy Variant Profile Cleanup v2` after current Portal work: when Guard / Wild / Spawn / Elite only differ by tuning, fold them toward one enemy class plus profile DataAssets instead of keeping separate Blueprints for each variant.
- `LPQ Blueprint/API Member Rename v4`: only if remaining Blueprint-facing names such as `CanInteractWithTunicPlayer()` become confusing enough to justify Blueprint function redirect / override validation.
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
