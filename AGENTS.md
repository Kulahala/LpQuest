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

For GAS/networking work, verify OwnerActor, AvatarActor, AttributeSet ownership, and replication behavior before adding gameplay on top. At the end of each stage, run a strict review pass with both a normal review and an adversarial review.

## Planning Workflow

For complex, multi-file, architecture-sensitive, networking-sensitive, or asset-heavy tasks, write a concrete plan before implementation. The plan should cover scope, affected systems/files, execution order, validation, documentation impact, and commit boundaries.

The agent must do this proactively even if the user forgets to ask for a plan. Do not start implementing while the plan is still being discovered. If new information materially changes the plan, pause, explain the revised plan, update the relevant docs, and continue only after approval. If the task is borderline, default to writing the plan first and ask for user approval before editing files.

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

Reusable error patterns belong in server-memory MCP, not in `plan.md`. Record trigger scenario, typical symptom/error text, wrong approach, correct fix, and applicable scope in memory; keep `plan.md` to current-stage notes. Stage-end strict review does not need server-memory by default; query it during review only if a concrete repeated error pattern appears.

## Documentation Roles

- `README.md`: project overview, current status, and public-facing direction.
- `ARCHITECTURE.md`: stable implemented architecture facts.
- `plan.md`: short active implementation plan and handoff notes.
- `docs/archive/stage-log.md`: completed stage history and old handoff notes kept for lookup only.
- `tunicplan.md`: long-term roadmap and milestone plan.
- `AGENTS.md`: contributor and AI collaboration rules.

Update `ARCHITECTURE.md` only when code changes actual architecture. Use `plan.md` for current task status and next steps. Do not turn `tunicplan.md` into a running changelog.

Use `plan.md` for the current stage's implementation scope, validation status, strict review notes, accepted risks, and next-step TODOs. Keep it short enough to serve as the next-session working board. When completed stage notes accumulate, move them to `docs/archive/stage-log.md` and leave only the active snapshot and near-term tasks in `plan.md`.

Do not delete or broadly rewrite `plan.md` just because chat context was compacted, a note looks old, or a previous conversation summary is incomplete. Before cleanup, read `plan.md`'s Active Snapshot, compare it with `git log`, `docs/archive/stage-log.md`, live source/assets, and recent commits, then remove only entries that are confirmed archived, completed, rejected, superseded, or no longer relevant to the near-term work.

After a stage is validated, strict-reviewed, and approved for commit, write the completed stage outcome to `docs/archive/stage-log.md` before creating the stage commit, and include that archive update in the same commit unless the user explicitly asks for a separate docs commit. At the same boundary, clear or replace the completed stage's `plan.md` Current stage / ready-to-commit notes so the active board does not keep pointing at already committed work. The archive entry does not need the final commit hash; its job is to make completed stages and their key decisions discoverable without chat history. If the completed stage appears as a TODO in `tunicplan.md`, remove it from the active TODO list and fold the durable result into the normal roadmap text or a completed-foundation section. Do not turn `tunicplan.md` into a second stage log. Do not archive unfinished plan drafts; archive only completed, rejected, or superseded outcomes.

Do not store reusable compiler/runtime/debugging lessons in `plan.md` except as a brief stage note that the issue occurred and was resolved. Move durable lessons to server-memory MCP.

Keep `ARCHITECTURE.md` focused on stable ownership, flow, boundaries, and class/function responsibilities. Avoid hard-coding tunable gameplay numbers such as damage, stamina cost, cooldown duration, regen rate, sweep size, movement speed, or camera distances unless the number is itself an architectural contract. Prefer naming the owning class, GameplayEffect, DataAsset, config key, or function instead. Use `plan.md` for temporary checkpoint values and validation notes.

If a StateTree, Blueprint, or other authored asset changes the stable gameplay architecture, state flow, ownership boundary, or data source of truth, update `ARCHITECTURE.md` in the same stage. Do not leave asset-only architecture changes documented only in chat or `plan.md`.

When `plan.md` records a StateTree asset handoff, use enough operational detail for the next session to recreate or review the graph: parent/child state topology, enter conditions, tasks, important property bindings, transition triggers, transition conditions including `bInvert`, and required Tasks completion mode. Keep stage-specific wiring steps and migration notes in `plan.md`. When that topology becomes the stable expected asset structure, summarize the durable topology in `ARCHITECTURE.md` so future StateTree edits have a current source of truth. Keep full historical graph iterations in `docs/archive/stage-log.md`.

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

Default to writing a commit body. Title-only commits are acceptable only for truly narrow non-behavioral changes such as single-file wording fixes, comment-only edits, or small compile fixes that do not change behavior.

The following changes must include a commit body:

- C++ plus `.uasset` changes;
- multiplayer, GAS, replication, RPC, GameplayAbility, or GameplayEffect behavior;
- architecture documentation updates;
- stage-level feature checkpoints;
- asset migrations, renames, or LFS-heavy updates.

The body should explain what changed, why it changed, and whether there are migration, compatibility, or validation notes. If the user manually verified behavior, write that as `User confirmed ...`. If validation was not run or was blocked, say so directly instead of implying it passed.

阶段型 gameplay 提交的正文默认使用中文，除类名、函数名、属性名、资产名、GameplayTag、StateTree、Blueprint、GAS、API 等必要专业词汇外，不要用英文段落概括。正文要像阶段收尾摘要，而不是抽象概括；优先贴近“提交内容包括”的风格，用短句写清实现了什么行为、关键默认值或调参、哪些 API/StateTree/权责边界保持不变、文档同步、验证状态，以及 strict review 中修了什么或接受了什么后续 TODO。提交正文应该让以后只看 Git 也能明白本阶段做了什么，不需要回翻聊天记录。

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

When a new system replaces an old system and the project is still early, prefer converging on one clear path instead of keeping multiple similar gameplay systems active through hidden fallbacks. Keep temporary migration paths only when there is a concrete migration window and a documented removal condition.

Unless a fallback is required to prevent data loss, preserve compatibility during a concrete migration window, or keep an intentionally supported minimal path working, do not add fallback behavior that makes missing configuration look successful. Prefer failing visibly with a targeted warning so missing Montage Notifies, assets, DataAssets, classes, tags, or authored references are fixed at the source instead of hidden by runtime substitute behavior.

If a user request is vague, directionally unclear, or has multiple plausible interpretations that would affect architecture, assets, commits, or gameplay behavior, ask a concise clarifying question before editing.

If a requested approach is technically unsound, high-risk, likely to damage the existing architecture, or clearly worse than another path, push back directly with the technical reason and propose a better alternative.

For low-risk read-only support work, lightweight subagents may be used to locate relevant files, trace call paths, and extract short `plan.md`, roadmap, or documentation excerpts. Keep plan decisions, architecture tradeoffs, multiplayer/GAS authority boundaries, Blueprint or asset architecture judgments, strict review conclusions, and behavior-sensitive implementation choices with the main agent.

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
