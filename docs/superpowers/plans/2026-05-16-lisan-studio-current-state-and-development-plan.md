# Lisan Studio Current State And Development Plan

> **Purpose for reviewers:** This is a single-file handoff for another AI or senior engineer to critique the current product state, architecture, roadmap, sequencing, and validation strategy. Please look for missing foundations, risky ordering, under-specified tests, overreach, and places where the plan could create rework or data-loss risk.

**Repository:** `C:\Users\Admin\arabic-code-studio-qt`

**Current implementation worktree:** `C:\Users\Admin\.config\superpowers\worktrees\arabic-code-studio-qt\codex-v1-foundation`

**Branch:** `codex/v1-foundation`

**Last known local validation in this thread:** `powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"` passed `5/5` Qt tests after the V1 Foundation extraction work.

**Homelab rule:** Do not run GUI, MSI install/uninstall, registry-mutating, destructive, or installed-app validation on active `WHITEDRAGON`. GUI/MSI/release validation belongs in `LisanStudio-QA` through the project-owned Homelab route.

---

## Executive Summary

Lisan Studio is a clean-room native Qt/C++ Windows IDE for Arabic-first `.apy` programming. The private beta is packaged as `0.1.0-beta` and is currently documented as ready for private beta handoff, with previously recorded polish notes resolved.

The product direction is not to clone VS Code's Electron architecture. The goal is workflow parity over time while preserving a native Qt desktop app, Arabic-first RTL correctness, and controlled Windows installer validation. The current app already has a functioning Qt shell, editor tabs, project tree, project search, Problems panel, runtime execution, settings dialog, MSI packaging, and Homelab validation lane.

The immediate architecture work, V1 Foundation, has been implemented on this branch: command registry, workbench state, editor session state, project operation rules, search dispatch helpers, runtime launch planning, and settings dialog model extraction. External review identified that V1 Core must not begin with find/replace. The next product-development block is now Document & Safety Foundation: a document registry, unsaved-change guard, minimal file watcher, atomic save path, file identity metadata, and editor IO migration. Only after that lands should find/replace and project-wide replace begin.

The highest-risk areas are data loss around unsaved buffers and file operations, mixed RTL/LTR editing correctness, runtime/language-service ownership, installer evidence drift, terminal/workspace trust, and whether the roadmap is sequenced tightly enough to avoid building advanced IDE UI before stable document/workspace/task abstractions exist.

---

## Current App State

### Product Baseline

Already present in the private beta:

- Native C++17 / Qt 6 Windows desktop application.
- Clean-room rebuild, not WPF, Emacs-hosted, Electron, or browser-shell based.
- Arabic-first RTL workbench with LTR islands for code, paths, commands, and output.
- `QPlainTextEdit` editor core.
- Open/save support for `.apy`, `.py`, `.md`, and `.txt`.
- Minimal `.apy` syntax highlighting.
- Hidden Unicode BiDi control detection.
- Project folder tree.
- Multiple editor tabs.
- Custom RTL top shell with integrated menus, primary run action, and unified command/search input.
- No inherited native `QMenuBar` or `QToolBar` shell surface.
- Bottom panel tabs for terminal, output, Problems, search results, and debug.
- Project text search with clickable file/line result rows.
- Problems panel entries for hidden BiDi controls and runtime failures.
- Run current `.apy` through bundled runtime with live output, exit code, elapsed time, and cancel action.
- RTL settings dialog with editor font controls, runtime diagnostics, and recent projects.
- Runtime diagnostics for bundled Python, `lughat-althuban`, run, lint, and format availability.
- WiX MSI packaging for `LisanStudio-0.1.0-beta.msi`.
- Installed smoke, MSI smoke, release evidence, screenshot capture, and manual beta QA packet generation.

### Private Beta Polish Status

The following reviewer notes have been resolved in code and documentation:

- Normal app launch is configured as a Windows GUI executable, avoiding an extra command window.
- Right-click Undo/Redo menu actions are Lisan-owned actions and trigger editor commands.
- Search result file text and line metadata are guarded in an RTL metadata cluster.
- Problems panel diagnostic subtext is right-anchored under the metadata row.

### Current Core Modules

`acs_core` currently includes:

- `CommandRegistry`: stable command IDs, labels, categories, shortcuts, enablement, and trigger callbacks.
- `WorkbenchState`: search generations, project root, editor session metadata, current editor session, paths, and dirty flags.
- `ApyHighlighter`: minimal `.apy` syntax highlighting.
- `EditorSurface`: `QPlainTextEdit`-based editor behavior, file IO, context menu, BiDi scanning.
- `ProjectModel`: project file discovery, ignored directories, openable file checks, project operation path/name rules.
- `RuntimeRunner`: explicit Python/runtime command construction, environment isolation, diagnostics, blocking run helper, launch-plan text.
- `RuntimeProblemParser`: parsing runtime stderr into Problems details.
- `SearchService`: project search, unsaved-buffer search, duplicate-free result merge.
- `SettingsDialogModel`: settings dialog state assembly for categories, font selection, runtime status, recent projects.
- `SettingsStore`: persisted editor font settings and recent projects.

The shell still lives primarily in `MainWindow`, which composes widgets and connects surfaces. The accepted direction is to keep extracting behavior into focused services as features touch each area, without a large rewrite.

### Current Test Targets

CMake defines these Qt tests:

- `acs_editor_tests`
- `acs_command_registry_tests`
- `acs_workbench_state_tests`
- `acs_project_runtime_tests`
- `acs_main_window_tests`

The normal local gate is:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
```

The test suite currently covers editor mixed-text behavior, hidden BiDi detection, syntax spans, editor context menu Undo/Redo, command registry, workbench state, project/search/runtime/settings services, and visible workbench UI behavior through `MainWindow` tests.

### Packaging And QA

Important scripts:

- `scripts\build.ps1`
- `scripts\validate.ps1`
- `scripts\package.ps1`
- `scripts\installed-smoke.ps1`
- `scripts\msi-smoke.ps1`
- `scripts\release-evidence.ps1`
- `scripts\beta-manual-check.ps1`

Important QA folders:

- `qa\tests\*.ps1` for static/package/release script guards.
- `qa\homelab\lisanstudio-metadata-checks.json` for project-owned Homelab metadata checks.

Current beta validation docs say the Homelab MSI/GUI/release evidence lane passed inside `LisanStudio-QA`, active `WHITEDRAGON` was not used for GUI/MSI validation, and manual installed-app QA was completed.

---

## Architectural Decisions To Preserve

These are accepted ADR constraints:

1. **Qt-first native rebuild:** Use C++17 / Qt 6; do not reuse WPF, Emacs-host, browser-shell, or Electron code.
2. **QPlainTextEdit-first editor core:** Use Qt's text editor as the first editor engine; switch only if torture tests expose unacceptable mixed-direction failures.
3. **Bundled `.apy` runtime:** Package controlled Python under `runtime\python` with `lughat-althuban`; do not rely on global Python or PATH.
4. **WiX MSI packaging:** First installer lane is a per-user WiX MSI with staged Qt/runtime/license payloads.
5. **Command registry:** Commands should have stable IDs and route menus, actions, command palette, shortcuts, and future extension surfaces through one source.
6. **Workbench service boundaries:** `MainWindow` composes UI; behavior and state should move into focused services when touched.
7. **Cross-platform core policy:** Core services should use Qt abstractions and avoid Windows-only assumptions outside packaging/QA.
8. **`.apy` language/runtime ownership:** IDE owns UI/client/cache/failure UX; runtime owns parser, diagnostics, formatting, symbols, tests, and debug protocol data.
9. **Extension and AI permission model:** Extension/AI features are late-stage and require isolation, permissions, secret exclusion, approval, diff preview, and auditability.

---

## Completed Development Work

### Beta Polish Closure

Commit: `ccab0e3 fix: close beta polish issues`

Implemented:

- Windows GUI subsystem fix.
- Lisan-owned editor context menu Undo/Redo actions.
- Search result row spacing tests/fix.
- Problems panel subtext alignment tests/fix.
- Beta validation and release note updates.

### V1 Foundation

Implemented on branch `codex/v1-foundation`:

- `6f33121 feat: add workbench command registry foundation`
- `f472356 feat: add workbench state foundation`
- `3046200 feat: extract editor session state`
- `39d89ed feat: extract project operation rules`
- `04bb0a0 feat: extract search dispatch helpers`
- `a758e29 feat: extract runtime launch planning`
- `dd08d76 feat: extract settings dialog model`

V1 Foundation is complete against its current plan:

- Command registry data types and tests.
- Current command registrations.
- Registry-backed command palette.
- Visible command-surface registry regression.
- Workbench search-generation tracking.
- Editor session state.
- Project operation rules.
- Search dispatch helpers.
- Runtime launch planning.
- Settings dialog model.
- Validation after each slice.

---

## Full Development Roadmap

### Version 1: Reliable Arabic-First Core Workbench

**Goal:** Make Lisan Studio a trustworthy daily editor for Arabic-first projects before adding heavy language intelligence or ecosystem features.

**Theme:** Polish the core loop: open project, edit mixed-direction files, search, run, inspect output/problems, install cleanly, and recover state.

**Completed V1 items:**

- Beta launch/UI polish.
- Command registry foundation.
- Initial workbench service boundaries.

**Remaining V1 scope:**

- Document registry with file identity, dirty state, external-change metadata, encoding, line-ending policy, and versioned text-edit surface.
- Unsaved-change guard that all close, reopen, run, project-switch, and app-exit paths use.
- Minimal file watcher or polling abstraction for external modify/delete detection.
- Atomic save strategy using write-temp then rename, with recovery behavior for partial writes.
- Editor IO migration so `EditorSurface` no longer owns the durable file read/write policy directly.
- In-file find and replace with RTL-aware match navigation and editor tests.
- Project-wide replace with preview and per-file acceptance.
- Multi-cursor editing for common keyboard and mouse gestures.
- Column/box selection where Qt support is reliable.
- Bracket/quote matching for Arabic and ASCII syntax.
- Auto-indent, indentation guides, visible whitespace, and trim-trailing-whitespace setting.
- Snippets for common `.apy` constructs.
- Split editor panes and side-by-side comparison.
- Persistent tabs, recent files, recent projects, and session restore.
- New file, new folder, rename, delete, reveal in explorer, copy path, and open containing folder.
- Unsaved-change protection across close, reopen, run, project switch, and app exit.
- File watcher for external changes with reload/compare choices.
- Workspace-level settings file separate from user settings.
- Complete command palette coverage for all app actions.
- Keyboard shortcuts editor with import/export.
- Dark/light theme foundation with Arabic readability checks.
- Status bar indicators for encoding, line ending, indentation, language mode, runtime, and Git placeholder.
- Breadcrumb path bar and symbol placeholder for V2 language services.
- Accessible focus order and keyboard-only navigation.
- Named run configurations, run history, rerun last command.
- Better cancellation and process cleanup.
- Output filtering, copy output, clear output, and save output.
- Real integrated terminal abstraction for local shell sessions.
- Signed installer plan, installer UI text polish, upgrade/reinstall/downgrade rules, crash/log bundle.

**V1 exit criteria:**

- A beta user can use Lisan Studio for a full `.apy` editing session without another editor for basic edits.
- No known data-loss path in open, edit, save, close, reload, or app exit.
- GUI/MSI/release evidence passes in `LisanStudio-QA`; active `WHITEDRAGON` is not used for GUI/MSI validation.
- All private beta notes are fixed or explicitly reclassified with evidence.

### Version 2: Language Intelligence And Debuggable `.apy`

**Goal:** Make `.apy` feel like a real programming language inside the IDE.

**Scope:**

- Define `.apy` language-service responses for diagnostics, symbols, completion, hover, signature help, references, rename, format, tests, and debug hooks.
- Add an IDE `LanguageServiceClient` with timeout, stale-state, unavailable-service handling.
- Add diagnostics from unsaved buffers.
- Add completion popup, hover, signature help, go-to/peek definition, references, rename preview, outline, workspace symbols, semantic highlighting, code actions, and format document/selection.
- Add `.apy` test discovery, Test Explorer, run all/file/selected, rerun failed, and failure navigation.
- Add debug adapter boundary, breakpoints, stepping, variables, watch, call stack, debug console, restart, stop, and exception breaks.
- Add golden runtime fixtures and Qt UI tests for every language/debug feature.

**V2 exit criteria:**

- Users can understand and modify a medium `.apy` project using diagnostics, navigation, rename, formatting, tests, and debugger without switching tools.
- Language-service failures degrade gracefully.
- Debug/test workflows produce release evidence artifacts.

### Version 3: Team Development Workbench

**Goal:** Make Lisan Studio viable for shared professional `.apy` repositories.

**Scope:**

- Git repository detection, branch/status indicator, Source Control view, staging/unstaging, commit/amend, fetch/pull/push, history, blame, and merge-conflict UI.
- `.lisan-workspace`, multi-root folders, per-folder settings/tasks/launch configs/language roots, and workspace trust.
- Profiles for app development, teaching, minimal editor, and internal QA with import/export.
- Multiple terminals, terminal profiles, environment activation, terminal search, and clickable links.
- Update channel plan, rollback policy, crash/log bundle with secret redaction, offline installer notes, and enterprise install notes.
- Temporary Git repository tests, workspace fixtures, terminal profile fixtures, and Homelab installed-app regression for release evidence.

**V3 exit criteria:**

- A small team can use Lisan Studio as the primary IDE for a shared `.apy` repository.
- Source-control UI avoids silent data loss and confirms destructive actions.
- Profiles/workspaces can be exported, imported, and used in a fresh install.
- Update/support flows are documented, tested, and reversible.

### Version 4: Ecosystem, Remote Development, And AI

**Goal:** Add extensions, remote development, AI assistance, notebooks, and collaboration without weakening user control or Arabic-first correctness.

**Scope:**

- Extension manifest, isolated host process, lifecycle, safe mode, API versioning, and contribution points.
- Extension permissions for file, process, terminal, network, secrets, and workspace access.
- Remote SSH, useful WSL route, container workspaces, remote explorer, search, Git, terminal, tasks, language service, and debug transport.
- AI side panel, inline suggestions, explain/fix/refactor actions, and agent mode with plan, diff preview, approval, secret exclusion, and audit log.
- `.apy` notebooks or interactive documents with cells, output persistence, state view, and export.
- Read-only review mode, comments/review workflow, and later live collaboration after a security model is tested.
- Tests for extension lifecycle/permissions/crashes, remote routes, AI context and secret exclusion, notebook execution/export, and Homelab isolation guarantees.

**V4 exit criteria:**

- Third-party extensions add useful features without modifying core.
- Remote development supports real work without weakening Windows GUI/MSI isolation.
- AI features are useful, inspectable, cancellable, privacy-controlled, and audit-friendly.

---

## Proposed Step Sequence From Here

This is the recommended execution sequence for the next work. Each item should be implemented test-first and committed as a small slice.

### Phase 0: Critique And Plan Hardening

1. Have another AI or senior engineer critique this document.
2. Ask specifically whether V1 is sequenced correctly or whether document/session/workspace abstractions must come before editing features.
3. Update the V1 Core plan with exact test-first tasks before implementing large features.
4. Keep the existing V1 Foundation branch or create a new branch/worktree for V1 Core, depending on merge strategy.

### Phase 0.5: Document And Safety Foundation

This phase is mandatory before find/replace. It supersedes the earlier recommendation to start with `v1-in-file-find-replace`.

1. Add `DocumentRegistry` with tests for open, close, mark dirty, query by path, duplicate-open detection, file identity, encoding, and line endings.
2. Add `UnsavedChangesGuard` as the single chokepoint for close, reopen, run, project switch, and app exit.
3. Add minimal `FileWatcher` or polling abstraction wired into document metadata.
4. Add atomic save helper using write-temp then rename.
5. Refactor `EditorSurface` to load/save through the document registry or a document IO service.
6. Add command registry IDs for `document.save`, `document.saveAll`, `document.revert`, and `document.closeWithPrompt`.
7. Add `MainWindow` integration tests that dirty buffers survive tab close, project switch, run, and app exit prompt paths.
8. Maintain `acs_editor_torture_tests` as the explicit torture suite before committing to multi-cursor/column-selection scope.

### Phase 1: In-File Find/Replace

1. Add editor-service tests for finding Arabic, English, mixed, and hidden-BiDi-adjacent text.
2. Add match navigation state and visible match selection using `QPlainTextEdit` APIs.
3. Add in-file find UI through the command registry.
4. Add replace current and replace all for the active editor.
5. Preserve RTL shell behavior and LTR code islands.
6. Run `scripts\validate.ps1` and commit.

### Phase 2: Safer File And Project Operations

Project-wide replace must not precede dirty-buffer guards and file-watcher behavior.

1. Complete new file/new folder/rename/delete/reveal/copy path/open containing folder command coverage.
2. Add file watcher reload/compare/keep-current behavior beyond the minimal Phase 0.5 hook.
3. Add tests for external change, deleted file, renamed file, and dirty buffer conflicts.
4. Keep destructive actions behind confirmation.

### Phase 3: Project-Wide Replace

1. Extend `SearchService` or add `ReplaceService` for preview rows.
2. Add tests for UTF-8 `.apy` replacement, skipped ignored folders, file-size limits, and per-file acceptance.
3. Add preview UI with accept/reject per file and per match.
4. Add a dry-run/preview-before-write requirement.
5. Add data-loss tests around unsaved open buffers and external file changes.
6. Run validation and commit.

### Phase 4: Core Editing Ergonomics

1. Add snippets for common `.apy` constructs.
2. Add bracket/quote matching for Arabic and ASCII syntax.
3. Add auto-indent.
4. Add indentation guides.
5. Add visible whitespace.
6. Add trim-trailing-whitespace setting.
7. Run a QPlainTextEdit feasibility spike for multi-cursor, column selection, visible whitespace, and match overlays before committing those features to V1.0.
8. Defer multi-cursor/column selection to V1.5 if the spike is inconclusive or fragile.

### Phase 5: Editor Layout And Session Recovery

1. Add split editor panes.
2. Add side-by-side comparison.
3. Persist open tabs, active tab, project root, editor splits, and selected bottom panel.
4. Add session restore tests using a temporary settings path.
5. Add explicit recovery behavior for missing files and moved projects.
6. Run validation and commit.

### Phase 6: Workspace Settings

1. Define workspace settings file shape, likely project-owned and separate from user settings.
2. Add `WorkspaceSettingsStore`.
3. Add merge/override rules: workspace default, user preference, session override.
4. Add tests for missing workspace file, invalid file, and safe fallback.
5. Do not execute workspace-provided scripts without workspace trust.
6. Define workspace trust data shapes before terminal or task execution features.

### Phase 7: Runtime Loop Hardening

1. Add named run configurations.
2. Add run history and rerun last command.
3. Add output filter/copy/clear/save.
4. Strengthen cancellation and process cleanup.
5. Add tests for timeout, cancel, failed start, nonzero exit, UTF-8 stdout/stderr, and reload-after-format behavior.

### Phase 8: Terminal Abstraction

This phase is gated by workspace trust from Phase 6.

1. Define a terminal service boundary before UI.
2. Add platform shell profiles for PowerShell and project shell.
3. Avoid admin privileges and avoid shell-string command construction.
4. Add tests for command/profile construction and file-link detection.
5. Add UI only after service tests pass.

### Phase 9: Workbench UX Completion

1. Add shortcut editor with import/export.
2. Add status indicators for encoding, line ending, indentation, language mode, runtime, and Git placeholder.
3. Add dark/light theme foundation.
4. Add focus-order and keyboard-only navigation tests.
5. Add breadcrumb path bar and symbol placeholder, with symbol implementation deferred to V2.

### Phase 10: Release Evidence

1. Run local validation.
2. Run relevant PowerShell QA tests.
3. Package only when the change warrants release evidence.
4. Run GUI/MSI/release validation through Homelab in `LisanStudio-QA`, not active `WHITEDRAGON`.
5. Generate/update manual QA packet when preparing a handoff build.

---

## Testing Strategy

### Local Test Gates

Use local Qt tests for source-level and headless UI behavior:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\validate.ps1"
```

Use focused CMake targets during TDD when possible:

- `acs_editor_tests` for editor behavior.
- `acs_command_registry_tests` for command metadata and routing.
- `acs_workbench_state_tests` for state/session behavior.
- `acs_project_runtime_tests` for project/search/runtime/settings service behavior.
- `acs_main_window_tests` for visible workbench integration.

### PowerShell QA Gates

Use `qa\tests\*.ps1` for packaging, MSI, release evidence, and script behavior. Parse touched PowerShell scripts with the PowerShell parser when editing scripts.

### Homelab Gates

Use container/headless routes for CLI, API, unit, integration, metadata, and headless checks when needed.

Use `LisanStudio-QA` for desktop/MSI/GUI validation. Do not run installed-app GUI/MSI validation on active `WHITEDRAGON`.

---

## Known Risks And Questions For Critique

Please critique these directly:

1. **V1 scope risk:** Is V1 too broad? Should multi-cursor, split panes, terminal, and file watcher be split into separate minor milestones?
2. **Data-loss risk:** Are there enough planned tests around dirty buffers, external file changes, project switch, app exit, replace-all, and session restore?
3. **Document model risk:** Should we introduce an explicit document/session model before in-file replace and project-wide replace?
4. **Editor-core risk:** Is `QPlainTextEdit` still acceptable for multi-cursor, column selection, visible whitespace, and match highlighting, or should those be deferred until a spike proves viability?
5. **Language-service sequencing:** Should V2 language-service contract work begin before all V1 editing features, because rename/format/diagnostics may influence document model design?
6. **Runtime ownership:** Is the split between IDE and `lughat-althuban` runtime clear enough for lint, format, diagnostics, tests, and debug?
7. **Command registry coverage:** Are there remaining visible command surfaces that can drift from registry metadata?
8. **Settings architecture:** Do we need workspace settings before snippets, trim-on-save, terminal profiles, and run configurations?
9. **Terminal risk:** Should terminal arrive later, after workspace trust and task model, to avoid unsafe command execution?
10. **Homelab coverage:** Are the current Homelab profiles enough for future GUI, debugger, terminal, and installer validation?
11. **Packaging risk:** Are same-version beta replacements acceptable, or should versioning change before broader beta distribution?
12. **Extension/AI deferral:** Is V4 late enough, or do permission/trust primitives need to be designed earlier to avoid rework in V2/V3?

---

## Recommended Immediate Next Action

Before coding the next feature, implement the safety prerequisite plan:

```text
docs\superpowers\plans\2026-05-16-v1-document-and-safety-foundation.md
```

That plan should contain:

- exact files to create/modify;
- failing test bodies for `DocumentRegistry`, `UnsavedChangesGuard`, minimal file watching, atomic save, and `MainWindow` guard integration;
- focused run commands and expected failures;
- minimal implementation steps;
- command registry additions for document lifecycle commands;
- main-window integration tests;
- validation and commit steps.

Only after Document & Safety Foundation lands should `docs\superpowers\plans\2026-05-16-v1-in-file-find-replace.md` be written, and it should consume the document registry rather than reaching directly into `EditorSurface`.

---

## Review Output Requested

The reviewer should return:

1. A short verdict: proceed, revise, or stop.
2. The top five architectural risks.
3. Any missing V1 prerequisite work.
4. Any tasks that should be reordered.
5. Any test gaps that could hide data loss, RTL corruption, runtime breakage, or installer regressions.
6. A suggested first implementation slice after V1 Foundation.
