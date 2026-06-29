# LpQuest Stage Log

This file archives completed local stage history moved out of `plan.md` so the active plan stays focused on current work and near-term tasks.

Use this file for historical context only. Current implementation direction should come from live source/assets, `ARCHITECTURE.md`, and the active `plan.md`.

## Historical Log

The content below is older local stage history. It can help recover why a decision was made, but it should not drive new implementation by itself.

## Current Status (Historical)

LpQuest is the UE 5.8 clean project that replaces the earlier LowpolyQuest prototype. Gameplay C++ has been migrated into the `LpQuest` runtime module and adapted to the `LPQUEST_API` export macro.

Stage 1 editor validation is complete: migrated C++ classes are visible, input and Blueprint assets exist, the KayKit prototype player mesh is visible, single-player movement works, Listen Server + 2 Players spawns and moves both players, and PlayerState-owned GAS initialization has been verified in PIE logs.

A basic player GAS debug path has been added in C++ and validated in Listen Server + 2 Players. Logged results showed `OwnerActor=TunicPlayerState`, `AvatarActor=BP_TunicPlayerCharacter`, non-null `AttributeSet`, and `Health/Stamina=100/100` on server and clients.

Light attack target validation is complete: the current C++ path keeps light attack server-authoritative, performs a short capsule sweep hit window on the server, logs hit enemy ASC/AttributeSet data when found, and logs the no-hit branch when no enemy is inside the sweep.

A minimal GAS damage path has been added and validated: when debug damage is enabled and the server sweep finds an enemy target, light attack applies an instant GameplayEffect to the enemy ASC instead of directly mutating Health.

Stage 3 death reliability is complete. Stage 4 early combat-GAS hardening is in place: light attack damage has moved from a proximity debug query to a server-side sweep hit window that can later be driven by animation notify states. Initial GameplayTags registration has been added, light attack now has a validated minimal GameplayAbility activation path, native light-attack Stamina/cooldown/action-lock gates are working in PIE, and minimal GAS Stamina regeneration has been compiled and accepted after PIE validation.

Enemy Attack Path v1 is implemented and user-validated. Player Death Policy v1 is implemented and user-validated: enemies can kill players through the server-authoritative GameplayEffect path, dead players cannot act, and dead players are no longer valid combat targets.

Patrol Route Authoring v1 implementation checkpoint:

- Added `ATunicEnemyPatrolRoute`, a placed patrol authoring actor with a `USplineComponent`, `RouteId`, loop setting, debug visibility toggle, and Blueprint-readable route helpers.
- `ATunicEnemyAIController` now resolves patrol routes by `PatrolRouteId` from matching `ATunicEnemyPatrolRoute` actors with spline points.
- Added `GetCurrentPatrolLocation()` on the enemy AI controller and native StateTree task `Get Current Tunic Patrol Location`, so Patrol MoveTo can bind to a destination vector instead of only a target actor.
- Patrol route point locations now project to NavMesh when possible, which prevents accidentally low/high spline point Z values from sending AI below or above the walkable surface.
- Patrol routes can optionally draw runtime debug spheres, lines, and point labels during PIE through `bDrawRuntimeDebugRoute`.
- Removed the old `ATunicEnemyPatrolPoint` route actor, old patrol-target StateTree task, and point-route fallback so patrol has one authoring model.
- Removed stale compatibility wrappers/fields that no longer match behavior: `RefreshCurrentCombatTargetFromPerception()` and `TargetSearchRadius_DEPRECATED`.
- No PCG, route DataAsset, search/investigation, threat table, group alert, Boss AI, combat result, reward, Spawner, Portal, XP, or RunState behavior was added.

Patrol Route Authoring v1 StateTree topology, superseded by Continuous Patrol / Stops v1.1:

- `ST_EnemyMeleeBasic` should treat `Patrol` as the no-current-target parent state and keep that parent free of route movement tasks.
- Historical v1 structure: `PatrolRoute` was the `Patrol` child state for enemies with a route: `Has Patrol Route`, `Get Current Tunic Patrol Location`, `Move To Destination=PatrolLocation`, `Delay`, and `Advance Tunic Patrol Target`. Continuous Patrol / Stops v1.1 replaces this with separate `PatrolMove`, `PatrolStop`, and `PatrolAdvance` states.
- `IdleAtHome` is the `Patrol` child state for enemies without a route: `NOT Has Patrol Route`, `Get Tunic Enemy Home Location`, `Move To Destination=HomeLocation`, and `Delay`.
- `Chase` uses `Has Current Target`, gets the current AI-controller target, and moves to that target actor.
- `Attack` tries the existing enemy melee Ability and delays for recovery.
- `PatrolRoute`, `IdleAtHome`, `Chase`, and `Attack` should keep Tasks completion mode as `All` when they combine instant Tunic tasks with `Move To` or `Delay`.

Patrol Route Authoring v1 validation completed:

- User confirmed compile and PIE validation passed.
- Route binding logs confirmed guard and spawn-wave AI controllers can bind `BP_TunicEnemyPatrolRoute` by `PatrolRouteId` with four spline points.
- User confirmed enemies can patrol the authored route after the StateTree was corrected to a clean `Patrol` parent with `PatrolRoute` / `IdleAtHome` child states.
- Root cause of the apparent route failure was authoring: spline points had been placed below the ground. The route actor now projects patrol locations to NavMesh when possible, and optional runtime debug draw can visualize points and links during PIE.
- StateTree editor note: Tunic custom conditions expose `bInvert` in condition instance data, but UE's StateTree UI may not prepend `NOT` to the condition label. Check the instance data instead of trusting the label text.

Patrol Route Authoring v1 strict review completed:

- Normal review found no blocking issue in the implemented path. Route lookup remains server-side in `ATunicEnemyAIController`, route authoring stays in `ATunicEnemyPatrolRoute`, and StateTree tasks only consume current patrol locations or advance the AI-controller-owned route index.
- Removing `ATunicEnemyPatrolPoint` and the old patrol-target task is consistent with the project's early-stage preference for one clear patrol authoring model instead of multiple overlapping systems.
- NavMesh projection is defensible because AI movement already depends on navigation. It fixes authoring Z mistakes without inventing a second ground-trace or XY-only height policy.
- Runtime debug draw is opt-in on the route actor and does not replicate gameplay state, alter combat, affect encounter membership, or involve Spawner/GameMode/GameState.
- No-route enemies remain legal: `PatrolRouteId=None` logs only at Verbose and should fall through to the StateTree `IdleAtHome` child when the asset uses inverted `Has Patrol Route`.
- Removed the old `Find Nearest Tunic Enemy Target` StateTree task and `FindNearestAvailableCombatTarget()` helper after confirming no current `Content/_Game/AI` asset text reference remained. This keeps target acquisition on the AI-controller-owned awareness/current-target path instead of leaving a parallel scan entry point.

Patrol Route Sampling v1 implementation checkpoint:

- `ATunicEnemyPatrolRoute` now separates spline control points from AI patrol target points. The route spline remains the visual/editing curve, while AI patrol locations are generated automatically by distance sampling along the spline.
- Added `PatrolSampleDistance`, defaulting to `800cm`, as the route-level sampling interval.
- Added a transient sampled patrol location cache rebuilt during construction and BeginPlay.
- `GetPatrolPointCount()` and `GetPatrolPointLocation()` now expose sampled patrol points to existing AIController / StateTree callers, so `ST_EnemyMeleeBasic` does not need new nodes.
- Sampled patrol locations still project to NavMesh, preserving the authoring-Z safety added in Patrol Route Authoring v1.
- Runtime debug draw now distinguishes the spline route curve from sampled patrol points so PIE can show what the AI will actually chase.
- No custom stop points, per-spline-point checkboxes, stand-and-look behavior, search/investigation, PCG, DataAsset route templates, Spawner, combat, XP, Portal, Floor Stub, or RunState behavior was added.

Patrol Route Sampling v1 validation completed:

- User confirmed compile/PIE validation passed.
- User confirmed shorter sample distance makes the path smoother, while any StateTree PatrolRoute Delay still causes stops at sampled points.
- Default `PatrolSampleDistance` was changed to `800cm` after validation feedback.
- Review fix: short loop routes now add a midpoint sample when the route length is shorter than the sample interval, avoiding a one-sample loop that advances to the same location forever.

Patrol Route Sampling v1 strict review completed:

- Normal review found no blocking issue after the short-loop fix. Sampling remains contained inside `ATunicEnemyPatrolRoute`; `ATunicEnemyAIController` and StateTree still consume the same patrol location/count API.
- The route actor still owns only authoring/debug data and sampled movement targets. It does not mutate combat, run state, Spawner membership, XP, Portal, or replicated gameplay state.
- Runtime debug draw remains opt-in and only visualizes the route curve plus sampled patrol points.
- Adversarial review can defend automatic distance sampling over per-spline-point checkboxes for v1 because it keeps spline control points as route-shape authoring and avoids adding editor metadata complexity before stop/hold semantics are needed.
- Accepted follow-up: if smooth movement needs short samples but patrol should only wait at sparse authored points, implement `Continuous Patrol / Stops v1.1` with separate movement samples and stop/hold points instead of overloading the current sample list.

Next recommended stage:

- After Continuous Patrol / Stops v1.1 is validated, continue with `Enemy Search / Investigation v1`: record last known target location, search briefly through StateTree, then return to the sampled spline patrol route or IdleAtHome.

Follow-up TODO:

- `Patrol Stops v1.2`: add explicitly authored stop/hold points only if patrol design needs guard posts, per-stop wait times, forced look directions, or inspect points. Keep this separate from spline control points so route-shape edits do not accidentally create stop-and-go AI behavior.

Continuous Patrol / Stops v1.1 implementation checkpoint:

- `ATunicEnemyPatrolRoute` now keeps the spline as the authoring curve, movement samples as patrol locations, and sparse stop samples as wait markers.
- `PatrolSampleDistance` remains the movement sample interval and now defaults to `400cm`.
- Added `PatrolStopSampleDistance`, defaulting to `3200cm`, and `PatrolStopHoldDuration`, defaulting to `1.0s`.
- `ATunicEnemyAIController` exposes `IsCurrentPatrolTargetStop()` and `GetCurrentPatrolStopHoldDuration()` while keeping the existing patrol count/location/advance API.
- Added native StateTree support for current patrol stop data: `Get Current Tunic Patrol Stop` outputs `HoldDuration`, and `Tunic Enemy Current Patrol Target Is Stop` checks whether the current movement sample is a stop.
- Runtime debug draw now separates the spline curve, yellow movement samples, and red stop samples.
- No Enemy Search / Investigation, custom stop checkboxes, per-stop facing, PCG, DataAsset route template, Spawner, combat, XP, Portal, Floor Stub, or RunState behavior was added.

Continuous Patrol / Stops v1.1 StateTree topology:

- Keep `Patrol` as the no-current-target parent router.
- `PatrolRoute` is the `Patrol` child used when a route exists. It keeps enter condition `Tunic Enemy Has Patrol Route`, has no movement/wait/advance tasks itself, and selects `PatrolMove` on state succeeded.
- `PatrolMove` has no enter condition. Its tasks are `Get Current Tunic Patrol Location` and `Move To Destination=PatrolLocation`, with no Delay and no Advance task in this state.
- `PatrolMove -> PatrolStop` uses `On State Succeeded` plus `Tunic Enemy Current Patrol Target Is Stop`.
- `PatrolMove -> PatrolAdvance` uses `On State Succeeded` plus `Tunic Enemy Current Patrol Target Is Stop` with `bInvert=true`.
- `PatrolStop` has no enter condition. Its tasks are `Get Current Tunic Patrol Stop` and built-in `Delay`, with `Delay.Duration` bound to `Get Current Tunic Patrol Stop.HoldDuration`.
- `PatrolStop -> PatrolAdvance` uses `On State Succeeded` with no condition.
- `PatrolAdvance` has no enter condition. Its only task is `Advance Tunic Patrol Target`.
- `PatrolAdvance -> PatrolMove` uses `On State Succeeded` with no condition.
- StateTree's UI may not display `NOT` on inverted custom conditions; check condition instance data and trust `bInvert`, not the label text.
- Keep `PatrolMove`, `PatrolStop`, `PatrolAdvance`, `Chase`, and `Attack` on Tasks completion mode `All` when they combine instant Tunic tasks with MoveTo or Delay.

Continuous Patrol / Stops v1.1 validation pending:

- User should compile `LpQuestEditor Win64 Development`.
- In Editor, update `ST_EnemyMeleeBasic` to remove the old `PatrolRoute` state that waited at every movement sample, and replace it with `PatrolMove`, `PatrolStop`, and `PatrolAdvance`.
- Set route values on `BP_TunicEnemyPatrolRoute`: start with `PatrolSampleDistance=400`, `PatrolStopSampleDistance=3200`, and `PatrolStopHoldDuration=1.0`.
- Enable runtime route debug draw when validating. Yellow points should be continuous movement samples; red points should be sparse stop samples.
- Single-player PIE should show enemies moving continuously through yellow samples and waiting only on red stop samples.
- Listen Server + 2 Players should show server-owned patrol movement, stop waiting, chase, attack, and death behavior consistently on both windows.

Continuous Patrol / Stops v1.1 strict review completed:

- Normal review found no blocking source issue. Patrol remains a single `ATunicEnemyPatrolRoute` system; no old patrol point fallback or parallel route model was reintroduced.
- Route sampling remains authoring/debug data only. It does not mutate Spawner membership, combat results, XP, Portal, Floor Stub, RunState, or replicated gameplay state.
- `ATunicEnemyAIController` keeps route index ownership and only exposes read-only stop queries plus the existing advance operation to StateTree.
- Native StateTree nodes only read current patrol stop data or query the current stop condition. They do not directly move characters, modify health, set run state, or apply GameplayTags.
- `ST_EnemyMeleeBasic` now has a documented handoff topology down to child states, enter conditions, tasks, bindings, transition triggers, conditions, `bInvert`, and Tasks completion mode.
- Adversarial review can defend automatic sparse stop samples over per-point checkboxes for v1.1 because the goal is to split continuous movement from waiting without adding authored stop metadata before level-design needs prove it.
- Accepted risk: build and PIE validation are still user-owned and pending for the C++/asset combination. Commit body must not claim compile or PIE success until the user confirms it.

Enemy AI v1 implementation checkpoint:

- Added `ATunicEnemyAIController` as the first server-authoritative enemy AI controller.
- The controller owns a `UStateTreeAIComponent` brain, starts it only on authority when `DefaultEnemyStateTree` is configured, and otherwise logs a safe idle state.
- Enemy characters now default to `ATunicEnemyAIController` with `AutoPossessAI=PlacedInWorldOrSpawned`, so Spawner-created enemies can be AI controlled.
- `ATunicEnemyCharacter::TryActivateEnemyMeleeAttack()` now returns whether the melee Ability activation request succeeded.
- Enemy death stops the enemy AI controller, movement, focus, and current target before existing death/reward/clear routing continues.
- Added native StateTree support nodes for current enemy target, patrol location, home location, patrol advance, enemy melee attack requests, current-target checks, patrol-route checks, and attack-range checks.
- Enemy AI v1 uses StateTree as the main decision layer. The existing prototype auto attack switch remains a default-off debug fallback and is not the intended AI path.

Enemy AI v1 validation completed:

- User should compile `LpQuestEditor Win64 Development`.
- In Editor, create/configure `ST_EnemyMeleeBasic` with `StateTree AI Component` schema, bind the native find-target task output to MoveTo's `TargetActor`, bind MoveTo's `AIController` from the StateTree context, and set `DefaultEnemyStateTree` on the enemy AI controller/BP.
- Confirm enemy Blueprints use `ATunicEnemyAIController` or a configured BP subclass and `Auto Possess AI = Placed in World or Spawned`.
- Confirm the test map has NavMesh coverage.
- Single-player PIE should show spawned enemies chasing a live player, requesting existing enemy melee attacks, damaging through GameplayEffect, ignoring dead players, and stopping AI on death.
- Listen Server + 2 Players should show server-only AI decisions with replicated movement, montage, Health, death, XP, Encounter Clear, Portal, and Floor Stub behavior intact.

Enemy AI v1 strict review result:

- No blocking code regressions found in the current chase/attack path.
- TODO: finish attack-range ownership cleanup in the StateTree asset. Code now exposes native `Tunic Enemy Target In Attack Range`; `ST_EnemyMeleeBasic` should replace any editor-authored Compare Distance attack transition with that native condition so `ATunicEnemyAIController::AttackActivationRange` is the single tuning source.
- TODO: narrow `EnsureLiveEnemyMovementMode()` if future enemy states intentionally keep a living enemy at `MOVE_None`. The current repair path is fine for v1, but it should not become the default answer for later stun/freeze/cutscene states.
- TODO: update `README.md` / current-status wording again after the next compile pass if the stage is considered fully closed.

AI Perception / Patrol / Aggro v1 implementation checkpoint:

- Added sight-only AI Perception support to `ATunicEnemyAIController`.
- The enemy AI controller now prefers perceived live player targets, keeps the current target stable while it remains perceived, and only switches target when the current one is lost or invalid.
- Added `AggroForgetDelay` as a small target-loss debounce before clearing aggro and returning to patrol/idle.
- Superseded by Patrol Route Authoring v1: the old `ATunicEnemyPatrolPoint` actor and point-route collection path were removed. Patrol now uses `ATunicEnemyPatrolRoute` spline routes only.
- Added native StateTree support nodes for current perceived target, current patrol location, home location, advancing patrol target, current-target condition, patrol-route condition, and AI-controller-owned attack-range condition.
- Spawner, GameMode, GameState, XP, Portal, Floor Stub, damage, death, and enemy melee Ability result ownership were not changed.

AI Perception / Patrol / Aggro v1 validation pending:

- User should compile `LpQuestEditor Win64 Development`.
- If a native Tunic StateTree condition appears in property/event payload searches but not in the transition or enter-condition `+` picker, verify the struct inherits from `FStateTreeConditionCommonBase`. Direct `FStateTreeConditionBase` inheritance can be reflected but not offered as a normal addable condition in the editor picker. CommonBase conditions also need an instance data type and `GetInstanceDataType()`; otherwise the StateTree editor can report `Malformed node, missing instance value`. Tunic's current boolean AI conditions expose a shared `bInvert` parameter; use that for `NOT HasCurrentTarget` or `NOT HasPatrolRoute` transitions instead of relying on an editor-level invert control that may not exist for custom conditions.
- Superseded by Patrol Route Authoring v1: in Editor, place `ATunicEnemyPatrolRoute` / `BP_TunicEnemyPatrolRoute`, edit the spline, and set matching `RouteId`.
- Set the enemy AI controller/BP `PatrolRouteId` to the route that should drive that enemy.
- Update `ST_EnemyMeleeBasic` to use the new current-target and patrol-target nodes instead of treating nearest-player scanning as the main path.
- Replace the old `Compare Distance` attack transition with native `Tunic Enemy Target In Attack Range`.
- Keep StateTree states that combine an instant Tunic task with `Move To` or `Delay` on Tasks completion mode `All`; `Any` can complete the state before movement/recover has time to run.
- Single-player PIE should verify patrol when no player is perceived, aggro/chase/attack when a player enters sight, return to patrol or idle after target loss, and no targeting of dead players.
- Listen Server + 2 Players should verify server-only AI decisions, target switching away from dead players, replicated movement/attack presentation, and unchanged XP/Encounter/Portal/Floor paths.

AI Perception / Patrol / Aggro v1 validation completed:

- User confirmed compile and PIE validation.
- StateTree custom condition exposure was fixed by using `FStateTreeConditionCommonBase`, adding condition instance data, and exposing a shared `bInvert` flag.
- User confirmed the updated StateTree patrol/chase/attack path works.
- Known v1 limitation: sight-only perception means an enemy spawned or idling with its back to the player may not aggro until the player enters its vision cone. This is acceptable for Sight v1, but spawn-wave / Risk-of-Rain-style enemies should later add proximity aggro, all-direction initial aggro, or spawner/director target assignment instead of relying only on patrol-facing sight.

AI Perception / Patrol / Aggro v1 strict review completed:

- Normal review found no blocking code issue in the implemented path. AI perception, current-target refresh, patrol routing, StateTree execution, and attack requests remain server-authoritative through `ATunicEnemyAIController`.
- Native StateTree nodes do not directly mutate Health, RunState, XP, GameplayTags, or final combat results. They only read/update AI intent, expose patrol/current-target data, and request the existing enemy melee Ability through the AI controller.
- Patrol points remain AI authoring data only. They do not affect Spawner encounter membership, rewards, floor state, or combat result ownership.
- Missing `DefaultEnemyStateTree` still idles safely through existing logging. Missing patrol routes can be handled by the StateTree `IdleAtHome` fallback if the asset uses `NOT HasPatrolRoute`.
- Target validation still goes through `ITunicCombatTargetInterface::IsCombatTargetAvailable()`, so dead players are not valid targets; target-loss debounce does not override that dead-target filter.
- Superseded cleanup: the old `AcquireTarget / Find Nearest Tunic Enemy Target` branch should not be used in final v1 assets, and the corresponding C++ task has now been removed.
- Adversarial review conclusion: Sight-only perception, placed patrol points, and explicit StateTree conditions are defensible for this stage because the goal is a readable patrol/chase/attack loop, not a full spawn director. The limitation that enemies may not aggro while facing away is accepted for Sight v1 and should be solved in the next aggro layer, not hidden inside the patrol route system.
- TODO: decide the next aggro layer before random wave behavior is tuned. Candidate fixes are proximity aggro radius, initial spawned-enemy target assignment from Spawner/Director, or wider/all-direction sight for selected enemy archetypes.

Enemy Awareness Policy v1 implementation checkpoint:

- Added `ETunicEnemyAwarenessPolicy` on `ATunicEnemyAIController`: `SightOnlyPatrol`, `SightAndProximity`, and `CombatSpawnAggro`.
- `SightOnlyPatrol` remains the default so existing placed patrol enemies do not become spawn-aggro enemies by accident.
- Added `ProximityAggroRadius` for small-radius all-direction target acquisition in `SightAndProximity` and ready `CombatSpawnAggro` modes.
- Added `CombatSpawnAggroDelay` and a server timer so combat-spawn enemies only enable active awareness target acquisition after a short delay, not on the spawn frame.
- Added optional idle scan rotation for enemies with no current target and no patrol route; idle scan is limited to the Home area so it does not fight return-home MoveTo.
- Superseded by Patrol Route Authoring v1 cleanup: the old `RefreshCurrentCombatTargetFromPerception()` compatibility wrapper was removed; StateTree tasks call `RefreshCurrentCombatTargetFromAwareness()` directly.
- The old nearest-target StateTree task now respects the configured awareness policy instead of falling back to unconditional all-player `TargetSearchRadius` scanning.
- Spawner, GameMode, GameState, XP, Portal, Floor Stub, damage, death, and enemy melee Ability result ownership were not changed.

Enemy Awareness Policy v1 validation completed:

- User confirmed `LpQuestEditor Win64 Development` compile passed.
- User confirmed PIE validation passed with the configured Guard / Wild / SpawnWave enemy Blueprint and AIController Blueprint variants.
- Note: Simulate-in-editor behavior can differ from full PIE for AI possession, GameMode/GameState, perception, and tick timing. Use full PIE / Listen Server PIE as the validation source for AI behavior.
- In Editor, set placed guard enemies to `SightOnlyPatrol`, general enemies to `SightAndProximity`, and Spawner wave enemy BPs to `CombatSpawnAggro` only when they should actively enter combat after spawning.
- Verify `ST_EnemyMeleeBasic` still uses current-target driven Chase/Attack and no longer depends on the old nearest-player scanning branch as the main path.
- Single-player PIE should verify Sight-only guards do not aggro from behind, proximity enemies can aggro when the player is close behind them, and combat-spawn enemies only acquire nearby players after `CombatSpawnAggroDelay`.
- Listen Server + 2 Players should verify server-only awareness decisions, dead-player filtering, target switching, and unchanged XP/Encounter/Portal/Floor paths.

Enemy Awareness Policy v1 strict review completed:

- Normal review found no blocking code issue in the implemented path. Awareness refresh, proximity targeting, combat-spawn delayed acquisition, idle scan, target validation, and target clearing remain server-authoritative in `ATunicEnemyAIController`.
- `CombatSpawnAggro` delay is guarded from Tick refresh, sight perception events, and the older nearest-target StateTree compatibility task before the timer marks the controller ready.
- `SightOnlyPatrol` no longer falls back to unconditional all-player `TargetSearchRadius` scanning; the old radius is kept only as a `_DEPRECATED` serialized field for Blueprint compatibility.
- Idle scan is limited to enemies with no current target, no patrol route, and near their Home location, so it should not fight patrol routes or return-home MoveTo.
- Spawner, GameMode, GameState, XP, Portal, Floor Stub, damage, death, and enemy melee Ability result ownership were not changed.
- Adversarial review conclusion: the policy enum split is defensible because the behavior difference is target acquisition policy, not a separate StateTree structure. Keeping Spawner out of target ownership is also defensible because encounter membership and AI intent remain separate systems.
- TODO: add Enemy Search / Investigation v1 later. Lost-target behavior should record a last known location, search briefly through StateTree states, then return to Patrol or IdleAtHome; do not grow the current Awareness Policy into a hidden C++ search FSM.

Dodge / Roll v1.1 implementation checkpoint:

- Added `UTunicGameplayAbility_Dodge` as a server-only player movement ability.
- Added native `UTunicDodgeCooldownGameplayEffect` that grants `Cooldown.Dodge` and `State.ActionLocked` for a short duration.
- Registered `Ability.Movement.Dodge` in GameplayTags config.
- `ATunicPlayerCharacter` now grants the Dodge ability alongside light attack, and Dodge input prefers Ability activation before the old bootstrap RPC fallback.
- Asset validation showed the current Dodge animation has no animation Root Motion, so `DodgeMontage` is now presentation-only.
- Dodge movement now uses a server-applied `FRootMotionSource_ConstantForce` short dash on `ATunicPlayerCharacter`.
- Dodge direction uses a recent non-zero movement input world direction, falling back to ActorForward after movement input has stopped.
- Client-owned players send the requested Dodge direction with the Dodge request RPC; the server stores that direction before activating the Dodge GameplayAbility, so displacement remains server-authoritative.
- `DodgeMovementInputDirectionGraceTime` is a short-term input-ordering tolerance so Space and movement input can be interpreted together without a full input buffer.
- v1.1 does not add Stamina cost, invulnerability, `LaunchCharacter`, direct actor teleport, cancel windows, DataAssets, or formal root-motion correction logic.

Dodge / Roll v1.1 validation pending:

- User should compile `LpQuestEditor Win64 Development`.
- With no `DodgeMontage`, pressing Dodge should log a server-confirmed request, move with manual RootMotionSource dash, and not crash.
- With the current in-place Dodge Montage configured on the player blueprint, pressing Dodge should play the Montage and move through manual server-authoritative dash.
- Active/recent WASD input should set Dodge direction; standing still after input grace time should Dodge along ActorForward.
- Dodge cooldown/action-lock should prevent Dodge spam and block light attack overlap in both directions.
- Listen Server + 2 Players should show server and client player Dodge presentation consistently.

Dodge / Roll v1.1 strict review pending:

- Review should verify Dodge does not grant `State.Invulnerable`, does not mutate Health/Stamina, and does not use `LaunchCharacter`, `SetActorLocation`, or `AddActorWorldOffset`.
- Review should verify manual RootMotionSource movement is server-side and Client-player Dodge request direction does not become client-authoritative displacement.
- Review should verify action-lock/cooldown tags keep Dodge and light attack mutually exclusive without adding client-side authority shortcuts.
- Non-blocking follow-up: when combo attacks, cancel windows, or action buffering are introduced, revisit `DodgeMovementInputDirectionGraceTime` and fold Dodge direction snapshots into the unified input state / input buffer instead of keeping one-off dodge-only timing logic.

Dodge / Roll v1.1 validation completed:

- User confirmed compile and PIE validation.
- With no `DodgeMontage`, Dodge still produced server-confirmed short dash movement.
- With the in-place Dodge Montage configured, Dodge played presentation and still moved through the server-applied manual RootMotionSource dash.
- User confirmed the direction fix: active/recent movement input drives Dodge direction, while standing Dodge after the grace window uses the character/mouse-facing direction.

Dodge / Roll v1.1 strict review completed:

- Normal review found one implementation risk: the override RootMotionSource could unnecessarily overwrite Z velocity during the dash. Fixed by setting `ERootMotionSourceSettingsFlags::IgnoreZAccumulate`.
- No direct Health/Stamina mutation, no `State.Invulnerable` grant, and no `LaunchCharacter`, `SetActorLocation`, or `AddActorWorldOffset` path were added.
- Client-provided Dodge direction is treated only as an input direction snapshot; actual Ability activation, cooldown/action-lock, and displacement remain server-side.
- Adversarial review conclusion: using RootMotionSource is defensible for current non-root-motion Dodge assets because it stays inside CharacterMovement's movement pipeline while avoiding hand teleports. The local `DodgeMovementInputDirectionGraceTime` remains acceptable as a short-term input-order tolerance until a shared combo/action input buffer exists.

Next short-term direction is Run/Floor Loop Foundation, not full gameplay-rule lock-in. The goal is to add the smallest server-authoritative session/encounter state skeleton that later systems can depend on, while deferring random spawn tuning, rewards, real floor transitions, revive, respawn, and final UI.

Proposed execution order:

1. Run State Foundation v1:
   - Add a minimal replicated run/floor state surface, likely through server-only `GameMode` decisions and replicated `GameState` state.
   - Implement Party Wipe v1: one dead player does not end the run, all participating players dead triggers a server-confirmed wipe state.
   - First version should log and expose Blueprint presentation hooks only; no restart, respawn, failure UI, or reward handling.

2. Encounter Clear v1:
   - Track the current test encounter's required enemies.
   - When all required enemies are dead, server confirms encounter cleared.
   - First version can use placed enemies or authored test encounter actors; do not introduce random spawn pressure yet.

3. Portal v1:
   - After encounter clear, expose or activate a prototype portal/transition point.
   - Require all living players to be near or confirm the portal before advancing.
   - First version may only log or switch replicated state; real map/floor loading can wait.

4. Spawn / Reward / Progression v1:
   - Add simple server-authoritative spawn pacing, shared run-local experience, non-shared run-local equipment drops, and floor pressure scaling.
   - Keep permanent out-of-run progression undecided until the run loop is playable.

Run State Foundation v1 implementation checkpoint:

- Added `ETunicRunState` with initial `CombatActive` and `PartyWiped` states.
- `ATunicGameState` now replicates the current run state and exposes Blueprint-readable state plus `OnRunStateChanged` presentation hook.
- `ATunicGameMode` now owns Party Wipe v1 evaluation by scanning current `GameState->PlayerArray` for controlled `ATunicPlayerCharacter` pawns.
- Player death now asks the server GameMode to re-evaluate party wipe after `SetDead(true)` succeeds.
- `Logout` also re-evaluates party wipe to keep the current PlayerArray state from going stale.
- Party Wipe v1 does not restart, respawn, revive, show failure UI, clear encounters, open portals, or apply rewards.

Run State Foundation v1 validation completed:

- User confirmed compile success.
- User confirmed Listen Server + 2 Players validation through logs:
  - One dead player produced `ParticipatingPlayers=2`, `DeadPlayers=1`, `Triggered=false`.
  - Both players dead produced `ParticipatingPlayers=2`, `DeadPlayers=2`, `Triggered=true`.
- Screenshot validation showed the dead client labeled `Player DEAD` while the remaining player stayed present until the second death.

Run State Foundation v1 strict review:

- Normal review found no client-side run-state mutation, no direct Health mutation, and no player-death ownership regression.
- `ATunicGameMode` owns the server-only Party Wipe decision; `ATunicGameState` only replicates the result and exposes presentation hooks.
- `ATunicPlayerCharacter` only notifies GameMode after its own server-authoritative death state is applied, so Character does not own whole-party failure decisions.
- The v1 participant rule is intentionally narrow: only current `PlayerArray` entries with a controlled `ATunicPlayerCharacter` Pawn are counted, preventing login/spawn gaps from triggering a false wipe.
- Adversarial review can defend not implementing Encounter Clear, Portal, rewards, revive, respawn, failure UI, or map reset because this checkpoint only establishes the first replicated run-state result.
- Non-blocking risk: `PartyWiped` is currently one-way. Future run restart, floor reset, or revive flows must define an explicit transition out of `PartyWiped` instead of assuming this v1 state can reset itself.
- Non-blocking risk: disconnected or future spectator/non-participant players are not modeled. A participant registry should be introduced only when reconnection, spectating, or lobby roles become real requirements.
- Non-blocking risk: UI is not yet wired to `OnRunStateChanged`; this is acceptable for v1 because logs and replicated state are the validation target.

Encounter Clear v1 implementation checkpoint:

- Added `EncounterCleared` to `ETunicRunState`.
- `ATunicGameState` now exposes `IsCombatActive()` and `IsEncounterCleared()` Blueprint-readable helpers.
- `ATunicGameMode` now owns Encounter Clear v1 evaluation by scanning active-level `ATunicEnemyCharacter` actors.
- Enemy death now asks the server GameMode to re-evaluate encounter clear after `SetDead(true)` succeeds.
- Run state transitions are intentionally one-way in v1: `CombatActive -> PartyWiped` or `CombatActive -> EncounterCleared`.
- Encounter Clear v1 does not open portals, apply rewards, spawn enemies, change floors, restart, or handle Boss-specific encounter logic.

Encounter Clear v1 validation completed:

- User confirmed compile success.
- User confirmed one-enemy validation: killing the only enemy changed the run state to `EncounterCleared`.
- User confirmed two-enemy validation:
  - Killing the first enemy produced `TotalEnemies=2`, `AliveEnemies=1`, `Triggered=false`.
  - Killing the second enemy produced `TotalEnemies=2`, `AliveEnemies=0`, `Triggered=true`.
- Screenshot validation showed `Run state changed to EncounterCleared` and server-side encounter clear evaluation logs.

Encounter Clear v1 strict review:

- Normal review found no client-side encounter completion path, no direct Health mutation, and no enemy death ownership regression.
- `ATunicGameMode` owns the server-only Encounter Clear decision; `ATunicGameState` only replicates the result and exposes presentation hooks.
- `ATunicEnemyCharacter` only notifies GameMode after its own server-authoritative death state is applied, so enemies do not own encounter completion.
- The v1 enemy rule is intentionally narrow: all active-level `ATunicEnemyCharacter` actors count, and clear requires `TotalEnemies > 0` plus no living enemies.
- Party Wipe and Encounter Clear are mutually guarded by `IsCombatActive()`, so a terminal state should not overwrite another terminal state.
- Adversarial review can defend not adding an Encounter Actor or registry yet because current validation uses placed enemies in a fixed test map; the scan-based bridge should be replaced when spawned waves, non-scoring enemies, rooms, or Boss encounters need explicit membership.
- Non-blocking risk: active-level scanning treats every `ATunicEnemyCharacter` as encounter-critical. Future patrol-only, summoned, ambient, or non-scoring enemies need an encounter membership rule.
- Non-blocking risk: `EncounterCleared` is one-way. Future portal, next-floor, restart, or reward flows must define explicit transitions rather than assuming this state resets itself.
- Non-blocking risk: `OnRunStateChanged` is not yet wired to UI, portal, rewards, or encounter presentation. This is acceptable for v1 because logs and replicated state are the validation target.

Portal v1 implementation checkpoint:

- Added `FloorTransitionReady` to `ETunicRunState`.
- `ATunicGameState` now exposes `IsFloorTransitionReady()` for Blueprint-readable state checks.
- `ATunicGameMode` now owns the server-only `EncounterCleared -> FloorTransitionReady` transition through `MarkFloorTransitionReady()`.
- Added placed `ATunicPortalActor` as the Portal v1 prototype.
- Portal v1 activates only after `EncounterCleared`, counts living players in a configurable radius on the server, pauses charging when not all living players are present, and keeps existing progress instead of resetting it.
- Dead players do not block portal charging.
- Portal v1 replicates active/charging/ready state, charge progress, and living-player counts, and exposes Blueprint presentation hooks for activation, charging changes, charge progress, and ready.
- Portal v1 does not load maps, grant rewards, spawn pressure enemies, run UI progress bars, restart the run, or directly mutate `ATunicGameState`.

Portal v1 validation completed:

- User confirmed compile/PIE validation passed.
- User confirmed the portal path logged `Run state changed to FloorTransitionReady`.
- Portal v1 reached the intended minimal success line: clear encounter, activate placed portal, charge from living-player presence, and transition to `FloorTransitionReady`.

Portal v1 strict review:

- Normal review found no client-side RunState mutation path. `ATunicPortalActor` only runs progression logic on authority and asks `ATunicGameMode` to perform the final state transition.
- `ATunicGameMode::MarkFloorTransitionReady()` only allows `EncounterCleared -> FloorTransitionReady`, so Portal v1 does not overwrite `CombatActive` or `PartyWiped`.
- Dead players are excluded from the portal presence requirement through `ATunicPlayerCharacter::IsDead()`, matching Player Death Policy v1.
- Portal Blueprint hooks are presentation-only and do not own map loading, rewards, enemy spawning, or direct GameState mutation.
- Adversarial review can defend a placed Actor plus PlayerArray scan for v1 because the current test map and run-state foundation already use simple authoritative scans; adding a Portal Manager or participant registry now would be premature.
- Non-blocking risk: Portal currently scans current `PlayerArray` living pawns like Party Wipe v1. Future spectators, reconnecting players, split parties, non-participants, or floor-specific party membership need a participant registry before this becomes final progression logic.
- Non-blocking risk: `FloorTransitionReady` is a terminal signal only. Future real floor loading must define reset/next-floor transitions, portal deactivation, spawn reset, reward timing, and player placement explicitly.

Floor Transition Stub v1 implementation checkpoint:

- Added replicated `CurrentFloorIndex` to `ATunicGameState`, starting at floor 1.
- `ATunicGameState` now exposes `GetCurrentFloorIndex()`, server-only `SetCurrentFloorIndex()`, and the `OnFloorIndexChanged` Blueprint presentation hook.
- `ATunicGameMode::MarkFloorTransitionReady()` now starts a short server timer after `EncounterCleared -> FloorTransitionReady`.
- `ATunicGameMode::CompleteFloorTransitionStub()` increments the replicated floor index, resets placed portals, and returns RunState to `CombatActive`.
- Encounter Clear evaluation now tracks the evaluated floor and requires seeing at least one living enemy on the current floor before another clear can trigger, preventing old dead enemies from immediately clearing the placeholder next floor.
- `ATunicPortalActor` now exposes `ResetPortalForNextFloorStub()` and an `OnPortalReset` Blueprint presentation hook. Portal reset clears active/charging/ready state, progress, and living-player counts.
- Floor Transition Stub v1 does not spawn enemies, grant rewards, load maps, revive players, move players, clean up dead enemies, or implement real next-floor content.

Floor Transition Stub v1 validation completed:

- User confirmed compile/PIE validation passed.
- User confirmed log output: `Floor transition stub completed | PreviousFloor=1 | NewFloor=2 | ResetPortals=1`.
- Floor Transition Stub v1 reached the intended minimal success line: Portal readiness advances the replicated floor index, resets placed portals, and returns RunState to `CombatActive` without map loading, rewards, spawn pressure, or revive.
- Post-review code guard added after the first validation: Encounter Clear now requires seeing at least one living enemy on the current floor before triggering clear, preventing old dead enemies from immediately clearing a placeholder next floor. User confirmed the guard revalidation passed.

Floor Transition Stub v1 strict review:

- Normal review found no client-side floor/run-state mutation path. `ATunicGameMode` owns floor advancement, `ATunicGameState` replicates the floor index, and `ATunicPortalActor` only exposes reset/presentation hooks.
- Review found and fixed one blocking v1 risk: returning to `CombatActive` with old dead enemies still in the level could allow an immediate false clear on the placeholder next floor. The current guard prevents clear until the current floor has seen at least one living enemy.
- Timer re-entry is bounded by `MarkFloorTransitionReady()` only accepting `EncounterCleared` and `CompleteFloorTransitionStub()` only accepting `FloorTransitionReady`.
- Adversarial review can defend not implementing spawn/reward/map loading/revive in this stage because the objective is only to bridge the run state from portal readiness back to a next-floor placeholder state.
- Non-blocking risk: old dead enemies still remain in the level after the stub. The guard prevents immediate re-clear, but Spawner v1 must define real enemy cleanup/spawn ownership and encounter membership.

Next stage ordering adjustment:

- Do not implement rewards, equipment drops, or portal pressure spawning immediately after Floor Transition Stub v1.
- Recommended next implementation stage is Spawner v1 only: define server-owned spawn points/waves, clean up or ignore prior-floor dead enemies, and make encounter clear use spawned encounter membership instead of all active-level enemies.
- Reward / Progression v1 should wait until the equipment system and reward ownership rules are clearer. Shared run-local experience can come earlier than equipment drops, but it should still hook into a real encounter completion/spawn manager rather than ad hoc enemy death checks.
- Portal Pressure v1 should wait until Spawner v1 and basic enemy AI/encounter membership are stable, because pressure enemies during the portal countdown need clear ownership and cleanup rules.

Spawner v1 implementation checkpoint:

- Added placed `ATunicEncounterSpawner` as the first server-owned fixed-wave encounter spawner.
- Spawner v1 uses authored spawn point actor references plus fixed `EnemyClass` / `Count` entries; it does not use DataAssets, DataTables, random spawn tables, or difficulty curves yet.
- Spawner v1 tracks only enemies it spawned as current encounter members and exposes alive/total count helpers for GameMode.
- `ATunicGameMode::EvaluateEncounterClear()` now queries the active spawner's spawned encounter members instead of scanning all active-level `ATunicEnemyCharacter` actors.
- `ATunicGameMode::BeginPlay()` asks the spawner to create the first floor wave when the run starts in `CombatActive`.
- `ATunicGameMode::CompleteFloorTransitionStub()` resets placed portals, resets placed spawners, advances the floor index, returns RunState to `CombatActive`, and asks the spawner to spawn the next floor wave.
- Spawner cleanup destroys prior spawned enemies before the next floor stub wave, so old spawned corpses or leftover alive enemies no longer define the next floor's encounter membership.
- Spawner Blueprint hooks are presentation-only: `OnEncounterSpawned` and `OnEncounterCleared` do not own RunState, rewards, AI, map loading, or difficulty scaling.

Spawner v1 validation completed:

- User confirmed compile/PIE validation passed.
- User confirmed the placed `BP_TunicEncounterSpawner` workflow is valid: the spawner only needs to exist in the level like a configuration/controller actor; spawn location comes from referenced spawn point actors, not from the spawner's own transform or range.
- User confirmed the run loop works with the spawner path after configuring spawn points and spawn entries.

Spawner v1 strict review:

- Normal review found no client-side enemy spawning path, no client-side RunState mutation, no direct Health mutation, and no regression in the server-owned Party Wipe / Encounter Clear / Portal / Floor Stub boundaries.
- `ATunicEncounterSpawner` owns spawned encounter membership and cleanup only; `ATunicGameMode` remains the class that transitions to `EncounterCleared`.
- Review fixed a small presentation-edge issue: `OnEncounterSpawned` now fires only after at least one enemy is actually spawned, avoiding misleading Blueprint presentation when SpawnEntries are empty or invalid.
- Adversarial review can defend the placed Spawner Actor plus authored spawn point references for v1 because the current map is still a fixed test map and the project needs encounter membership before random director logic.
- Non-blocking risk: GameMode currently uses the first placed `ATunicEncounterSpawner` it finds. Multiple concurrent encounter rooms, non-scoring enemies, boss phases, or portal-pressure waves will need explicit encounter selection or a manager before this becomes final.
- Non-blocking risk: Spawner v1 destroys its own previous spawned enemies on floor stub reset. That is correct for the placeholder loop, but real floor transition/map loading should define cleanup timing, loot persistence, and death presentation lifetime deliberately.

Shared XP v1 implementation checkpoint:

- Added replicated `SharedRunExperience` to `ATunicGameState`.
- `ATunicGameState` now exposes `GetSharedRunExperience()`, server-only `AddSharedRunExperience(...)`, and `OnSharedRunExperienceChanged(...)` for Blueprint presentation.
- Added enemy-side `ExperienceReward` configuration and `GetExperienceReward()`.
- Enemy death now routes through `ATunicGameMode::HandleEnemyDeath(...)` instead of directly calling encounter clear evaluation.
- `ATunicGameMode` grants XP only when the dead enemy belongs to the active `ATunicEncounterSpawner` membership and RunState is still `CombatActive`.
- `ATunicEncounterSpawner` now exposes `IsEncounterEnemy(...)` for GameMode reward routing.
- Shared XP v1 does not add XP pickups, equipment drops, levels, upgrade choices, UI progress bars, permanent progression, or SaveGame persistence.

Shared XP v1 validation completed:

- User confirmed compile and PIE validation passed.
- Spawned enemy kills increase `SharedRunExperience` by the enemy's `ExperienceReward`.
- Shared XP starts at `0` on `ATunicGameState` and changes only when a qualifying encounter enemy dies.
- Only active `ATunicEncounterSpawner` members award XP; manually placed non-spawner enemies remain excluded by design.

Shared XP v1 strict review:

- Normal review found no blocking issue in the implemented path: clients cannot mutate XP directly, enemies do not write GameState XP directly, XP does not move into PlayerState or GAS attributes, and `HandleEnemyDeath` is gated by authority, `CombatActive`, and spawner membership.
- Duplicate XP is guarded by the existing enemy `SetDead(...)` state transition path; the reward route runs when death is first applied.
- Adversarial review can defend the current ownership split: `ATunicGameState` is the replicated shared pool, `ATunicGameMode` owns reward routing, `ATunicEnemyCharacter` only exposes reward value, and `ATunicEncounterSpawner` owns encounter membership.
- Non-blocking risk: Shared XP currently has no HUD, startup log, or visible initial-state presentation except Blueprint hook/log access on value changes. A real HUD should bind to the replicated GameState value later.
- Non-blocking risk: XP is not reset on floor transition. This is intentional for run-local growth, but run restart / party wipe / new run must define explicit reset behavior.
- Non-blocking risk: hand-placed enemies do not award XP unless they are explicitly introduced into encounter membership later. If authored scene enemies need to count, add an explicit registered-member path on the spawner instead of returning to global scene scanning.

Shared XP Presentation v1 implementation checkpoint:

- Added `UTunicRunStatusWidget` as a C++ fallback HUD widget that displays floor index, run state, and shared run XP.
- `ATunicPlayerController` now creates the run-status widget locally through a configurable `RunStatusWidgetClass`.
- `ATunicGameState` now broadcasts BlueprintAssignable presentation events for run state, floor index, and shared run XP changes while preserving the existing BlueprintNativeEvent hooks.
- The widget reads replicated GameState data only; it does not mutate XP, RunState, Floor, rewards, or combat state.
- Shared XP Presentation v1 does not add level-up, XP bars, upgrade choices, equipment drops, permanent progression, CommonUI, or final HUD styling.

Shared XP Presentation v1 validation completed:

- User confirmed compile and PIE validation passed.
- The local run-status HUD shows floor, run state, and shared XP.
- Spawned enemy kills update shared XP in the HUD.
- Encounter Clear, Portal readiness, and Floor Stub transitions update HUD-visible run/floor state.

Shared XP Presentation v1 strict review:

- Normal review found no blocking issue: the HUD is local presentation only, reads replicated `ATunicGameState` values, and does not mutate XP, RunState, Floor, rewards, or combat state.
- Delegate lifecycle is bounded to widget construct/destruct, uses `AddUniqueDynamic`, and unbinds from the previously bound GameState.
- `ATunicPlayerController` owns local viewport creation only; it does not own run progression or replicated state decisions.
- Adversarial review can defend the C++ fallback widget for v1 because the goal is deterministic PIE visibility without requiring a hand-authored WBP asset before the gameplay loop is stable.
- Non-blocking risk: C++ fallback widget is intentionally plain and should become a WBP subclass or be replaced by final HUD work once XP levels, equipment, party status, and portal prompts are stable.

Run Level Stub v1 implementation checkpoint:

- Added replicated `SharedRunLevel` to `ATunicGameState`, defaulting to 1.
- Added `SharedExperiencePerLevel`, defaulting to 5, for the current v1 formula `Level = 1 + SharedRunExperience / SharedExperiencePerLevel`.
- `ATunicGameState::AddSharedRunExperience(...)` now recalculates shared run level on the server after XP changes.
- Added level presentation events/hooks: `OnSharedRunLevelChangedEvent` and `OnSharedRunLevelChanged(...)`.
- `UTunicRunStatusWidget` now displays `Level: X` and refreshes when shared run level changes.
- Run Level Stub v1 does not apply attributes, skills, upgrade choices, equipment, rewards, XP bars, persistence, or per-player progression.

Run Level Stub v1 validation completed:

- User confirmed compile and PIE validation passed.
- HUD shows `XP: 0` and `Level: 1` at start.
- Enemy `ExperienceReward` is currently kept at 5 as a development-phase default so one spawned enemy kill can move shared XP to the first level threshold; rebalance later when enemy archetype/DataAsset tuning begins.
- Shared XP and shared run level update through the replicated GameState path and remain visible in the HUD.
- Portal/Floor Stub does not clear XP or Level.

Run Level Stub v1 strict review:

- Normal review found no blocking issue: Level remains a server-derived `ATunicGameState` value, not a HUD/PlayerController/PlayerState/client-owned value.
- Level changes are triggered only after server-side `AddSharedRunExperience(...)` changes shared XP; clients receive replicated XP and Level.
- The HUD reads `GetSharedRunLevel()` and listens to presentation events only; it does not mutate XP, Level, RunState, Floor, rewards, or combat state.
- Adversarial review can defend Level as a replicated GameState placeholder because it is shared, run-local, derived from shared XP, and currently has no gameplay authority beyond presentation.
- Non-blocking risk: `ExperienceReward=5` is useful for current development iteration but may be too high as a project default once enemy archetype/DataAsset tuning begins.
- Non-blocking risk: XP and Level replicate as separate properties, so a client could briefly see XP update before Level catches up. This is acceptable for v1 HUD readability, but a future polished HUD can coalesce updates or animate after both values settle.

Run Upgrade Choice Stub v1 implementation checkpoint:

- Added replicated `PendingRunUpgradeChoices` to `ATunicPlayerState`, defaulting to 0.
- Added `GetPendingRunUpgradeChoices()`, server-only `AddPendingRunUpgradeChoices(...)`, and presentation events/hooks for pending choice changes.
- `ATunicGameMode::HandleEnemyDeath(...)` now compares shared run level before and after XP reward; each level gained grants one pending run upgrade choice to every current `ATunicPlayerState`.
- `UTunicRunStatusWidget` now displays `Upgrade Available: X` for the local owning player's pending choices.
- Run Upgrade Choice Stub v1 does not generate upgrade options, consume pending choices, apply attributes, grant abilities, open a selection UI, pause gameplay, or change equipment.

Run Upgrade Choice Stub v1 validation completed:

- User confirmed compile success.
- User confirmed PIE validation passed: the HUD shows pending upgrade availability, shared level-up grants pending upgrade choices, and the current XP/Level/HUD loop remains functional.
- Pending choices are not cleared by the current Portal/Floor Stub loop.
- Run Upgrade Choice Stub v1 still does not generate upgrade options, consume pending choices, apply attributes, grant abilities, open a selection UI, pause gameplay, or change equipment.

Run Upgrade Choice Stub v1 strict review:

- Normal review found no blocking issue: pending choices live on `ATunicPlayerState`, shared XP/Level stay on `ATunicGameState`, and reward-to-pending routing stays server-owned in `ATunicGameMode`.
- `ATunicGameMode::HandleEnemyDeath(...)` compares shared run level before and after the server XP award, then grants pending choices only for positive level deltas.
- The HUD reads replicated `ATunicGameState` plus the owning `ATunicPlayerState`; it does not consume pending choices, send RPCs, apply upgrades, mutate attributes, or alter combat/floor systems.
- Adversarial review can defend putting pending choices on `ATunicPlayerState` because the state is per-player, replicated to each client, and explicitly not shared progression or GAS attributes.
- Adversarial review can defend keeping option generation and reward application out of this checkpoint because those require separate authority, UI, attribute/ability, and persistence rules.
- Non-blocking risk: late-joining players do not receive historical pending choices. This is accepted v1 behavior and should be revisited when participant registration or reconnect policy is introduced.
- Non-blocking risk: there is no consume/apply path yet, so pending choices can only accumulate. The next upgrade-selection stage must add server-authoritative consume semantics before any real reward effects are granted.

Upgrade Selection Stub v1 implementation checkpoint:

- Added server-only pending-choice consumption on `ATunicPlayerState`.
- Added owning `ATunicPlayerController` request/RPC path for the local upgrade-selection stub.
- Added a temporary `Select Upgrade` button to the C++ fallback `UTunicRunStatusWidget`.
- The button is enabled only when the local player's pending choice count is greater than 0.
- Consuming a choice only decrements `PendingRunUpgradeChoices`, logs the result, and refreshes replicated HUD state.
- Upgrade Selection Stub v1 does not generate real options, apply attributes, grant skills, grant equipment, pause gameplay, or save progression.

Upgrade Selection Stub v1 validation completed:

- User confirmed compile success.
- User confirmed PIE validation passed for the upgrade-selection stub.
- The current C++ fallback HUD button can consume pending choices through the server path and refresh the local HUD count.
- The stage still does not generate real options, apply attributes, grant skills, grant equipment, pause gameplay, or save progression.

Upgrade Selection Stub v1 strict review:

- Normal review found no blocking issue: the HUD only requests selection through the owning `ATunicPlayerController`; it does not directly mutate `ATunicPlayerState`.
- Pending consume is server-owned on `ATunicPlayerState::TryConsumePendingRunUpgradeChoice()` and guards against negative pending counts.
- `ATunicPlayerController::ServerRequestSelectRunUpgradeStub()` consumes only the owning controller's `ATunicPlayerState`, so one player's click does not affect another player's pending count.
- The selection stub does not affect XP, Level, Health, Stamina, abilities, equipment, Spawner, Portal, Floor Stub, player death, or combat state.
- Adversarial review can defend the temporary HUD button because v1 needs an asset-free validation入口 and the request still crosses the server authority boundary before changing state.
- Non-blocking risk: the fallback HUD is not final UI and does not handle focus/input-mode policy. A real upgrade selection UI should define whether gameplay pauses, whether movement input remains active, and how controller/gamepad focus is handled.
- Non-blocking risk: the consumed hook has no selected option payload. This is intentional for the single-option stub, but real upgrade choices must add an option ID/DataAsset and server validation before applying rewards.

## Completed

- Built `LpQuestEditor Win64 Development` with UE 5.8 successfully after migration.
- Fixed UE 5.8 deprecation warning by replacing direct `NetUpdateFrequency` assignment with `SetNetUpdateFrequency(100.0f)`.

- Created UE 5.8 C++ project shell: `E:\UnRealEngine\LpQuest`.
- Migrated gameplay C++ skeleton into `Source/LpQuest/Public` and `Source/LpQuest/Private`.
- Updated module dependencies in `LpQuest.Build.cs` for Enhanced Input, GAS, StateTree, AI, and UMG.
- Enabled required plugins in `LpQuest.uproject`: GameplayAbilities, GameplayStateTree, StateTree, AnimationLocomotionLibrary, and AnimationWarping.
- Created content directory skeleton under `Content/_Game` and `Content/_External/KayKit`.
- Rewrote project documentation for UE 5.8, multiplayer PvE, PlayerState-owned ASC, and short/long plan split.
- Enabled and configured the official UE 5.8 Model Context Protocol plugin for editor automation.
- Created Enhanced Input assets under `Content/_Game/Input`: `IA_Move`, `IA_Dodge`, `IA_LightAttack`, `IA_HeavyAttack`, `IA_Aim`, `IA_Interact`, `IA_SwitchWeapon`, and `IMC_Player`.
- Created and configured `BP_TunicPlayerCharacter`, `BP_TunicPlayerController`, and `BP_TunicGameMode`.
- Set the OpenWorld template map as the current default validation map because the empty `LV_TestMap` was visually unhelpful.
- Imported cleaned KayKit Adventurers and Skeletons prototype assets under `Content/_External/KayKit`.
- Assigned a visible KayKit prototype mesh to the player Blueprint.
- Fixed screen-relative movement direction in C++ after Enhanced Input modifiers were configured.
- Validated single-player PIE fixed camera and WASD movement.
- Validated Listen Server + 2 Players spawning and movement.
- Initialized Git with UE-focused `.gitignore`, Git LFS tracking for binary assets, and a Stage 1 baseline commit.
- Stopped tracking `plan.md` and `tunicplan.md`; both remain local-only planning notes.
- Added `ATunicPlayerCharacter` GAS initialization logging with a Blueprint-facing enable/disable switch for later settings UI integration.
- Validated the GAS debug path in Listen Server + 2 Players:
  - Server pawns logged `Authority=true`.
  - Owning clients logged autonomous proxy roles.
  - Other players logged simulated proxy roles.
  - ASC OwnerActor and AvatarActor matched the intended PlayerState-owned ASC architecture.
- Added the initial server-aware `Dodge` debug request path:
  - Local input calls a request function.
  - Client-owned pawns send a reliable server RPC.
  - Server logs accepted dodge requests and calls the existing `OnDodgeRequested` Blueprint hook.
  - No real dodge movement, invulnerability, stamina cost, montage, or cooldown has been added yet.
- Validated the `Dodge` debug request path in Listen Server + 2 Players:
  - Pressing Space in each client window produced a server-side `Dodge request accepted on server` log.
  - Server logs showed the correct `BP_TunicPlayerCharacter`, `TunicPlayerState`, non-null ASC, non-null AttributeSet, and unchanged `Health/Stamina=100/100`.
  - This confirms the current dodge input path reaches server authority without client-side combat state mutation.
- Added an AGENTS.md stage rule: each stage ends with a review pass for reliability, decoupling, architecture debt, and multiplayer/GAS shortcuts.
- Added the initial server-aware `LightAttack` debug request path:
  - Local input calls a request function.
  - Client-owned pawns send a reliable server RPC.
  - Server logs accepted light attack requests and calls the existing `OnLightAttackRequested` Blueprint hook.
  - No damage, weapon trace, target mutation, montage dependency, stamina cost, or cooldown has been added yet.
- Validated the `LightAttack` debug request path in Listen Server + 2 Players:
  - Pressing Left Mouse Button in each client window produced server-side `Light attack request accepted on server` logs.
  - Server logs showed the correct `BP_TunicPlayerCharacter`, `TunicPlayerState`, non-null ASC, non-null AttributeSet, and unchanged `Health/Stamina=100/100`.
  - This confirms the current light attack input path reaches server authority without client-side damage, traces, or attribute mutation.
- Added enemy GAS initialization logging:
  - Logs enemy ASC, OwnerActor, AvatarActor, AttributeSet, Health/MaxHealth, Stamina/MaxStamina, authority, LocalRole, and RemoteRole.
  - Intended validation is `OwnerActor=EnemyCharacter`, `AvatarActor=EnemyCharacter`, and non-null AttributeSet.
- Created and placed a minimal `BP_TunicEnemyCharacter` for validation.
- Validated enemy GAS initialization in PIE:
  - Server log showed `Authority=true`, enemy-owned ASC, `OwnerActor=BP_TunicEnemyCharacter`, `AvatarActor=BP_TunicEnemyCharacter`, non-null AttributeSet, and `Health/Stamina=100/100`.
  - Client log showed the replicated enemy as simulated proxy with the same owner/avatar/attribute values.
  - This confirms the enemy self-owned ASC path is ready for later damage, targeting, and AI work.
- Completed the stage review pass for Stage 1 and the current Stage 2 validation checkpoint:
  - Stage 1 editor setup is reliable enough to build on: inputs, player/controller/game mode Blueprints, player mesh, single-player movement, and listen-server spawning are verified.
  - Player GAS, dodge request, light attack request, and enemy GAS validation all preserve server authority and avoid client-side damage or direct health mutation.
  - Current debug code is intentionally narrow. Shared server input request logging is kept as a private helper instead of a new framework.
  - The remaining risk is that debug toggles are per-character defaults rather than a centralized settings UI; this is acceptable until the later pause/settings screen pass.
- Added a debug-only server-side light attack target query:
  - On accepted server light attack requests, the player performs a short sphere overlap around the character.
  - The query logs the nearest `ATunicEnemyCharacter` target found.
  - The log includes target ASC, AttributeSet, Health/MaxHealth, Stamina/MaxStamina, and target network roles.
  - This does not apply damage, GameplayEffects, hit reactions, target mutation, stamina costs, cooldowns, or montage logic.
- Built `LpQuestEditor Win64 Development` successfully after adding the light attack target query.
- Validated the light attack target query in PIE:
  - One attack found `BP_TunicEnemyCharacter` at `TargetDistance=141.2`.
  - The found target had non-null ASC and AttributeSet.
  - Logged target `Health/Stamina=100/100`, confirming the server can read enemy GAS state before damage is added.
  - A second attack logged `Light attack target query found no enemy`, confirming the no-target debug branch works.
- Completed a strict review of the current Stage 2 debug/targeting checkpoint:
  - Normal review found no blocking multiplayer/GAS authority issue.
  - Fixed one review finding: disabling `bLogLightAttackTargetQuery` now skips both the debug overlap query and the target log instead of only suppressing the log after doing the query.
  - Adversarial review can defend the current target query only as a temporary validation tool. It is not the final melee hit model because it is proximity-based, not animation/weapon-window based.
  - Remaining non-blocking risk: Reliable RPCs and proximity overlap are acceptable for debug validation, but real combat must move to gated GAS activation, action locks/cooldowns, and authoritative hit confirmation.
- Added a minimal debug-only GAS damage path:
  - `UTunicDamageGameplayEffect` is an instant GameplayEffect that subtracts 10 Health.
  - `ATunicPlayerCharacter` defaults light attack debug damage to that effect class.
  - Server-side light attack applies the effect to the found enemy ASC, using the player ASC as the preferred effect context source.
  - The path logs target Health before and after application.
  - This still has no animation hit window, weapon trace, hit reaction, death handling, stamina cost, cooldown, or action lock.
- Built `LpQuestEditor Win64 Development` successfully after adding `UTunicDamageGameplayEffect`.
- Validated minimal GAS damage in PIE:
  - Light attack found `BP_TunicEnemyCharacter`.
  - Enemy Health decreased through the GameplayEffect path.
  - Follow-up target query showed `TargetHealth=90.0/100.0`, confirming the enemy ASC/AttributeSet state changed on the server.
- Completed the final strict review for Stage 2:
  - No blocking issue found for the debug validation checkpoint.
  - Normal review found no direct Health mutation from input, no client-side damage authority, and no local-only projectile/combat result shortcut.
  - Adversarial review can defend the minimal GameplayEffect damage path as a validation bridge, but not as final combat design.
  - Stage 3 should begin with Health clamping and minimal death-state rules before formal weapon traces or animation hit windows.
- Started Stage 3 with minimal Health clamp and death-state rules:
  - `UTunicAttributeSet` now clamps Health and Stamina against their max values.
  - `MaxHealth` and `MaxStamina` are clamped to at least 1.
  - `ElementalPower` is clamped to non-negative values.
  - Current Health/Stamina are re-clamped after max-value GameplayEffect changes.
  - `ATunicEnemyCharacter` now listens for Health changes through its ASC and marks itself dead on the server when Health reaches 0.
  - Enemy death state replicates through `bIsDead`, disables capsule collision and character movement, calls a Blueprint death-state hook, and applies `State.Dead` as a loose gameplay tag once the tag dictionary is loaded.
  - The current debug light-attack target query and damage path skip enemies that are already dead.
  - Added a temporary default death presentation for validation: `OnDeathStateChanged(true)` rotates and flattens the enemy mesh so death is visible in PIE even before real death animations exist.
- Validated the Stage 3 reliability checkpoint in PIE:
  - Enemy Health reached `0.0/100.0` and logged `Enemy entered death state` on the server.
  - Further light attack target queries reported no enemy after death, confirming dead enemies are no longer selected by the current debug overlap path.
  - Temporary death presentation was visible in PIE.
- Completed the strict review for the Stage 3 reliability checkpoint:
  - Normal review found no direct client-side damage, no direct Health mutation from input, and no local-only death authority path.
  - Attribute clamping is centralized in `UTunicAttributeSet`, so later damage GameplayEffects inherit the same bounds.
  - Enemy death state is server-authored and replicated through `bIsDead`; client-side presentation is driven by replication, not by local combat prediction.
  - Adversarial review can defend the current death state as a minimal checkpoint, but not as the final death system.
  - Remaining non-blocking risks at the time: `State.Dead` still needed formal tag registration, the mesh rotate/flatten presentation was temporary, and future non-debug damage paths must also respect `IsDead()` or GAS death tags. The tag registration part is now addressed by the config checkpoint below.
- Started the light attack hit-confirmation checkpoint:
  - Replaced the proximity debug enemy query with a server-side capsule sweep hit window.
  - Added `BeginLightAttackHitWindow`, `ProcessLightAttackHitWindow`, and `EndLightAttackHitWindow` as Blueprint-callable hooks for later animation notify state integration.
  - The current input path still runs one immediate Begin/Process/End window as a validation fallback until a montage/notify path exists.
  - The sweep filters dead enemies and prevents repeat damage to the same enemy within one hit window.
  - Damage still flows through the existing minimal instant GameplayEffect, not direct Health mutation.
- Validated and strictly reviewed the light attack hit-confirmation checkpoint:
  - Manual validation confirmed the server-side sweep applies damage when the enemy is in front, does not apply damage from side/behind placements, continues skipping dead enemies, and the fixed camera no longer retracts when close to enemies.
  - Normal review found no blocking server-authority issue, no direct client-side damage, no direct Health mutation from input, and no regression to dead-target damage.
  - Adversarial review can defend the current sweep as a better validation bridge than the earlier proximity overlap because the damage decision now depends on a forward hit volume and a hit window.
  - Remaining non-blocking risks: the immediate Begin/Process/End path is still a validation fallback, future AnimNotify-driven windows must execute on the server path, capsule sweep is not final weapon/socket tracing, and disabled SpringArm collision also means world geometry will not retract the fixed camera.
- Registered the initial GameplayTags dictionary through config:
  - `Config/DefaultGameplayTags.ini` enables tag import from project config and defines the current element, state, ability, cooldown, and enemy-type tags.
  - `State.Dead` is now formally registered for the current enemy death loose tag path.
- Added the first minimal player GameplayAbility:
  - `UTunicGameplayAbility_LightAttack` is tagged `Ability.Attack.Sword.Light` and uses server-only execution.
  - `ATunicPlayerCharacter` grants the light attack ability to the PlayerState-owned ASC during ASC initialization on the server.
  - Light attack input now tries to activate the ability by tag first, then falls back to the old server RPC path if ability activation is not available.
  - Ability activation logs `Light attack ability activated on server`, which should appear before the existing light attack request/hit sweep logs when the GAS path is active.
  - The ability currently delegates to the existing server-side light attack request and hit-window path. It does not yet own stamina cost, cooldown, action lock, montage playback, or hit reactions.
- Completed a strict static review for the GameplayTags and minimal light attack GameplayAbility checkpoint:
  - Normal review found no intentional client-side damage path, no direct Health mutation from input, and no new client-owned combat authority.
  - Ability granting is server-only and guarded against duplicate grants by ability class.
  - Light attack activation now prefers ASC ability activation but keeps the old server RPC fallback so validation is not blocked if ability spec replication or tag refresh timing is not ready.
  - Adversarial review can defend the change as a narrow migration step from character-owned RPC combat toward GAS activation.
  - Remaining risks before closing this checkpoint at the time: compile result was not yet reported, PIE had not yet confirmed the new `Light attack ability activated on server` log, and the old RPC fallback still needed to be removed once ability activation was proven reliable. Compile and PIE activation have since been validated; the fallback-removal risk remains.
- Fixed a UE 5.8 compile issue in the new light attack GameplayAbility:
  - `FGameplayTagContainer AbilityTags` in the ability constructor shadowed `UGameplayAbility::AbilityTags` and failed under C4458.
  - Renamed the local container to `DefaultAbilityTags`.
  - This pattern has been recorded in server memory for future GameplayAbility code.
- Moved the initial tag list into `Config/DefaultGameplayTags.ini` instead of `Config/Tags/LpQuestGameplayTags.ini` so the tags appear directly in Project Settings' main `GameplayTagList`.
- Fixed the `DefaultGameplayTags.ini` section split after Editor still showed zero tags:
  - `ImportTagsFromConfig` and `WarnOnInvalidTags` stay under `/Script/GameplayTags.GameplayTagsSettings`.
  - `GameplayTagList` entries now live under `/Script/GameplayTags.GameplayTagsList`, matching UE's underlying config class for the tag list.
- Updated `DefaultGameplayTags.ini` tag rows to use UE config array append syntax (`+GameplayTagList=...`) after Editor still showed an empty tag list.
- Reverted the tag list back under `/Script/GameplayTags.GameplayTagsSettings` after Project Settings still showed zero rows; this matches the settings panel and keeps the array append syntax.
- Validated the minimal light attack GameplayAbility path in PIE:
  - Server log showed `Light attack ability activated on server`.
  - Ability ActorInfo matched the intended architecture: `OwnerActor=TunicPlayerState`, `AvatarActor=BP_TunicPlayerCharacter`.
  - The same activation flowed into the existing server request, hit sweep, and debug GameplayEffect damage path.
  - Enemy Health changed from `100.0` to `90.0` through the GameplayEffect path.
- Completed strict review after the successful GameplayAbility PIE validation:
  - Normal review found no new client-side damage path, direct Health mutation, local-only hit authority, or ASC ownership regression.
  - `UTunicGameplayAbility_LightAttack` is server-only and delegates to the same server-authoritative hit window already validated.
  - `ATunicPlayerCharacter` still owns camera/input/presentation and grants the initial player ability to the PlayerState-owned ASC on server initialization.
  - Adversarial review can defend this as a narrow GAS migration checkpoint, not a final combat system.
  - Remaining risks: old light-attack RPC fallback should be removed once ability activation is stable across more inputs, the immediate Begin/Process/End hit window is still a validation fallback, and cooldown/cost/action-lock rules are still missing.
  - `Config/Tags/LpQuestGameplayTags.ini` is an empty leftover tag source that Windows currently denies deleting; do not stage it unless it is intentionally reused.
- Added the first light attack GAS gates:
  - `UTunicLightAttackCostGameplayEffect` applies an instant `-10` Stamina modifier.
  - `UTunicLightAttackCooldownGameplayEffect` applies a short `0.35s` duration and grants `Cooldown.Attack` plus `State.ActionLocked` through UE 5.8's `UTargetTagsGameplayEffectComponent` path instead of deprecated GameplayEffect tag fields.
  - `UTunicGameplayAbility_LightAttack` now uses those native cost/cooldown GameplayEffects and blocks activation while `Cooldown.Attack`, `State.ActionLocked`, or `State.Dead` is present.
  - Light attack input no longer falls back to the old RPC when the player ASC and light attack ability class exist but ability activation fails. This prevents cooldown or cost failure from being bypassed by the bootstrap RPC path.
  - The old `ServerRequestLightAttack` path remains only as a bootstrap fallback for missing ASC/ability setup and should still be removed once the GAS activation path is considered stable enough.
  - User validated this checkpoint after recompiling and launching the editor: temporary debug draw made the Stamina/Health changes visible in PIE, and the light attack gate behavior passed the current manual check.
- Added temporary world debug draw for attribute validation:
  - `ATunicPlayerCharacter` draws player Health/Stamina above the character in cyan.
  - `ATunicEnemyCharacter` draws enemy Health/Stamina above the character in red, and shows a grey `DEAD` label after death.
  - Both classes expose `SetAttributeDebugDrawEnabled` for later Blueprint/settings UI toggles.
  - This is intentionally not final HUD work; it exists to make GAS cost, damage, replication, and death validation visible during PIE.
- Fixed a startup crash in the native light attack cooldown GameplayEffect:
  - Crash evidence showed `NewObject with empty name can't be used to create default subobjects` while constructing `UTunicLightAttackCooldownGameplayEffect`.
  - Root cause was calling `FindOrAddComponent<UTargetTagsGameplayEffectComponent>()` inside a native `UGameplayEffect` constructor; in UE 5.8 this can use `NewObject` with an empty name during CDO construction.
  - The cooldown effect now uses `CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"))` before applying granted tags.
  - This reusable error pattern has been recorded in server memory.
- Completed strict review for the GAS gate + temporary debug draw checkpoint:
  - Normal review found no blocking client-side damage path, direct Health/Stamina mutation from input, local-only hit authority, or ASC ownership regression.
  - The light attack path now prevents cooldown/cost activation failure from falling through to the old `ServerRequestLightAttack` bootstrap RPC when the ASC and light attack ability class are available.
  - Non-blocking risk: temporary `DrawDebugString` calls run every Tick on player/enemy actors and create transient HUD debug text each frame. This is acceptable for current PIE validation but should be replaced by a centralized debug widget/HUD path before spawning many enemies or exposing it as real settings UI.
  - Non-blocking risk: the cooldown GameplayEffect depends on registered GameplayTags. If `Cooldown.Attack` is missing, cooldown tag granting silently becomes ineffective; current project config and user validation indicate this is working now, but future tag edits should be checked.
  - Adversarial review can defend the checkpoint as a narrow GAS validation step: cost/cooldown are native GameplayEffects, activation is server-only, damage remains server-side through GameplayEffects, and debug draw is explicitly temporary.
  - Remaining design debt at the time: old light attack bootstrap RPC still exists, hit timing is still immediate Begin/Process/End, Stamina regeneration had not been added yet, and debug draw is not final UI. The minimal Stamina regeneration part is now addressed by the checkpoint below.
- Added minimal GAS Stamina regeneration:
  - `UTunicStaminaRegenGameplayEffect` is an infinite periodic GameplayEffect that restores `+2.5` Stamina every `0.25s`, for a baseline of 10 Stamina per second.
  - The effect is applied to the PlayerState-owned player ASC from `ATunicPlayerCharacter::ApplyDefaultEffects` on the server after ASC actor info initialization.
  - `ApplyDefaultEffects` first checks the current Character handle, then scans the PlayerState-owned ASC for an already-active regen effect class, preventing duplicate infinite regen effects across repeated ASC initialization or future Pawn swaps.
  - This does not yet include attack-delay, exhaustion, tag-gated blocking, or DataAsset tuning.
- Validated and accepted the minimal Stamina regeneration checkpoint:
  - The project compiled after the native GameplayEffect constructor fix.
  - PIE validation showed Stamina cost through light attack and recovery through the debug draw path.
  - This checkpoint remains a minimal baseline; combat-delay, exhaustion, tag-gated regen blocking, and DataAsset tuning are intentionally deferred.
- Started the Stage 4 animation-timing bridge:
  - Added the animation NotifyState bridge that is now consolidated as `UTunicAnimNotifyState_CombatHitWindow`.
  - The notify state forwards animation NotifyBegin/NotifyTick/NotifyEnd through `ITunicCombatHitWindowSourceInterface`; the player character maps that shared interface to `BeginLightAttackHitWindow`, `ProcessLightAttackHitWindow`, and `EndLightAttackHitWindow`.
  - The notify state does not apply damage, mutate attributes, or inspect target ASC state directly; hit confirmation and damage remain owned by the existing server-authoritative player character path.
  - This checkpoint still needs `LpQuestEditor Win64 Development` compilation and PIE validation with a real light attack animation or montage.
- Added initial fixed-view movement and attack-facing support for the player character:
  - WASD movement uses the fixed camera's projected screen up/down/left/right directions on the ground plane.
  - Movement does not depend on current character facing.
  - `ATunicPlayerController` shows the mouse cursor by default for attack-facing validation.
  - Locally controlled players continuously face the current mouse cursor direction.
  - Mouse-facing yaw is sent to the server on a throttled unreliable RPC, and light attack forces one immediate yaw update before ability activation so the server sweep should use the intended attack direction.
  - User validated fixed-view movement in PIE.
- Added initial light attack Montage playback support:
  - `ATunicPlayerCharacter` now exposes `LightAttackMontage` for Blueprint defaults.
  - The server-authoritative light attack path multicasts Montage playback when a Montage is configured.
  - When a Montage is configured, the immediate Begin/Process/End hit-window fallback is skipped so damage should come from the Montage's `Tunic Combat Hit Window` notify state.
  - When no Montage is configured, the old immediate hit-window fallback remains available for bootstrap validation.
- Validated the first light attack Montage/Notify checkpoint in PIE:
  - Light attack Montage plays from the GAS activation path.
  - The attack Montage's hit-window notify state ticks the server-side hit sweep during the Montage window; this path is now served by the shared `Tunic Combat Hit Window` NotifyState.
  - Enemy Health decreases through the existing GameplayEffect damage path.
  - The repeated `Light attack hit sweep completed` logs are expected while the notify state is active; the current hit-window target set should still limit each enemy to one damage application per attack window.
- Added and validated a temporary visual weapon attachment on the player Blueprint:
  - A right-hand weapon socket and `WeaponMesh` component provide the current sword visual.
  - Light attack damage validation still passes with the weapon visual attached.
  - The weapon is still presentation-only; it does not own hit detection or damage.
  - A weapon collision/physics issue initially displaced enemies during attacks; setting the visual `WeaponMesh` to no collision/no physics fixed the displacement without changing gameplay damage logic.
- Completed the Stage 4 strict review after Montage/Notify damage, mouse-facing, and visual weapon validation:
  - Normal review found no blocking client-side damage path, direct Health mutation from input, or local-only hit authority.
  - The server-only light attack GameplayAbility still owns cost/cooldown commitment and delegates final hit confirmation to the server-side character hit window.
  - The hit-window NotifyState remains a timing bridge only; it does not mutate attributes or inspect target ASC state directly. This bridge is now consolidated as the shared `Tunic Combat Hit Window`.
  - Visual weapon collision was correctly isolated as presentation-only after the enemy displacement issue was fixed by disabling weapon collision/physics.
  - Non-blocking risk: when `LightAttackMontage` is configured, the old immediate hit-window fallback is skipped. Bad Montage/Slot/Notify setup can therefore produce cost/cooldown with no damage, which is acceptable now because the current assets have been manually validated.
  - Non-blocking risk: the Ability ends immediately after starting the Montage, so action lock timing depends on the native cooldown/action-lock GameplayEffect duration rather than Montage completion.
  - Non-blocking risk: continuous mouse-facing can rotate the character during an active attack window. This matches the current feel target but should later be gated by action-lock, lock-on, or attack-facing rules if combat needs committed attack direction.
  - Adversarial review can defend the current Stage 4 path as a narrow prototype bridge: animation timing now drives the server-authoritative sweep, damage still flows through GameplayEffects, and the visual weapon remains separate from hit authority.

## Stage 5 / Deferred Fixes

- Enemy feedback:
  - Implemented the first minimal enemy hit reaction hook in C++:
    - `ATunicEnemyCharacter::HandleHitReaction` is server-only and skips dead enemies.
    - `MulticastPlayHitReaction` broadcasts confirmed non-lethal hit presentation to all peers.
    - `OnHitReaction` is a Blueprint-native presentation hook and does not own damage, stun, knockback, or AI state.
  - Connected the hook from the current light attack damage path after GameplayEffect damage reduces target Health.
  - Moved the old hard-coded C++ death flatten/rotate out of the default death-state implementation. Death authority still disables capsule collision, stops movement, applies `State.Dead`, and calls the Blueprint death-state hook once per instance.
  - Validation completed:
    - User compiled successfully after removing the incorrect `const` qualifier from `ApplyLightAttackDebugDamage`.
    - `BP_EnemyCharacterBase` now provides the minimal shared presentation graph: `OnHitReaction` prints/debugs and plays the configured hit reaction Montage when present; `OnDeathStateChanged(true)` prints/debugs the death event.
    - Single-player and Listen Server + 2 Players validation passed: both windows see the hit reaction Montage and hit print, both windows see the death print, and dead enemy collision is disabled.
    - Death Montage validation completed: `BP_EnemyCharacterBase` now plays a configured death Montage from `OnDeathStateChanged(true)`, both Listen Server PIE windows see the death animation and death print, and dead enemy collision remains disabled.
  - Follow-up presentation direction:
    - Added a C++ default presentation path for common enemy hit/death Montage playback.
    - `DefaultHitReactionMontage` and `DefaultDeathMontage` are editable C++ properties, so base and concrete enemy Blueprints can configure assets in Details without duplicating graph nodes.
    - Keep `OnHitReaction` and `OnDeathStateChanged` as Blueprint extension hooks for special effects, one-off enemy behavior, audio, VFX, or debug prints.
    - Move this presentation config into DataAssets later when enemy variants need broader tuning for hit reactions, death behavior, sounds, FX, and animation sets.
  - C++ default Montage validation completed:
    - User compiled successfully.
    - `BP_EnemyCharacterBase` now configures inherited `DefaultHitReactionMontage` and `DefaultDeathMontage`.
    - Blueprint hit/death graphs keep lightweight extension behavior while C++ owns common Montage playback.
    - PIE validation passed: non-lethal hit and death Montages play correctly, death prints fire on both peers, and dead enemy collision remains disabled.
- Ability and montage timing:
  - Revisit `UTunicGameplayAbility_LightAttack` so action lock and ability lifetime can follow Montage completion or explicit gameplay events instead of relying only on fixed-duration GameplayEffects.
  - Added a lightweight server warning if a configured light attack Montage finishes without any combat hit-window notify beginning for that Montage activation.
- Attack-facing rules:
  - Decide whether attacks should keep continuous mouse-facing during active hit windows or cache/freeze the committed attack direction.
  - Gate future facing changes through action-lock, lock-on, aim, stun, and interrupt rules.
- Weapon and hit confirmation:
  - Keep the current visual `WeaponMesh` collisionless and presentation-only.
  - Replace the current capsule sweep bridge with weapon/socket traces or a weapon component once the equipment direction is clearer.
  - Move weapon stats, attack timing, montage references, and hit shapes into DataAssets when multiple weapons/classes need variant tuning.
- Combat interfaces:
  - Add a narrow combat-target interface before expanding target types beyond `ATunicEnemyCharacter`. The goal is to remove the light attack path's hard dependency on the enemy class while keeping damage application server-authoritative through GameplayEffects.
  - Candidate first interface: `ITunicCombatTargetInterface` or equivalent, exposing target eligibility, target ASC access, and confirmed-hit presentation hooks. Do not let the interface directly mutate Health.
  - Implemented the first `ITunicCombatTargetInterface` pass:
    - `ATunicEnemyCharacter` implements target availability, ASC/AttributeSet access, and confirmed-hit reaction routing.
    - `ATunicPlayerCharacter` light attack hit windows now route hit actors through the interface instead of hard-casting to enemy class.
    - Per-hit-window dedupe now tracks target actors, so future combat targets can participate without enemy-specific containers.
    - At this checkpoint, player characters did not implement the interface yet. This was superseded by the later Combat Hit Response Policy v1 below, where players implement the interface as no-friendly-damage hit-reaction targets.
  - Combat target interface validation completed:
    - User compiled successfully after regenerating the Rider `.slnx` for an unrelated Rider/MSBuild run-configuration issue.
    - Single-player/light attack validation passed: attacking an enemy still reduces enemy Health through the existing GameplayEffect path.
    - Listen Server + 2 Players validation passed for both server-controlled and client-controlled attacks: both PIE windows see enemy damage/feedback.
    - Kill validation passed: death presentation remains visible on both peers, dead enemy collision is disabled, and dead enemies do not continue to receive ordinary hit feedback.
  - Later interface candidates to evaluate when their systems are introduced: interactable actors, lock-on targets, health-bar providers, team/faction agents, and pickup/equipment targets.
- Debug and settings:
  - Replace per-character debug draw/log toggles with a centralized debug HUD or settings path before scaling enemy counts or exposing pause-menu toggles.
- Stage 5 strict review:
  - Normal review found no blocking client-side damage path, direct Health mutation from input, or local-only death/feedback authority path.
  - The hit reaction hook only fires after the server applies GameplayEffect damage and observes a non-lethal Health reduction; killing hits are left to the replicated death path.
  - `OnHitReaction` and `OnDeathStateChanged` remain presentation hooks. They do not own target selection, damage, stun, knockback, AI state, or replicated combat state.
  - The Blueprint base presentation graph is acceptable for the current prototype because the user validated both Listen Server PIE windows receiving hit montage/print and death print.
  - Adversarial review can defend `NetMulticast, Unreliable` for prototype hit presentation because lost hit reaction visuals do not change combat state. If hit reactions later become gameplay-relevant, this must move to replicated state, GameplayCues, or another reliable/stateful path.
  - The common enemy hit/death Montage playback path now lives in C++ editable properties. Blueprint event graphs should avoid also playing the same Montages unless intentionally layering extra presentation.
  - Non-blocking risk: death presentation now has a minimal Montage path, but final death polish is still not done. Current validation uses Montage auto blend-out disabled to keep the corpse pose from returning to idle.
  - The light attack path no longer hard-casts hit actors to `ATunicEnemyCharacter`; the first combat target interface pass removes that specific coupling while keeping target damage server-authoritative.
  - Combat target interface focused review found no blocking issue:
    - The interface is only a routing contract for target availability, target ASC/AttributeSet access, and confirmed-hit presentation; it does not apply GameplayEffects or mutate attributes.
    - The current first implementation on `ATunicEnemyCharacter` is defensible because enemies own their ASC directly and already own the death/hit presentation hooks.
    - Historical note: before Combat Hit Response Policy v1, keeping `ATunicPlayerCharacter` off the interface was intentional. The later v1 implementation now adds players as no-friendly-damage hit-reaction targets while still deferring player death/respawn policy.
    - Non-blocking risk: `ApplyLightAttackDebugDamage` is still named as a debug bridge even though it is now routed through a generic target interface. Rename or replace it when weapon/DataAsset damage moves out of the prototype path.
  - Implemented Combat Hit Response Policy v1:
    - Added `ETunicCombatTeam` and `FTunicCombatRules` as the first centralized team/friendly-fire query path.
    - Extended `ITunicCombatTargetInterface` with target team access.
    - `ATunicPlayerCharacter` now implements the combat target interface using its PlayerState-owned ASC/AttributeSet.
    - Light attack hit handling now separates hit reaction permission from damage permission.
    - Player-to-player hits trigger server-confirmed player hit reaction presentation without applying the damage GameplayEffect.
    - Same-team non-neutral hits now trigger hit reaction presentation without applying damage, so future enemy-to-enemy hits can use the same no-friendly-damage feedback rule once enemy attack hit confirmation exists.
    - Player-to-enemy hits still apply the existing GameplayEffect damage path and retain enemy hit/death presentation behavior.
    - Self-hit is filtered out before target handling.
  - Combat Hit Response Policy v1 validation completed:
    - User confirmed compile success.
    - User configured player `DefaultHitReactionMontage`.
    - Listen Server + 2 Players validation passed: server-player-to-client-player and client-player-to-server-player hits trigger player hit reaction presentation without Health loss.
    - Enemy hit/death regression passed: players can still damage enemies through GameplayEffects.
    - Enemy-to-enemy no-damage hit reaction is rule-ready but not yet runtime-validated because enemy attack hit confirmation is not implemented.
  - Combat Hit Response Policy v1 focused review completed:
    - No blocking client-side damage path, direct Health mutation, or local-only authority shortcut found.
    - Damage permission is centralized in `FTunicCombatRules::CanApplyDamage`; hit feedback permission is centralized in `FTunicCombatRules::CanTriggerHitReaction`.
    - Player combat targets route to PlayerState-owned ASC/AttributeSet and do not create player GAS state on the character.
    - Same-team hit feedback is presentation-only and still uses server-confirmed multicast hooks.
    - Non-blocking risk: same-team enemy-to-enemy hit feedback cannot be PIE-validated until the enemy attack hit confirmation path exists.

## Enemy Attack Path v1

- Implemented the first server-authoritative enemy melee attack path:
  - Added `UTunicGameplayAbility_EnemyMeleeAttack`, tagged `Ability.Attack.Enemy.Melee`, as a server-only enemy ASC ability.
  - Added a native enemy melee cooldown/action-lock GameplayEffect instead of reusing the player light-attack cooldown class.
  - `ATunicEnemyCharacter` now grants the melee attack ability from its self-owned ASC initialization path.
  - Added configurable enemy melee attack Montage, damage GameplayEffect class, sweep shape/offsets, logging toggles, and a default-off prototype auto attack trigger.
  - Added `BeginEnemyMeleeHitWindow`, `ProcessEnemyMeleeHitWindow`, and `EndEnemyMeleeHitWindow` as Blueprint-callable server-authoritative hit-window functions.
  - Added `ITunicCombatHitWindowSourceInterface` and `UTunicAnimNotifyState_CombatHitWindow` as the shared attack-window bridge for player and enemy attack Montages.
  - `ATunicPlayerCharacter` maps the shared combat hit-window interface to the existing light-attack hit window; `ATunicEnemyCharacter` maps it to the new enemy melee hit window.
  - Removed the old player/enemy-specific attack NotifyState classes from source now that current assets are expected to use the shared `Tunic Combat Hit Window`.
  - Enemy melee hit handling now routes through `ITunicCombatTargetInterface` and `FTunicCombatRules`: Enemy-to-Player can apply GameplayEffect damage plus hit reaction, Enemy-to-Enemy can trigger no-damage hit reaction, and self-hit is filtered.
  - Enemy melee damage uses a clearly named `ApplyEnemyMeleeDamage` helper and does not reuse the player `ApplyLightAttackDebugDamage` bridge.
- Validation completed:
  - Player and enemy attack Montages now use the single shared `Tunic Combat Hit Window` NotifyState on their real hit frames.
  - User confirmed the current PIE validation passed after replacing the old player-specific hit-window Notify with the shared combat hit-window Notify.
  - User confirmed the Enemy Attack Path v1 validation passed: enemies can attack players, player Health decreases through the server GameplayEffect path, player hit reaction is visible, player light attack still damages enemies, player-to-player hits remain no-damage hit reactions, and dead enemies stay filtered.
  - Source scan confirms the old player/enemy-specific attack NotifyState classes have been removed and no code/doc references to the old Notify names remain.
- Deferred by design:
  - Full AIController/StateTree patrol/chase/attack selection.
  - Player death/respawn/input-disable/camera/UI policy after Health reaches 0.
  - Enemy attack DataAsset tuning, weapon/socket traces, and final enemy archetype data.
- Strict review:
  - Normal review found one real implementation issue: `UTunicGameplayAbility_EnemyMeleeAttack` ended successful activations with `bWasCancelled=true`, which could confuse later Ability-end/cancel listeners even though current prototype validation still worked.
  - Fixed that issue by ending successful enemy melee attack activations as a normal non-cancelled end.
  - No blocking client-side damage, direct Health mutation from input, client-owned AI decision, or local-only hit authority was found in the reviewed path.
  - Adversarial review can defend the current split: the shared `Tunic Combat Hit Window` is timing-only, character hit windows remain server-authoritative, target routing goes through `ITunicCombatTargetInterface`, team/damage permission goes through `FTunicCombatRules`, and Health changes still flow through GameplayEffects.
  - Remaining risks are accepted prototype debt: the auto attack trigger is not final AI, player death policy is not implemented, hit shapes are still capsule sweep bridges, and action-lock/Ability lifetime still need a more formal Montage/event policy later.

## Next Short Plan

1. Implement Player Death Policy v1.
2. Keep the first pass narrow: direct death, no rescue/revive, no respawn UI, no spectator/free-camera mode, and no final party-wipe flow.
3. After validation, run strict review, update docs if needed, then wait for explicit commit approval.

## Player Death Policy v1 Plan

Summary:

- Add a minimal server-authoritative player death boundary now that enemies can damage players.
- First behavior: when player Health reaches 0, the player becomes dead and unable to perform gameplay actions.
- Do not implement rescue, revive, respawn, spectator/free-camera, run reset, floor fail UI, or permanent progression.

Confirmed design decisions:

- Player death v1 is direct death / cannot act. Rescue and revive are deferred.
- Party wipe v1 will be simple and deferred to a later checkpoint.
- Dead players must not be valid combat targets.
- Dead players should not perform movement, attack, dodge, interact, aim, or weapon switch gameplay actions.
- Later death camera behavior may become spectator or free-camera, but it is not part of v1.

Implementation scope:

1. Player death state:
   - Add a replicated player death state on `ATunicPlayerCharacter`, likely `bIsDead` with `OnRep_IsDead`, mirroring the enemy's minimal death path where appropriate.
   - Subscribe to the PlayerState-owned AttributeSet Health change path after player ASC initialization.
   - On the server, set player dead when Health reaches 0.
   - Add the existing `State.Dead` loose gameplay tag to the PlayerState-owned ASC when death is applied.

2. Combat target filtering:
   - Update `ATunicPlayerCharacter::IsCombatTargetAvailable()` to return false when dead.
   - Keep the player target ASC/AttributeSet routed through `ATunicPlayerState`; do not create a character-owned player ASC.
   - Confirm enemy melee and player light attack paths naturally skip dead players through the existing combat target interface checks.

3. Input and movement blocking:
   - Gate gameplay input handlers and server request handlers so dead players cannot start light attack, dodge, heavy attack, aim, interact, or weapon switch behavior.
   - Stop or disable character movement on death for the first pass.
   - Do not build spectator/free-camera input yet.

4. Presentation hooks:
   - Add `OnDeathStateChanged(bool bNewIsDead)` as a Blueprint-native presentation hook for player death.
   - Add optional `DefaultDeathMontage` on the player character if it matches the existing enemy presentation pattern without adding a new framework.
   - Keep death presentation cosmetic; it must not mutate Health or drive authority.

5. Logs/debug:
   - Add focused death logs similar to existing GAS debug logs.
   - Update player debug draw to show dead state if low-risk.

Documentation:

- Update `ARCHITECTURE.md` only if the implemented player death state becomes a stable fact.
- Update `README.md` current status after validation.
- Keep `tunicplan.md` as long-term direction only; rescue/revive, spectator/free-camera, and party wipe remain future TODO/recommendations there.

Validation plan:

- User compiles `LpQuestEditor Win64 Development`.
- Single-player PIE:
  - Enemy attacks player until Health reaches 0.
  - Player enters death state, movement/actions stop, death presentation/log triggers.
  - Enemy no longer treats dead player as an available combat target.
- Listen Server + 2 Players:
  - Enemy can kill one player; the dead player cannot act.
  - The living player remains controllable.
  - Dead player is no longer a valid target.
  - Player-to-player no-damage hit reactions still do not reduce Health.
- Regression:
  - Player light attack still damages enemies.
  - Enemy death path still works.
  - Enemy attack path still applies GameplayEffect damage to living players only.

Accepted deferrals:

- Party wipe resolution.
- Revive/rescue mechanics.
- Respawn points or run restart.
- Death camera / spectator / free-camera.
- Floor portal and reward flow.

Implementation checkpoint:

- Added minimal replicated player death state on `ATunicPlayerCharacter`.
- Player death is detected on the server through the PlayerState-owned Health attribute delegate after player ASC initialization.
- Death applies `State.Dead` to the PlayerState-owned ASC, stops movement, disables capsule collision, blocks player gameplay input/request paths, and marks the player unavailable through `ITunicCombatTargetInterface`.
- Added player `OnDeathStateChanged(bool)` Blueprint presentation hook and optional `DefaultDeathMontage`.
- Added player death logging and updated debug draw to label dead players.
- No rescue, revive, respawn, spectator/free-camera, party wipe, reward, or portal behavior was added.

Validation completed:

- User confirmed compile success.
- User confirmed PIE validation passed: enemies can kill players, player death state triggers, dead players cannot act, and dead players stop being valid enemy targets.
- Existing player/enemy attack paths stayed functional during validation.

Strict review:

- Normal review found no blocking client-side death path, direct Health mutation from input, player ASC ownership regression, or local-only combat authority shortcut.
- Player death detection is server-side through the PlayerState-owned Health attribute delegate, while replicated `bIsDead` drives client-side death presentation and movement/collision shutdown.
- Dead-player filtering is correctly centralized at `ITunicCombatTargetInterface::IsCombatTargetAvailable()`, so enemy auto attack and enemy melee hit windows skip dead players without per-enemy hard casts.
- Death input blocking is covered at both local input handlers and server request/execution gates for movement-facing, light attack, dodge, heavy attack, aim, interact, and weapon switch.
- Adversarial review can defend keeping v1 death state on `ATunicPlayerCharacter` because this checkpoint has no pawn swap, respawn, spectator camera, or revive system; the player ASC/AttributeSet still remain on `ATunicPlayerState`.
- Non-blocking risk: death currently disables the capsule and CharacterMovement permanently. Future revive/respawn must define a deliberate `SetDead(false)` restoration path instead of trying to reuse the v1 one-way death function.
- Non-blocking risk: death does not yet cancel all active GameplayAbilities or formal ongoing presentation states. This is acceptable because current attack abilities end immediately, but sustained aim/charge/channel abilities must add a death cancel policy when they are introduced.
- Non-blocking risk: death camera, party wipe, revive/rescue, respawn, floor fail UI, and run/floor reset remain explicitly deferred.

## Build Command

```powershell
& "D:\UE\UE_5.8\Engine\Build\BatchFiles\Build.bat" LpQuestEditor Win64 Development -Project="E:\UnRealEngine\LpQuest\LpQuest.uproject" -WaitMutex
```

## Current Risks

- Existing generated folders from the new project are local build products; do not treat them as source.
- `plan.md` and `tunicplan.md` are intentionally local-only and will not appear on GitHub unless this policy changes.
- KayKit assets are prototype-only; do not spend time polishing asset organization beyond what validation needs.
- The current enemy death state is intentionally minimal. It does not yet destroy actors, play montages, hide meshes, award loot, notify StateTree, or block all future non-debug damage paths.
- Default C++ enemy presentation now supports configured hit/death Montages plus Blueprint hooks. Final corpse handling, blend-out policy, cleanup, loot, and StateTree notification are still not implemented.
- GameplayTags are now registered through config. If the editor was already open before this change, restart the editor or refresh GameplayTags before assuming `State.Dead` is present at runtime.
- The light attack GameplayAbility now has minimal cost/cooldown/action-lock gates, but it is still not final GAS combat design because hit confirmation is still a capsule sweep bridge, hit reaction is presentation-only, and the old bootstrap RPC still exists for missing ASC/ability setup.
- Do not expand into real dodge invulnerability, dodge stamina cost, projectiles, or broader enemy behaviors until the light attack timing path is animation-driven enough to avoid building new systems on the immediate debug fallback.
- Later pause/settings UI should not rely on the current per-character debug setter semantics as the final design. Client-side UI toggles will not automatically control server-side logging; use a centralized debug/settings path when that UI is implemented.
- Before turning debug input requests into real gameplay, add server-side gates such as GAS ability activation, action locks, cooldowns, or rate limiting. Do not let reliable input RPCs become spam-prone combat commands.
- Do not keep copying the current request/RPC/log pattern for every action indefinitely. Before expanding to heavy attack, aim, interact, or weapon swap, decide whether GAS ability activation or a small shared action request layer should own the pattern.
- The current light attack sweep is intentionally a validation bridge. Replace or wrap it with animation notify timing and later real weapon/socket traces before treating it as final melee hit detection.
- The shared combat hit-window Notify only helps if the relevant animation or Montage fires on the server path. If the Montage is only played as local client presentation, the server-authoritative hit window will not run from the Notify.
- Attack-facing currently sends yaw from the owning client to the server when light attack starts. This is acceptable for the current co-op prototype, but later lock-on, aim modes, stun/action-lock states, and anti-spam validation should gate whether facing can change during specific actions.
- Enemy melee attack now has a prototype auto attack trigger, but it is not final AI. Keep it default-off except for validation and replace the decision layer with StateTree later.
- Player death v1 is validated, but it is intentionally one-way. Full respawn, revive, spectator/free-camera, party wipe, floor fail UI, portal, and reward policies remain deferred.
- Future sustained abilities or aim/charge states must define a death cancel policy so death cannot leave long-lived gameplay or presentation state active.

## Editor Tooltip Coverage v1

Summary:

- Added Chinese `ToolTip` metadata to current designer-facing C++ reflection APIs so Blueprint Details panels, Blueprint nodes, and native StateTree nodes are easier to understand during tuning.
- This stage intentionally uses Chinese metadata in headers. Class names, asset names, `StateTree`, `GAS`, `Blueprint`, `AIController`, `GameplayTag`, and other technical terms remain in English where clearer.
- No gameplay logic, default values, categories, asset data, or StateTree asset structure were changed.

Coverage:

- `ATunicEnemyPatrolRoute`: route id, loop/debug options, movement sample distance, stop sample distance, stop hold duration, NavMesh projection, runtime debug draw, and BlueprintPure route helpers.
- `ATunicEnemyAIController`: StateTree asset, attack range, Sight perception, aggro/forget delay, awareness policy, proximity/combat-spawn settings, patrol route binding, idle scan settings, target helpers, patrol helpers, and melee request helper.
- `TunicEnemyStateTreeTasks`: current-target, patrol-location, home-location, patrol-stop, advance-patrol, melee-request tasks, and target/route/range/stop conditions. `bInvert` explicitly warns that UE StateTree UI may not show `NOT` in the visible condition label.
- `ATunicEncounterSpawner`, `ATunicPortalActor`, `ATunicGameState`, `ATunicGameMode`, `ATunicPlayerController`, `ATunicPlayerState`, `UTunicRunStatusWidget`, `ATunicPlayerCharacter`, `ATunicEnemyCharacter`, `ATunicCharacterBase`, and `UTunicAnimNotifyState_CombatHitWindow`: added tooltip coverage for the common Blueprint tuning, presentation hook, debug, combat, dodge, XP, floor, portal, and run-status entries.

Validation status:

- Static metadata scan found no remaining common `EditAnywhere` / `EditDefaultsOnly` / `EditInstanceOnly` properties, Blueprint functions, or native StateTree node structs missing tooltip metadata in `Source/LpQuest/Public`.
- `git diff --check` found no whitespace errors or conflict markers. It only reported Git line-ending warnings.
- User still needs to compile `LpQuestEditor Win64 Development` because Chinese UHT metadata must be verified by the actual UE build.
- User confirmed compile success after the UENUM metadata syntax fix. The reusable error pattern was recorded to memory MCP instead of treating `plan.md` as the long-term error library.

Editor validation checklist:

- Open `BP_TunicEnemyPatrolRoute` and hover `Patrol Sample Distance`, `Patrol Stop Sample Distance`, `Draw Runtime Debug Route`, and NavMesh projection fields.
- Open enemy AIController blueprints and hover `AwarenessPolicy`, `SightRadius`, `PatrolRouteId`, `IdleScanYawRate`, and combat-spawn/proximity settings.
- Open `ST_EnemyMeleeBasic` and hover Tunic StateTree tasks/conditions. If UE does not show some native struct tooltips, record it as an Editor UI limitation rather than adding a custom editor plugin.
- In a Blueprint graph, search a few BlueprintCallable/Pure nodes such as run status, pending upgrade, portal, spawner, dodge, or melee helpers and confirm their tooltips are readable.

Strict review notes:

- Normal review: this is metadata-only and does not introduce runtime branches, client authority, new assets, or changed defaults.
- Adversarial review: direct Chinese metadata is acceptable for this early project because the goal is immediate designer/debug usability, not formal localization. If UHT or source encoding causes trouble, the fallback is English metadata plus Chinese explanation in documentation, not a localization framework.
- Remaining risk: StateTree Editor tooltip display is UE-version/UI dependent. The code can provide native metadata, but v1 does not build custom editor extensions to force every tooltip to appear.

## Enemy Search / Investigation v1

Commit:

- `8e43df7 [Feature] 增加敌人最后已知位置调查（Add Enemy Investigation）`

Summary:

- Added server-side last-known target investigation state to `ATunicEnemyAIController`.
- When an enemy loses a valid target past the existing aggro forget path, the AI can record the last known target location, move there, search briefly, then clear investigation state and return to `PatrolRoute` or `IdleAtHome`.
- Added native StateTree task / condition support for reading last-known location, reading investigation duration, clearing investigation state, and checking whether investigation data exists.
- Updated `ST_EnemyMeleeBasic` to use `Patrol -> Investigate / PatrolRoute / IdleAtHome`; `Investigate -> InvestigateMove / InvestigateSearch / InvestigateClear`.
- Kept StateTree as the intent-flow owner only. Combat damage, Health, XP, RunState, Spawner membership, and GameplayTags remain outside investigation tasks.

Validation and review:

- User confirmed compile and focused PIE validation passed.
- Strict review accepted the current `CombatSpawnAggro` / `SightAndProximity` proximity reacquire behavior as an intentional awareness-policy boundary, not an investigation bug.
- Remaining validation risk at the time was broader Listen Server + 2 Players regression coverage.

## KayKit Character Asset Organization

Commit:

- `69059bc [Chore] 整理 KayKit 角色素材路径（Organize KayKit Character Assets）`

Summary:

- Moved KayKit prototype character assets into clearer project-owned content folders.
- This was a simple content organization stage, not a gameplay architecture change.
- No combat, AI, GAS, RunState, Spawner, Portal, patrol, or dodge behavior was changed by this cleanup.

## Dodge Invulnerability v1.1

Commit:

- `e6ad79d [Feature] 增加 Dodge 无敌帧（Add Dodge Invulnerability）`

Summary:

- Added `UTunicDodgeInvulnerabilityGameplayEffect`.
- A successful server-side Dodge Ability commit grants a short `State.Invulnerable` window.
- Enemy melee damage and player light-attack debug damage check `State.Invulnerable` before applying damage GameplayEffects.
- Invulnerable targets can still be hit by sweeps, but damage and hit reaction are skipped.
- Added a temporary `Dodge Invulnerable!` debug message when the server blocks damage through the invulnerability tag.
- Raised and widened the prototype camera C++ defaults for better visibility.

Validation and review:

- User confirmed compile and focused gameplay validation passed.
- User confirmed Listen Server + 2 Players validation passed, with both windows seeing the blocked-hit debug message.
- Stage kept Stamina cost, perfect dodge, counter reward, cancel window, formal VFX/UI, and Montage Notify-driven invulnerability out of scope.

## Dodge Tuning v1.2

Commit:

- `6131abd [Feature] 整理 Dodge 调参入口（Tune Dodge Timing）`

Summary:

- Added `DodgeCooldownActionLockDuration` and `DodgeInvulnerabilityDuration` to `UTunicGameplayAbility_Dodge`.
- Cooldown/action-lock and invulnerability durations are now driven through GameplayEffect spec duration overrides instead of being locked to native GE constructor defaults.
- Added `DodgeMontagePlayRate` on `ATunicPlayerCharacter`.
- Kept `DodgeDistance`, `DodgeDuration`, and server RootMotionSource movement under player-character tuning.
- Dodge Montage remains presentation-only. It does not define invulnerability, movement distance, or combat result.
- `BP_PlayerCharacterBase.uasset` included user-confirmed Dodge tuning asset changes.

Validation and review:

- User confirmed compile, focused gameplay testing, and Listen Server + 2 Players testing passed.
- Strict review completed with no blocking findings.
- `Dodge Montage Window v1.3` was moved out of the near-term path and kept as a long-term recommendation only if the project later needs perfect dodge, counter timing, slow motion, or multiple dodge animations with different gameplay windows.

## Enemy Attack Telegraph v1

Commit:

- `4d463aa [Feature] 增加敌人攻击前摇提示（Add Enemy Attack Telegraph）`

Summary:

- Enemy melee attack now enters a server-controlled telegraph / windup before playing the attack Montage or fallback hit window.
- Default telegraph duration is `0.35s`.
- Debug telegraph visualization was changed from capsule-like spheres to a foot-centered forward fan, with `EnemyMeleeTelegraphDebugArcAngleDegrees` defaulting to `90deg`.
- Telegraph start stops current AI movement so the warning area and later hit window do not drift apart during windup.
- `OnEnemyMeleeTelegraphStarted` is a Blueprint presentation hook only. It does not cause damage.
- Real damage remains server-authoritative through hit window sweep, target filtering, `State.Invulnerable` check, and GameplayEffect application.
- Enemy death clears telegraph timers, hit-window state, and already-hit target tracking.
- The stage also fixed UE Unity Build anonymous-namespace helper collisions by giving player and enemy invulnerability helper functions unique names.

Validation and review:

- User confirmed Enemy Attack Telegraph v1 tested successfully after the foot-centered fan debug change.
- User confirmed `LpQuestEditor Win64 Development` compile success after the Unity Build helper rename fix.
- Strict review fixed a debug fan segment clamp issue and found no blocking authority or combat-boundary regression.

## Dodge Client Smoothing v1 (Superseded Before Commit)

Summary:

- A local presentation smoothing attempt was implemented but not committed as a completed stage.
- The attempt let owning clients locally pre-play `DodgeMontage`, skip duplicate multicast playback during a short local presentation window, and briefly enable camera lag.
- User testing showed the owning-client dash still visibly stutters. The likely remaining issue is network position correction from server-authoritative high-speed dash replication, not GPU/CPU frame time and not `State.Invulnerable`.
- This stage should not be treated as solved. The next serious direction is `Dodge Movement Prediction v2`: client-predicted movement with server confirmation, while server keeps final Dodge validity, invulnerability, cooldown/action-lock, Health, and damage authority.

## Dodge Movement Prediction v2

Commit:

- `44a4d9c [Feature] 增加 Dodge 客户端预测移动（Add Predicted Dodge Movement）`

Summary:

- Changed `UTunicGameplayAbility_Dodge` to `LocalPredicted`.
- Dodge input sends `GameplayEvent.Movement.Dodge` through the player ASC with a direction payload.
- Client and server use the same direction, `DodgeDistance`, and `DodgeDuration` to start `UAbilityTask_ApplyRootMotionConstantForce`.
- `State.Invulnerable` remains server-granted only. Damage, Health, cooldown/action-lock authority, and final correction remain server-authoritative.
- Removed the old default `ServerRequestDodge`, manual server-only `FRootMotionSource_ConstantForce` dash path, `DodgeRootMotionSourcePriority`, and v1 smoothing-only camera/presentation path so multiple Dodge movement systems do not coexist.
- `DodgeDistance` and `DodgeDuration` remain real `ATunicPlayerCharacter` tuning values consumed by the AbilityTask.

Validation and review:

- User confirmed compile and gameplay validation passed.
- User confirmed Listen Server + 2 Players validation passed and owning-client Dodge no longer visibly stutters.
- `p.NetShowCorrections 1` produced no visible correction draw during the successful test, which is acceptable because it only draws/logs when client movement corrections actually occur.
- Strict review found no blocking authority, double-movement, old-path, or Montage double-play regression.
- Accepted follow-up: `UAbilityTask_ApplyRootMotionConstantForce` uses engine-owned RootMotionSource priority. If ordinary movement input visibly fights dash later, consider `Custom Dodge SavedMove v3` instead of reintroducing a fake exposed priority field.

## Enemy Attack Shape Profile v1

Commit:

- `53f3e7f [Feature] 增加敌人扇形攻击判定（Add Enemy Fan Attack Shape）`

Summary:

- Enemy melee hit confirmation now uses a server explicit fan attack shape instead of the old fixed capsule sweep default.
- `ATunicEnemyCharacter` filters targets by `EnemyMeleeAttackRange`, `EnemyMeleeAttackAngleDegrees`, `EnemyMeleeAttackHalfHeight`, and `EnemyMeleeAttackOriginForwardOffset`.
- The hit path still reuses existing team rules, `State.Invulnerable`, hit reaction, and GameplayEffect damage application.
- `ATunicEnemyAIController::AttackActivationRange` default changed to `200cm`, slightly below the default `EnemyMeleeAttackRange=220cm`, so StateTree enters Attack inside the real hit shape instead of stopping just outside range.
- Orange telegraph fan and red hit-window fan are multicast debug presentation, so Listen Server clients can see both. Debug fan draw uses radial fill lines instead of only an empty outline.
- Old `EnemyMeleeSweepRadius`, `EnemyMeleeSweepHalfHeight`, `EnemyMeleeSweepStartOffset`, and `EnemyMeleeSweepEndOffset` remain only as legacy serialized fields. The current default enemy melee logic no longer reads them.

Validation and review:

- User confirmed focused testing passed.
- Strict review fixed the attack-height filter to check target bounds overlap instead of only checking actor origin Z.
- The stage kept weapon/socket trace, `BoxComponent OnOverlap` damage, Attack DataAsset, and player light-attack shape changes out of scope.

## Reward / Progression v1

Commit:

- `97a9881 [Feature] 增加固定升级奖励闭环（Add Fixed Run Upgrade Reward）`

Summary:

- Added the first real run-local reward loop: spawned encounter enemy deaths grant shared run XP, shared run Level grants each player a pending upgrade choice, and the local HUD selection request asks the server to apply a fixed upgrade GameplayEffect to that player's PlayerState-owned ASC.
- Added `UTunicRunUpgradeMaxHealthGameplayEffect` as the v1 fixed run upgrade. It is instant and applies `MaxHealth +20` followed by `Health +20`, so a full-health player can move from `100/100` to `120/120`.
- Added `ATunicGameMode::DefaultRunUpgradeGameplayEffectClass` and `ATunicGameMode::TrySelectRunUpgradeForPlayer()` as the server reward decision and application path.
- Updated `ATunicPlayerController` with `RequestSelectRunUpgrade()` / `ServerRequestSelectRunUpgrade()`. The old `RequestSelectRunUpgradeStub()` remains only as a compatibility wrapper.
- Updated `UTunicRunStatusWidget` to call the real upgrade request path.
- Kept `ATunicGameState` responsible for replicated shared XP / Level, and `ATunicPlayerState` responsible for each player's pending upgrade count and ASC.

Validation and review:

- User confirmed the focused upgrade path works.
- Strict review found no blocking issue. Pending choices are consumed only after the server validates the PlayerState, ASC, default GameplayEffect class, and spec creation.
- No client-side XP, pending choice, Health, MaxHealth, or reward authority was introduced.
- Accepted follow-up: manually placed enemies do not grant XP unless they are encounter members tracked by `ATunicEncounterSpawner`. `Placed Encounter Enemies v1` is the next small Spawner / reward follow-up.
- Remaining validation note: broader Listen Server + 2 Players reward regression was not separately claimed unless the user validates it in a later pass.

## Placed Encounter Enemies v1

Commit:

- `0755152 [Feature] 支持手摆 Encounter 成员（Add Placed Encounter Members）`

Summary:

- `ATunicEncounterSpawner` now supports explicit hand-placed encounter members through `PlacedEncounterEnemies`.
- Registered placed enemies count for current test encounter clear and grant XP through the existing `ATunicGameMode` reward path.
- Unregistered hand-placed enemies remain whiteboard actors: they do not grant XP and do not block or advance the current Portal v1 clear flow.
- Added `HasActiveEncounter()`, placed/total count helpers, and active placed-member snapshots while preserving the old spawned-enemy count meaning.
- `ResetEncounterForNextFloor()` still only destroys spawned enemies; placed enemies are not auto-destroyed, revived, or reset in v1.
- Documentation records that the current `EncounterCleared -> PortalActive` rule is a temporary test bridge. The long-term floor loop should move toward portal resource readiness, portal interaction, Boss kill, and charge pressure instead of "all registered enemies dead opens portal."

Validation and review:

- User confirmed focused validation passed.
- Strict review found no blocking issue.
- Accepted follow-up: future `Portal Event Foundation v1` should remove the registered-enemy kill gate from portal activation and make encounter membership a reward/test/Boss-minion grouping tool instead of the final floor-completion rule.

## Dead Enemy Targeting / Melee Hit Bounds Fix

Commit:

- `b6ad196 [Fix] 修复死敌锁定和近战命中判定（Fix Dead Enemy Targeting and Melee Hit Bounds）`

Summary:

- Dead enemies no longer reacquire players or rotate toward targets after death. `ATunicEnemyAIController` now stops or clears targeting/focus when the controlled enemy cannot run AI.
- Fixed the "visually inside enemy hit fan but no damage" case caused by player actor bounds including camera components.
- Enemy melee target bounds now use character capsule bounds for character targets, and collision-only actor bounds as the fallback for non-character targets.
- Enemy fan range and angle checks now allow target capsule radius edge overlap, so a player touching the edge of the fan can be treated consistently with the visible hit.
- The damage path remains server-authoritative through hit window query, target filtering, `State.Invulnerable` check, hit reaction, and GameplayEffect application.

Validation and review:

- User confirmed the fix passed focused testing.
- Strict review found no blocking issue.
- Accepted limitation: red fan debug draw still shows the base fan shape, not the extra target-radius tolerance envelope used by the hit query.

## Portal Event Foundation v1

Commit:

- `b8789e7 [Feature] 增加交互式传送门事件（Add Interactive Portal Event）`

Summary:

- Added `ITunicInteractableInterface` as the first shared server-confirmed interaction contract for Portal now and pickups later.
- `ATunicPlayerCharacter::Interact()` now keeps the local presentation hook and sends a server RPC. The server selects the nearest valid interactable actor in `InteractionRadius`.
- `ATunicPortalActor` implements the interactable interface, exposes `InteractionRadius`, and starts the Portal Event through `ATunicGameMode::TryStartPortalEvent()`.
- Added `ETunicRunState::PortalEventActive` at the end of the enum to avoid shifting older run-state values.
- Portal activation and charging now depend on `PortalEventActive`, not `EncounterCleared`.
- `EvaluateEncounterClear()` and `SpawnEncounterForCurrentFloor()` stay strict `CombatActive` only, so the temporary encounter-clear flow cannot overwrite an active Portal Event.
- `UTunicRunStatusWidget` now displays `PortalEventActive` instead of falling back to `Unknown`.
- Documentation records that encounter clear is now a test/reward state, while the long-term floor loop should use Portal interaction, Boss death, and charge pressure.

Validation and review:

- User confirmed Portal testing passed.
- Strict review found no blocking issue.
- Accepted limitation: v1 uses a world scan for interactable target selection. Switch to overlap registration or an interaction component when pickup/interactable density grows.
- Accepted limitation: `PortalEventActive` is global run state and assumes one active test portal. Add per-portal event ownership only when multiple portals can coexist.
- Accepted limitation: no final in-world prompt or screen message yet. HUD RunState and logs are enough for this stage.

## Boss Spawn On Portal v1

Commit:

- `aa85bb3 [Feature] 增加 Portal 测试 Boss 生成（Add Portal Test Boss Spawn）`

Summary:

- `ATunicPortalActor` can optionally spawn one configured existing `ATunicEnemyCharacter` subclass Blueprint as a test Boss when the Portal Event becomes active.
- The Portal exposes `PortalBossEnemyClass` and optional `PortalBossSpawnPoint` for authored test setup.
- If no Boss class is configured, the Portal keeps the previous direct-charge behavior.
- If a Boss class is configured, portal charging is blocked until the spawned Boss is dead, destroyed, or otherwise gone.
- Floor stub reset destroys any remaining Portal-spawned Boss and clears the Portal-owned weak reference.
- The Boss is not registered into `ATunicEncounterSpawner` and does not receive special XP, drops, UI, phase logic, or a dedicated Boss C++ class in this v1 stage.

Validation and review:

- User confirmed focused testing passed.
- Strict review found no blocking issue. Spawn is server-only, duplicate spawn is guarded, reset cleanup is explicit, and configured spawn failure blocks charging instead of silently skipping the Boss gate.
- `tunicplan.md` records deferred Boss C++ class, Elite class, Boss DataAsset, Boss UI, Boss phase, and Boss reward work as later adoption points rather than current scope.

## Portal Charge Pressure v1

Commit:

- `27670e0 [Feature] 完成 Portal 充能压力循环（Finish Portal Charge Pressure Loop）`

Summary:

- Added Portal-owned pressure spawning after the Portal Boss gate opens. The Portal now spawns ordinary pressure enemies during the charging phase instead of handing that job to `ATunicEncounterSpawner`.
- Pressure spawning uses the Portal's authored pressure enemy class, optional spawn-point list, spawn interval, max alive cap, and per-event XP budget.
- Leaving the charge radius pauses charging only; pressure spawning continues until the alive cap is reached or the Portal becomes ready.
- Portal ready and floor reset clean up remaining Portal-owned pressure enemies.
- `ATunicGameMode::HandleEnemyDeath()` still awards encounter XP first. Non-encounter enemies can now receive shared XP only if the active Portal owns them as pressure enemies and still has budget.
- The Portal pressure loop is intentionally small and Portal-local. It does not add a generic Director, DataAsset, or encounter membership reuse in v1.

Validation and review:

- User confirmed focused testing passed.
- User confirmed the Portal-spawned Boss still gives no XP; this is an accepted v1 limitation and should be handled by a separate Boss reward stage if needed.
- Strict review found no blocking issue. Pressure spawn is authority-only, waits for the Boss gate, respects the alive cap, does not enter encounter membership, and is cleaned on ready/reset.
- `Boss Reward v1` remains a separate future follow-up if Boss deaths should start granting XP/drop/reward.


