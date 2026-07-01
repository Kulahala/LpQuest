# ARCHITECTURE.md

This file stores stable architecture facts for LpQuest. Update it when implemented code changes the actual architecture.

## Project Overview

- Unreal Engine 5.8 C++ project.
- Runtime module: `LpQuest`.
- V1 targets listen-server co-op PvE, starting with two players.
- Target style: lowpoly, fixed isometric, compact action combat.
- Long-term roadmap: `tunicplan.md`.
- Active execution plan: `plan.md`.

## Module Strategy

Start with one runtime module, `LpQuest`. Do not split modules until a concrete boundary exists.

Current naming surface uses `LPQ` for Blueprint categories, runtime debug CVars, editor-facing node/display names, C++ reflected type names, source file names, generated header includes, Blueprint-facing API names, Blueprint asset names, and Config Blueprint paths. Old `Tunic` native names remain only as CoreRedirect compatibility for migrated content and historical notes.

Current runtime dependencies:

- `Core`
- `CoreUObject`
- `Engine`
- `InputCore`
- `EnhancedInput`
- `GameplayAbilities`
- `GameplayTags`
- `GameplayTasks`
- `AIModule`
- `StateTreeModule`
- `GameplayStateTreeModule`
- `UMG`

Optional later runtime module dependencies:

- `MotionWarping` for selected melee/Boss correction. The plugin may be enabled during prototype setup, but the runtime module should not add a `MotionWarping` dependency until gameplay code actually uses it.
- `Niagara` for elemental VFX.

Avoid early dependencies on PCG, CommonUI, DLSS, Streamline, Reflex, and release/performance tooling until the project actually needs them.

## System Boundaries

### Character

`ALPQCharacterBase` provides shared `ACharacter` movement plus Ability System Component and AttributeSet accessors. It does not create ASC/AttributeSet by default; ownership is supplied by PlayerState or enemy subclasses.

`ALPQPlayerCharacter` owns local presentation and input: fixed isometric `SpringArm + Camera`, Enhanced Input bindings, fixed-view movement, attack-facing intent, and Blueprint-facing request hooks for dodge, attacks, aim, interact, and weapon switching. The fixed camera disables SpringArm collision retraction so nearby enemies do not shrink the view. The unified interact input is `IA_Interact` and should be mapped to `E` in `IMC_Player`; the local character fires a presentation hook and sends a server RPC, then the server selects the nearest valid `ILPQInteractableInterface` target in range. It initializes player GAS from `ALPQPlayerState` in `PossessedBy` and `OnRep_PlayerState`, using PlayerState as OwnerActor and Character as AvatarActor.

Player death is a minimal replicated character state layered over PlayerState-owned GAS. `ALPQPlayerCharacter` listens to the PlayerState-owned Health attribute on the server, replicates `bIsDead`, stops movement, disables capsule collision, blocks gameplay input/request paths, plays optional death presentation, calls a Blueprint death-state hook, and adds `State.Dead` to the PlayerState-owned ASC. The player Character owns the current death presentation/input boundary; the player ASC and AttributeSet remain on `ALPQPlayerState`.

Player movement remains fixed-view relative to the camera screen axes: WASD maps to the fixed camera's projected screen up/down/left/right directions on the ground plane. Movement does not depend on current character facing. Locally controlled players continuously face the current mouse cursor direction and send throttled yaw updates to the server; light attack input forces one immediate yaw update before activation so server-authoritative melee sweeps use the same attack direction.

Current dodge input sends `GameplayEvent.Movement.Dodge` through the PlayerState-owned player ASC with a target-data direction payload. `ULPQGameplayAbility_Dodge` is tagged `Ability.Movement.Dodge` and uses `LocalPredicted` execution so the owning client and server activate the same Ability from the same direction event. The Ability applies the native cooldown/action-lock GameplayEffect with explicit `Cooldown.Dodge` / `State.ActionLocked` duration, applies the separate `State.Invulnerable` GameplayEffect only on server authority, and starts `UAbilityTask_ApplyRootMotionConstantForce` for the short dash using the player character's `DodgeDistance` and `DodgeDuration`. The player character owns movement tuning, input-direction grace, Dodge Montage presentation, montage play rate, and multicast presentation routing; the Ability owns cooldown/action-lock and invulnerability durations. The configured Dodge Montage remains presentation only and does not decide movement distance, invulnerability timing, or final combat results. Dodge does not consume Stamina, apply gameplay rewards, rely on animation Root Motion, or use Montage NotifyState windows for invulnerability.

Current light attack input activates a minimal `UGameplayAbility` tagged `Ability.Attack.Sword.Light` through the player ASC. The ability executes on the server, commits its native light-attack Stamina cost GameplayEffect, applies its native cooldown/action-lock GameplayEffect, logs the accepted request, and then delegates to the player character's light-attack execution path. If a light attack Montage is configured on the player character, the server multicasts Montage playback and expects `ULPQAnimNotifyState_CombatHitWindow` on that Montage to drive the server-side hit window through `ILPQCombatHitWindowSourceInterface`. If no Montage is configured, the immediate hit-window path remains as a player light-attack bootstrap fallback. The hit window performs a short capsule sweep in front of the character, logs sweep results, routes hit actors through `ILPQCombatTargetInterface`, and uses `FLPQCombatRules` to separate hit-reaction permission from damage permission. Enemy targets can receive the current light-attack GameplayEffect damage path when damage application is enabled. Friendly player targets can receive server-confirmed hit-reaction presentation without damage. If the player ASC and light attack ability class are available, failed ability activation does not fall back to the old RPC path; this keeps cooldown and cost gates authoritative. The current damage path checks `State.Invulnerable` before applying damage GameplayEffects. Final WeaponActor traces and broader damage-immunity policy are not implemented yet.

`ALPQEnemyCharacter` owns its own ASC and AttributeSet because enemies do not have PlayerStates. It initializes GAS with itself as OwnerActor and AvatarActor. It grants a minimal server-only enemy melee attack GameplayAbility to its self-owned ASC. The current enemy melee path is requested by AI/StateTree through the enemy melee GameplayAbility. A successful enemy melee Ability starts a server-controlled telegraph/windup window, multicasts a Blueprint presentation hook plus optional foot-centered fan debug draw from the configured attack shape, then plays the configured attack Montage. Enemy melee damage requires `ULPQAnimNotifyState_CombatHitWindow` on that Montage to run server-side hit confirmation through `ILPQCombatHitWindowSourceInterface`; a missing Montage or missing hit-window Notify logs a warning and does not apply fallback damage. The enemy hit window uses a foot/forward anchored fan-shaped attack profile, filters targets by range, angle, and height, and then applies the existing combat target rules and GameplayEffect damage path. The telegraph does not apply damage. It currently has a minimal replicated death state: when server Health reaches 0, the enemy replicates `bIsDead`, cancels active telegraph/hit-window state, disables capsule collision and character movement, plays configured default death presentation, calls a Blueprint death-state hook, adds the registered `State.Dead` loose gameplay tag, and asks `ALPQGameMode` to handle enemy death. Enemies expose an XP reward value and optional dropped pickup class for GameMode reward routing, but they do not directly grant XP, spawn drops, or mutate run progression state. Non-lethal confirmed hits can call a multicast hit-reaction hook that plays configured default hit presentation and then calls a Blueprint presentation hook; this is presentation-only and does not own damage, stun, knockback, or AI state changes.

`ILPQCombatTargetInterface` is the narrow target routing contract for server-confirmed combat hits. It exposes target availability, target team, target ASC/AttributeSet access, and confirmed-hit presentation routing. It does not apply damage or mutate Health directly. `ALPQEnemyCharacter` implements it as an enemy target. `ALPQPlayerCharacter` implements it as a player target using its PlayerState-owned ASC/AttributeSet and returns unavailable when dead. Same-team non-neutral hits can trigger hit reaction presentation without damage; cross-team Player/Enemy hits can apply damage. Team and friendly-fire decisions go through `FLPQCombatRules`, not per-character ad hoc checks.

Current enemy GAS validation is debug-only: enemy initialization logs ASC, OwnerActor, AvatarActor, AttributeSet, attributes, and network role before any damage, targeting, or AI behavior is layered on top.

### Player Framework

`ALPQPlayerController` owns local Enhanced Input Mapping Context installation and enables the visible mouse cursor used by attack-facing validation. It exposes the default mapping context and priority for later Blueprint or asset configuration without hard-coded asset paths. It also creates the local run-status HUD widget for each locally controlled player and forwards run-upgrade selection requests to the server; it does not own progression state or grant rewards locally.

`ALPQPlayerState` owns persistent player GAS state: player ASC and replicated AttributeSet. It also owns the current per-player run-upgrade pending-choice count and the server-authoritative consume operation. This is replicated personal run-local progression state; concrete upgrade effects are granted to the PlayerState-owned ASC by `ALPQGameMode`. It also replicates the current minimal equipment marker, `CurrentEquipmentId`, which v1 pickup interactions set as a single `FName` rather than a full inventory or equipment-slot system.

`ALPQGameMode` sets the default Pawn to `ALPQPlayerCharacter`, PlayerController to `ALPQPlayerController`, PlayerStateClass to `ALPQPlayerState`, and GameStateClass to `ALPQGameState`.

`ALPQGameMode` owns the current server-authoritative run-state and run-reward decision point. Party Wipe v1 is evaluated on the server by scanning the current `GameState->PlayerArray` for controlled `ALPQPlayerCharacter` pawns and checking their replicated character death state. Enemy death is routed through GameMode; every enemy that enters `HandleEnemyDeath()` awards shared XP from its own `ExperienceReward`, and `ExperienceReward=0` is the supported no-XP configuration. Ordinary enemy death does not change RunState, so remaining enemies can continue AI in `CombatActive`. After XP routing, GameMode can spawn one configured `ALPQPickupActor` subclass from the dead enemy's transform when that enemy has `DroppedPickupClass` set. If awarded shared XP increases the shared run level, GameMode grants each current player a personal pending upgrade choice on their `ALPQPlayerState`. When a player selects a run upgrade, GameMode validates the pending choice and applies the configured default upgrade GameplayEffect to that player's PlayerState-owned ASC. Portal Event Foundation v1 starts through `ALPQGameMode::TryStartPortalEvent()` after a server-validated player interaction with a Combat Event Portal, then transitions the run to `PortalEventActive`. GameMode also stores the selected Combat Event Portal as a server-only active event owner, so only the interacted Portal can respond to the global `PortalEventActive` state; other branch portals cannot take over that event. Direct Floor Exit Portals instead call GameMode's direct floor-exit path from `CombatActive`, require all living players in the Portal radius, and do not enter `PortalEventActive`. Portal completion and direct floor exit both mark `FloorTransitionReady` with a destination ID. Floor Transition Stub v1 then delays briefly, advances the replicated floor index, writes the replicated floor destination ID, resets placed portals and floor-wave spawn sources, clears the selected Portal owner, returns the run state to `CombatActive`, and asks the spawn sources to create the next fixed wave. Characters expose their own death state, but they do not decide whole-party failure, portal event start, floor-transition readiness, floor advancement, enemy spawning, or reward application.

`ALPQGameState` owns the replicated run state surface through `ELPQRunState`, the current run-local floor index, the current floor destination ID, the shared run XP pool, and the derived shared run level. The implemented states are `CombatActive`, `PartyWiped`, `FloorTransitionReady`, and `PortalEventActive`. `ALPQGameState` replicates the state, floor index, floor destination ID, shared XP, and shared run level to clients and exposes Blueprint presentation hooks for state, floor, destination, XP, and level changes; it does not decide when the party is wiped, the portal event starts, the portal is ready, the floor advances, or rewards are granted. `IsCombatActive()` means ordinary combat systems may still run and currently returns true for both `CombatActive` and `PortalEventActive`. The current shared run level grants per-player pending upgrade choices through GameMode, but GameState does not apply the resulting upgrade effects.

`ULPQRunStatusWidget` is the current minimal HUD surface for run-loop validation. It is created locally by `ALPQPlayerController`, binds to `ALPQGameState` and the owning `ALPQPlayerState` presentation events, displays floor index with destination ID, run state, shared run XP, shared run level, and the local player's pending upgrade choices, and exposes a button that requests server run-upgrade selection through the owning PlayerController. It does not directly mutate XP, run state, floor index, floor destination, level, pending choices, rewards, or combat state.

`ALPQPortalActor` is the current placed Portal actor and the first `ILPQInteractableInterface` implementation. It separates completion mode from destination ID. In `CombatEvent` mode, a living player in `InteractionRadius` presses the unified interact key, the server asks GameMode to start the Portal Event, and the replicated run state becomes `PortalEventActive`; this is the default for existing portals. The interacted Combat Event Portal becomes the active event owner for that event, and only that owner Portal can activate, spawn its Boss, run pressure spawning, charge, and pass its `PortalDestinationId` into floor transition. Non-owner Combat Event portals reject interaction during the active event and ignore the global `PortalEventActive` tick. The Portal can optionally spawn one configured `ALPQEnemyCharacter` subclass as a test Boss from a configured Actor transform; if a Boss class is configured, portal charging is blocked until that spawned Boss is dead or gone. After the Boss gate is open, the Portal can spawn Portal-owned pressure enemies from configured Actor transforms at a configured interval until the Portal becomes ready. Leaving the charge radius pauses charging but does not pause pressure spawning. Portal-spawned Boss and pressure enemies use the same enemy death reward rule as any other enemy: their own `ExperienceReward` decides shared XP, and `ExperienceReward=0` makes them no-XP enemies. While charging is allowed, the server counts living players in the configured charge radius and accumulates replicated charge progress while all living players are present. Dead players do not block portal charging. When charging completes, the portal asks `ALPQGameMode` to mark floor transition ready with its `PortalDestinationId` and cleans up any remaining Portal-owned pressure enemies. In `DirectFloorExit` mode, player interaction stays in `CombatActive`, requires all living players in the Portal radius, and directly asks GameMode to start the same floor-transition stub with the Portal destination ID; Direct Floor Exit cannot bypass an already selected Combat Event. `PortalDestinationId=None` is a rejected configuration. During the current transition stub, GameMode can reset placed portals for the next floor placeholder loop and the Portal cleans up any remaining spawned Boss or pressure enemies. The portal does not load maps, directly mutate `ALPQGameState`, or require a dedicated Boss C++ class.

`ALPQPickupActor` is the first minimal world pickup implementation. It implements `ILPQInteractableInterface`, exposes a `PickupId` and display name for authored Blueprint setup, and uses the same server-validated `E` interaction path as the Portal. Pickup actors can be placed in the world or spawned by GameMode from an enemy's optional `DroppedPickupClass`. On a valid server interaction from a living player, it writes `PickupId` to that player's `ALPQPlayerState::CurrentEquipmentId` and destroys itself. `PickupId=None` rejects interaction with a warning instead of pretending the pickup succeeded. The pickup actor does not grant abilities, modify attributes, own inventory, change attack data, or decide enemy drop routing.

`ALPQFloorWaveEnemySpawnSource` is the current ordinary floor-wave spawn actor. It is server-owned and combines authored spawn location, enemy class, count, optional random radius, optional NavMesh projection, generated-enemy tracking, and generated-enemy cleanup in one placed Actor. `SpawnRadius=0` uses the source Actor's exact transform without random offset or NavMesh projection; positive radius samples a random XY point and can project it to NavMesh before spawning. GameMode scans floor-wave spawn sources while the run is `CombatActive`, asks them to spawn for the current floor, and resets them during the floor-transition stub. Spawn sources do not decide RunState, Portal events, rewards, AI behavior, or map loading.

`GameMode` remains server-only. `PlayerController` owns local input setup. `PlayerState` owns replicated player gameplay state.

### Ability System

GAS owns abilities, cooldowns, damage effects, elemental states, action locks, invulnerability, stun, and death tags. Combat results should be server-authoritative.

`ULPQAbilitySystemComponent` is the project ASC subclass. Player ASC instances use `Mixed` replication mode on `ALPQPlayerState`; enemy ASC instances use `Minimal` replication mode on `ALPQEnemyCharacter`.

`ULPQAttributeSet` defines replicated `Health`, `MaxHealth`, `Stamina`, `MaxStamina`, and `ElementalPower` attributes with `OnRep_*` notification hooks. It clamps Health and Stamina against their max values, keeps MaxHealth and MaxStamina at least 1, clamps ElementalPower to non-negative values, and re-clamps current Health/Stamina after max-value GameplayEffect changes.

`ULPQGameplayAbility_LightAttack` is the first minimal player GameplayAbility. It is server-only, granted by `ALPQPlayerCharacter` during player ASC initialization, owns a native Stamina cost GameplayEffect and a native short cooldown/action-lock GameplayEffect, blocks activation while `Cooldown.Attack`, `State.ActionLocked`, or `State.Dead` is present, and delegates to the player character's server-side light attack execution path. The player character currently owns Montage playback configuration and the hit-window implementation. Enemy hit reactions are a server-confirmed presentation hook; final WeaponActor traces are not implemented yet.

`ULPQGameplayAbility_Dodge` is the first minimal player movement GameplayAbility. It is `LocalPredicted`, granted by `ALPQPlayerCharacter` during player ASC initialization, uses a native dodge cooldown/action-lock GameplayEffect, applies a separate native dodge invulnerability GameplayEffect on server authority, blocks activation while `Cooldown.Dodge`, `State.ActionLocked`, or `State.Dead` is present, and starts a predicted GAS root-motion task for movement. The ability owns cooldown/action-lock and invulnerability durations as editable gameplay timing values; the player character owns distance, movement duration, input-direction grace, and Montage play-rate tuning. The ability does not mutate attributes directly and the client-predicted movement does not grant client-side damage or Health authority.

`ULPQGameplayAbility_EnemyMeleeAttack` is the first minimal enemy attack GameplayAbility. It is server-only, granted by `ALPQEnemyCharacter` during enemy ASC initialization, uses a native enemy melee cooldown/action-lock GameplayEffect with the shared attack lock tags, and delegates concrete execution to the enemy character's server-side melee telegraph and attack path. Successful activations end as normal Ability completions, not cancellations. The ability does not directly trace, mutate attributes, or own AI intent.

`ULPQStaminaRegenGameplayEffect` is a minimal server-applied infinite periodic GameplayEffect on the player ASC. It owns the current baseline Stamina regeneration tuning. It does not yet include combat-delay, exhaustion, or tag-gated regen blocking rules.

`ULPQRunUpgradeMaxHealthGameplayEffect` is the first fixed run-upgrade GameplayEffect. It is applied by `ALPQGameMode` to the selected player's PlayerState-owned ASC after consuming one pending run-upgrade choice. The effect is instant and adds to `MaxHealth` before adding to `Health`, so the v1 upgrade both increases the cap and immediately fills the same amount. Future run-upgrade choices can replace the configured GameMode effect class or move to DataAssets without moving reward authority to clients.

Temporary debug draw on player and enemy characters renders replicated Health/Stamina values above actors for PIE validation. Runtime debug drawing and development logs are gated by lightweight `LPQ.Debug.*` Console Variables plus each system's local debug bool; each gameplay system still owns its own debug implementation. This is a world debug aid, not the final HUD or settings UI path.

Movement, camera, simple interaction, and editor-only setup should not be forced into GameplayAbilities. Player input should activate abilities or validated server requests; input handlers must not directly apply damage, death, or elemental states.

### Combat

Weapon hit windows should be animation-notify driven. Weapon actors may detect hits, but final damage and elemental state application should flow through GAS.

`ULPQAnimNotifyState_CombatHitWindow` is the shared C++ bridge from animation timing to server-authoritative attack hit-window functions. The notify state does not apply damage directly; it casts the mesh owner to `ILPQCombatHitWindowSourceInterface` and forwards Begin/Tick/End timing. `ALPQPlayerCharacter` maps that interface to its light-attack hit window, while `ALPQEnemyCharacter` maps it to its melee hit window. Target filtering, team rules, GameplayEffect damage, and hit reaction routing remain in the server-side character path.

For multiplayer PvE, authoritative hit confirmation, projectile spawning, damage GameplayEffects, death, and elemental state changes happen on the server. Clients may play presentation but should not own final combat results.

### AI

Enemy AI should use StateTree from the start. AI and StateTree execution should be server-authoritative. Clients observe replicated movement, combat state, montage/cue presentation, and death state.

`ALPQEnemyAIController` is the first enemy AI controller. It owns a `UStateTreeAIComponent` brain that is started only on the server when a valid enemy pawn and StateTree asset are present. It also owns `UAIPerceptionComponent` sight awareness, the configured enemy awareness policy, current target storage, last-known target investigation state, proximity aggro checks, combat-spawn delayed aggro, idle scan rotation, aggro forget timing, patrol-route state, range checks, focus, stopping movement, and requesting the current target melee attack. Target acquisition filters valid player targets through `ILPQCombatTargetInterface::IsCombatTargetAvailable()`. StateTree assets should consume the AI-controller-owned current target and last-known investigation state rather than running their own nearest-player scan task.

`ALPQEnemyPatrolRoute` is the placed patrol authoring actor. It owns a `USplineComponent` so designers can draw a visible route in the level. Spline control points shape the route, movement samples provide the AI's patrol locations, and sparse stop samples mark where the StateTree may wait. Enemy AI controllers bind to the route through `PatrolRouteId`, read the current sampled patrol location, query whether that point is a stop, and advance the route index. Runtime patrol target locations are projected to navigation when possible so accidentally low or high spline point Z values do not send AI below or above the walkable surface. The route actor can optionally draw the route curve, movement samples, and stop samples during PIE validation. Patrol routes do not own encounter membership, rewards, spawn logic, combat results, or replicated gameplay state.

`ST_EnemyMeleeBasic` uses `Patrol` as the no-current-target parent state. That parent state should remain an intent router rather than owning route movement tasks directly. Its investigation branch should consume the AI-controller-owned last-known target location, move there, search briefly, then clear that investigation state before returning to patrol or idle. Its route branch should move through sampled patrol locations continuously and only wait when `LPQ Enemy Current Patrol Target Is Stop` is true. The current stop duration is read from the route through the AI controller and can be bound into UE's built-in Delay task. `IdleAtHome` remains the fallback for enemies without a route or investigation target. `Chase` consumes the AI-controller-owned current target and moves to that actor. `Attack` requests the existing enemy melee GameplayAbility and does not own hit confirmation or damage. StateTree states that combine instant LPQ tasks with `Move To` or `Delay` use Tasks completion mode `All` so instant tasks do not prematurely complete movement, investigation, or recovery states.

Current `ST_EnemyMeleeBasic` handoff topology: `Patrol` is the no-current-target router. Under `Patrol`, `Investigate` should be ordered before `PatrolRoute` and `IdleAtHome`; it contains `InvestigateMove` (`Get LPQ Enemy Last Known Target Location` -> `Move To Destination=LastKnownTargetLocation`), `InvestigateSearch` (`Get LPQ Enemy Investigation Duration` -> built-in `Delay.Duration`), and `InvestigateClear` (`Clear LPQ Enemy Last Known Target Location`). `PatrolRoute` uses enter condition `LPQ Enemy Has Patrol Route` and contains `PatrolMove`, `PatrolStop`, and `PatrolAdvance`: `PatrolMove` gets current patrol location and moves to it; on success it transitions to `PatrolStop` when `LPQ Enemy Current Patrol Target Is Stop` is true, otherwise to `PatrolAdvance` with that condition inverted through `bInvert`; `PatrolStop` delays for the route-provided hold duration, then goes to `PatrolAdvance`; `PatrolAdvance` advances the AI-controller-owned patrol index, then returns to `PatrolMove`. `IdleAtHome` is the no-route fallback and should use the inverted `LPQ Enemy Has Patrol Route` condition when needed. Investigation, patrol move/stop/advance, chase, and attack states that pair instant LPQ tasks with `Move To` or `Delay` should use Tasks completion mode `All`. UE's StateTree UI may not show `NOT` on custom inverted conditions; check condition instance data and trust `bInvert`.

StateTree decides intent: idle, patrol, investigate, chase, combat, attack, recover, hit react, dead. GameplayAbilities execute concrete actions. Native LPQ StateTree nodes may read the current target selected by the AI controller, read and clear last-known investigation state, read patrol locations, advance patrol targets, check attack range through the AI controller, and ask the enemy AI controller to request melee attack activation. They must not directly mutate Health, RunState, XP, GameplayTags, or final combat state.

### Assets

KayKit assets are prototype-only. Keep imported external assets under `Content/_External/KayKit`; game-owned assets live under `Content/_Game`.

Current asset directory skeleton:

- `Content/_Game/Characters/Player`
- `Content/_Game/Characters/Enemies`
- `Content/_Game/Weapons`
- `Content/_Game/Abilities`
- `Content/_Game/AI/StateTrees`
- `Content/_Game/Data`
- `Content/_Game/Maps`
- `Content/_External/KayKit`

## Gameplay Tags

The initial gameplay tag dictionary is registered through `Config/DefaultGameplayTags.ini`.

- `Element.Fire`
- `Element.Ice`
- `Element.Lightning`
- `State.Burning`
- `State.Chilled`
- `State.Frozen`
- `State.Shocked`
- `State.Invulnerable`
- `State.Stunned`
- `State.Dead`
- `State.ActionLocked`
- `Ability.Attack.Enemy.Melee`
- `Ability.Attack.Sword.Light`
- `Ability.Attack.Sword.Heavy`
- `Ability.Attack.Bow.Shoot`
- `Ability.Spell.Cast`
- `Cooldown.Attack`
- `Cooldown.Dodge`
- `Cooldown.WeaponSwap`
- `Cooldown.Spell`
- `GameplayEvent.Movement.Dodge`
- `Enemy.Type.Melee`
- `Enemy.Type.Ranged`
- `Enemy.Type.Mage`
- `Enemy.Type.Boss`

## Non-Goals For V1

- No matchmaking, lobby browser, dedicated server deployment, rollback netcode, or large-scale multiplayer.
- No full open-world simulation.
- No direct copy of old giant player/enemy architecture.
- No full puzzle-system recreation from Tunic.
- No final fox protagonist requirement before the combat prototype works.
