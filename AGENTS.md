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

## Documentation Roles

- `README.md`: project overview, current status, and public-facing direction.
- `ARCHITECTURE.md`: stable implemented architecture facts.
- `plan.md`: short active implementation plan and handoff notes.
- `tunicplan.md`: long-term roadmap and milestone plan.
- `AGENTS.md`: contributor and AI collaboration rules.

Update `ARCHITECTURE.md` only when code changes actual architecture. Use `plan.md` for current task status and next steps. Do not turn `tunicplan.md` into a running changelog.

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

Simple changes do not need a long body. For complex changes, the body should explain what changed, why it changed, and whether there are migration, compatibility, or validation notes.

Do not claim that something was tested, built, deployed, packaged, or manually verified unless it was actually done or the user explicitly confirmed it. Do not repeat low-value details that are obvious from the diff.

Do not commit immediately after making changes. Stop after implementation and verification, summarize the diff and validation status, then wait for the user's explicit approval before committing.

Pull requests should include a short summary, affected areas (`Source`, `Config`, or `Content`), build/test results, and screenshots or clips for visible gameplay/editor changes. Call out `.uasset` edits explicitly.

## Agent-Specific Instructions

Read real project files before changing behavior. Keep edits small and aligned with the single-module structure. Do not delete generated folders or user assets unless explicitly requested.

If Serena MCP is available, try activating `E:\UnRealEngine\LpQuest` before symbol-level C++ navigation or C++ edits. In this UE C++ project, treat empty or stale Serena symbol results as non-authoritative because UHT, generated headers, UBT include paths, and changing `compile_commands.json` can make the LSP index incomplete.

If a Serena call fails or cannot find known project symbols, report the concrete reason instead of silently falling back. Include the failing operation, the error or symptom, and the most likely fix, such as reactivating the project, restarting the MCP/client, regenerating `compile_commands.json`, rebuilding the editor target, or refreshing the LSP index. Then immediately fall back to CodeGraph, `rg`, direct source reads, compiler output, and local UE Engine headers.

If `.codegraph/` exists, try CodeGraph before text search for code navigation. If CodeGraph service fails, mention the failure and fall back to `rg` / direct file reads.

If Serena or CodeGraph results appear stale, mention it and suggest reactivation, restart, or re-indexing. After adding/removing C++ files or changing module dependencies, remind the user that `compile_commands.json` may need regeneration for Serena/clangd accuracy.

Do not treat user-tuned `.uasset` changes as review findings unless the user explicitly asks to review assets.

For multiplayer or GAS changes, do not take shortcuts that make later networking harder: avoid direct client-side damage, direct health mutation from input, local-only projectile authority, or AI logic that can run on clients.
