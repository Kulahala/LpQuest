# plan.md - LpQuest Active Implementation Plan

## How To Read This File

Hard rule: before starting, replacing, reviewing, or committing a stage, first check whether the previous completed / rejected / superseded stage outcome is recorded in `docs/archive/stage-log.md`. If the previous stage is complete and missing from the archive, archive it before editing this file or implementing the new stage. Keep this file as the current working board, not the long-term stage history.

Read the `Active Snapshot` as the current handoff. Historical completed stage notes live in `docs/archive/stage-log.md`; use that archive only for context and do not treat old TODOs, old risks, old validation gaps, or old debugging notes as current truth without checking live source/assets and recent commits.

Reusable compiler/runtime/debugging lessons belong in server-memory MCP, not in this file. If a future stage hits a repeated error pattern, record the durable lesson in memory and keep `plan.md` limited to the current stage outcome.

## Active Snapshot

Current working state:

- Core loop is in place: server-authoritative player/enemy combat, enemy death, player death v1, RunState, Portal Event, Floor Stub, Spawner, Shared XP, shared run Level, pending upgrade choices, and fixed run-local upgrade selection.
- Enemy AI uses `ATunicEnemyAIController` + StateTree + Sight/awareness policy + last-known investigation state + spline patrol route. StateTree owns intent flow; GameplayAbility / combat hit windows own final attack results.
- Enemy melee attack uses `GameplayAbility -> telegraph -> Montage/fallback hit window -> server attack shape query -> GameplayEffect damage`.
- Dodge movement is client-predicted through GAS `LocalPredicted` activation and `UAbilityTask_ApplyRootMotionConstantForce`; invulnerability, damage immunity, cooldown/action-lock, Health, and final correction remain server-authoritative.
- Encounter membership supports generated enemies plus explicit `PlacedEncounterEnemies`. Registered placed enemies can grant XP through encounter ownership; unregistered placed enemies do not. Ordinary enemy death does not move RunState out of `CombatActive`.
- Portal Event can be started through the unified `E` interact path. Portal can spawn one configured existing enemy Blueprint as a test Boss; charging is blocked until that Boss is dead or gone.

Most recent commits:

- `1f42212 [Refactor] 清理旧 RunState 和升级 Stub（Clean RunState and Upgrade Stub）`
- `5fb8499 [Fix] 拆掉敌人清场 RunState 推进（Decouple Enemy Clear RunState）`
- `1623d91 [Docs] 调整阶段归档提交规则（Refine Stage Archive Commit Rule）`
- `3e654f7 [Docs] 归档敌人奖励来源阶段（Archive Enemy Reward Source Stage）`
- `1b29094 [Feature] 统一敌人死亡 XP 来源（Unify Enemy Reward Source）`

Current stage:

- Legacy cleanup pass before returning to mainline pickup/drop work.
- `RunState Cleanup v2` is complete, archived, and committed. Current source/docs no longer reference `EncounterCleared`, `EvaluateEncounterClear`, `OnEncounterCleared`, or `RequestSelectRunUpgradeStub`.
- Next cleanup candidate: `Enemy Prototype AutoAttack Cleanup v1`, deleting the disabled prototype enemy auto-attack path now that StateTree-driven melee validation covers the current enemy attack flow.

Near-term TODOs:

- `Enemy Prototype AutoAttack Cleanup v1`: remove the disabled `bEnablePrototypeAutoAttack` path, prototype range/interval fields, tick update helper, and target query helper from `ATunicEnemyCharacter`.
- `Enemy Melee Legacy Sweep Field Cleanup v1`: later, remove old serialized `EnemyMeleeSweep*` fields only after confirming Blueprint assets no longer need compatibility with the replaced capsule sweep tuning.
- `Player Light Attack Naming Cleanup v1`: later, rename `ApplyLightAttackDebugDamage` when player weapon/attack data stops using the current prototype light-attack bridge.
- `Enemy Drop Source v1`: after `Pickup / Equipment Interaction v1` exists, add one server-authoritative enemy drop routing path instead of special-casing Boss / pressure / encounter drops separately.
- `Enemy Variant Profile Cleanup v2` after current Portal work: when Guard / Wild / Spawn / Elite only differ by tuning, fold them toward one enemy class plus profile DataAssets instead of keeping separate Blueprints for each variant.
- `Safe Travel Portal v1`: later, when safe zones / shop areas / hub return need pure interaction travel, add a separate lightweight interactable travel portal instead of subclassing the combat Portal Event actor or disabling most of its Boss/charging/pressure behavior.
- `Pickup / Equipment Interaction v1`: reuse `ITunicInteractableInterface` and `IA_Interact=E` for weapon or item pickup after the equipment path is ready.

Current accepted risks:

- Pressure spawn uses Portal-local logic and a world iterator XP hook rather than a generic Director. Add `Spawn Director / Spawn Profile v2` only when exploration spawns, portal pressure, Boss minions, or floor scaling repeat the same spawn ownership logic.
- Pressure spawn points are simple actor references on the Portal. Add richer spawn selection only when map layout, navigation safety, or enemy archetype rules need it.
- Boss v1 remains an existing enemy Blueprint spawned by Portal. Add Boss-specific class/component only when phase logic, Boss UI, arena mechanics, or dedicated Boss-only state appears.
- Portal Event state is still global. Add per-portal event ownership only when multiple active portals can coexist.
- Player death v1 is still one-way. Revive, respawn, spectator/free-camera, party wipe resolution, and floor fail UI remain deferred.
- `FloorTransitionStub` remains intentionally active as the current placeholder floor loop. Rename or replace it only when real floor travel/loading exists.

Build command:

```powershell
& "D:\UE\UE_5.8\Engine\Build\BatchFiles\Build.bat" LpQuestEditor Win64 Development -Project="E:\UnRealEngine\LpQuest\LpQuest.uproject" -WaitMutex
```
