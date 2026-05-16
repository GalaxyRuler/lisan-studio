# Full-Featured IDE Roadmap Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Grow Lisan Studio from the current private beta into a full-featured Arabic-first IDE with VS Code-class workflows, delivered in four major product versions.

**Architecture:** Keep Lisan Studio a native Qt/C++ desktop app, but evolve it into a workbench architecture: editor core, command registry, project model, language services, runtime/debug services, source-control services, extension host, and release/QA lanes. Preserve project-owned Homelab validation for Windows GUI, MSI, screenshots, and installed-app evidence in `LisanStudio-QA`.

**Tech Stack:** C++17, Qt 6, CMake, Ninja, MSYS2 UCRT64, WiX MSI, bundled Python/`lughat-althuban`, PowerShell QA scripts, Homelab isolated runner, future language-service and plugin APIs.

---

## Research Baseline

The roadmap is based on the current Lisan Studio repo state plus VS Code's documented feature families:

- [Basic editing](https://code.visualstudio.com/docs/editor/codebasics): multi-cursor editing, find/replace, split editors, minimap, breadcrumbs, outline, and file navigation.
- [IntelliSense](https://code.visualstudio.com/docs/editor/intellisense): completions, parameter hints, quick info, member lists, and language-aware suggestions.
- [Debugging](https://code.visualstudio.com/docs/editor/debugging): breakpoints, stepping, variables, call stack, watch expressions, debug console, and launch configurations.
- [Source control](https://code.visualstudio.com/docs/editor/versioncontrol): Git status, diffs, staging, commits, branches, merge-conflict handling, and repository workflows.
- [Integrated terminal](https://code.visualstudio.com/docs/terminal/basics): multiple terminals, shells, profiles, cwd behavior, terminal search, links, and task integration.
- [Tasks](https://code.visualstudio.com/docs/editor/tasks): repeatable build, test, lint, and tool commands attached to workspace configuration.
- [Testing](https://code.visualstudio.com/docs/editor/testing): test discovery, test explorer, run/debug tests, and result navigation.
- [Workspaces and multi-root workspaces](https://code.visualstudio.com/docs/editor/multi-root-workspaces): workspace files, per-folder settings, and cross-project navigation.
- [Profiles](https://code.visualstudio.com/docs/editor/profiles): user profiles for settings, extensions, keybindings, snippets, UI state, and workflow separation.
- [Extension marketplace](https://code.visualstudio.com/docs/editor/extension-marketplace) and [Extension API](https://code.visualstudio.com/api): installable capabilities, contribution points, themes, languages, commands, menus, views, and extension lifecycle.
- [Remote development](https://code.visualstudio.com/docs/remote/remote-overview): SSH, WSL, containers, remote terminals, and remote workspace execution.
- [Copilot in VS Code](https://code.visualstudio.com/docs/copilot/overview): inline suggestions, chat, agent workflows, project context, and customization.
- [Notebooks](https://code.visualstudio.com/docs/datascience/jupyter-notebooks): literate coding, cells, outputs, variables, and interactive documentation.

## Current Lisan Studio Baseline

Already present in the private beta:

- Native Qt 6 / C++17 Windows desktop shell.
- Arabic-first RTL workbench with LTR islands for code, paths, commands, and output.
- `QPlainTextEdit` editor core with `.apy`, `.py`, `.md`, and `.txt` open/save.
- Minimal `.apy` syntax highlighting.
- Hidden BiDi control detection.
- Project tree, tabs, project search, Problems panel, output panel, debug tab placeholder, terminal tab placeholder, and settings dialog.
- Bundled Python runtime and `lughat-althuban` execution.
- WiX MSI packaging, installed smoke, MSI smoke, release evidence, screenshot capture, and professional manual QA packet generation.
- Homelab route for GUI/MSI/release validation inside `LisanStudio-QA`, with active `WHITEDRAGON` forbidden for GUI/MSI validation.

Private beta polish completed after this roadmap was first drafted:

- Normal app launch is configured as a Windows GUI executable.
- Right-click Undo/Redo menu actions are Lisan-owned and trigger editor commands.
- Search result metadata spacing is guarded by Qt geometry tests.
- Problems panel diagnostic subtext alignment is guarded by Qt geometry tests.

## Product Assumptions

- "Full-featured like VS Code" means workflow parity, not cloning VS Code's Electron architecture or extension compatibility.
- Lisan's differentiator remains Arabic-first programming with strong mixed RTL/LTR correctness.
- `.apy` is the first-class language. Python interop remains important, but `.apy` gets the deepest IDE support.
- GUI, installer, installed-app, screenshot, and release-evidence validation stay in `LisanStudio-QA` through Homelab.
- Container/VPS routes are appropriate for metadata checks, JSON/schema checks, static analysis, docs generation, and non-GUI helper tests.
- Public plugin and AI ecosystems should not arrive before the core editor, language service, debug, source control, and trust boundaries are stable.

## Version Strategy

Version 1: Reliable Arabic-First Core Workbench.

Version 2: Language Intelligence and Debuggable `.apy`.

Version 3: Team Development Workbench.

Version 4: Ecosystem, Remote Development, and AI.

The versions are product milestones. They do not need to match the current `0.1.0-beta` package label exactly. The current beta is the foundation feeding Version 1.

## Cross-Version Architecture Work

- [ ] Establish a command registry so menus, command palette, toolbar actions, shortcuts, context menus, and tests all call the same command objects.
- [ ] Split `src/MainWindow.cpp` into focused workbench components once a change touches the relevant area: shell, editor tabs, project explorer, bottom panel, command palette, search, problems, runtime, and settings.
- [ ] Create service interfaces for project files, settings, search, language intelligence, runtime execution, debugging, Git, tasks, and extensions.
- [ ] Keep editor data and UI rendering separate enough that tests can verify mixed Arabic/English behavior without depending on fragile pixel assumptions.
- [ ] Keep Homelab project metadata current whenever a version introduces new GUI, MSI, VM, container, or remote validation workflows.

## Version 1: Reliable Arabic-First Core Workbench

**Goal:** Make Lisan Studio a trustworthy daily editor for Arabic-first projects before adding heavy language intelligence or ecosystem features.

**Primary users:** Private beta users, language designers, early `.apy` developers, and internal QA.

**Theme:** Polish the core loop: open project, edit mixed-direction files, search, run, inspect output/problems, install cleanly, and recover state.

### V1 Feature Scope

- [x] Fix current beta polish issues:
  - [x] Remove the extra command window on normal app launch.
  - [x] Make context-menu Undo/Redo actions clickable and consistent with keyboard shortcuts.
  - [x] Tighten search result row layout.
  - [x] Tighten Problems panel subtext alignment.
- [ ] Strengthen core editing:
  - [ ] In-file find and replace with RTL-aware match highlighting.
  - [ ] Project-wide replace with preview and per-file acceptance.
  - [ ] Multi-cursor editing for common keyboard and mouse gestures.
  - [ ] Column/box selection where Qt support is reliable.
  - [ ] Bracket/quote matching for Arabic and ASCII syntax.
  - [ ] Auto-indent, indentation guides, visible whitespace, and trim-trailing-whitespace setting.
  - [ ] Snippets for common `.apy` constructs.
  - [ ] Split editor panes and side-by-side comparison.
  - [ ] Persistent tabs, recent files, recent projects, and session restore.
- [ ] Strengthen file and project operations:
  - [ ] New file, new folder, rename, delete, reveal in explorer, copy path, and open containing folder.
  - [ ] Unsaved-change protection across close, reopen, run, project switch, and app exit.
  - [ ] File watcher for external changes with reload/compare choices.
  - [ ] Workspace-level settings file owned by the project, separate from user settings.
- [ ] Strengthen workbench UX:
  - [ ] Complete command palette coverage for all app actions.
  - [ ] Keyboard shortcuts editor with import/export.
  - [ ] Theme foundation for dark and light themes, with Arabic readability checks.
  - [ ] Status bar indicators for encoding, line ending, indentation, language mode, runtime, and Git placeholder.
  - [ ] Breadcrumb path bar and symbol placeholder that can be filled by V2 language services.
  - [ ] Accessible focus order and keyboard-only navigation across top shell, editor, project tree, and bottom panel.
- [ ] Strengthen runtime loop:
  - [ ] Named run configurations for current file, project entry point, and custom arguments.
  - [ ] Run history and rerun last command.
  - [ ] Better cancellation and process cleanup.
  - [ ] Output filtering, copy output, clear output, and save output.
  - [ ] Terminal tab becomes a real integrated terminal for local shell sessions that do not require admin privileges.
- [ ] Strengthen packaging and release:
  - [ ] Signed installer plan, even if signing itself is handled outside this repo.
  - [ ] Installer UI text polished for Arabic-first users.
  - [ ] Upgrade/reinstall/downgrade rules documented and tested.
  - [ ] Crash/log bundle command that collects non-secret diagnostics.
  - [ ] Manual QA Word packet remains the primary human-review artifact.

### V1 Required Tests

- [ ] Qt editor tests for every mixed RTL/LTR editing behavior added.
- [ ] Project model tests for file operations and external changes.
- [ ] Runtime tests for run configurations, cancellation, and output preservation.
- [ ] Static QA tests for packaging docs and release evidence language.
- [ ] Homelab `FullLisanSmoke` pass inside `LisanStudio-QA`.
- [ ] Manual installed-app QA pass using the Word review packet.

### V1 Exit Criteria

- [ ] A beta user can use Lisan Studio for a full `.apy` editing session without opening another editor for basic edits.
- [ ] No known data-loss path in open, edit, save, close, reload, or app exit.
- [ ] GUI/MSI/release evidence pass in `LisanStudio-QA`; active `WHITEDRAGON` is not used for GUI/MSI validation.
- [ ] All private-beta non-blocking notes are either fixed or explicitly reclassified with evidence.

## Version 2: Language Intelligence and Debuggable `.apy`

**Goal:** Make `.apy` feel like a real programming language inside the IDE: completions, navigation, diagnostics, formatting, refactoring, tests, and debugging.

**Primary users:** `.apy` application builders and language/runtime maintainers.

**Theme:** Move from text editor plus run button to language-aware development environment.

### V2 Feature Scope

- [ ] Build a Lisan language service layer:
  - [ ] Parse `.apy` files into a stable syntax tree or consume a parser from the language runtime if one exists.
  - [ ] Maintain an incremental document model for unsaved editor buffers.
  - [ ] Produce diagnostics with file, range, severity, code, and suggested fix metadata.
  - [ ] Normalize mixed Arabic/English identifiers, strings, paths, comments, and hidden BiDi handling.
- [ ] Add IntelliSense-class language features:
  - [ ] Completion list for keywords, local symbols, imports/modules, snippets, and runtime functions.
  - [ ] Hover quick info for symbols, diagnostics, and runtime functions.
  - [ ] Signature help for callable constructs.
  - [ ] Go to definition and peek definition.
  - [ ] Find all references.
  - [ ] Rename symbol with preview.
  - [ ] Document symbols, workspace symbols, and outline view.
  - [ ] Semantic highlighting beyond the current minimal highlighter.
  - [ ] Code actions for common diagnostics, including hidden BiDi cleanup.
  - [ ] Format document and format selection.
- [ ] Add test workflows:
  - [ ] Test discovery for `.apy` projects.
  - [ ] Test Explorer view in the bottom or side workbench.
  - [ ] Run all, run file, run selected test, rerun failed, and debug selected test.
  - [ ] Test output and failure navigation.
- [ ] Add debugging:
  - [ ] Launch configurations for `.apy` programs.
  - [ ] Breakpoints in editor gutter.
  - [ ] Step over, step into, step out, continue, pause, restart, stop.
  - [ ] Variables, watch, call stack, debug console, and exception breakpoints.
  - [ ] Debug adapter boundary, even if the first adapter is project-local.
- [ ] Add task workflows:
  - [ ] Workspace task file for build, run, lint, format, test, package, and custom commands.
  - [ ] Task problem matchers for `.apy` diagnostics.
  - [ ] Task status integration with output, Problems, and status bar.

### V2 Required Tests

- [ ] Golden `.apy` fixtures for parser, diagnostics, completions, hover, symbols, rename, and formatting.
- [ ] Qt integration tests for completion popup, hover, outline, rename preview, breakpoints, and test explorer.
- [ ] Runtime/debug protocol tests with deterministic sample programs.
- [ ] Homelab GUI lane for debugger and Test Explorer interactions inside `LisanStudio-QA`.

### V2 Exit Criteria

- [ ] A user can understand and modify a medium `.apy` project using navigation, diagnostics, rename, formatting, tests, and debugger without switching tools.
- [ ] Language-service failures degrade gracefully: no editor crash, stale diagnostics are marked stale, and the user can keep editing.
- [ ] Debug/test workflows produce artifacts that can be attached to release evidence.

## Version 3: Team Development Workbench

**Goal:** Make Lisan Studio viable for team projects: source control, multi-root workspaces, profiles, tasks, shared settings, and reliable update/support workflows.

**Primary users:** Small teams, educators, open-source maintainers, and internal product teams.

**Theme:** Add the workflows that make an IDE a team workbench rather than a single-file editor.

### V3 Feature Scope

- [ ] Git source-control UI:
  - [ ] Repository detection and branch/status indicator.
  - [ ] Source Control view with changed files, staged files, untracked files, conflicts, and clean state.
  - [ ] File diff, inline diff, side-by-side diff, and word diff.
  - [ ] Stage, unstage, discard with confirmation, commit, amend, branch switch, pull, fetch, push.
  - [ ] Merge-conflict editor with accept current, accept incoming, accept both, compare, and mark resolved.
  - [ ] File history, blame/annotate, and commit details.
  - [ ] Guardrails so destructive Git operations require explicit confirmation.
- [ ] Workspace model:
  - [ ] `.lisan-workspace` file format.
  - [ ] Multi-root workspace support.
  - [ ] Per-folder settings, tasks, launch configs, and language-service roots.
  - [ ] Workspace trust prompts for scripts, tasks, debug configs, and extension execution.
- [ ] Profiles and sync-ready settings:
  - [ ] Profiles for teaching, app development, minimal editor, and internal QA.
  - [ ] Profile-scoped settings, shortcuts, snippets, themes, UI layout, and extension enablement.
  - [ ] Import/export profile packages.
  - [ ] Settings merge conflict display.
- [ ] Terminal and environment management:
  - [ ] Multiple integrated terminals.
  - [ ] Terminal profiles for PowerShell, Git Bash/MSYS2, Python runtime shell, and project shell.
  - [ ] Environment activation for bundled runtime and external runtimes.
  - [ ] Terminal links for file paths, URLs, and diagnostic lines.
- [ ] Project templates and onboarding:
  - [ ] New project wizard.
  - [ ] Sample project gallery.
  - [ ] Guided first-run experience that does not feel like a marketing page.
  - [ ] Built-in docs viewer for `.apy` language basics and Lisan Studio workflows.
- [ ] Distribution and support:
  - [ ] Auto-update architecture and rollback policy.
  - [ ] Crash report bundle with local user consent and secret redaction.
  - [ ] Update channel selection: beta, stable, internal.
  - [ ] Offline installer and enterprise install notes.

### V3 Required Tests

- [ ] Git tests using temporary repositories for clean, dirty, staged, conflicted, branch, merge, and rebase states.
- [ ] Workspace tests for single-root, multi-root, nested folder, missing folder, and untrusted workspace behavior.
- [ ] Terminal tests for profiles and file-link detection.
- [ ] Homelab installed-app regression for Git UI, workspace trust prompts, installer upgrade, and rollback behavior.

### V3 Exit Criteria

- [ ] A small team can use Lisan Studio as the primary IDE for a shared `.apy` repository.
- [ ] Source-control UI avoids silent data loss and requires confirmation for destructive actions.
- [ ] Profiles and workspaces can be exported, imported, and used in a fresh install.
- [ ] Update and support flows are documented, tested, and reversible.

## Version 4: Ecosystem, Remote Development, and AI

**Goal:** Make Lisan Studio extensible, AI-assisted, and usable across local and remote development environments while keeping Arabic-first quality and user control.

**Primary users:** Power users, extension authors, remote developers, educators, and organizations standardizing on Lisan.

**Theme:** Open the platform only after the core workbench and team workflows are reliable.

### V4 Feature Scope

- [ ] Extension platform:
  - [ ] Extension manifest format.
  - [ ] Extension host process with crash isolation.
  - [ ] Contribution points for commands, menus, views, themes, snippets, language features, tasks, debuggers, and settings.
  - [ ] Extension API versioning and compatibility policy.
  - [ ] Extension enable/disable, reload, uninstall, and safe mode.
  - [ ] Extension trust and permission prompts for file, process, network, terminal, and secret access.
  - [ ] Local extension gallery before public marketplace.
  - [ ] Public marketplace only after signing, review, abuse handling, and rollback flows exist.
- [ ] Theming and customization:
  - [ ] User themes for colors and token styles.
  - [ ] Icon themes.
  - [ ] Keymap import/export and presets.
  - [ ] Language packs, including Arabic UI quality gates.
  - [ ] Layout customization and view containers.
- [ ] Remote development:
  - [ ] Remote SSH workspace route.
  - [ ] WSL workspace route if Windows users need it.
  - [ ] Container workspace route for repeatable CLI/headless projects.
  - [ ] Homelab/VPS integration for heavy headless builds and tests.
  - [ ] Remote terminal, file explorer, search, Git, tasks, language service, and debug transport.
  - [ ] Clear boundary: Windows GUI/MSI validation remains in `LisanStudio-QA`, not remote Linux routes.
- [ ] AI-assisted development:
  - [ ] AI side panel with project-aware chat.
  - [ ] Inline suggestions and explain/fix/refactor actions.
  - [ ] Agent mode with explicit plan, diff preview, and user approval before edits.
  - [ ] Tool integration for file search, diagnostics, tests, Git status, and Homelab route selection.
  - [ ] Project rules file for coding style, Arabic/RTL requirements, and safety boundaries.
  - [ ] Privacy controls for file inclusion, secret exclusion, network use, and audit logging.
  - [ ] Offline or local-model adapter boundary where practical.
- [ ] Notebook and learning workflows:
  - [ ] `.apy` notebook or interactive document format.
  - [ ] Cell execution, output rendering, variable/state view, and export.
  - [ ] Course/tutorial mode for Arabic-first programming lessons.
  - [ ] Rich examples that can become tests.
- [ ] Collaboration:
  - [ ] Shared session protocol plan.
  - [ ] Read-only review mode before live co-editing.
  - [ ] Comment/review workflow for files and notebooks.
  - [ ] Security model for session identity, file access, terminal access, and extension behavior.

### V4 Required Tests

- [ ] Extension host tests for lifecycle, crashes, permissions, API compatibility, and contribution registration.
- [ ] Remote route tests for file sync, terminal, search, Git, tasks, and debug transport.
- [ ] AI tests using recorded fixtures for prompt context selection, secret exclusion, diff generation, and approval gates.
- [ ] Notebook tests for cell execution, output persistence, and export.
- [ ] Homelab tests proving heavy headless work routes away from active `WHITEDRAGON` and GUI/MSI work remains isolated.

### V4 Exit Criteria

- [ ] Third-party extensions can add useful features without modifying Lisan Studio core.
- [ ] Remote development supports real project work without weakening local MSI/GUI safety boundaries.
- [ ] AI features are useful, inspectable, cancellable, and privacy-controlled.
- [ ] The product can support power users without compromising Arabic-first correctness.

## Capability Matrix

| Capability | Lisan Version | Main Workbench Area | Why It Lands There |
| --- | --- | --- | --- |
| Fix beta launch/UI polish | V1 | Shell, editor, search, problems | Trust before breadth |
| Find/replace, multi-cursor, snippets | V1 | Editor | Daily editing baseline |
| Split editors and session restore | V1 | Workbench/editor tabs | Core IDE ergonomics |
| File operations and file watcher | V1 | Project explorer | Prevent external-change surprises |
| Run configurations and output controls | V1 | Runtime/output | Makes current `.apy` loop dependable |
| Real integrated terminal | V1 | Terminal panel | Needed before task workflows feel complete |
| `.apy` diagnostics and symbols | V2 | Language service | Foundation for intelligence |
| Completion, hover, rename, formatting | V2 | Editor/language service | VS Code-class coding assistance |
| Test Explorer | V2 | Testing panel | Makes validation discoverable |
| Debugger | V2 | Debug panel | IDE parity for real programming |
| Task runner | V2 | Tasks/output/problems | Connects build/test/lint workflows |
| Git UI | V3 | Source Control view | Team development baseline |
| Multi-root workspaces | V3 | Workspace/project model | Team and monorepo support |
| Profiles | V3 | Settings/workbench | Different workflows without app forks |
| Auto-update and support bundle | V3 | Distribution/support | Required for broader users |
| Extension platform | V4 | Extension host/API | Ecosystem after core stability |
| Remote SSH/WSL/container workspaces | V4 | Remote services | Power-user and heavy-workflow parity |
| AI assistant and agent mode | V4 | AI panel/tools | High leverage after trust boundaries |
| Notebooks/tutorial mode | V4 | Notebook/runtime | Education and literate programming |

## Versioned Implementation Plan Files To Create Next

Create these narrower plans before implementation starts. Each should follow the same `docs/superpowers/plans/YYYY-MM-DD-<feature-name>.md` format and include exact tests.

- [x] `docs/superpowers/plans/2026-05-16-v1-foundation.md`
- [x] `docs/superpowers/plans/2026-05-16-v1-core-workbench.md`
- [x] `docs/superpowers/plans/2026-05-16-v2-language-debug.md`
- [x] `docs/superpowers/plans/2026-05-16-v3-team-workbench.md`
- [x] `docs/superpowers/plans/2026-05-16-v4-ecosystem-ai.md`

## Immediate Next Slice Recommendation

Start with a V1 hardening slice, because it turns current manual QA notes into shippable improvements and reduces risk before larger architecture changes.

### Task 1: V1 Beta Polish Closure

**Files:**
- Modify: `src/main.cpp`
- Modify: `src/MainWindow.cpp`
- Modify: `src/MainWindow.h`
- Modify: `tests/TestMainWindow.cpp`
- Modify: `tests/TestEditorSurface.cpp`
- Modify: `docs/BETA_VALIDATION.md`
- Modify: `docs/RELEASE_NOTES.md`

- [ ] **Step 1: Add or update tests for the right-click Undo/Redo context menu.**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
```

Expected before the fix: a focused test fails or the existing manual note remains unverified.

- [ ] **Step 2: Fix the command action plumbing so context menu actions call the same editor commands as shortcuts.**

Use the command-registry direction when touching this area. Avoid adding separate one-off lambdas that will drift from shortcuts and menu actions.

- [ ] **Step 3: Add tests for search result and Problems panel row geometry.**

Assert stable column spacing using widget geometry instead of screenshot-only validation.

- [ ] **Step 4: Fix search and Problems panel layouts.**

Keep RTL reading order and mixed LTR file/line islands intact.

- [ ] **Step 5: Remove the launch command window for normal installed app launch.**

Prefer build/linker configuration or app subsystem configuration over hiding a console after it appears.

- [ ] **Step 6: Run local validation.**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
```

Expected: all Qt tests pass locally.

- [ ] **Step 7: Run Homelab MSI/GUI validation inside `LisanStudio-QA`.**

Run from the Homelab repo only when ready for the isolated GUI/MSI lane:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\tools\codex-runner\Invoke-HomelabRuntimeLane.ps1" -ProjectPath "C:\Users\Admin\arabic-code-studio-qt" -Profile desktopMsiSmoke -Stage FullLisanSmoke -NoDryRun -AllowMutation -IUnderstandThisRunsBoundedRuntime
```

Expected: pass, with `activeWhitedragonUsed` equal to `false`.

## Non-Goals Until Their Version

- [ ] Do not start a public extension marketplace before V4 extension permissions, signing, review, and rollback exist.
- [ ] Do not add AI agent file-editing before V4 approval, diff preview, secret exclusion, and audit logging exist.
- [ ] Do not prioritize remote development before V3 workspace/source-control foundations are stable.
- [ ] Do not add VS Code extension compatibility unless a separate technical feasibility plan proves it is safer than a native Lisan extension API.
- [ ] Do not weaken Arabic/RTL tests to fit generic IDE behavior.

## Self-Review

Spec coverage:

- [ ] The plan covers editor, language intelligence, debugger, testing, terminal, tasks, source control, workspaces, profiles, extensions, remote development, AI, notebooks, packaging, update/support, and Homelab validation.
- [ ] The plan groups features into Version 1 through Version 4.
- [ ] The plan uses VS Code research as a capability baseline while preserving Lisan's native Qt and Arabic-first identity.

Placeholder scan:

- [ ] No unresolved placeholder markers remain.
- [ ] Deferrals are assigned to a version or explicitly marked as non-goals until their version.

Implementation readiness:

- [ ] The immediate next slice is small enough to implement first.
- [ ] Each major version has test and exit criteria.
- [ ] GUI/MSI safety boundaries remain explicit.
