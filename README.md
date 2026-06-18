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
- `LpQuestEditor Win64 Development` builds successfully on UE 5.8.\n- Next validation step is minimal Editor setup for listen-server PIE testing.

## Documentation

- `tunicplan.md` is the long-term roadmap.
- `plan.md` is the current short implementation plan and handoff surface.
- `ARCHITECTURE.md` stores stable architecture facts.
- `AGENTS.md` stores contributor and AI collaboration rules.

## Development Notes

Do not copy the old Soulslike project's giant player/enemy classes. Reuse only proven patterns: data-driven attacks, montage notify windows, weapon hit windows, target validation, distance hysteresis, and debugging habits. Keep the playable slice small and network-authoritative from the start.


