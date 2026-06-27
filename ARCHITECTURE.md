# ARCHITECTURE.md

This file stores stable architecture facts for LpQuest. Update it when implemented code changes the actual architecture.

## Project Overview

- Unreal Engine 5.8 C++ project.
- Runtime module: `LpQuest`.
- V1 targets listen-server co-op PvE, starting with two players.
- Target style: lowpoly, fixed isometric, Tunic-like action combat.
- Long-term roadmap: `tunicplan.md`.
- Active execution plan: `plan.md`.

## Module Strategy

Start with one runtime module, `LpQuest`. Do not split modules until a concrete boundary exists.

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

`ATunicCharacterBase` provides shared `ACharacter` movement plus Ability System Component and AttributeSet accessors. It does not create ASC/AttributeSet by default; ownership is supplied by PlayerState or enemy subclasses.

`ATunicPlayerCharacter` owns local presentation and input: fixed isometric `SpringArm + Camera`, Enhanced Input bindings, fixed-view movement, attack-facing intent, and Blueprint-facing request hooks for dodge, attacks, aim, interact, and weapon switching. The fixed camera disables SpringArm collision retraction so nearby enemies do not shrink the view. It initializes player GAS from `ATunicPlayerState` in `PossessedBy` and `OnRep_PlayerState`, using PlayerState as OwnerActor and Character as AvatarActor.

Player death is a minimal replicated character state layered over PlayerState-owned GAS. `ATunicPlayerCharacter` listens to the PlayerState-owned Health attribute on the server, replicates `bIsDead`, stops movement, disables capsule collision, blocks gameplay input/request paths, plays optional death presentation, calls a Blueprint death-state hook, and adds `State.Dead` to the PlayerState-owned ASC. The player Character owns the current death presentation/input boundary; the player ASC and AttributeSet remain on `ATunicPlayerState`.

Player movement remains fixed-view relative to the camera screen axes: WASD maps to the fixed camera's projected screen up/down/left/right directions on the ground plane. Movement does not depend on current character facing. Locally controlled players continuously face the current mouse cursor direction and send throttled yaw updates to the server; light attack input forces one immediate yaw update before activation so server-authoritative melee sweeps use the same attack direction.

Current dodge input sends `GameplayEvent.Movement.Dodge` through the PlayerState-owned player ASC with a target-data direction payload. `UTunicGameplayAbility_Dodge` is tagged `Ability.Movement.Dodge` and uses `LocalPredicted` execution so the owning client and server activate the same Ability from the same direction event. The Ability applies the native cooldown/action-lock GameplayEffect with explicit `Cooldown.Dodge` / `State.ActionLocked` duration, applies the separate `State.Invulnerable` GameplayEffect only on server authority, and starts `UAbilityTask_ApplyRootMotionConstantForce` for the short dash using the player character's `DodgeDistance` and `DodgeDuration`. The player character owns movement tuning, input-direction grace, Dodge Montage presentation, montage play rate, and multicast presentation routing; the Ability owns cooldown/action-lock and invulnerability durations. The configured Dodge Montage remains presentation only and does not decide movement distance, invulnerability timing, or final combat results. Dodge does not consume Stamina, apply gameplay rewards, rely on animation Root Motion, or use Montage NotifyState windows for invulnerability.

Current light attack input activates a minimal `UGameplayAbility` tagged `Ability.Attack.Sword.Light` through the player ASC. The ability executes on the server, commits its native light-attack Stamina cost GameplayEffect, applies its native cooldown/action-lock GameplayEffect, logs the accepted request, and then delegates to the player character's light-attack execution path. If a light attack Montage is configured on the player character, the server multicasts Montage playback and expects `UTunicAnimNotifyState_CombatHitWindow` on that Montage to drive the server-side hit window through `ITunicCombatHitWindowSourceInterface`. If no Montage is configured, the old immediate hit-window path remains as a bootstrap fallback. The hit window performs a short capsule sweep in front of the character, logs sweep results, routes hit actors through `ITunicCombatTargetInterface`, and uses `FTunicCombatRules` to separate hit-reaction permission from damage permission. Enemy targets can receive the current minimal instant GameplayEffect damage path when debug damage is enabled. Friendly player targets can receive server-confirmed hit-reaction presentation without damage. If the player ASC and light attack ability class are available, failed ability activation does not fall back to the old RPC path; this keeps cooldown and cost gates authoritative. The current damage path checks `State.Invulnerable` before applying damage GameplayEffects. Final WeaponActor traces and broader damage-immunity policy are not implemented yet.

`ATunicEnemyCharacter` owns its own ASC and AttributeSet because enemies do not have PlayerStates. It initializes GAS with itself as OwnerActor and AvatarActor. It grants a minimal server-only enemy melee attack GameplayAbility to its self-owned ASC. The current enemy melee path can be triggered by AI/StateTree or the prototype server-only auto attack switch. A successful enemy melee Ability starts a server-controlled telegraph/windup window, multicasts a Blueprint presentation hook plus optional foot-centered fan debug draw from the configured attack shape, then plays the configured attack Montage or the bootstrap fallback hit window. `UTunicAnimNotifyState_CombatHitWindow` or the fallback window runs server-side hit confirmation through `ITunicCombatHitWindowSourceInterface`; the enemy hit window uses a foot/forward anchored fan-shaped attack profile, filters targets by range, angle, and height, and then applies the existing combat target rules and GameplayEffect damage path. The telegraph does not apply damage. It currently has a minimal replicated death state: when server Health reaches 0, the enemy replicates `bIsDead`, cancels active telegraph/hit-window state, disables capsule collision and character movement, plays configured default death presentation, calls a Blueprint death-state hook, adds the registered `State.Dead` loose gameplay tag, and asks `ATunicGameMode` to handle enemy death. Enemies expose an XP reward value for GameMode reward routing, but they do not directly grant XP or mutate run progression state. Non-lethal confirmed hits can call a multicast hit-reaction hook that plays configured default hit presentation and then calls a Blueprint presentation hook; this is presentation-only and does not own damage, stun, knockback, or AI state changes.

`ITunicCombatTargetInterface` is the narrow target routing contract for server-confirmed combat hits. It exposes target availability, target team, target ASC/AttributeSet access, and confirmed-hit presentation routing. It does not apply damage or mutate Health directly. `ATunicEnemyCharacter` implements it as an enemy target. `ATunicPlayerCharacter` implements it as a player target using its PlayerState-owned ASC/AttributeSet and returns unavailable when dead. Same-team non-neutral hits can trigger hit reaction presentation without damage; cross-team Player/Enemy hits can apply damage. Team and friendly-fire decisions go through `FTunicCombatRules`, not per-character ad hoc checks.

Current enemy GAS validation is debug-only: enemy initialization logs ASC, OwnerActor, AvatarActor, AttributeSet, attributes, and network role before any damage, targeting, or AI behavior is layered on top.

### Player Framework

`ATunicPlayerController` owns local Enhanced Input Mapping Context installation and enables the visible mouse cursor used by attack-facing validation. It exposes the default mapping context and priority for later Blueprint or asset configuration without hard-coded asset paths. It also creates the local run-status HUD widget for each locally controlled player and forwards run-upgrade selection requests to the server; it does not own progression state or grant rewards locally.

`ATunicPlayerState` owns persistent player GAS state: player ASC and replicated AttributeSet. It also owns the current per-player run-upgrade pending-choice count and the server-authoritative consume operation. This is replicated personal run-local progression state; concrete upgrade effects are granted to the PlayerState-owned ASC by `ATunicGameMode`.

`ATunicGameMode` sets the default Pawn to `ATunicPlayerCharacter`, PlayerController to `ATunicPlayerController`, PlayerStateClass to `ATunicPlayerState`, and GameStateClass to `ATunicGameState`.

`ATunicGameMode` owns the current server-authoritative run-state and run-reward decision point. Party Wipe v1 is evaluated on the server by scanning the current `GameState->PlayerArray` for controlled `ATunicPlayerCharacter` pawns and checking their replicated character death state. Encounter Clear now uses the active placed `ATunicEncounterSpawner` as the current encounter membership source instead of treating every level enemy as encounter-critical. Enemy death is routed through GameMode so active encounter members can award shared run XP before clear evaluation; if that shared XP increases the shared run level, GameMode grants each current player a personal pending upgrade choice on their `ATunicPlayerState`. When a player selects a run upgrade, GameMode validates the pending choice and applies the configured default upgrade GameplayEffect to that player's PlayerState-owned ASC. Portal v1 completes through a GameMode-owned `EncounterCleared -> FloorTransitionReady` transition. Floor Transition Stub v1 then delays briefly, advances the replicated floor index, resets placed portals and encounter spawners, returns the run state to `CombatActive`, and asks the spawner to create the next fixed wave. Characters expose their own death state, but they do not decide whole-party failure, encounter completion, floor-transition readiness, floor advancement, enemy spawning, or reward application.

`ATunicGameState` owns the replicated run state surface through `ETunicRunState`, the current run-local floor index, the shared run XP pool, and the derived shared run level. The first implemented states are `CombatActive`, `PartyWiped`, `EncounterCleared`, and `FloorTransitionReady`. `ATunicGameState` replicates the state, floor index, shared XP, and shared run level to clients and exposes Blueprint presentation hooks for state, floor, XP, and level changes; it does not decide when the party is wiped, the encounter is cleared, the portal is ready, the floor advances, or rewards are granted. The current shared run level grants per-player pending upgrade choices through GameMode, but GameState does not apply the resulting upgrade effects.

`UTunicRunStatusWidget` is the current minimal HUD surface for run-loop validation. It is created locally by `ATunicPlayerController`, binds to `ATunicGameState` and the owning `ATunicPlayerState` presentation events, displays floor index, run state, shared run XP, shared run level, and the local player's pending upgrade choices, and exposes a button that requests server run-upgrade selection through the owning PlayerController. It does not directly mutate XP, run state, floor index, level, pending choices, rewards, or combat state.

`ATunicPortalActor` is the current placed Portal v1 actor. It activates only after the replicated run state reaches `EncounterCleared`, then the server counts living players in its configured radius and accumulates replicated charge progress while all living players are present. Dead players do not block portal charging. When charging completes, the portal asks `ATunicGameMode` to mark floor transition ready. During the current transition stub, GameMode can reset placed portals for the next floor placeholder loop. The portal does not spawn enemies, grant rewards, load maps, advance floors, or directly mutate `ATunicGameState`.

`ATunicEncounterSpawner` is the current placed Spawner v1 actor. It is server-owned, uses authored spawn point actor references plus fixed spawn entries to create generated encounter enemies, and can also register explicit hand-placed `ATunicEnemyCharacter` references as placed encounter members for the current test encounter. Active encounter membership is generated members plus explicit placed members, not every enemy in the level. Registered placed members participate in encounter clear and reward routing through their own `ExperienceReward`; unregistered placed enemies do not affect XP or portal activation. The spawner can clean up generated enemies before the next floor stub wave, but it does not destroy or respawn hand-placed members. It exposes Blueprint hooks for presentation when an encounter is activated or cleared, but it does not directly mutate run state, grant rewards, scale difficulty, run AI decisions, spawn portal-pressure enemies, or load maps. `ATunicGameMode` queries the spawner's active encounter counts and remains the only class that transitions the run to `EncounterCleared`.

`GameMode` remains server-only. `PlayerController` owns local input setup. `PlayerState` owns replicated player gameplay state.

### Ability System

GAS owns abilities, cooldowns, damage effects, elemental states, action locks, invulnerability, stun, and death tags. Combat results should be server-authoritative.

`UTunicAbilitySystemComponent` is the project ASC subclass. Player ASC instances use `Mixed` replication mode on `ATunicPlayerState`; enemy ASC instances use `Minimal` replication mode on `ATunicEnemyCharacter`.

`UTunicAttributeSet` defines replicated `Health`, `MaxHealth`, `Stamina`, `MaxStamina`, and `ElementalPower` attributes with `OnRep_*` notification hooks. It clamps Health and Stamina against their max values, keeps MaxHealth and MaxStamina at least 1, clamps ElementalPower to non-negative values, and re-clamps current Health/Stamina after max-value GameplayEffect changes.

`UTunicGameplayAbility_LightAttack` is the first minimal player GameplayAbility. It is server-only, granted by `ATunicPlayerCharacter` during player ASC initialization, owns a native Stamina cost GameplayEffect and a native short cooldown/action-lock GameplayEffect, blocks activation while `Cooldown.Attack`, `State.ActionLocked`, or `State.Dead` is present, and delegates to the player character's server-side light attack execution path. The player character currently owns Montage playback configuration and the hit-window implementation. Enemy hit reactions are a server-confirmed presentation hook; final WeaponActor traces are not implemented yet.

`UTunicGameplayAbility_Dodge` is the first minimal player movement GameplayAbility. It is `LocalPredicted`, granted by `ATunicPlayerCharacter` during player ASC initialization, uses a native dodge cooldown/action-lock GameplayEffect, applies a separate native dodge invulnerability GameplayEffect on server authority, blocks activation while `Cooldown.Dodge`, `State.ActionLocked`, or `State.Dead` is present, and starts a predicted GAS root-motion task for movement. The ability owns cooldown/action-lock and invulnerability durations as editable gameplay timing values; the player character owns distance, movement duration, input-direction grace, and Montage play-rate tuning. The ability does not mutate attributes directly and the client-predicted movement does not grant client-side damage or Health authority.

`UTunicGameplayAbility_EnemyMeleeAttack` is the first minimal enemy attack GameplayAbility. It is server-only, granted by `ATunicEnemyCharacter` during enemy ASC initialization, uses a native enemy melee cooldown/action-lock GameplayEffect with the shared attack lock tags, and delegates concrete execution to the enemy character's server-side melee telegraph and attack path. Successful activations end as normal Ability completions, not cancellations. The ability does not directly trace, mutate attributes, or own AI intent.

`UTunicStaminaRegenGameplayEffect` is a minimal server-applied infinite periodic GameplayEffect on the player ASC. It owns the current baseline Stamina regeneration tuning. It does not yet include combat-delay, exhaustion, or tag-gated regen blocking rules.

`UTunicRunUpgradeMaxHealthGameplayEffect` is the first fixed run-upgrade GameplayEffect. It is applied by `ATunicGameMode` to the selected player's PlayerState-owned ASC after consuming one pending run-upgrade choice. The effect is instant and adds to `MaxHealth` before adding to `Health`, so the v1 upgrade both increases the cap and immediately fills the same amount. Future run-upgrade choices can replace the configured GameMode effect class or move to DataAssets without moving reward authority to clients.

Temporary debug draw on player and enemy characters renders replicated Health/Stamina values above actors for PIE validation. This is a world debug aid, not the final HUD or settings UI path.

Movement, camera, simple interaction, and editor-only setup should not be forced into GameplayAbilities. Player input should activate abilities or validated server requests; input handlers must not directly apply damage, death, or elemental states.

### Combat

Weapon hit windows should be animation-notify driven. Weapon actors may detect hits, but final damage and elemental state application should flow through GAS.

`UTunicAnimNotifyState_CombatHitWindow` is the shared C++ bridge from animation timing to server-authoritative attack hit-window functions. The notify state does not apply damage directly; it casts the mesh owner to `ITunicCombatHitWindowSourceInterface` and forwards Begin/Tick/End timing. `ATunicPlayerCharacter` maps that interface to its light-attack hit window, while `ATunicEnemyCharacter` maps it to its melee hit window. Target filtering, team rules, GameplayEffect damage, and hit reaction routing remain in the server-side character path.

For multiplayer PvE, authoritative hit confirmation, projectile spawning, damage GameplayEffects, death, and elemental state changes happen on the server. Clients may play presentation but should not own final combat results.

### AI

Enemy AI should use StateTree from the start. AI and StateTree execution should be server-authoritative. Clients observe replicated movement, combat state, montage/cue presentation, and death state.

`ATunicEnemyAIController` is the first enemy AI controller. It owns a `UStateTreeAIComponent` brain that is started only on the server when a valid enemy pawn and StateTree asset are present. It also owns `UAIPerceptionComponent` sight awareness, the configured enemy awareness policy, current target storage, last-known target investigation state, proximity aggro checks, combat-spawn delayed aggro, idle scan rotation, aggro forget timing, patrol-route state, range checks, focus, stopping movement, and requesting the current target melee attack. Target acquisition filters valid player targets through `ITunicCombatTargetInterface::IsCombatTargetAvailable()`. StateTree assets should consume the AI-controller-owned current target and last-known investigation state rather than running their own nearest-player scan task.

`ATunicEnemyPatrolRoute` is the placed patrol authoring actor. It owns a `USplineComponent` so designers can draw a visible route in the level. Spline control points shape the route, movement samples provide the AI's patrol locations, and sparse stop samples mark where the StateTree may wait. Enemy AI controllers bind to the route through `PatrolRouteId`, read the current sampled patrol location, query whether that point is a stop, and advance the route index. Runtime patrol target locations are projected to navigation when possible so accidentally low or high spline point Z values do not send AI below or above the walkable surface. The route actor can optionally draw the route curve, movement samples, and stop samples during PIE validation. Patrol routes do not own encounter membership, rewards, spawn logic, combat results, or replicated gameplay state.

`ST_EnemyMeleeBasic` uses `Patrol` as the no-current-target parent state. That parent state should remain an intent router rather than owning route movement tasks directly. Its investigation branch should consume the AI-controller-owned last-known target location, move there, search briefly, then clear that investigation state before returning to patrol or idle. Its route branch should move through sampled patrol locations continuously and only wait when `Tunic Enemy Current Patrol Target Is Stop` is true. The current stop duration is read from the route through the AI controller and can be bound into UE's built-in Delay task. `IdleAtHome` remains the fallback for enemies without a route or investigation target. `Chase` consumes the AI-controller-owned current target and moves to that actor. `Attack` requests the existing enemy melee GameplayAbility and does not own hit confirmation or damage. StateTree states that combine instant Tunic tasks with `Move To` or `Delay` use Tasks completion mode `All` so instant tasks do not prematurely complete movement, investigation, or recovery states.

StateTree decides intent: idle, patrol, investigate, chase, combat, attack, recover, hit react, dead. GameplayAbilities execute concrete actions. Native Tunic StateTree nodes may read the current target selected by the AI controller, read and clear last-known investigation state, read patrol locations, advance patrol targets, check attack range through the AI controller, and ask the enemy AI controller to request melee attack activation. They must not directly mutate Health, RunState, XP, GameplayTags, or final combat state. The current prototype auto attack on enemies is only a debug validation trigger for the enemy melee path and is not the main AI path.

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
