# Repository Guidelines

## Project Structure & Module Organization

This is a UE 5.8 C++ project named `LpQuest`. The primary project file is `LpQuest.uproject`.

- `Source/LpQuest/` contains the single runtime module.
- `Source/LpQuest/Public` contains minimal public gameplay API.
- `Source/LpQuest/Private` contains implementation details.
- `Config/` stores project and engine defaults.
- `Content/_Game/` is for game-owned assets, Blueprints, maps, DataAssets, and animation assets.
- `Content/_External/KayKit/` is for imported prototype art only.
- `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`, `.sln`, and `.slnx` are generated local files.

## Truth Sources

Source code, Config files, `.uproject`, `.Build.cs`, and actual assets are the primary truth sources.

If documents disagree, use this order: source/assets > `ARCHITECTURE.md` > `plan.md` > `tunicplan.md` > `README.md` > `AGENTS.md`.

Do not copy old `E:\UnRealEngine\Test` or `E:\UnRealEngine\LowpolyQuest` classes blindly; those projects are reference material only.

## Architecture Direction

V1 targets a listen-server co-op PvE vertical slice. Build single-player feel quickly, but keep gameplay authority multiplayer-friendly from the start.

- Player ASC and AttributeSet live on `ATunicPlayerState`.
- `ATunicPlayerCharacter` is the ASC AvatarActor and owns camera, input, movement, mesh, and local presentation.
- Enemies own their ASC directly on `ATunicEnemyCharacter`.
- Combat results, projectile spawning, damage GameplayEffects, death, and AI decisions are server-authoritative.
- Clients may own local input and camera presentation, not final combat state.

## Build, Test, and Development Commands

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

## Build Ownership

The user compiles manually. Do not run UBT, `Build.bat`, packaging, cooking, or other long-running build commands unless the user explicitly asks for it.

When C++ changes are made, state the expected build target and ask the user to compile, or wait for the user to report the compile result.

## Coding Style & Naming Conventions

Follow Unreal Engine C++ conventions: tabs for indentation, PascalCase for classes/functions, `b` prefixes for booleans, and Unreal prefixes such as `U`, `A`, `F`, `E`, and `I`. Use `LPQUEST_API` for exported runtime module types.

Keep public headers narrow. Prefer forward declarations in headers and concrete includes in `.cpp` files. Add module dependencies in `LpQuest.Build.cs` only when code actually uses them.

Use reflection macros only for data or behavior that must be visible to Blueprints, serialization, networking, GAS, or the editor.

## Testing Guidelines

No dedicated test module exists yet. For now, every gameplay change should at least pass:

- `LpQuestEditor Win64 Development` build.
- Focused single-player PIE check when relevant.
- Listen-server PIE check for networking-sensitive logic.

For GAS/networking work, verify OwnerActor, AvatarActor, AttributeSet ownership, and replication behavior before adding gameplay on top.

At the end of each stage, do a strict review pass before moving on. Run both a normal review and an adversarial review: first look for bugs, reliability risks, behavior regressions, missing validation, coupling problems, architecture debt, and multiplayer/GAS shortcuts; then defend the key design choices and mark anything that cannot be defended as a risk.

## Planning Workflow

For complex, multi-file, architecture-sensitive, networking-sensitive, or asset-heavy tasks, write a concrete plan before implementation. The plan should cover scope, affected systems/files, execution order, validation, documentation impact, and commit boundaries.

The agent must do this proactively even if the user forgets to ask for a plan. Do not start implementing while the plan is still being discovered. If new information materially changes the plan, pause and explain the revised plan before continuing. If the task is borderline, default to writing the plan first and ask for user approval before editing files.

Plans and roadmap constraints are defaults for deliberate work, not permanent bans. If the user's actual requirement conflicts with the current plan or reveals that a planned architecture no longer fits, adjust the plan deliberately: explain the mismatch, compare the viable options, update the relevant docs, and then implement the revised plan after approval.

Complex or cross-boundary tasks include:

- multi-file C++ changes;
- multiplayer, GAS, replication, RPC, GameplayAbility, or GameplayEffect changes;
- Blueprint asset architecture changes;
- Content asset reorganization;
- architecture/documentation synchronization;
- commits that include both source and `.uasset` changes;
- any task that changes ownership boundaries, authority paths, or long-term extension points.

Small single-file fixes, narrow documentation edits, and simple diagnostic commands may proceed with a short intent statement instead of a full plan.

## Memory Review Hook

During the code-writing to compile/runtime-validation loop, proactively query server-memory MCP when a compiler error, runtime error, repeated warning, or confusing tool failure appears. Use it to check for relevant prior error patterns and fixes before choosing a repair.

If server-memory MCP is unavailable during error复盘, say so explicitly and include the memory lookup that would have been useful. Do not silently skip this step when fixing compile/runtime errors.

Stage-end strict review does not need server-memory by default. It should focus on live source/assets, architecture boundaries, multiplayer/GAS authority, coupling, validation gaps, and long-term risk. Query server-memory during a review only if the review uncovers an actual error pattern or repeated failure mode that memory could help diagnose.

## Documentation Roles

- `README.md`: project overview, current status, and public-facing direction.
- `ARCHITECTURE.md`: stable implemented architecture facts.
- `plan.md`: short active implementation plan and handoff notes.
- `tunicplan.md`: long-term roadmap and milestone plan.
- `AGENTS.md`: contributor and AI collaboration rules.

Update `ARCHITECTURE.md` only when code changes actual architecture. Use `plan.md` for current task status and next steps. Do not turn `tunicplan.md` into a running changelog.

Keep `ARCHITECTURE.md` focused on stable ownership, flow, boundaries, and class/function responsibilities. Avoid hard-coding tunable gameplay numbers such as damage, stamina cost, cooldown duration, regen rate, sweep size, movement speed, or camera distances unless the number is itself an architectural contract. Prefer naming the owning class, GameplayEffect, DataAsset, config key, or function instead. Use `plan.md` for temporary checkpoint values and validation notes.

Before any commit, check whether `README.md`, `ARCHITECTURE.md`, and `AGENTS.md` need updates for the completed change. Only update them when their documented scope actually changed.

## Commit & Pull Request Guidelines

Commit messages should be clear and match the current repository style.

If the repository has an explicit commit format, follow it. If not, use bilingual titles by default:

- `[Feature] 中文标题（English Title）`
- `[Fix] 中文标题（English Title）`
- `[Docs] 中文标题（English Title）`
- `[Refactor] 中文标题（English Title）`
- `[Test] 中文标题（English Title）`
- `[Chore] 中文标题（English Title）`

The Chinese title should describe the actual change. The English title should be concise and natural, not a rigid word-for-word translation. Keep titles short; put complex details in the body.

Simple changes do not need a long body. Use a title-only commit only for narrow changes such as one- or two-file edits, documentation-only wording updates, or small compile fixes.

Use a commit body for complex changes, especially changes that include any of the following:

- C++ plus `.uasset` changes;
- multiplayer, GAS, replication, RPC, GameplayAbility, or GameplayEffect behavior;
- architecture documentation updates;
- stage-level feature checkpoints;
- asset migrations, renames, or LFS-heavy updates.

For complex changes, the body should explain what changed, why it changed, and whether there are migration, compatibility, or validation notes. If the user manually verified behavior, write that as `User confirmed ...`. If validation was not run or was blocked, say so directly instead of implying it passed.

Do not claim that something was tested, built, deployed, packaged, or manually verified unless it was actually done or the user explicitly confirmed it. Do not repeat low-value details that are obvious from the diff.

Do not commit immediately after making changes. Stop after implementation and verification, summarize the diff and validation status, then wait for the user's explicit approval before committing.

Pull requests should include a short summary, affected areas (`Source`, `Config`, or `Content`), build/test results, and screenshots or clips for visible gameplay/editor changes. Call out `.uasset` edits explicitly.

## Git LFS & Asset Commits

`.gitattributes` is the source of truth for Git LFS routing. Unreal assets and large external assets such as `.uasset`, `.umap`, `.fbx`, `.png`, and related binary media should be staged with normal Git commands so LFS filters run automatically.

Do not disable Git LFS filters for `git add` or `git commit`. If local Git commands fail because of Git for Windows or shell issues, LFS-filter-disabling commands may be used only for read-only inspection such as `status`, `log`, `diff`, or `cat-file`.

Before committing large asset batches, verify at least one staged binary asset is an LFS pointer, for example:

```powershell
git cat-file -p :Content/_Game/Characters/Player/Knight/BP_TunicPlayerCharacter.uasset
```

The output should begin with `version https://git-lfs.github.com/spec/v1`.

Use `git add -A` only after the user confirms the whole working tree represents intentional project changes. Otherwise stage focused paths to keep commit intent clear.

## Agent-Specific Instructions

Read real project files before changing behavior. Keep edits small and aligned with the single-module structure. Do not delete generated folders or user assets unless explicitly requested.

If a user request is vague, directionally unclear, or has multiple plausible interpretations that would affect architecture, assets, commits, or gameplay behavior, ask a concise clarifying question before editing.

If a requested approach is technically unsound, high-risk, likely to damage the existing architecture, or clearly worse than another path, push back directly with the technical reason and propose a better alternative.

Default code-navigation architecture:

- `CodeGraph`: primary source for code structure, call chains, and blast-radius checks.
- `ARCHITECTURE.md`: architecture boundaries, responsibility ownership, and design intent.
- `rg` plus direct source reads: final fallback for exact text, implementation details, and stale-index checks.
- Rider: IDE-side semantic checks, navigation, and human verification. Do not assume Rider MCP replaces source-level code intelligence unless the required Rider tools are actually exposed.
- Unreal MCP: live Editor work for assets, Blueprints, levels, actors, gameplay tags, materials, widgets, and runtime/editor state.

If `.codegraph/` exists, try CodeGraph before text search for code navigation, call paths, and blast-radius checks. If CodeGraph service fails or appears stale, mention the failure and fall back to `rg` / direct file reads.

For live Unreal Editor work such as inspecting or changing levels, actors, assets, Blueprints, Gameplay Tags, GAS assets, materials, or widgets, use the `unreal-mcp` skill when the MCP server is available. Discover tools through `list_toolsets` / `describe_toolset`, dispatch through `call_tool`, keep calls sequential, and confirm the user has a save/recovery point before bulk or hard-to-undo asset edits.

Do not treat user-tuned `.uasset` changes as review findings unless the user explicitly asks to review assets.

For multiplayer or GAS changes, do not take shortcuts that make later networking harder: avoid direct client-side damage, direct health mutation from input, local-only projectile authority, or AI logic that can run on clients.
