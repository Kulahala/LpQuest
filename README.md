# LpQuest

LpQuest is a UE 5.8 C++ lowpoly multiplayer PvE action-combat prototype inspired by Tunic-like fixed isometric exploration and compact combat.

## Direction

- Listen-server co-op PvE vertical slice, starting with two players.
- Fixed isometric camera rather than third-person free camera.
- Lightweight GAS for abilities, cooldowns, replicated attributes, damage, elemental states, and server-authoritative combat.
- StateTree-driven enemy AI, executed on the server.
- V1 player kit starts with sword, bow, dodge, interact, and weapon switching.
- V1 elemental design: Fire, Ice, and Lightning.
- Run structure leans toward co-op PvE room/floor progression: players fight randomly spawned enemy waves, pick up experience or equipment drops, clear the encounter, then move to the next floor.
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
- Dodge / Roll v2 is validated in C++: dodge input activates a `LocalPredicted` GameplayAbility from a direction payload, moves through GAS `ApplyRootMotionConstantForce` on the predicting client and server, keeps data-tuned cooldown/action-lock and server-only `State.Invulnerable`, and plays the configured Dodge Montage as presentation with an adjustable play rate. The client predicts movement for responsiveness but still does not own damage, Health, invulnerability, or final server correction.
- Stage 4 animation-timed light attack validation is complete for the prototype path: the player uses fixed-view movement with continuous mouse-facing, light attack plays a Montage from the server-authoritative GAS path, a combat hit-window Notify drives the server-side hit sweep during the Montage window, damage still flows through GameplayEffects, and the temporary sword mesh is visual-only with collision disabled.
- Stage 5 combat feedback hardening has a validated minimal path: confirmed non-lethal enemy hits now have a server-triggered multicast presentation hook visible to both listen-server PIE windows, enemy hit/death presentation can use configured default Montages plus Blueprint extension hooks, friendly player hit reaction works without friendly damage, and the light attack path routes targets through a narrow combat target interface with separate hit-reaction and damage permission. Enemy Attack Path v1 is validated with the shared `Tunic Combat Hit Window` Notify: enemies can attack players through the server-authoritative GameplayAbility / GameplayEffect path while same-team hits remain no-damage presentation feedback. Enemy melee attacks now use a server-controlled telegraph plus a configurable foot-centered fan attack shape for readable range and hit-window damage confirmation.
- Player Death Policy v1 is validated: enemies can kill players through the server-authoritative GameplayEffect path, dead players cannot act, and dead players are no longer available combat targets.
- Run State Foundation v1 has initial Party Wipe, Encounter Clear, and Portal readiness paths in C++: GameMode evaluates all current players dead, Spawner-owned encounter enemies cleared, and portal completion, while GameState replicates `CombatActive` / `PartyWiped` / `EncounterCleared` / `FloorTransitionReady` state for clients and Blueprint presentation.
- Portal v1 is validated: a placed portal actor activates after encounter clear, charges while all living players stand in range, and then marks floor transition ready without loading a new map or granting rewards.
- Floor Transition Stub v1 is validated: after portal readiness, the server advances a replicated floor index, resets placed portals, and returns the run state to combat without granting rewards or loading a new map.
- Spawner v1 is implemented in C++: a placed encounter spawner can spawn fixed waves from authored spawn point actors, track current encounter membership, clean up prior-floor enemies, and let GameMode clear the encounter from spawned members rather than every enemy in the level.
- Reward / Progression v1 now has a C++ path: spawned encounter enemy deaths add enemy-configured XP into a replicated GameState shared run XP pool, derive a shared run level, grant per-player pending upgrade choices, and let each player spend a pending choice through the local HUD to receive a server-applied run upgrade GameplayEffect. The current fixed v1 upgrade increases `MaxHealth` by 20 and restores `Health` by 20 on that player's PlayerState-owned ASC.
- Enemy AI v1 has the C++ StateTree chase/attack loop in place, and AI Perception / Patrol / Awareness / Investigation support has been added: enemies use a server AIController with a StateTree AI brain, sight perception, configurable awareness policies, proximity aggro, delayed combat-spawn aggro, idle scan, last-known target investigation, visual spline patrol routes with automatic distance-sampled movement points, sparse stop points, native StateTree nodes, and the existing enemy melee Ability request path. The prototype auto-attack switch is no longer the intended AI path.
- Formal weapon/socket traces, final hit-reaction animation/content polish, perfect dodge/counter rewards, bow projectiles, final HUD/settings UI, revive/respawn, final death presentation polish, equipment drops, real floor loading, and fully authored enemy StateTree assets are not implemented yet.

## Documentation

- `ARCHITECTURE.md` stores stable architecture facts.
- `AGENTS.md` stores contributor and AI collaboration rules.
- `plan.md` and `tunicplan.md` are local-only planning notes and are intentionally ignored by Git.

## Development Notes

Do not copy the old Soulslike project's giant player/enemy classes. Reuse only proven patterns: data-driven attacks, montage notify windows, weapon hit windows, target validation, distance hysteresis, and debugging habits. Keep the playable slice small and network-authoritative from the start.


