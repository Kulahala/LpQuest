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

Current dodge input is debug-only: locally owned clients request dodge through a server RPC, the server logs the accepted request, and then calls the existing Blueprint hook. Real dodge movement, invulnerability, stamina cost, montage, and cooldown behavior are not implemented yet.

Current light attack input activates a minimal `UGameplayAbility` tagged `Ability.Attack.Sword.Light` through the player ASC. The ability executes on the server, commits its native light-attack Stamina cost GameplayEffect, applies its native cooldown/action-lock GameplayEffect, logs the accepted request, and then delegates to the player character's light-attack execution path. If a light attack Montage is configured on the player character, the server multicasts Montage playback and expects `UTunicAnimNotifyState_CombatHitWindow` on that Montage to drive the server-side hit window through `ITunicCombatHitWindowSourceInterface`. If no Montage is configured, the old immediate hit-window path remains as a bootstrap fallback. The hit window performs a short capsule sweep in front of the character, logs sweep results, routes hit actors through `ITunicCombatTargetInterface`, and uses `FTunicCombatRules` to separate hit-reaction permission from damage permission. Enemy targets can receive the current minimal instant GameplayEffect damage path when debug damage is enabled. Friendly player targets can receive server-confirmed hit-reaction presentation without damage. If the player ASC and light attack ability class are available, failed ability activation does not fall back to the old RPC path; this keeps cooldown and cost gates authoritative. The old server RPC path remains only as a bootstrap fallback for cases where the ASC or ability class is unavailable. Final WeaponActor traces and formal damage immunity rules are not implemented yet.

`ATunicEnemyCharacter` owns its own ASC and AttributeSet because enemies do not have PlayerStates. It initializes GAS with itself as OwnerActor and AvatarActor. It grants a minimal server-only enemy melee attack GameplayAbility to its self-owned ASC. The current enemy melee path can be triggered by a prototype server-only auto attack switch, plays a configured attack Montage through multicast presentation, and uses `UTunicAnimNotifyState_CombatHitWindow` or a bootstrap fallback window to run server-side hit confirmation through `ITunicCombatHitWindowSourceInterface`. It currently has a minimal replicated death state: when server Health reaches 0, the enemy replicates `bIsDead`, disables capsule collision and character movement, plays configured default death presentation, calls a Blueprint death-state hook, adds the registered `State.Dead` loose gameplay tag, and asks `ATunicGameMode` to handle enemy death. Enemies expose an XP reward value for GameMode reward routing, but they do not directly grant XP or mutate run progression state. Non-lethal confirmed hits can call a multicast hit-reaction hook that plays configured default hit presentation and then calls a Blueprint presentation hook; this is presentation-only and does not own damage, stun, knockback, or AI state changes.

`ITunicCombatTargetInterface` is the narrow target routing contract for server-confirmed combat hits. It exposes target availability, target team, target ASC/AttributeSet access, and confirmed-hit presentation routing. It does not apply damage or mutate Health directly. `ATunicEnemyCharacter` implements it as an enemy target. `ATunicPlayerCharacter` implements it as a player target using its PlayerState-owned ASC/AttributeSet and returns unavailable when dead. Same-team non-neutral hits can trigger hit reaction presentation without damage; cross-team Player/Enemy hits can apply damage. Team and friendly-fire decisions go through `FTunicCombatRules`, not per-character ad hoc checks.

Current enemy GAS validation is debug-only: enemy initialization logs ASC, OwnerActor, AvatarActor, AttributeSet, attributes, and network role before any damage, targeting, or AI behavior is layered on top.

### Player Framework

`ATunicPlayerController` owns local Enhanced Input Mapping Context installation and enables the visible mouse cursor used by attack-facing validation. It exposes the default mapping context and priority for later Blueprint or asset configuration without hard-coded asset paths. It also creates the local run-status HUD widget for each locally controlled player; that widget is presentation-only and reads replicated `ATunicGameState` data.

`ATunicPlayerState` owns persistent player GAS state: player ASC and replicated AttributeSet. It also owns the current per-player run-upgrade pending-choice count; this is a replicated personal run-local progression placeholder and does not apply attributes or abilities yet.

`ATunicGameMode` sets the default Pawn to `ATunicPlayerCharacter`, PlayerController to `ATunicPlayerController`, PlayerStateClass to `ATunicPlayerState`, and GameStateClass to `ATunicGameState`.

`ATunicGameMode` owns the current server-authoritative run-state decision point. Party Wipe v1 is evaluated on the server by scanning the current `GameState->PlayerArray` for controlled `ATunicPlayerCharacter` pawns and checking their replicated character death state. Encounter Clear now uses the active placed `ATunicEncounterSpawner` as the current encounter membership source instead of treating every level enemy as encounter-critical. Enemy death is routed through GameMode so spawned encounter members can award shared run XP before clear evaluation; if that shared XP increases the shared run level, GameMode grants each current player a personal pending upgrade choice on their `ATunicPlayerState`. Portal v1 completes through a GameMode-owned `EncounterCleared -> FloorTransitionReady` transition. Floor Transition Stub v1 then delays briefly, advances the replicated floor index, resets placed portals and encounter spawners, returns the run state to `CombatActive`, and asks the spawner to create the next fixed wave. Characters expose their own death state, but they do not decide whole-party failure, encounter completion, floor-transition readiness, floor advancement, enemy spawning, or reward application.

`ATunicGameState` owns the replicated run state surface through `ETunicRunState`, the current run-local floor index, the shared run XP pool, and the derived shared run level. The first implemented states are `CombatActive`, `PartyWiped`, `EncounterCleared`, and `FloorTransitionReady`. `ATunicGameState` replicates the state, floor index, shared XP, and shared run level to clients and exposes Blueprint presentation hooks for state, floor, XP, and level changes; it does not decide when the party is wiped, the encounter is cleared, the portal is ready, the floor advances, or rewards are granted. The current shared run level is a visible run-local growth placeholder derived from shared XP; it does not drive attributes, skills, equipment, or rewards yet.

`UTunicRunStatusWidget` is the current minimal HUD surface for run-loop validation. It is created locally by `ATunicPlayerController`, binds to `ATunicGameState` and the owning `ATunicPlayerState` presentation events, and displays floor index, run state, shared run XP, shared run level, and the local player's pending upgrade choices. It does not mutate XP, run state, floor index, level, pending choices, rewards, or combat state.

`ATunicPortalActor` is the current placed Portal v1 actor. It activates only after the replicated run state reaches `EncounterCleared`, then the server counts living players in its configured radius and accumulates replicated charge progress while all living players are present. Dead players do not block portal charging. When charging completes, the portal asks `ATunicGameMode` to mark floor transition ready. During the current transition stub, GameMode can reset placed portals for the next floor placeholder loop. The portal does not spawn enemies, grant rewards, load maps, advance floors, or directly mutate `ATunicGameState`.

`ATunicEncounterSpawner` is the current placed Spawner v1 actor. It is server-owned, uses authored spawn point actor references plus fixed spawn entries to create encounter enemies, tracks only the enemies it spawned as the current encounter membership, and can clean up those spawned enemies before the next floor stub wave. It exposes Blueprint hooks for presentation when an encounter is spawned or cleared, but it does not directly mutate run state, grant rewards, scale difficulty, run AI decisions, spawn portal-pressure enemies, or load maps. `ATunicGameMode` queries the spawner's alive spawned-member count and remains the only class that transitions the run to `EncounterCleared`.

`GameMode` remains server-only. `PlayerController` owns local input setup. `PlayerState` owns replicated player gameplay state.

### Ability System

GAS owns abilities, cooldowns, damage effects, elemental states, action locks, invulnerability, stun, and death tags. Combat results should be server-authoritative.

`UTunicAbilitySystemComponent` is the project ASC subclass. Player ASC instances use `Mixed` replication mode on `ATunicPlayerState`; enemy ASC instances use `Minimal` replication mode on `ATunicEnemyCharacter`.

`UTunicAttributeSet` defines replicated `Health`, `MaxHealth`, `Stamina`, `MaxStamina`, and `ElementalPower` attributes with `OnRep_*` notification hooks. It clamps Health and Stamina against their max values, keeps MaxHealth and MaxStamina at least 1, clamps ElementalPower to non-negative values, and re-clamps current Health/Stamina after max-value GameplayEffect changes.

`UTunicGameplayAbility_LightAttack` is the first minimal player GameplayAbility. It is server-only, granted by `ATunicPlayerCharacter` during player ASC initialization, owns a native Stamina cost GameplayEffect and a native short cooldown/action-lock GameplayEffect, blocks activation while `Cooldown.Attack`, `State.ActionLocked`, or `State.Dead` is present, and delegates to the player character's server-side light attack execution path. The player character currently owns Montage playback configuration and the hit-window implementation. Enemy hit reactions are a server-confirmed presentation hook; final WeaponActor traces are not implemented yet.

`UTunicGameplayAbility_EnemyMeleeAttack` is the first minimal enemy attack GameplayAbility. It is server-only, granted by `ATunicEnemyCharacter` during enemy ASC initialization, uses a native enemy melee cooldown/action-lock GameplayEffect with the shared attack lock tags, and delegates concrete execution to the enemy character's server-side melee attack path. Successful activations end as normal Ability completions, not cancellations. The ability does not directly trace, mutate attributes, or own AI intent.

`UTunicStaminaRegenGameplayEffect` is a minimal server-applied infinite periodic GameplayEffect on the player ASC. It owns the current baseline Stamina regeneration tuning. It does not yet include combat-delay, exhaustion, or tag-gated regen blocking rules.

Temporary debug draw on player and enemy characters renders replicated Health/Stamina values above actors for PIE validation. This is a world debug aid, not the final HUD or settings UI path.

Movement, camera, simple interaction, and editor-only setup should not be forced into GameplayAbilities. Player input should activate abilities or validated server requests; input handlers must not directly apply damage, death, or elemental states.

### Combat

Weapon hit windows should be animation-notify driven. Weapon actors may detect hits, but final damage and elemental state application should flow through GAS.

`UTunicAnimNotifyState_CombatHitWindow` is the shared C++ bridge from animation timing to server-authoritative attack hit-window functions. The notify state does not apply damage directly; it casts the mesh owner to `ITunicCombatHitWindowSourceInterface` and forwards Begin/Tick/End timing. `ATunicPlayerCharacter` maps that interface to its light-attack hit window, while `ATunicEnemyCharacter` maps it to its melee hit window. Target filtering, team rules, GameplayEffect damage, and hit reaction routing remain in the server-side character path.

For multiplayer PvE, authoritative hit confirmation, projectile spawning, damage GameplayEffects, death, and elemental state changes happen on the server. Clients may play presentation but should not own final combat results.

### AI

Enemy AI should use StateTree from the start. AI and StateTree execution should be server-authoritative. Clients observe replicated movement, combat state, montage/cue presentation, and death state.

StateTree decides intent: idle, patrol, alert, chase, combat, attack, recover, hit react, dead. GameplayAbilities execute concrete actions. The current prototype auto attack on enemies is only a validation trigger for the enemy melee path; StateTree should replace that decision layer without taking over final hit confirmation or damage application.

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
