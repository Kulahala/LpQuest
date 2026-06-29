# LpQuest

LpQuest is a UE 5.8 C++ lowpoly multiplayer PvE action-combat prototype inspired by Tunic-like fixed isometric exploration and compact combat.

## Direction

- Listen-server co-op PvE vertical slice, starting with two players.
- Fixed isometric camera rather than third-person free camera.
- Lightweight GAS for abilities, cooldowns, replicated attributes, damage, elemental states, and server-authoritative combat.
- StateTree-driven enemy AI, executed on the server.
- V1 player kit starts with sword, bow, dodge, interact, and weapon switching.
- V1 elemental design: Fire, Ice, and Lightning.
- Long-term run structure leans toward a Risk-of-Rain-like co-op floor loop: players explore for resources, trigger a portal event, defeat a boss, survive portal charging pressure, then move to the next floor.
- The available external prototype asset pool supports several player/enemy silhouettes and equipment families, including knights, rangers, mages, rogues, skeleton melee/ranged/mage enemies, bows, crossbows, swords, axes, daggers, staffs, wands, shields, spellbooks, arrows, resources, dungeon pieces, forest props, graveyard props, and medieval buildings.
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
- Stage 4 animation-timed light attack validation is complete: the player uses fixed-view movement with continuous mouse-facing, light attack plays a Montage from the server-authoritative GAS path, a combat hit-window Notify drives the server-side hit sweep during the Montage window, damage still flows through GameplayEffects, and the temporary sword mesh is visual-only with collision disabled.
- Stage 5 combat feedback hardening has a validated minimal path: confirmed non-lethal enemy hits now have a server-triggered multicast presentation hook visible to both listen-server PIE windows, enemy hit/death presentation can use configured default Montages plus Blueprint extension hooks, friendly player hit reaction works without friendly damage, and the light attack path routes targets through a narrow combat target interface with separate hit-reaction and damage permission. Enemy Attack Path v1 is validated with the shared `Tunic Combat Hit Window` Notify: enemies can attack players through the server-authoritative GameplayAbility / GameplayEffect path while same-team hits remain no-damage presentation feedback. Enemy melee attacks now use a server-controlled telegraph plus a configurable foot-centered fan attack shape for readable range and Montage Notify-driven damage confirmation.
- Player Death Policy v1 is validated: enemies can kill players through the server-authoritative GameplayEffect path, dead players cannot act, and dead players are no longer available combat targets.
- Run State Foundation v1 has Party Wipe, Portal Event, and Portal readiness paths in C++: GameMode evaluates all current players dead, player-started portal events, and portal completion, while GameState replicates `CombatActive` / `PartyWiped` / `FloorTransitionReady` / `PortalEventActive` state for clients and Blueprint presentation. Ordinary enemy death does not drive the run out of `CombatActive`.
- Portal Event Foundation v1 is implemented in C++: `IA_Interact` is the unified interact input path for `E`, the player sends server-validated interact requests through a reusable interactable interface, and a placed portal can start `PortalEventActive` from player interaction instead of relying on encounter clear as the long-term activation gate. Boss Spawn On Portal v1 lets the Portal optionally spawn one existing enemy Blueprint as a test Boss; charging waits for that Boss to die, then still requires all living players in range before marking floor transition ready. Portal Charge Pressure v1 lets the Portal spawn ordinary pressure enemies during charging with a per-event XP budget. Enemy Reward Source v1 unifies server XP routing so encounter enemies, Portal Bosses, and budgeted Portal pressure enemies can all award shared XP from their enemy configuration.
- Floor Transition Stub v1 is validated: after portal readiness, the server advances a replicated floor index, resets placed portals, and returns the run state to combat without granting rewards or loading a new map.
- Spawner v1 is implemented in C++: a placed encounter spawner can spawn fixed waves from authored spawn point actors, explicitly register hand-placed enemies as encounter members, track current encounter membership for XP ownership/debug counts, and clean up generated prior-floor enemies without making ordinary enemy clear a Portal or RunState gate.
- Reward / Progression v1 now has a C++ path: server-approved enemy reward sources add enemy-configured XP into a replicated GameState shared run XP pool, derive a shared run level, grant per-player pending upgrade choices, and let each player spend a pending choice through the local HUD to receive a server-applied run upgrade GameplayEffect. The current fixed v1 upgrade increases `MaxHealth` by 20 and restores `Health` by 20 on that player's PlayerState-owned ASC.
- Pickup / Equipment Interaction v1 has a minimal C++ path: world pickup actors reuse the unified `E` interact flow, server-confirmed pickup writes a replicated `CurrentEquipmentId` FName to the interacting player's PlayerState, and the pickup actor is consumed. Enemy Drop Source v1 lets enemies optionally configure a pickup Blueprint class that the server spawns on death through the existing GameMode enemy-death path.
- Enemy AI v1 has the C++ StateTree chase/attack loop in place, and AI Perception / Patrol / Awareness / Investigation support has been added: enemies use a server AIController with a StateTree AI brain, sight perception, configurable awareness policies, proximity aggro, delayed combat-spawn aggro, idle scan, last-known target investigation, visual spline patrol routes with automatic distance-sampled movement points, sparse stop points, native StateTree nodes, and the existing enemy melee Ability request path.
- Formal weapon/socket traces, final hit-reaction animation/content polish, perfect dodge/counter rewards, bow projectiles, final HUD/settings UI, revive/respawn, final death presentation polish, equipment drops, real floor loading, and fully authored enemy StateTree assets are not implemented yet.

## Documentation

- `ARCHITECTURE.md` stores stable architecture facts.
- `AGENTS.md` stores contributor and AI collaboration rules.
- `plan.md` is the tracked active-stage working board; `tunicplan.md` remains the local long-term planning outline.

## Development Notes

Do not copy the old Soulslike project's giant player/enemy classes. Reuse only proven patterns: data-driven attacks, montage notify windows, weapon hit windows, target validation, distance hysteresis, and debugging habits. Keep the playable slice small and network-authoritative from the start.


