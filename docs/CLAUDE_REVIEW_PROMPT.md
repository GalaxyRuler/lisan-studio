# Lisan Studio Project Brief And Claude Review Prompt

## Purpose

This document is a handoff brief for an external AI reviewer, especially Claude, to critique Lisan Studio's current architecture, V1 execution plan, sequencing, test strategy, and release risk.

The desired review is not a cheerleading pass. We want a sharp critique that finds missing prerequisites, risky ordering, data-loss paths, weak test coverage, unsafe terminal or installer assumptions, and places where the roadmap creates unnecessary rework.

## Project Summary

Lisan Studio is a clean-room native Qt/C++ Windows IDE for Arabic-first `.apy` programming. It is not an Electron app, not a VS Code fork, and not intended to copy VS Code internals. The long-term goal is VS Code-class workflow parity while preserving a native Qt desktop architecture, Arabic-first RTL correctness, mixed Arabic/English editing quality, and controlled Windows installer validation.

The product is currently a private beta evolving into V1: Reliable Arabic-First Core Workbench.

Core principles:

- Native C++17 and Qt 6 desktop app.
- Arabic-first workbench with RTL UI and LTR islands for code, paths, commands, and output.
- `QPlainTextEdit` editor core for now, with explicit torture tests before advanced editor commitments.
- Bundled `.apy` runtime through controlled Python and `lughat-althuban`; no reliance on global Python or user PATH.
- WiX MSI packaging, with GUI/MSI validation isolated in `LisanStudio-QA`.
- Command registry as the source for visible actions, command palette entries, shortcuts, and future extension surfaces.
- Workbench behavior should keep moving out of `MainWindow` into focused services as features touch each area.
- Test-first development, small slices, squash merges.
- No GUI automation, MSI install/uninstall, registry mutation, or destructive validation on active WHITEDRAGON.

## Current Branch And Validation State

Repository:

```text
C:\Users\Admin\arabic-code-studio-qt
```

Current branch:

```text
main
```

Current state when this brief was written:

```text
main...origin/main [ahead 53]
```

Most recent local validation:

```powershell
.\scripts\validate.ps1
```

Result:

```text
6/6 Qt tests passed
```

Additional recent QA validation:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\qa\tests\Test-DiagnosticsBundle.ps1
```

Result:

```text
Test-DiagnosticsBundle.ps1 passed
```

## Current App Capabilities

Already present in the app:

- Native Qt 6 Windows desktop shell.
- Arabic-first RTL workbench.
- LTR islands for code, file paths, commands, and output.
- `.apy`, `.py`, `.md`, and `.txt` open/save.
- Multiple editor tabs.
- Minimal `.apy` syntax highlighting.
- Hidden Unicode BiDi control detection.
- Project tree.
- Project search with clickable results.
- In-file find/replace.
- Project-wide replace preview, per-row/per-file acceptance, and accepted-row apply.
- Problems panel for hidden BiDi warnings and runtime failures.
- Output panel with transcript filtering, copy, clear, save, and clickable output links.
- Runtime execution for current `.apy`, lint, format, cancellation, run history, and rerun last.
- Named run configuration model and store.
- Workspace settings store and workspace trust primitive.
- Terminal profile model and trusted-workspace gate; actual shell execution remains intentionally deferred.
- Settings dialog with categories, runtime diagnostics, editor font controls, theme preference, and shortcut editing.
- Shortcut settings model, persistence, application, import, export, reset, and per-command override helper.
- Dark/light theme foundation.
- Status bar indicators for encoding, line ending, indentation, language mode, runtime, and Git placeholder.
- Breadcrumb path bar with symbol placeholder for future V2 language services.
- Editor snippets.
- Bracket matching.
- Auto-indent.
- Indentation guides.
- Visible whitespace.
- Trim trailing whitespace on save.
- Recent files and session restore.
- Safe project/file operations for new file, new folder, rename, delete guard, reveal, copy path, and open containing folder.
- Diagnostics bundle script that collects non-secret support evidence.
- WiX MSI packaging and Homelab metadata for isolated GUI/MSI validation.

## Current Core Architecture

Important core modules include:

- `CommandRegistry`: stable command IDs, labels, categories, shortcuts, enablement, trigger callbacks.
- `DocumentFileIO`: UTF-8 load, line-ending detection, file identity, atomic UTF-8 save via Qt save semantics.
- `DocumentRegistry`: document IDs, paths, text, dirty state, encoding, line endings, file identity, external state, versioned text edits, reload/keep-current behavior.
- `DocumentChangePoller`: minimal polling abstraction over `DocumentRegistry` for external modified/deleted detection.
- `UnsavedChangesGuard`: pure decision model for save/discard/cancel operations.
- `WorkbenchState`: search generations, project root, editor session metadata.
- `EditorSurface`: `QPlainTextEdit` behavior, editor commands, find/replace surface, snippets/whitespace/indent helpers, BiDi scanning.
- `EditorFindService`: pure find/replace behavior.
- `ProjectModel`: project discovery and ignored/openable-file rules.
- `ProjectFileOperations`: safe path/name/root/collision preflight checks.
- `ProjectReplaceService`: preview, selection, and apply accepted rows with stale-content refusal.
- `SearchService`: project and open-buffer search.
- `RuntimeRunner`: runtime diagnostics and launch-plan construction.
- `RuntimeHistory`: run history.
- `RuntimeRunConfigurationStore`: JSON run configuration persistence.
- `RuntimeProblemParser`: runtime stderr/stdout problem extraction.
- `TerminalProfileModel`: explicit terminal profile and workspace-trust-gated launch plan.
- `TerminalLinkParser`: file/line/column link parsing.
- `OutputTranscript`: structured output transcript and filtering.
- `SettingsStore`: user settings, theme, shortcuts, recent files/projects, session snapshot.
- `WorkspaceSettingsStore`: project-owned workspace settings and trust flag.
- `SettingsDialogModel`: settings-dialog presentation model.
- `ShortcutSettingsModel`: shortcut validation, conflict detection, import/export data model.
- `ApySnippetService`: `.apy` snippets.
- `ApyHighlighter`: minimal syntax highlighting.

`MainWindow` still composes most UI and many workflows. The accepted direction is to keep extracting behavior into services as work touches each area rather than doing one risky mega-refactor.

## Recent Completed Work

Recent squash-merged slices include:

- `feat: add document change poller`
- `feat: collect diagnostics bundle`
- `feat: add document edit contract`
- `feat: version document records`
- `feat: resolve external document changes`
- `feat: summarize external document changes`
- `feat: declare workbench focus order`
- `feat: edit shortcut bindings`
- `feat: set shortcut overrides`
- `feat: reset shortcut settings`
- `feat: import shortcut settings`
- `feat: export shortcut settings`
- `feat: add shortcut settings page`
- `feat: show effective shortcuts in command palette`
- `feat: apply persisted shortcuts`
- `feat: open output file links`
- `feat: expose output transcript links`
- `feat: add terminal link parser`
- `feat: apply persisted theme preference`
- `feat: persist theme preference from settings dialog`
- `feat: add theme preference setting`
- `feat: persist shortcut settings json`
- `feat: add shortcut settings model`
- `feat: add editor breadcrumb placeholder`
- `feat: add workbench status indicators`

The earlier V1 foundation and beta polish work also landed:

- Command registry foundation.
- Workbench state foundation.
- Editor session state extraction.
- Project operation rules.
- Search dispatch helpers.
- Runtime launch planning.
- Settings dialog model extraction.
- Windows GUI subsystem fix.
- Lisan-owned Undo/Redo context menu actions.
- Search result layout fixes.
- Problems panel subtext alignment fixes.

## Current Test And QA Gates

Qt test targets:

- `acs_editor_tests`
- `acs_command_registry_tests`
- `acs_workbench_state_tests`
- `acs_project_runtime_tests`
- `acs_main_window_tests`
- `acs_editor_torture_tests`

Normal local gate:

```powershell
.\scripts\validate.ps1
```

Relevant PowerShell QA tests:

- `qa\tests\Test-BetaManualCheck.ps1`
- `qa\tests\Test-DiagnosticsBundle.ps1`
- `qa\tests\Test-ReleaseEvidenceFailureLogging.ps1`
- `qa\tests\Test-ReleaseEvidenceHandoffSections.ps1`
- packaging/MSI/static QA tests under `qa\tests\*.ps1`

Homelab/runner boundary:

- CLI/headless validation can run locally or through container/headless routes.
- GUI, MSI, installed-app, screenshot, release-evidence, registry-touching, or destructive validation must run in `LisanStudio-QA`, not active WHITEDRAGON.
- The project runner config is `.codex/homelab-runner.json`.

## Roadmap

### V1: Reliable Arabic-First Core Workbench

Goal: make Lisan Studio trustworthy for daily `.apy` editing before deeper language intelligence.

V1 themes:

- Data-safe documents, saves, reloads, and close/project-switch/exit behavior.
- Core editing: find/replace, snippets, bracket matching, indent tools, whitespace tools.
- Project navigation and safe file operations.
- Project-wide replace with preview and acceptance.
- Runtime loop hardening: named run configs, history, rerun, cancellation, output controls.
- Workbench UX: command palette coverage, shortcuts editor, themes, status indicators, breadcrumb, focus order.
- Session recovery and recent files/projects.
- Terminal service boundary and trust model before any real command execution.
- Release support: diagnostics bundle, installer/release evidence, Homelab validation.

Remaining V1 work that may still need critique or completion:

- Decide whether multi-cursor and column selection belong in V1.0 or V1.5.
- Add deeper QPlainTextEdit torture/feasibility tests for multi-cursor, column selection, and match overlays if still in scope.
- Split `MainWindow` into smaller workbench components if feature pressure makes it unsafe to keep growing.
- Complete real terminal execution only if workspace trust and task/trust model are strong enough.
- Revisit installer version policy: same-version beta replacement versus bumping package label before wider beta.
- Run approval-gated Homelab GUI/MSI/release evidence in `LisanStudio-QA` when preparing the next handoff build.

### V2: Language Intelligence And Debuggable `.apy`

Goal: make `.apy` feel like a real programming language inside the IDE.

Scope:

- Language-service boundary.
- Diagnostics from unsaved buffers.
- Completion, hover, signature help.
- Go to definition, references, rename preview.
- Outline and workspace symbols.
- Semantic highlighting.
- Code actions and format document/selection.
- Test discovery and Test Explorer.
- Debug adapter boundary, breakpoints, stepping, variables, watches, call stack, debug console.

Important dependency already started in V1:

- `DocumentRegistry` now has versioned documents and version-checked text edits, so V2 can reject stale diagnostics/format/rename operations.

### V3: Team Development Workbench

Goal: make Lisan Studio viable for shared professional `.apy` repositories.

Scope:

- Git source-control UI.
- Branch/status indicators.
- Staging, commits, fetch/pull/push.
- Diff and merge conflict UI.
- Multi-root workspaces.
- Workspace trust expansion for tasks, launch configs, terminals, and extension execution.
- Profiles and import/export.
- Multiple terminals and environment activation.
- Update, rollback, and support bundle strategy.

### V4: Ecosystem, Remote Development, And AI

Goal: add extensions, remote development, AI assistance, notebooks, and collaboration without weakening user control.

Scope:

- Extension manifest, isolated host, lifecycle, safe mode, permissions.
- Extension contribution points.
- Remote SSH/WSL/container workspaces.
- AI side panel, inline suggestions, agent mode with plan/diff/approval/audit.
- Secret exclusion and privacy controls.
- `.apy` notebooks or interactive documents.
- Read-only review and collaboration features.

## Known Risks To Review

Please critique these directly:

1. **V1 scope risk:** V1 may still be too broad. Multi-cursor, column selection, split panes, and terminal execution may belong in V1.5.
2. **Data-loss risk:** Project-wide replace, external file changes, dirty open buffers, session restore, and run/format flows need continued scrutiny.
3. **Document model risk:** The document layer now has identity, dirty state, external state, versioning, and text edits, but it may still be too thin for V2 diagnostics/format/rename.
4. **MainWindow risk:** `MainWindow.cpp` still owns a large amount of UI composition and workflow wiring.
5. **QPlainTextEdit ceiling:** Advanced editing features may push beyond what Qt's standard widget can support cleanly.
6. **Terminal trust risk:** Terminal execution is not live yet. If added, it must remain gated by workspace trust and avoid shell-string command construction.
7. **Installer/versioning risk:** Current docs and scripts are still centered around `0.1.0-beta`; same-version beta replacement may be ambiguous for broader distribution.
8. **Homelab coverage risk:** Future debugger, terminal, installer, and GUI flows may need expanded `LisanStudio-QA` validation profiles.
9. **Command registry drift:** Visible command surfaces need ongoing generative tests so menu/action/palette/shortcut coverage stays aligned.
10. **RTL and BiDi risk:** Every editor feature needs mixed Arabic/English and hidden-BiDi-adjacent tests.

## Claude Review Prompt

Copy the prompt below into Claude, ideally with this file and the relevant source tree attached or accessible.

```text
You are reviewing Lisan Studio, a native Qt/C++ Arabic-first IDE for `.apy` programming.

Your job is to critique the project, not to praise it. Please read this brief and review the architecture, roadmap, V1 execution plan, sequencing, validation strategy, and risk posture.

Project context:

- Lisan Studio is a clean-room native Qt 6 / C++17 Windows desktop IDE.
- It is Arabic-first: RTL shell, LTR islands for code/paths/output, mixed Arabic/English editing.
- It uses `QPlainTextEdit` as the current editor core.
- It bundles Python plus `lughat-althuban` for `.apy` execution.
- It uses WiX MSI packaging.
- GUI/MSI/installed-app validation must run in the isolated `LisanStudio-QA` Homelab lane, never on active WHITEDRAGON.
- The project is using TDD, small implementation slices, and squash merges.
- Current local validation passes: `.\scripts\validate.ps1` passes 6/6 Qt tests.

Current architecture highlights:

- `CommandRegistry` for stable command/action/palette/shortcut metadata.
- `DocumentFileIO` for UTF-8 load, file identity, line endings, and atomic saves.
- `DocumentRegistry` for document IDs, text, dirty state, path, encoding, line endings, external file state, versioning, reload/keep-current, and version-checked text edits.
- `DocumentChangePoller` for minimal external-change polling.
- `UnsavedChangesGuard` for save/discard/cancel decisions.
- `ProjectReplaceService` for preview, selection, and safe accepted-row apply.
- `WorkspaceSettingsStore` and workspace trust primitive.
- `TerminalProfileModel` exists, but real terminal execution remains deferred.
- `MainWindow` still composes most UI and workflow wiring.

Current product capabilities include:

- Editor tabs, project tree, project search, in-file find/replace, project replace preview/apply, Problems panel, output transcript/filter/copy/clear/save/link navigation, runtime run/lint/format/cancel/history/rerun, settings dialog, theme/shortcut settings, status indicators, breadcrumb placeholder, snippets, bracket matching, auto-indent, indentation guides, visible whitespace, trim-on-save, recent files, session restore, safe project/file operations, diagnostics bundle, and Homelab release lanes.

Roadmap:

- V1: Reliable Arabic-First Core Workbench.
- V2: Language Intelligence and Debuggable `.apy`.
- V3: Team Development Workbench.
- V4: Ecosystem, Remote Development, and AI.

Please produce a rigorous review with these sections:

1. Verdict: proceed, revise, or stop.
2. Top 10 architectural risks, ordered by severity.
3. V1 scope critique: what should remain V1.0, what should move to V1.5 or V2.
4. Data-loss critique: unsaved buffers, external file changes, project replace, run/format, session restore, app exit.
5. Editor-core critique: whether `QPlainTextEdit` is still viable for the remaining planned editor features.
6. Document model critique: whether current `DocumentRegistry` is enough for V2 diagnostics, format, rename, and stale edit handling.
7. Runtime and terminal critique: safety, workspace trust, command construction, cancellation, output handling.
8. Installer/release critique: versioning, same-version beta replacement, Homelab coverage, release evidence.
9. Test gap list: specific tests that are missing and could hide regressions.
10. Sequencing recommendation: the next 10 implementation slices, in order, each small enough for TDD.
11. Any codebase decomposition recommendations, especially around `MainWindow`.
12. Any questions you need answered before approving V1 completion.

Be concrete. Name files/modules where possible. Do not give generic advice. If you think a feature should be deferred, say why and what evidence would justify keeping it.
```

