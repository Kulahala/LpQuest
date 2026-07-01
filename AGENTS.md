# Repository Guidelines

## Repository Truth

This is a UE 5.8 C++ project named `LpQuest`. The primary project file is `LpQuest.uproject`, and the single runtime module lives under `Source/LpQuest/`.

- `Source/LpQuest/Public` exposes minimal gameplay API.
- `Source/LpQuest/Private` contains implementation details.
- `Config/` stores project and engine defaults.
- `Content/_Game/` is for game-owned assets, Blueprints, maps, DataAssets, and animation assets.
- `Content/_External/KayKit/` is for imported prototype art only.
- `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`, `.sln`, and `.slnx` are generated local files.

Source code, Config files, `.uproject`, `.Build.cs`, and actual assets are the primary truth sources. If documents disagree, use this order:

`source/assets > ARCHITECTURE.md > plan.md > tunicplan.md > README.md > AGENTS.md`.

Do not copy old `E:\UnRealEngine\Test` or `E:\UnRealEngine\LowpolyQuest` classes blindly; those projects are reference material only. Stable implemented architecture belongs in `ARCHITECTURE.md`, not here.

## Build And Validation

Open the editor:

```powershell
& "D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "E:\UnRealEngine\LpQuest\LpQuest.uproject"
```

Build the editor target:

```powershell
& "D:\UE\UE_5.8\Engine\Build\BatchFiles\Build.bat" LpQuestEditor Win64 Development -Project="E:\UnRealEngine\LpQuest\LpQuest.uproject" -WaitMutex
```

Run automation tests when they exist:

```powershell
& "D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "E:\UnRealEngine\LpQuest\LpQuest.uproject" -run=Automation
```

The user owns Unreal builds and PIE validation. Do not run UBT, `Build.bat`, packaging, cooking, or other long-running UE build commands unless the user explicitly asks for it.

For gameplay changes, hand off the exact `LpQuestEditor Win64 Development` target and focused PIE checks. Use Listen Server + 2 Players for networking-sensitive logic. Do not claim build, test, PIE, deployment, or packaging success unless it actually happened or the user explicitly confirmed it.

## Naming

Follow Unreal Engine C++ conventions: tabs for indentation, PascalCase for classes/functions, `b` prefixes for booleans, Unreal prefixes such as `U`, `A`, `F`, `E`, and `I`, and `LPQUEST_API` for exported runtime module types.

Use the `LPQ` project prefix for native reflected types, C++ source/header filenames, CVars, Blueprint categories, Config paths, and project-level APIs. Keep project code filenames consistently prefixed with `LPQ`.

Do not add `LPQ` mechanically to local variables, helper functions, Actor labels, local display strings, StateTree node names, Montage Notify names, or editor-only labels when a short domain name is clearer. Editor assets may use concise domain names such as `Portal_Combat_Next`, `BP_EnemyPatrolRoute`, or `CombatHitWindow` unless a prefix is needed to disambiguate.

Keep public headers narrow. Prefer forward declarations in headers and concrete includes in `.cpp` files. Add module dependencies in `LpQuest.Build.cs` only when code actually uses them. Use reflection macros only for data or behavior that must be visible to Blueprints, serialization, networking, GAS, or the editor.

## Planning And Docs

For complex, multi-file, architecture-sensitive, networking-sensitive, or asset-heavy tasks, write a concrete plan before implementation. The plan should cover scope, affected systems, execution order, validation, documentation impact, and commit boundary.

Do this proactively even if the user forgets to ask. Do not start complex implementation while the plan is still being discovered; wait until the plan is accepted or the user explicitly requests implementation. If new information materially changes an approved plan, pause, explain the mismatch, update the relevant docs, and continue only after approval.

Complex work includes multi-file C++, multiplayer, GAS, replication, RPC, GameplayAbility, GameplayEffect, Blueprint asset architecture, Content reorganization, architecture docs, source plus `.uasset` commits, or any change to ownership boundaries, authority paths, or long-term extension points. Small single-file fixes, narrow documentation edits, and simple diagnostics may proceed with a short intent statement.

When a UE staged plan is requested or `ue-stage-workflow` is used, also use `ponytail` when available and load only the most relevant UE domain skill before finalizing the plan. Pick the matching surface, such as `ue5-world-interaction`, `ue5-cpp-gameplay`, `ue5-blueprint-workflow`, `ue5-ui-umg-slate`, `ue5-save-load-replication`, `ue5-debug-validation`, `ue5-architecture`, `ue5-performance-packaging`, or `unreal-mcp`. Do not load broad or unrelated UE skills just because they are available.

Document roles:

- `README.md`: project overview, current status, and public-facing direction.
- `ARCHITECTURE.md`: stable implemented architecture facts, ownership, authority boundaries, class/function responsibilities, and durable asset topology.
- `plan.md`: active stage scope, current progress, validation, review notes, accepted risks, and near-term next steps.
- `docs/archive/stage-log.md`: completed, rejected, or superseded stage outcomes.
- `tunicplan.md`: long-term roadmap, milestone direction, and optional Recommendations with adoption triggers.
- `AGENTS.md`: long-lived collaboration rules, build ownership, tool routing, commit policy, and safety constraints.

`AGENTS.md` is not a stage board. Add new rules here only when a pattern has repeated, one mistake would be expensive, or the rule will affect agent behavior long-term. Put current stage status in `plan.md`, future route decisions in `tunicplan.md`, and stable implementation facts in `ARCHITECTURE.md`.

Before starting, replacing, reviewing, or committing a stage, check whether the previous completed / rejected / superseded outcome is archived in `docs/archive/stage-log.md`. Keep `plan.md` short and current; do not broadly rewrite it after context compaction without checking `git log`, stage archive, live source/assets, and recent commits.

After a stage is validated, strict-reviewed, and approved for commit, archive the completed outcome before committing and include that archive update in the same commit unless the user asks for a separate docs commit. At the same boundary, clear completed-stage / ready-to-commit notes from `plan.md` and fold completed roadmap TODOs out of active `tunicplan.md` lists when applicable. Do not archive unfinished plan drafts.

When strict review finds a non-blocking risk, triage it instead of leaving it only in chat: fix blockers immediately, keep current-stage accepted risks in `plan.md`, and put durable future-stage risks in `tunicplan.md` under the stage or Recommendation that will expose them.

Update `ARCHITECTURE.md` when code or authored assets change stable architecture, state flow, ownership boundaries, or the data source of truth. Do not hard-code tunable gameplay numbers there unless the number is itself an architectural contract.

When `plan.md` records a StateTree, Blueprint, or authored-asset handoff, include enough operational detail to recreate or review the graph: topology, enter conditions, tasks, property bindings, transition triggers, `bInvert`, and task completion mode. When that topology becomes stable, summarize it in `ARCHITECTURE.md`.

Before any commit, check whether `README.md`, `ARCHITECTURE.md`, and `AGENTS.md` need updates. Update only documents whose scope actually changed.

## Review And Memory

Formal review prioritizes bugs, behavioral regressions, missing validation, coupling, architecture debt, and authority shortcuts. Findings go first, ordered by severity with file/line references.

Strict or stage-end review has two passes:

1. Normal review: find bugs, risks, regressions, and missing tests.
2. Adversarial review: defend key design choices only where technically defensible; mark weak points as risks or follow-ups.

During the code-writing to compile/runtime-validation loop, query server-memory MCP when a compiler error, runtime error, repeated warning, or confusing tool failure appears. Reusable error patterns belong in server-memory MCP, not in `plan.md`; record trigger, symptom, wrong approach, correct fix, and scope. If server-memory MCP is unavailable, say so and include the lookup that would have been useful.

Stage-end strict review does not need memory by default. Query memory during review only if a concrete repeated error pattern appears.

## Git And LFS

Do not commit immediately after making changes. Stop after implementation and verification, summarize the diff and validation status, then wait for explicit user approval.

Use bilingual commit titles unless the repository establishes a stronger format:

- `[Feature] 中文标题（English Title）`
- `[Fix] 中文标题（English Title）`
- `[Docs] 中文标题（English Title）`
- `[Refactor] 中文标题（English Title）`
- `[Test] 中文标题（English Title）`
- `[Chore] 中文标题（English Title）`

Default to a commit body. Title-only commits are acceptable only for tiny non-behavioral changes such as wording-only edits, comment-only edits, or very small non-behavioral compile fixes.

Complex commits must include a body: C++ plus `.uasset`, multiplayer/GAS/replication/RPC/gameplay behavior, architecture docs, stage checkpoints, asset migrations, renames, or LFS-heavy updates. The body should explain what changed, why, migration/compatibility notes, and validation status. Do not include false validation claims or low-value diff detail.

Stage-level gameplay commit bodies should be primarily Chinese, keeping class names, function names, assets, GameplayTags, StateTree, Blueprint, GAS, and APIs in their original form. Write them like compact closeout notes: implemented behavior, important defaults, preserved boundaries, doc sync, validation status, and strict-review fixes or accepted follow-ups.

`.gitattributes` is the source of truth for Git LFS routing. Stage Unreal assets and large binary media with normal Git commands so LFS filters run automatically. Do not disable LFS filters for `git add` or `git commit`; LFS-filter-disabling commands are allowed only for read-only inspection if local Git tooling fails.

Before committing large asset batches, verify at least one staged binary asset is an LFS pointer:

```powershell
git cat-file -p :Content/_Game/Characters/Player/Knight/BP_Player_Knight.uasset
```

The output should begin with `version https://git-lfs.github.com/spec/v1`. Use `git add -A` only after the user confirms the whole working tree is intentional; otherwise stage focused paths.

Pull requests should summarize affected areas (`Source`, `Config`, `Content`), validation status, and screenshots or clips for visible gameplay/editor changes. Call out `.uasset` edits explicitly.

## Gameplay Safety Rules

Read real project files before changing behavior. Keep edits small and aligned with the single-module structure. Do not delete generated folders or user assets unless explicitly requested.

For multiplayer and GAS work, keep final gameplay state server-authoritative: no direct client-side damage, no direct Health/Stamina mutation from input, no local-only projectile authority, and no AI authority on clients. Clients may own local input and presentation, not final combat state. Verify OwnerActor, AvatarActor, AttributeSet ownership, and replicated attributes before building gameplay on top.

When a new system replaces an old one and the project is still early, converge on one clear path instead of keeping multiple similar gameplay systems active through hidden fallbacks. Keep temporary migration paths only when there is a concrete migration window and a documented removal condition.

Unless a fallback prevents data loss, preserves a concrete migration window, or keeps an intentionally supported minimal path working, do not make missing configuration look successful. Prefer targeted warnings so missing Montage Notifies, assets, DataAssets, classes, tags, or authored references are fixed at the source.

New runtime debug drawing or development log switches should use the central `FLPQDebugSettings` / `LPQ.Debug.*` Console Variable gate unless there is a concrete reason not to. Keep system-local `bDraw...` / `bLog...` booleans only as per-Actor or per-Blueprint opt-in/out controls, and do not hide missing-configuration or failure warnings behind debug CVars.

New placed, selectable, referenced, or configured gameplay/helper Actors must have a visible editor marker, icon, billboard, preview component, or default mesh. Actors that players need to see, find, or interact with at runtime, such as portals and world pickups, must also have a visible runtime fallback mesh unless a required authored mesh is deliberately enforced with a warning. Do not leave default Actor instances discoverable only through the World Outliner.

If a user request is vague or has multiple plausible interpretations that would affect architecture, assets, commits, or gameplay behavior, ask one concise clarifying question before editing. If a requested approach is unsound, high-risk, or clearly worse than another path, push back directly with the technical reason and propose the safer alternative.

Do not treat user-tuned `.uasset` changes as review findings unless the user explicitly asks to review assets.

## Tool Routing

Default code navigation:

- `CodeGraph`: primary source for code structure, call chains, and blast-radius checks when `.codegraph/` exists.
- `ARCHITECTURE.md`: ownership boundaries and design intent.
- `rg` plus direct source reads: final fallback for exact text, implementation details, and stale-index checks.
- Rider: IDE-side semantic checks, navigation, and human verification; do not assume Rider MCP replaces source-level code intelligence unless the required tools are exposed.
- Unreal MCP: live Editor work for assets, Blueprints, levels, actors, gameplay tags, materials, widgets, and runtime/editor state.

If `.codegraph/` exists, try CodeGraph before text search for code navigation, call paths, and blast-radius checks. If CodeGraph service fails or appears stale, mention the failure and fall back to `rg` / direct file reads.

For live Unreal Editor work, use the `unreal-mcp` skill when the MCP server is available. Discover toolsets, dispatch sequentially, and confirm the user has a save/recovery point before bulk or hard-to-undo asset edits.

If the VibeUE plugin is enabled, treat `VibeUE.*` toolsets and AgentSkills as an extension of the same Unreal native MCP endpoint. Do not add a second Unreal MCP server just for VibeUE.

Low-risk read-only support work may be delegated to lightweight subagents. Keep plan decisions, architecture tradeoffs, multiplayer/GAS authority boundaries, Blueprint or asset architecture judgments, strict review conclusions, and behavior-sensitive implementation choices with the main agent.
