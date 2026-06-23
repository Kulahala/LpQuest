# LpQuest

LpQuest is a UE 5.8 C++ lowpoly multiplayer PvE action-combat prototype inspired by Tunic-like fixed isometric exploration and compact combat.

## Direction

- Listen-server co-op PvE vertical slice, starting with two players.
- Fixed isometric camera rather than third-person free camera.
- Lightweight GAS for abilities, cooldowns, replicated attributes, damage, elemental states, and server-authoritative combat.
- StateTree-driven enemy AI, executed on the server.
- V1 player kit starts with sword, bow, dodge, interact, and weapon switching.
- V1 elemental design: Fire, Ice, and Lightning.
- KayKit assets are prototype art only.

## V1 Target

The first vertical slice should prove:

- listen-server PIE with at least two players
- screen-relative movement under a fixed isometric camera
- PlayerState-owned ASC with Character as AvatarActor
- replicated Health/Stamina attributes
- sword attack with animation notify hit windows
- bow projectile flow with server-spawned projectiles
- dodge with invulnerability timing
- elemental reactions on combat targets
- StateTree melee/ranged/mage/boss enemy prototypes
- a small lowpoly co-op PvE test map ending in a boss encounter

## Current Status

- UE 5.8 project shell exists as `LpQuest`.
- Gameplay C++ skeleton has been migrated from the previous prototype into the `LpQuest` module.
- Implemented base character, player character, enemy character, project ASC, replicated AttributeSet, PlayerState-owned player ASC, player controller, game mode, and game state.
- `LpQuestEditor Win64 Development` builds successfully on UE 5.8.
- Stage 1 editor setup is in place: Enhanced Input actions/mapping context, player character/controller/game mode Blueprints, KayKit prototype character mesh, and OpenWorld-based default map setup.
- Single-player PIE validates the fixed isometric camera and screen-relative WASD movement.
- Listen-server PIE with two players validates that both players spawn and move.
- A small GAS debug path validates PlayerState-owned ASC and replicated AttributeSet setup per player in Listen Server + 2 Players.
- Stage 2 server-authoritative validation is complete: debug-only dodge and light attack request paths work through the server, enemy ASC initialization is logged, and a minimal instant GameplayEffect damage path has reduced enemy Health through GAS.
- Stage 3 death reliability is complete: Health/Stamina clamping, minimal replicated enemy death state, dead-target filtering, Blueprint death-state hook wiring, initial GameplayTags registration, a minimal light attack GameplayAbility entry point, native light-attack Stamina/cooldown gates, minimal GAS Stamina regeneration, and temporary world debug draw for player/enemy attributes are implemented.
- Stage 4 animation-timed light attack validation is complete for the prototype path: the player uses fixed-view movement with continuous mouse-facing, light attack plays a Montage from the server-authoritative GAS path, `UTunicAnimNotifyState_LightAttackHitWindow` drives the server-side hit sweep during the Montage window, damage still flows through GameplayEffects, and the temporary sword mesh is visual-only with collision disabled.
- Stage 5 combat feedback hardening has a validated minimal path: confirmed non-lethal enemy hits now have a server-triggered multicast presentation hook visible to both listen-server PIE windows, and enemy hit/death presentation can use configured default Montages plus Blueprint extension hooks.
- Formal weapon/socket traces, final hit-reaction animation/content polish, dodge invulnerability, bow projectiles, final HUD/settings UI, final death presentation polish, and StateTree enemy behavior are not implemented yet.

## Documentation

- `ARCHITECTURE.md` stores stable architecture facts.
- `AGENTS.md` stores contributor and AI collaboration rules.
- `plan.md` and `tunicplan.md` are local-only planning notes and are intentionally ignored by Git.

## Development Notes

Do not copy the old Soulslike project's giant player/enemy classes. Reuse only proven patterns: data-driven attacks, montage notify windows, weapon hit windows, target validation, distance hysteresis, and debugging habits. Keep the playable slice small and network-authoritative from the start.


