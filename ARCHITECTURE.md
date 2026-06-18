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

Optional later dependencies:

- `MotionWarping` for selected melee/Boss correction.
- `Niagara` for elemental VFX.

Avoid early dependencies on PCG, CommonUI, DLSS, Streamline, Reflex, and release/performance tooling until the project actually needs them.

## System Boundaries

### Character

`ATunicCharacterBase` provides shared `ACharacter` movement plus Ability System Component and AttributeSet accessors. It does not create ASC/AttributeSet by default; ownership is supplied by PlayerState or enemy subclasses.

`ATunicPlayerCharacter` owns local presentation and input: fixed isometric `SpringArm + Camera`, Enhanced Input bindings, screen-relative movement, and Blueprint-facing request hooks for dodge, attacks, aim, interact, and weapon switching. It initializes player GAS from `ATunicPlayerState` in `PossessedBy` and `OnRep_PlayerState`, using PlayerState as OwnerActor and Character as AvatarActor.

Current dodge input is debug-only: locally owned clients request dodge through a server RPC, the server logs the accepted request, and then calls the existing Blueprint hook. Real dodge movement, invulnerability, stamina cost, montage, and cooldown behavior are not implemented yet.

Current light attack input is debug-only: locally owned clients request light attack through a server RPC, the server logs the accepted request, performs a short server-side nearby enemy query, logs the target enemy ASC/AttributeSet if one is found, applies a minimal instant GameplayEffect damage path to the enemy ASC when debug damage is enabled, and then calls the existing Blueprint hook. Weapon traces, montage dependency, stamina cost, cooldown behavior, hit reactions, death behavior, Health clamping, and damage immunity rules are not implemented yet.

`ATunicEnemyCharacter` owns its own ASC and AttributeSet because enemies do not have PlayerStates. It initializes GAS with itself as OwnerActor and AvatarActor.

Current enemy GAS validation is debug-only: enemy initialization logs ASC, OwnerActor, AvatarActor, AttributeSet, attributes, and network role before any damage, targeting, or AI behavior is layered on top.

### Player Framework

`ATunicPlayerController` owns local Enhanced Input Mapping Context installation. It exposes the default mapping context and priority for later Blueprint or asset configuration without hard-coded asset paths.

`ATunicPlayerState` owns persistent player GAS state: player ASC and replicated AttributeSet.

`ATunicGameMode` sets the default Pawn to `ATunicPlayerCharacter`, PlayerController to `ATunicPlayerController`, PlayerStateClass to `ATunicPlayerState`, and GameStateClass to `ATunicGameState`.

`ATunicGameState` is the replicated PvE session/encounter state placeholder.

`GameMode` remains server-only. `PlayerController` owns local input setup. `PlayerState` owns replicated player gameplay state.

### Ability System

GAS owns abilities, cooldowns, damage effects, elemental states, action locks, invulnerability, stun, and death tags. Combat results should be server-authoritative.

`UTunicAbilitySystemComponent` is the project ASC subclass. Player ASC instances use `Mixed` replication mode on `ATunicPlayerState`; enemy ASC instances use `Minimal` replication mode on `ATunicEnemyCharacter`.

`UTunicAttributeSet` defines replicated `Health`, `MaxHealth`, `Stamina`, `MaxStamina`, and `ElementalPower` attributes with `OnRep_*` notification hooks.

Movement, camera, simple interaction, and editor-only setup should not be forced into GameplayAbilities. Player input should activate abilities or validated server requests; input handlers must not directly apply damage, death, or elemental states.

### Combat

Weapon hit windows should be animation-notify driven. Weapon actors may detect hits, but final damage and elemental state application should flow through GAS.

For multiplayer PvE, authoritative hit confirmation, projectile spawning, damage GameplayEffects, death, and elemental state changes happen on the server. Clients may play presentation but should not own final combat results.

### AI

Enemy AI should use StateTree from the start. AI and StateTree execution should be server-authoritative. Clients observe replicated movement, combat state, montage/cue presentation, and death state.

StateTree decides intent: idle, patrol, alert, chase, combat, attack, recover, hit react, dead. GameplayAbilities execute concrete actions.

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

## Planned Gameplay Tags

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
