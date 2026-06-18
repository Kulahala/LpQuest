# LpQuest Long-Term Plan

## Summary

Build a UE 5.8 C++ lowpoly multiplayer PvE action-combat vertical slice inspired by Tunic-like fixed isometric exploration and compact combat. The project starts from a clean C++ module and avoids copying giant player/enemy classes from older prototypes.

Default choices:

- Project type: UE 5.8 C++ blank project.
- Camera: fixed isometric, no third-person free camera by default.
- Multiplayer: listen-server co-op PvE first, starting with two players.
- Combat foundation: lightweight GAS with server-authoritative combat results.
- Enemy AI: StateTree, server-authoritative.
- V1 content: sword + bow, Fire/Ice/Lightning elements, melee/ranged/mage/Boss prototypes.
- Assets: KayKit Adventurers and Skeletons are prototype art only.

## Architecture Direction

Use one runtime module, `LpQuest`, until a concrete module boundary exists.

Player GAS ownership:

- `ATunicPlayerState` owns player ASC and AttributeSet.
- `ATunicPlayerCharacter` is the AvatarActor and owns camera, input, movement, mesh, and local presentation.
- `ATunicPlayerController` owns local input mapping setup.

Enemy GAS ownership:

- `ATunicEnemyCharacter` owns enemy ASC and AttributeSet.
- Enemy AI and StateTree run on the server.

Combat authority:

- Client input requests ability activation or validated server actions.
- Server confirms hits, spawns projectiles, applies GameplayEffects, changes death/state tags, and drives AI.
- Clients observe replicated attributes, tags, movement, and presentation cues.

## Milestones

### Stage 0 - Clean UE 5.8 Migration

- Create clean UE 5.8 C++ project `LpQuest`.
- Migrate C++ skeleton into the `LpQuest` module.
- Update module dependencies and plugins.
- Rebuild documentation for the new project.
- Compile `LpQuestEditor Win64 Development`.

### Stage 1 - Character, Camera, and Input

- Fixed isometric camera.
- Screen-relative movement.
- Enhanced Input actions and mapping context.
- Player Blueprint and GameMode setup.
- Single-player PIE movement validation.
- Listen-server two-player spawn and movement validation.

### Stage 1.5 - Multiplayer GAS Foundation

- PlayerState-owned ASC and AttributeSet.
- Character AvatarActor initialization in `PossessedBy` and `OnRep_PlayerState`.
- Replication-ready attributes.
- Basic debug checks for OwnerActor, AvatarActor, Health, and Stamina.
- Listen-server validation with two players.

### Stage 2 - GAS Basics

- GameplayTags for state, cooldown, abilities, and elements.
- Initial AbilitySet DataAsset.
- Health/Stamina GameplayEffects.
- Death, action lock, invulnerability, stun, and cooldown tags.
- Basic HUD for local Health/Stamina.

### Stage 3 - Equipment and Sword

- EquipmentComponent and WeaponDataAsset.
- Sword attached to socket.
- Light attack GameplayAbility.
- Montage playback and AnimNotify hit window.
- Server-authoritative damage GE and hit react.

### Stage 4 - Bow and Projectile

- Bow equipment and switching.
- Aim/charge/release flow.
- Server-spawned replicated arrow projectile.
- Projectile hit applies damage and optional elemental tag.
- Soft target assist only; no heavy suction.

### Stage 5 - Elements

- Fire, Ice, and Lightning GameplayTags and GameplayEffects.
- Burning DOT.
- Chilled/Frozen movement or action restriction.
- Shock burst on selected states.
- Element reaction DataAsset or GAS execution path.

### Stage 6 - StateTree Enemies

- Melee minion: patrol, chase, attack, hit react, dead.
- Ranged minion: keep distance, shoot, retreat.
- Mage: cast elemental attacks.
- Shared server-side target validation and death cleanup.

### Stage 7 - Boss Prototype

- Boss archetype and phase DataAsset.
- At least two phases.
- Phase-based skill pool.
- Boss death triggers encounter completion state.

### Stage 8 - Vertical Slice Map

- Small lowpoly co-op PvE map.
- Two players enter, fight small enemy groups, get/use bow, trigger elemental reactions, and defeat a Boss.

## Test Plan

Basic validation:

- UE 5.8 editor target compiles.
- C++ classes are visible in the Editor.
- Single-player PIE starts without missing input warnings.
- Listen-server PIE starts with two players.

Multiplayer validation:

- Each player has independent `ATunicPlayerState` ASC and AttributeSet.
- Health/Stamina replicate from server to clients.
- Client input does not directly apply damage or death.
- Projectiles are spawned by server.
- Enemy StateTree runs on server.

Gameplay validation:

- WASD follows screen direction under fixed camera.
- Dodge grants and clears invulnerability timing.
- Sword only damages during notify hit windows.
- Bow projectile applies server-confirmed damage.
- Element states apply, expire, and replicate correctly.

Asset validation:

- KayKit meshes import with acceptable scale, orientation, and collision.
- Weapon sockets are usable for V1 combat.
- Animation retargeting gaps are recorded instead of hard-coding systems to one asset pack.

## Assumptions

- V1 is listen-server co-op PvE, not matchmaking or dedicated-server production.
- V1 is a compact vertical slice, not an open world.
- Final protagonist art is not required before combat architecture works.
- The project should demonstrate UE C++, GAS, networking boundaries, StateTree AI, data-driven combat, and clean architecture.
